//
// Created by user on 2/1/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_DECODE_ENGINE_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_DECODE_ENGINE_H
#pragma once
#include "../../j_video_player/utils/clock.h"
#include "../../j_video_player/utils/scope_guard.h"
#include "../../j_video_player/utils/simple_fifo.h"
#include "ffmpeg_audio_resampler.h"
#include "ffmpeg_codec.h"
#include "ffmpeg_demuxer.h"
#include "ffmpeg_frame_queue.h"
#include "ffmpeg_headers.h"
#include "ffmpeg_image_converter.h"
#include "ffmpeg_packet_queue.h"
#include "ffmpeg_waitable_frame_queue.h"
#include "ffmpeg_waitable_packet_queue.h"
#include <cassert>
#include <iostream>

namespace ffmpeg_utils {

class VideoOutputConfig {
public:
  int width{0};
  int height{0};
  AVPixelFormat pixel_format{AVPixelFormat::AV_PIX_FMT_NONE};
};

class AudioOutputConfig {
public:
  int num_channels{0};
  int channel_layout{AV_CH_LAYOUT_MONO};
  int sample_rate{0};
};

enum class DecodeEngineState {
  kStopped = 0,
  kStarting,
  kDecoding,
  kStopping,
  kPaused
};

class FFMPEGDecodeEngine {
public:
  ~FFMPEGDecodeEngine() { stop(); }
  int openFile(const std::string &file_path) {
    int ret = demuxer.openFile(file_path);
    RETURN_IF_ERROR_LOG(ret, "Could not open file %s\n", file_path.c_str());
    demuxer.dumpFormat();

    findStreams();

    ret = prepareCodecs();
    RETURN_IF_ERROR_LOG(ret, "Prepare codecs failed\n");

    initVideoOutputConfig();
    initAudioOutputConfig();

    audio_sample_fifo =
        std::make_unique<AudioSampleFIFO>(1024 * audio_codec_ctx->channels);

    is_opened_ok_ = true;
    return 0;
  }

  bool isOpenedOk() const { return is_opened_ok_; }

  VideoOutputConfig getInputFileVideoConfig() const { return video_config_; }

  AudioOutputConfig getInputFileAudioConfig() const { return audio_config_; }

  int start() { return start(nullptr, nullptr); }

  int start(VideoOutputConfig *video_config, AudioOutputConfig *audio_config) {
    if (!isOpenedOk()) {
      return -1;
    }

    if (video_config != nullptr) {
      prepareImageConverter(video_config);
    }

    if (audio_config != nullptr) {
      prepareAudioResampler(audio_config);
    }

    state_.store(DecodeEngineState::kStarting);

    startThreads();

    state_.store(DecodeEngineState::kDecoding);
    return 0;
  }

  int stop() {
    state_.store(DecodeEngineState::kStopping);

    // clear queue before stop threads to make sure thread can exit successfully
    clearQueue();
    stopThreads();

    state_.store(DecodeEngineState::kStopped);

    return 0;
  }

  void seek(double target_pos) {
    if (!seek_req_) {
      std::lock_guard lg(seek_mut_);
      auto pos = target_pos;
      if (pos < 0) {
        pos = 0;
      }

      seek_pos_ = (int64_t)(pos * AV_TIME_BASE);
      seek_flags_ = AVSEEK_FLAG_BACKWARD;
      seek_req_ = true;
    }
  }

  AVFrame *pullVideoFrame() {
    auto *frame = video_frame_sync_que.tryPop();
    return frame;
  }

  AVFrame *pullAudioFrame() {
    auto *frame = audio_frame_sync_que.tryPop();
    return frame;
  }

