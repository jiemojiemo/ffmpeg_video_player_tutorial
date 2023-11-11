//
// Created by user on 11/7/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_FRAME_H
#define FFMPEG_VIDEO_PLAYER_J_FRAME_H
#include "j_video_player/ffmpeg_utils/ffmpeg_headers.h"

namespace j_video_player {
class Frame {
public:
  explicit Frame(AVRational tb) : f(av_frame_alloc()), time_base(tb) {}

  ~Frame() {
    if (f) {
      av_frame_unref(f);
      av_frame_free(&f);
    }
  }

  /**
   * get the pts of the frame, based on AV_TIME_BASE
   * @return the pts of the frame
   */
  int64_t pts() const {
    return av_rescale_q(f->pts, time_base, AV_TIME_BASE_Q);
  }

  AVFrame *f{nullptr};
  AVRational time_base;
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_FRAME_H
