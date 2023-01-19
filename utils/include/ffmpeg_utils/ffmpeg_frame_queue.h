//
// Created by user on 1/19/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_FRAME_QUEUE_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_FRAME_QUEUE_H

#include "ffmpeg_utils/ffmpeg_headers.h"
#include <queue>
#include <thread>
namespace ffmpeg_utils {
class FrameQueue {
public:
  ~FrameQueue() {
    std::lock_guard lg(mut_);

    for (; !frame_que_.empty();) {
      auto *f = frame_que_.front();
      av_frame_unref(f);
      av_frame_free(&f);

      frame_que_.pop();
    }
  }

  void cloneAndPush(AVFrame *frame) {
    std::lock_guard lg(mut_);
    auto new_frame = av_frame_clone(frame);
    frame_que_.push(new_frame);
  }

  AVFrame *pop() {
    std::lock_guard lg(mut_);
    if (!frame_que_.empty()) {
      auto *return_pkt = frame_que_.front();
      frame_que_.pop();
      return return_pkt;
    }
    return nullptr;
  }

  size_t size() const {
    std::lock_guard lg(mut_);
    return frame_que_.size();
  }

private:
  std::queue<AVFrame *> frame_que_;
  mutable std::mutex mut_;
};
} // namespace ffmpeg_utils

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_FRAME_QUEUE_H