  //
  /**
   * pull audio samples with target audio config
   * @param out_num_samples_per_channel the number of samples you wanted
   * @param out_num_channels the number of channels you wanted
   * @param out_buffer buffer to hold this samples
   * @return {number of output samples, last audio frame pts(seconds)}
   * @note only support int16 format
   */
  std::pair<int, double> pullAudioSamples(size_t out_num_samples_per_channel,
                                          size_t out_num_channels,
                                          int16_t *out_buffer) {

    const auto total_need_samples =
        out_num_samples_per_channel * out_num_channels;
    auto num_samples_need = total_need_samples;
    auto *sample_fifo = audio_sample_fifo.get();
    int sample_index = 0;
    int16_t s = 0;

    auto getSampleFromFIFO = [&]() {
      for (; num_samples_need > 0;) {
        if (sample_fifo->pop(s)) {
          out_buffer[sample_index++] = s;
          --num_samples_need;
        } else {
          break;
        }
      }
    };

    auto resampleAudioAndPushToFIFO = [&](AVFrame *frame) {
      assert(frame->channels == audio_config_.num_channels);

      int num_samples_out_per_channel = audio_resampler.convert(
          (const uint8_t **)frame->data, frame->nb_samples);
      int num_samples_total = num_samples_out_per_channel * frame->channels;
      auto *int16_resample_data =
          reinterpret_cast<int16_t *>(audio_resampler.resample_data[0]);
      for (int i = 0; i < num_samples_total; ++i) {
        sample_fifo->push(std::move(int16_resample_data[i]));
      }
    };

    for (;;) {
      // try to get samples from fifo
      getSampleFromFIFO();

      if (num_samples_need <= 0) {
        break;
      } else {
        auto *frame = pullAudioFrame();
        ON_SCOPE_EXIT([&frame] {
          if (frame != nullptr) {
            av_frame_unref(frame);
            av_frame_free(&frame);
          }
        });

        if (frame == nullptr) {
          printf("no audio frame, set remains to zero\n");
          auto remain = total_need_samples - num_samples_need;
          std::fill_n(out_buffer + num_samples_need, remain, 0);
          return {remain, last_audio_frame_pts_};
        } else {
          last_audio_frame_pts_ = frame->pts * av_q2d(audio_stream->time_base);
          resampleAudioAndPushToFIFO(frame);
        }
      }
    }
    return {out_num_samples_per_channel, last_audio_frame_pts_};
  }

  DecodeEngineState state() const { return state_; }

  FFMPEGDemuxer demuxer;
  FFMPEGCodec video_codec;
  FFMPEGCodec audio_codec;
  FFMPEGImageConverter img_conv;
  FFMPEGAudioResampler audio_resampler;

  int video_stream_index{-1};
  int audio_stream_index{-1};

  AVStream *video_stream{nullptr};
  AVStream *audio_stream{nullptr};
  AVCodecContext *audio_codec_ctx{nullptr};
  AVCodecContext *video_codec_ctx{nullptr};

  WaitablePacketQueue audio_packet_sync_que;
  WaitableFrameQueue audio_frame_sync_que{AUDIO_FRAME_QUEUE_SIZE};

  WaitablePacketQueue video_packet_sync_que;
  WaitableFrameQueue video_frame_sync_que{VIDEO_PICTURE_QUEUE_SIZE};

  using AudioSampleFIFO = utils::SimpleFIFO<int16_t>;
  std::unique_ptr<AudioSampleFIFO> audio_sample_fifo;

private:
  static constexpr size_t MAX_AUDIOQ_SIZE = (5 * 16 * 1024);
  static constexpr size_t MAX_VIDEOQ_SIZE = (5 * 256 * 1024);
  static constexpr size_t VIDEO_PACKET_QUEUE_SIZE = 10;
  static constexpr size_t AUDIO_PACKET_QUEUE_SIZE = 10;
  static constexpr size_t VIDEO_PICTURE_QUEUE_SIZE = 2;
  static constexpr size_t AUDIO_FRAME_QUEUE_SIZE = 10;
  static constexpr int FF_SEEK_PACKET_INDEX = 10;

  std::atomic<DecodeEngineState> state_{DecodeEngineState::kStopped};
  bool is_opened_ok_{false};

  std::unique_ptr<std::thread> demux_thread_{nullptr};
  std::unique_ptr<std::thread> video_decode_thread_{nullptr};
  std::unique_ptr<std::thread> audio_decode_thread_{nullptr};

