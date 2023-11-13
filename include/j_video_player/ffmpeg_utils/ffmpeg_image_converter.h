//
// Created by user on 7/20/22.
//

#ifndef FFMPEG_AND_SDL_TUTORIAL_FFMPEG_IMAGE_CONVERTER_H
#define FFMPEG_AND_SDL_TUTORIAL_FFMPEG_IMAGE_CONVERTER_H
#pragma once
#include "ffmpeg_common_utils.h"
#include "ffmpeg_headers.h"
#include "j_video_player/modules/j_frame.h"
#include <memory>
#include <utility>

namespace ffmpeg_utils {

class FFMPEGImageConverter {
public:
  ~FFMPEGImageConverter() { clear(); }
  int prepare(int srcW, int srcH, enum AVPixelFormat srcFormat, int dstW,
              int dstH, enum AVPixelFormat dstFormat, int flags,
              SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param) {

    // clear internal state before another prepare
    clear();

    sws_ctx = sws_getContext(srcW, srcH, srcFormat, dstW, dstH, dstFormat,
                             flags, srcFilter, dstFilter, param);
    if (sws_ctx == nullptr) {
      LOGE("Cannot initialize the conversion context\n");
      return -1;
    }

    frame = av_frame_alloc();
    frame->width = dstW;
    frame->height = dstH;
    frame->format = dstFormat;
    frame->format = (int)dstFormat;
    frame->width = dstW;
    frame->height = dstH;
    frame->channels = 0;
    frame->channel_layout = 0;
    frame->nb_samples = 0;
    return av_frame_get_buffer(frame, 16);
  }

  std::pair<int, AVFrame *> convert(const AVFrame *in_frame) {
    if (sws_ctx == nullptr || frame == nullptr) {
      return {-1, nullptr};
    }

    frame->pict_type = in_frame->pict_type;
    frame->pts = in_frame->pts;
    frame->pkt_dts = in_frame->pkt_dts;
    frame->key_frame = in_frame->key_frame;
    frame->coded_picture_number = in_frame->coded_picture_number;
    frame->display_picture_number = in_frame->display_picture_number;

    int output_height = sws_scale(
        sws_ctx, (uint8_t const *const *)in_frame->data, in_frame->linesize, 0,
        in_frame->height, frame->data, frame->linesize);

    return {output_height, frame};
  }

  std::shared_ptr<j_video_player::Frame>
  convert2(const std::shared_ptr<j_video_player::Frame> &input_frame) {
    if (sws_ctx == nullptr || frame == nullptr) {
      return nullptr;
    }

    auto *in_frame = input_frame->f;
    frame->pict_type = in_frame->pict_type;
    frame->pts = in_frame->pts;
    frame->pkt_dts = in_frame->pkt_dts;
    frame->key_frame = in_frame->key_frame;
    frame->coded_picture_number = in_frame->coded_picture_number;
    frame->display_picture_number = in_frame->display_picture_number;

    sws_scale(sws_ctx, (uint8_t const *const *)in_frame->data,
              in_frame->linesize, 0, in_frame->height, frame->data,
              frame->linesize);

    return std::make_shared<j_video_player::Frame>(frame,
                                                   input_frame->time_base);
  }

  void clear() {
    if (sws_ctx != nullptr) {
      sws_freeContext(sws_ctx);
      sws_ctx = nullptr;
    }
    if (frame != nullptr) {
      av_frame_free(&frame);
      frame = nullptr;
    }
  }

  constexpr static int kAlign = 16;
  struct SwsContext *sws_ctx{nullptr};
  AVFrame *frame{nullptr};
  uint8_t *frame_buffer{nullptr};
};
} // namespace ffmpeg_utils

#endif // FFMPEG_AND_SDL_TUTORIAL_FFMPEG_IMAGE_CONVERTER_H
