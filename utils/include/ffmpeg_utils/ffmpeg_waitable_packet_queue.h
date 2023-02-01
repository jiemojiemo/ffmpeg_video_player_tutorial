//
// Created by user on 1/20/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_WAITABLE_PACKET_QUEUE_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_WAITABLE_PACKET_QUEUE_H
#pragma once

#include "ffmpeg_utils/ffmpeg_headers.h"
#include <queue>
#include <thread>

namespace ffmpeg_utils {
class WaitablePacketQueue {
public:
  explicit WaitablePacketQueue(size_t queue_size = INT16_MAX)
      : queue_size_(queue_size) {}

  ~WaitablePacketQueue() { clear(); }

  size_t capacity() const { return queue_size_; }
  size_t size() const {
    std::lock_guard lg(mut_);
    return packet_que_.size();
  }

  bool tryPush(AVPacket *pkt) {
    std::lock_guard lg(mut_);
    if (packet_que_.size() >= queue_size_) {
      return false;
    }

    auto new_pkt = av_packet_clone(pkt);
    packet_que_.push(new_pkt);
    total_pkt_size_ += new_pkt->size;
    data_cond_.notify_one();
    return true;
  }

  void waitAndPush(AVPacket *pkt) {
    std::unique_lock<std::mutex> lk(mut_);
    has_space_cond_.wait(lk,
                         [this]() { return packet_que_.size() < queue_size_; });
    auto new_pkt = av_packet_clone(pkt);
    packet_que_.push(new_pkt);
    total_pkt_size_ += new_pkt->size;
    data_cond_.notify_one();
  }

  AVPacket *tryPop() {
    std::lock_guard lg(mut_);
    if (!packet_que_.empty()) {
      auto *return_pkt = packet_que_.front();
      packet_que_.pop();
      total_pkt_size_ -= return_pkt->size;
      has_space_cond_.notify_one();
      return return_pkt;
    }
    return nullptr;
  }

  AVPacket *waitAndPop() {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this]() { return !packet_que_.empty(); });
    auto *return_pkt = packet_que_.front();
    packet_que_.pop();
    total_pkt_size_ -= return_pkt->size;
    has_space_cond_.notify_one();
    return return_pkt;
  }

  void clear() {
    std::lock_guard lg(mut_);

    for (; !packet_que_.empty();) {
      auto *pkt = packet_que_.front();
      av_packet_unref(pkt);
      av_packet_free(&pkt);

      packet_que_.pop();
    }

    total_pkt_size_ = 0;
    has_space_cond_.notify_all();
  }

  size_t totalPacketSize() const { return total_pkt_size_; }

private:
  mutable std::mutex mut_;
  std::condition_variable data_cond_;
  std::condition_variable has_space_cond_;
  std::queue<AVPacket *> packet_que_;
  const size_t queue_size_{0};
  std::atomic<size_t> total_pkt_size_{0};
};

} // namespace ffmpeg_utils

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_WAITABLE_PACKET_QUEUE_H
