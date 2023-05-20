//
// Created by user on 1/20/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_WAITABLE_FRAME_QUEUE_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_WAITABLE_FRAME_QUEUE_H
#pragma once
#include "ffmpeg_headers.h"
#include <queue>
#include <thread>

namespace ffmpeg_utils {
class WaitableFrameQueue {
public:
  explicit WaitableFrameQueue(size_t queue_size) : queue_size_(queue_size) {}

  ~WaitableFrameQueue() { clear(); }

  size_t capacity() const { return queue_size_; }
  size_t size() const {
    std::lock_guard lg(mut_);
    return que_.size();
  }

  bool tryPush(AVFrame *pkt) {
    std::lock_guard lg(mut_);
    if (que_.size() >= queue_size_) {
      return false;
    }

    auto new_frame = av_frame_clone(pkt);
    que_.push(new_frame);
    data_cond_.notify_one();
    return true;
  }

  void waitAndPush(AVFrame *pkt) {
    std::unique_lock<std::mutex> lk(mut_);
    has_space_cond_.wait(lk, [this]() { return que_.size() < queue_size_; });
    auto new_frame = av_frame_clone(pkt);
    que_.push(new_frame);
    data_cond_.notify_one();
  }

  AVFrame *tryPop() {
    std::lock_guard lg(mut_);
    if (!que_.empty()) {
      auto *return_pkt = que_.front();
      que_.pop();
      has_space_cond_.notify_one();
      return return_pkt;
    }
    return nullptr;
  }

  AVFrame *waitAndPop() {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this]() { return !que_.empty(); });
    auto *return_pkt = que_.front();
    que_.pop();
    has_space_cond_.notify_one();
    return return_pkt;
  }

  void clear() {
    std::lock_guard lg(mut_);

    for (; !que_.empty();) {
      auto *pkt = que_.front();
      av_frame_unref(pkt);
      av_frame_free(&pkt);

      que_.pop();
    }

    has_space_cond_.notify_all();
  }

  AVFrame *front() {
    // TODO: fix this if queue stuck on waitAndPush
    std::lock_guard lg(mut_);
    if (!que_.empty()) {
      return que_.front();
    } else {
      return nullptr;
    }
  }

private:
  mutable std::mutex mut_;
  std::condition_variable data_cond_;
  std::condition_variable has_space_cond_;
  std::queue<AVFrame *> que_;
  const size_t queue_size_{0};
};

} // namespace ffmpeg_utils

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_WAITABLE_FRAME_QUEUE_H
