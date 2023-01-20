//
// Created by user on 1/20/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_WAITABLE_QUEUE_H
#define FFMPEG_VIDEO_PLAYER_WAITABLE_QUEUE_H
#pragma once
#include <queue>
#include <thread>
namespace utils {
template <typename T> class WaitableQueue {
public:
  WaitableQueue(size_t queue_size = INT16_MAX) : queue_size_(queue_size) {}
  bool empty() const {
    std::lock_guard lg(mut_);
    return q_.empty();
  }

  bool full() const {
    std::lock_guard lg(mut_);
    return q_.size() == queue_size_;
  }

  size_t capacity() const { return queue_size_; }

  size_t size() const {
    std::lock_guard lg(mut_);
    return q_.size();
  }

  void push(T new_value) {
    std::lock_guard lg(mut_);
    if (q_.size() >= queue_size_) {
      return;
    }
    q_.push(std::move(new_value));
    data_cond_.notify_one();
  }

  bool try_push(T new_value) {
    std::lock_guard lg(mut_);
    if (q_.size() >= queue_size_) {
      return false;
    }

    q_.push(std::move(new_value));
    data_cond_.notify_one();
    return true;
  }

  bool try_pop(T &value) {
    std::lock_guard lg(mut_);
    if (q_.empty()) {
      return false;
    }

    value = std::move(q_.front());
    q_.pop();
    has_space_cond_.notify_one();
    return true;
  }

  std::shared_ptr<T> try_pop() {
    std::lock_guard lg(mut_);
    if (q_.empty()) {
      return std::shared_ptr<T>();
    }

    auto res = std::make_shared<T>(std::move(q_.front()));
    q_.pop();
    has_space_cond_.notify_one();
    return res;
  }

  void wait_and_pop(T &value) {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this]() { return !q_.empty(); });
    value = std::move(q_.front());
    q_.pop();
    has_space_cond_.notify_one();
  }

  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this]() { return !q_.empty(); });
    auto res = std::make_shared<T>(std::move(q_.front()));
    q_.pop();
    has_space_cond_.notify_one();
    return res;
  }

  void wait_and_push(T new_value) {
    std::unique_lock<std::mutex> lk(mut_);
    has_space_cond_.wait(lk, [this]() { return q_.size() < queue_size_; });
    q_.push(std::move(new_value));
  }

  void flush() {
    std::lock_guard lg(mut_);
    for (; !q_.empty();) {
      q_.pop();
    }
    has_space_cond_.notify_one();
  }

private:
  mutable std::mutex mut_;
  std::condition_variable data_cond_;
  std::condition_variable has_space_cond_;
  std::queue<T> q_;
  size_t queue_size_{0};
};
} // namespace utils

#endif // FFMPEG_VIDEO_PLAYER_WAITABLE_QUEUE_H
