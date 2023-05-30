//
// Created by user on 1/17/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_CODEC_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_CODEC_H
#include "ffmpeg_common_utils.h"
#include "ffmpeg_headers.h"

namespace ffmpeg_utils {
class FFMPEGCodec {
public:
  ~FFMPEGCodec() { close(); }

  int prepare(enum AVCodecID id, const AVCodecParameters *par) {
    codec_ = avcodec_find_decoder(id);
    if (codec_ == nullptr) {
      return -1;
    }

    codec_context_ = avcodec_alloc_context3(codec_);
    if (codec_context_ == nullptr) {
      return -1;
    }

    int ret = 0;
    if (par) {
      ret = avcodec_parameters_to_context(codec_context_, par);
      RETURN_IF_ERROR(ret);
    }

    ret = avcodec_open2(codec_context_, codec_, NULL);
    return ret;
  }

  int sendPacketToCodec(AVPacket *packet) {
    int ret = avcodec_send_packet(codec_context_, packet);
    if (ret < 0) {
      printf("Error sending packet for decoding %s.\n", av_err2str(ret));
      return ret;
    }
    return 0;
  }

  bool isValid() const { return codec_context_ != nullptr && codec_ != nullptr; }

  int receiveFrame(AVFrame *frame) {
    return avcodec_receive_frame(codec_context_, frame);
  }

  void flush_buffers(){
    return avcodec_flush_buffers(codec_context_);
  }

  const AVCodec *getCodec() const { return codec_; }
  AVCodec *getCodec() { return codec_; }
  const AVCodecContext *getCodecContext() const { return codec_context_; }
  AVCodecContext *getCodecContext() { return codec_context_; }

private:
  void close() {
    if (codec_context_ != nullptr) {
      avcodec_free_context(&codec_context_);
    }
  }
  AVCodec *codec_{nullptr};
  AVCodecContext *codec_context_{nullptr};
};
} // namespace ffmpeg_utils

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_CODEC_H
