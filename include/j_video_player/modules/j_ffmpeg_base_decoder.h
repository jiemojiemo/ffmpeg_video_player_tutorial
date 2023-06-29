//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_FFMPEG_BASE_DECODER_H
#define FFMPEG_VIDEO_PLAYER_J_FFMPEG_BASE_DECODER_H

#include "j_video_player/ffmpeg_utils/ffmpeg_codec.h"
#include "j_video_player/ffmpeg_utils/ffmpeg_demuxer.h"
#include "j_video_player/ffmpeg_utils/ffmpeg_headers.h"
#include "j_video_player/modules/j_i_decoder.h"
#include "j_video_player/utils/scope_guard.h"
#include <thread>
namespace j_video_player {
enum class DecoderState { kDecoding, kPaused, kStopped, kSeeking };

class FFMPEGBaseDecoder : public IDecoder {
public:
  ~FFMPEGBaseDecoder() override = default;
  void start() override {
    if (!isDecodingThreadRunning()) {
      startDecodingThread();
    }
    state_ = DecoderState::kDecoding;
  }
  void pause() override { state_ = DecoderState::kPaused; }
  void stop() override {
    state_ = DecoderState::kStopped;
    uninit();
  }
  void seek(float seek_pos) override {
    if (!isDecodingThreadRunning()) {
      return;
    }
    if (seek_pos < 0) {
      seek_pos = 0;
    }
    if (seek_pos > getDuration()) {
      seek_pos = getDuration();
    }

    seek_rel_ =
        static_cast<int64_t>((seek_pos - getCurrentPosition()) * AV_TIME_BASE);
    seek_pos_ = static_cast<int64_t>(seek_pos * AV_TIME_BASE);
    state_ = DecoderState::kSeeking;
    seeking_ = true;
    printf("seek_pos_:%lld\n", seek_pos_.load());
  }
  float getDuration() override { return duration_; }
  float getCurrentPosition() override {
    return position_ / (float)AV_TIME_BASE;
  }
  bool isPlaying() const override { return state_ == DecoderState::kDecoding; }
  DecoderState getState() const { return state_.load(); }
  std::string getURL() const { return url_; }
  AVMediaType getMediaType() const { return media_type_; }
  AVCodecContext *getCodecContext() const {
    if (codec_) {
      return codec_->getCodecContext();
    }
    return nullptr;
  }

  // clear cache after seek
  virtual void clearCache() {}

  virtual void onPrepareDecoder() = 0;
  virtual void OnDecoderDone() = 0;
  virtual void OnFrameAvailable(AVFrame *frame) = 0;

  int init(const std::string &url, AVMediaType media_type) {
    url_ = url;
    media_type_ = media_type;

    int ret = initDemux(url);
    RETURN_IF_ERROR(ret);

    ret = initCodec(media_type);
    RETURN_IF_ERROR(ret);

    frame_ = av_frame_alloc();

    return 0;
  }

  bool isInitSucc() const { return demux_->isValid() && codec_->isValid(); }

  void uninit() {
    stopDecodingThread();
    releaseFFMPEGResources();
  }

  void releaseFFMPEGResources() {
    demux_ = nullptr;
    codec_ = nullptr;
    if (frame_) {
      av_frame_unref(frame_);
      av_frame_free(&frame_);
      frame_ = nullptr;
    }
  }

  void stopDecodingThread() {
    if (decode_thread_) {
      state_ = DecoderState::kStopped;
      if (decode_thread_->joinable()) {
        decode_thread_->join();
        decode_thread_ = nullptr;
      }
    }
  }

private:
  static void decodingThread(FFMPEGBaseDecoder *decoder) {
    printf("decodingThread\n");
    do {
      if (!decoder->isInitSucc()) {
        break;
      }
      decoder->onPrepareDecoder();
      decoder->DecodingLoop();
    } while (false);

    decoder->releaseFFMPEGResources();
    decoder->OnDecoderDone();
  }

  void startDecodingThread() {
    decode_thread_ =
        std::make_unique<std::thread>(&FFMPEGBaseDecoder::decodingThread, this);
  }

  void DecodingLoop() {
    using namespace std::literals;
    printf("DecodingLoop start...\n");
    for (;;) {
      for (; state_ == DecoderState::kPaused;) {
        std::this_thread::sleep_for(10ms);
      }

      if (state_ == DecoderState::kStopped) {
        break;
      }

      if (state_ == DecoderState::kSeeking) {
        seek();
        state_ = DecoderState::kDecoding;
      }

      if (decodingOnePacket() != 0) {
        state_ = DecoderState::kPaused;
      }
    }
  }

