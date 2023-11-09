//
// Created by user on 11/7/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_FRAME_H
#define FFMPEG_VIDEO_PLAYER_J_FRAME_H
#include "j_video_player/ffmpeg_utils/ffmpeg_headers.h"

namespace j_video_player {
class Frame {
public:
  Frame() : f(av_frame_alloc()) {}

  ~Frame() {
    if (f) {
      av_frame_unref(f);
      av_frame_free(&f);
    }
  }

  AVFrame *f{nullptr};
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_FRAME_H