  std::mutex seek_mut_;
  std::atomic<bool> seek_req_{false};
  int seek_flags_{0};
  int64_t seek_pos_{0}; // AV_TIME_BASE based
  int64_t seek_rel_{0}; // AV_TIME_BASE based
  AVPacket seek_packet_{.stream_index = FF_SEEK_PACKET_INDEX}; // as a event

  double last_audio_frame_pts_{0}; // based on seconds

  VideoOutputConfig video_config_{};
  AudioOutputConfig audio_config_{};

  void startThreads() {
    demux_thread_ = std::make_unique<std::thread>(
        &FFMPEGDecodeEngine::demuxThreadFunction, this);
    video_decode_thread_ = std::make_unique<std::thread>(
        &FFMPEGDecodeEngine::videoDecodeThreadFunction, this);
    audio_decode_thread_ = std::make_unique<std::thread>(
        &FFMPEGDecodeEngine::audioDecodeThreadFunction, this);
  }

  void clearQueue() {
    video_packet_sync_que.clear();
    video_frame_sync_que.clear();

    audio_packet_sync_que.clear();
    audio_frame_sync_que.clear();
  }

  void stopThreads() {
    if (demux_thread_ != nullptr && demux_thread_->joinable()) {
      demux_thread_->join();
      demux_thread_ = nullptr;
    }

    if (video_decode_thread_ != nullptr && video_decode_thread_->joinable()) {
      video_decode_thread_->join();
      video_decode_thread_ = nullptr;
    }

    if (audio_decode_thread_ != nullptr && audio_decode_thread_->joinable()) {
      audio_decode_thread_->join();
      audio_decode_thread_ = nullptr;
    }
  }

  void findStreams() {
    video_stream_index = demuxer.getVideoStreamIndex();
    audio_stream_index = demuxer.getAudioStreamIndex();

    if (video_stream_index != -1) {
      video_stream = demuxer.getStream(video_stream_index);
    }

    if (audio_stream_index != -1) {
      audio_stream = demuxer.getStream(audio_stream_index);
    }
  }

  int prepareCodecs() {
    int ret = 0;
    if (video_stream != nullptr) {
      ret = video_codec.prepare(video_stream->codecpar->codec_id,
                                video_stream->codecpar);
      RETURN_IF_ERROR_LOG(ret, "Prepare video codec failed\n");

      video_codec_ctx = video_codec.getCodecContext();
    }

    if (audio_stream != nullptr) {
      ret = audio_codec.prepare(audio_stream->codecpar->codec_id,
                                audio_stream->codecpar);
      RETURN_IF_ERROR_LOG(ret, "Prepare audio codec failed\n");

      audio_codec_ctx = audio_codec.getCodecContext();
    }
    return ret;
  }

  void initVideoOutputConfig() {
    if (video_codec_ctx != nullptr) {
      video_config_.width = video_codec_ctx->width;
      video_config_.height = video_codec_ctx->height;
      video_config_.pixel_format = video_codec_ctx->pix_fmt;
    }
  }

  void initAudioOutputConfig() {
    if (audio_codec_ctx != nullptr) {
      audio_config_.num_channels = audio_codec_ctx->channels;
      audio_config_.channel_layout = audio_codec_ctx->channel_layout;
      audio_config_.sample_rate = audio_codec_ctx->sample_rate;
    }
  }

