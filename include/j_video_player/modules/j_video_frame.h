//
// Created by user on 11/7/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_VIDEO_FRAME_H
#define FFMPEG_VIDEO_PLAYER_J_VIDEO_FRAME_H
#include "j_video_player/ffmpeg_utils/ffmpeg_headers.h"

namespace j_video_player {
class VideoFrame {
public:
  VideoFrame() : f(av_frame_alloc()) {}

  ~VideoFrame() {
    if (f) {
      av_frame_unref(f);
      av_frame_free(&f);
    }
  }

  AVFrame *f{nullptr};
};

}

#endif // FFMPEG_VIDEO_PLAYER_J_VIDEO_FRAME_H