  int initDemux(const std::string &url) {
    demux_ = std::make_unique<ffmpeg_utils::FFmpegDmuxer>();
    int ret = demux_->openFile(url);
    RETURN_IF_ERROR_LOG(ret, "demux_->openFile failed\n");
    duration_ = demux_->getFormatContext()->duration / (float)AV_TIME_BASE;
    return ret;
  }

  int initCodec(AVMediaType media_type) {
    codec_ = std::make_unique<ffmpeg_utils::FFmpegCodec>();
    stream_index_ = (media_type == AVMEDIA_TYPE_VIDEO)
                        ? demux_->getVideoStreamIndex()
                        : demux_->getAudioStreamIndex();
    auto *av_stream = demux_->getStream(stream_index_);
    if (av_stream == nullptr) {
      printf("av_stream is nullptr\n");
      return -1;
    }
    int ret =
        codec_->prepare(av_stream->codecpar->codec_id, av_stream->codecpar);
    RETURN_IF_ERROR_LOG(ret, "codec_->prepare failed\n");

    return ret;
  }

  bool isDecodingThreadRunning() const {
    return decode_thread_ != nullptr && decode_thread_->joinable();
  }

  int decodingOnePacket() {
    int ret = 0;
    AVPacket *pkt = nullptr;
    std::tie(ret, pkt) = demux_->readPacket();
    ON_SCOPE_EXIT([&pkt] { av_packet_unref(pkt); });

    if (pkt == nullptr) {
      return -1;
    }

    if (stream_index_ != pkt->stream_index) {
      return 0;
    }

    if (ret != 0) {
      printf("readPacket failed\n");
      return -1;
    }

    ret = codec_->sendPacketToCodec(pkt);
    if (ret == AVERROR_EOF) {
      printf("sendPacketToCodec AVERROR_EOF\n");
      return -1;
    }

    // 1 packet may contain multiple frames
    for (;;) {
      auto ret0 = codec_->receiveFrame(frame_);
      ON_SCOPE_EXIT([this] { av_frame_unref(frame_); });
      if (ret0 == 0) {
        updateTimeStamp();

        // do accurate seek
        if (skipThisFrame()) {
          continue;
        }

        OnFrameAvailable(frame_);
      } else {
        break;
      }
    }

    return 0;
  };

  void updateTimeStamp() {
    // change frame pts to based on AV_TIME_BASE
    // so it easy to video/audio sync
    frame_->pts =
        av_rescale_q(frame_->pts, demux_->getStream(stream_index_)->time_base,
                     AV_TIME_BASE_Q);
    position_ = frame_->pts;
  }

  bool skipThisFrame() {
    if (seeking_) {
      auto current_pts = position_.load();
      // discard frames before seek position
      if (current_pts < seek_pos_) {
        return true;
      } else {
        seeking_ = false;
        return false;
      }
    }

    return false;
  }

  void seek() {
    int ret = seekToTargetPosition(seek_pos_, seek_rel_);
    if (ret < 0) {
      printf("seekToTargetPosition failed\n");
    } else {
      codec_->flush_buffers();
      clearCache();
    }
  }

  int seekToTargetPosition(int64_t seek_pos, int64_t seek_rel) {
    auto seek_flags = 0;
    if (seek_rel < 0) {
      seek_flags = AVSEEK_FLAG_BACKWARD;
    }
    auto min_ts = (seek_rel > 0) ? (seek_pos - seek_rel + 2) : (INT64_MIN);
    auto max_ts = (seek_rel < 0) ? (seek_pos - seek_rel - 2) : (seek_pos);
    return demux_->seek(min_ts, seek_pos, max_ts, seek_flags);
  }

  std::string url_{};
  AVMediaType media_type_{AVMEDIA_TYPE_UNKNOWN};
  std::unique_ptr<ffmpeg_utils::FFmpegDmuxer> demux_;
  std::unique_ptr<ffmpeg_utils::FFmpegCodec> codec_;
  std::unique_ptr<std::thread> decode_thread_;
  std::atomic<DecoderState> state_{DecoderState::kStopped};
  std::atomic<int64_t> position_{0};
  std::atomic<int64_t> seek_pos_{-1};
  std::atomic<int64_t> seek_rel_{0};
  std::atomic<bool> seeking_{false};
  double duration_{0.0f};

  AVFrame *frame_ = nullptr;
  int stream_index_{-1};
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_FFMPEG_BASE_DECODER_H