  int prepareImageConverter(VideoOutputConfig *video_config) {
    assert(video_config != nullptr);
    return img_conv.prepare(video_codec_ctx->width, video_codec_ctx->height,
                            video_codec_ctx->pix_fmt, video_config->width,
                            video_config->height, video_config->pixel_format,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
  }

  int prepareAudioResampler(AudioOutputConfig *audio_config) {
    int max_frames_size = audio_config->sample_rate * 3; // 3s samples
    return audio_resampler.prepare(
        audio_codec_ctx->channels, audio_config->num_channels,
        audio_codec_ctx->channel_layout, audio_config->channel_layout,
        audio_codec_ctx->sample_rate, audio_config->sample_rate,
        audio_codec_ctx->sample_fmt, AV_SAMPLE_FMT_S16, max_frames_size);
  }

  int seekToNearestPreIFramePos(int64_t seek_pos, int64_t seek_rel,
                                int seek_flags) const {
    auto min_ts = (seek_rel > 0) ? (seek_pos - seek_rel + 2) : (INT64_MIN);
    auto max_ts = (seek_rel < 0) ? (seek_pos - seek_rel - 2) : (seek_pos);

    return avformat_seek_file(demuxer.getFormatContext(), -1, min_ts, seek_pos,
                              max_ts, seek_flags);
  }

  void clearPacketQueueAndPushSeekEvent(int64_t seek_pos) {
    seek_packet_.pos = seek_pos;

    if (video_stream_index >= 0) {
      video_packet_sync_que.clear();
      video_packet_sync_que.tryPush(&seek_packet_); // send packet as a event
    }

    if (audio_stream_index >= 0) {
      audio_packet_sync_que.clear();
      audio_packet_sync_que.tryPush(&seek_packet_);
    }
  }

  void doSeekInDemuxThread() {
    int64_t seek_pos = 0;
    int64_t seek_rel = 0;
    int seek_flags = 0;
    {
      std::lock_guard lg(seek_mut_);
      seek_pos = seek_pos_;
      seek_rel = seek_rel_;
      seek_flags = seek_flags_;
    }

    int ret = seekToNearestPreIFramePos(seek_pos, seek_rel, seek_flags);

    if (ret < 0) {
      fprintf(stderr, "%s: error while seeking %s\n",
              demuxer.getFormatContext()->url, av_err2str(ret));
    } else {
      clearPacketQueueAndPushSeekEvent(seek_pos);
    }
  }

  void demuxThreadFunction() {
    using namespace std::literals;

    AVPacket *packet{nullptr};
    int ret = 0;
    for (;;) {
      if (canExit()) {
        break;
      }

      if (seek_req_) {
        doSeekInDemuxThread();
        seek_req_ = false;
      }

      // sleep if packet size in queue is very large
      if (video_packet_sync_que.totalPacketSize() >= MAX_VIDEOQ_SIZE ||
          audio_packet_sync_que.totalPacketSize() >= MAX_AUDIOQ_SIZE) {
        std::this_thread::sleep_for(10ms);
        continue;
      }

      // scope this to call av_packet_unref if exit scope
      {
        std::tie(ret, packet) = demuxer.readPacket();
        ON_SCOPE_EXIT([&packet] { av_packet_unref(packet); });

        // read end of file, just exit this thread
        if (ret == AVERROR_EOF || packet == nullptr) {
          printf("demux thread read eof\n");
          break;
        }

        if (packet->stream_index == video_stream_index) {
          video_packet_sync_que.tryPush(packet);
        } else if (packet->stream_index == audio_stream_index) {
          audio_packet_sync_que.tryPush(packet);
        }
      }
    }
  }

  int videoDecodeThreadFunction() {
    AVFrame *frame = av_frame_alloc();
    if (frame == nullptr) {
      printf("Could not allocate frame on video decode thread.\n");
      return -1;
    }
    ON_SCOPE_EXIT([&frame] {
      av_frame_unref(frame);
      av_frame_free(&frame);
    });

    bool seeking_flag = false;
    int64_t target_seek_pos_avtimebase = 0;
    auto stream_time_base = video_stream->time_base;

    for (;;) {
      if (canExit()) {
        break;
      }

      if (video_packet_sync_que.size() != 0) {
        int ret = decodePacketAndPushToFrameQueue(
            video_packet_sync_que, video_codec, frame, video_frame_sync_que,
            seeking_flag, target_seek_pos_avtimebase, stream_time_base);

        RETURN_IF_ERROR_LOG(ret, "decode audio packet failed\n");
      }
    }
    return 0;
  }

  int audioDecodeThreadFunction() {
    AVFrame *frame = av_frame_alloc();
    if (frame == nullptr) {
      printf("Could not allocate frame.\n");
      return -1;
    }
    ON_SCOPE_EXIT([&frame] {
      av_frame_unref(frame);
      av_frame_free(&frame);
    });

    bool seeking_flag = false;
    int64_t target_seek_pos_avtimebase = 0;
    auto stream_time_base = audio_stream->time_base;

    for (;;) {
      if (canExit()) {
        break;
      }

      if (audio_packet_sync_que.size() != 0) {
        int ret = decodePacketAndPushToFrameQueue(
            audio_packet_sync_que, audio_codec, frame, audio_frame_sync_que,
            seeking_flag, target_seek_pos_avtimebase, stream_time_base);
        RETURN_IF_ERROR_LOG(ret, "decode audio packet failed\n");
      }
    }
    return 0;
  }

  int decodePacketAndPushToFrameQueue(WaitablePacketQueue &packet_queue,
                                      FFMPEGCodec &codec, AVFrame *out_frame,
                                      WaitableFrameQueue &out_frame_queue,
                                      bool &seeking_flag,
                                      int64_t &target_seek_pos_avtimebase,
                                      AVRational stream_time_base) {
    auto *pkt = packet_queue.waitAndPop();
    ON_SCOPE_EXIT([&pkt] {
      if (pkt != nullptr) {
        av_packet_unref(pkt);
        av_packet_free(&pkt);
      }
    });

    // seek stuff here
    if (pkt->stream_index == FF_SEEK_PACKET_INDEX) {
      avcodec_flush_buffers(codec.getCodecContext());
      out_frame_queue.clear();

      seeking_flag = true;
      target_seek_pos_avtimebase = pkt->pos;
      return 0;
    }

    int ret = codec.sendPacketToCodec(pkt);
    if (ret < 0) {
      printf("Error sending packet for decoding %s.\n", av_err2str(ret));
      return -1;
    }

    while (ret >= 0) {
      ret = codec.receiveFrame(out_frame);
      ON_SCOPE_EXIT([&out_frame] { av_frame_unref(out_frame); });

      // need more packet
      if (ret == AVERROR(EAGAIN)) {
        break;
      } else if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
        // EOF exit loop
        break;
      } else if (ret < 0) {
        printf("Error while decoding.\n");
        return -1;
      }

      if (seeking_flag) {
        auto cur_frame_pts_avtimebase =
            av_rescale_q(out_frame->pts, stream_time_base, AV_TIME_BASE_Q);
        if (cur_frame_pts_avtimebase < target_seek_pos_avtimebase) {
          break;
        } else {
          seeking_flag = false;
        }
      }

      /**
       * wait if queue is full, this thread never quit if stuck here
       * you need flush queue before stop this thread
       */
      if (codec.getCodecContext()->codec_type ==
          AVMediaType::AVMEDIA_TYPE_VIDEO) {
        convertVideoFrameAndPushToQueue(out_frame, out_frame_queue);
      } else if (codec.getCodecContext()->codec_type ==
                 AVMediaType::AVMEDIA_TYPE_AUDIO) {

        /**
         * push audio frame to queue directly
         * resample will be done when call pullAudioSamples()
         */
        out_frame_queue.waitAndPush(out_frame);
      }
    }

    return 0;
  };

  void convertVideoFrameAndPushToQueue(AVFrame *frame,
                                       WaitableFrameQueue &out_frame_queue) {
    //    out_frame_queue.waitAndPush(frame);
    //    return;
    auto r = img_conv.convert(frame);
    if (r.first != -1 && r.second != nullptr) {
      std::cout << "convert pts:" << frame->pts << std::endl;
      out_frame_queue.waitAndPush(r.second);
    } else {
      printf("convert failed");
      out_frame_queue.waitAndPush(frame);
    }
  }

  bool canExit() const {
    auto s = state();
    return s == DecodeEngineState::kStopping ||
           s == DecodeEngineState::kStopped;
  }
};

} // namespace ffmpeg_utils

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_DECODE_ENGINE_H
