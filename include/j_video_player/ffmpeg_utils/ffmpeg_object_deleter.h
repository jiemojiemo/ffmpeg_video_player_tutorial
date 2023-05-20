//
// Created by user on 7/26/22.
//

#ifndef FFMPEG_AND_SDL_TUTORIAL_FFMPEG_OBJECT_DELETER_H
#define FFMPEG_AND_SDL_TUTORIAL_FFMPEG_OBJECT_DELETER_H

#pragma once
#include "ffmpeg_utils/ffmpeg_headers.h"

namespace ffmpeg_utils {
class FrameDeleter {
public:
  void operator()(AVFrame *f) const {
    if (f != nullptr) {
      av_frame_unref(f);
      av_frame_free(&f);
    }
  };
};

class PacketDeleter {
public:
  void operator()(AVPacket *p) const {
    if (p != nullptr) {
      av_packet_unref(p);
      av_packet_free(&p);
    }
  };
};

} // namespace ffmpeg_utils

#endif // FFMPEG_AND_SDL_TUTORIAL_FFMPEG_OBJECT_DELETER_H
