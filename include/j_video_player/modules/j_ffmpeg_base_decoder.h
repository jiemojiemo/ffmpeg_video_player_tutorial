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
enum class DecoderState {
  kDecoding,
  kPaused,
  kStopped,
};

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
  void seek(float pos) override {
    if (!isDecodingThreadRunning()) {
      return;
    }
    position_ = static_cast<int64_t>(pos * AV_TIME_BASE);
    state_ = DecoderState::kDecoding;
  }
  float getDuration() override { return 0; }
  float getCurrentPosition() override {
    return position_ / (float)AV_TIME_BASE;
  }
  DecoderState getState() const { return state_.load(); }
  std::string getURL() const { return url_; }
  AVMediaType getMediaType() const { return media_type_; }
  AVCodecContext *getCodecContext() const {
    if (codec_) {
      return codec_->getCodecContext();
    }
    return nullptr;
  }

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

      if (decodingOnePacket() != 0) {
        state_ = DecoderState::kPaused;
        break;
      }
    }
  }

  int initDemux(const std::string &url) {
    demux_ = std::make_unique<ffmpeg_utils::FFMPEGDemuxer>();
    int ret = demux_->openFile(url);
    RETURN_IF_ERROR_LOG(ret, "demux_->openFile failed\n");
    return ret;
  }

  int initCodec(AVMediaType media_type) {
    codec_ = std::make_unique<ffmpeg_utils::FFMPEGCodec>();
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

    while (true) {
      int ret0 = codec_->sendPacketToCodec(pkt);
      if (ret0 < 0) {
        printf("sendPacketToCodec failed\n");
        return ret0;
      }

      ret0 = codec_->receiveFrame(frame_);
      ON_SCOPE_EXIT([this] { av_frame_unref(frame_); });

      if (ret0 == AVERROR(EAGAIN)) {
        return 0;
      } else if (ret0 == AVERROR_EOF || ret0 == AVERROR(EINVAL)) {
        printf("sendPacketToCodec AVERROR_EOF||EINVAL\n");
        return -1;
      } else if (ret0 < 0) {
        printf("sendPacketToCodec failed\n");
        return -1;
      }

      if (ret0 == 0) {
        // change frame pts to based on AV_TIME_BASE
        // so it easy to video/audio sync
        frame_->pts = av_rescale_q(frame_->pts,
                                   demux_->getStream(stream_index_)->time_base,
                                   AV_TIME_BASE_Q);
        OnFrameAvailable(frame_);
      }

      return 0;
    }
  };

  std::string url_{};
  AVMediaType media_type_{AVMEDIA_TYPE_UNKNOWN};
  std::unique_ptr<ffmpeg_utils::FFMPEGDemuxer> demux_;
  std::unique_ptr<ffmpeg_utils::FFMPEGCodec> codec_;
  std::unique_ptr<std::thread> decode_thread_;
  std::atomic<DecoderState> state_{DecoderState::kStopped};
  std::atomic<int64_t> position_{0};

  AVFrame *frame_ = nullptr;
  int stream_index_{-1};
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_FFMPEG_BASE_DECODER_H
