//
// Created by user on 1/19/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_PACKET_QUEUE_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_PACKET_QUEUE_H
#pragma once
#include "ffmpeg_headers.h"
#include <queue>
#include <thread>
namespace ffmpeg_utils {
class PacketQueue {
public:
  ~PacketQueue() {
    std::lock_guard lg(mut_);

    for (; !packet_que_.empty();) {
      auto *pkt = packet_que_.front();
      av_packet_unref(pkt);
      av_packet_free(&pkt);

      packet_que_.pop();
    }
  }

  void cloneAndPush(AVPacket *pkt) {
    std::lock_guard lg(mut_);
    auto new_pkt = av_packet_clone(pkt);
    packet_que_.push(new_pkt);
    total_pkt_size_ += new_pkt->size;
  }

  AVPacket *pop() {
    std::lock_guard lg(mut_);
    if (!packet_que_.empty()) {
      auto *return_pkt = packet_que_.front();
      packet_que_.pop();
      total_pkt_size_ -= return_pkt->size;
      return return_pkt;
    }
    return nullptr;
  }

  size_t size() const {
    std::lock_guard lg(mut_);
    return packet_que_.size();
  }

  size_t totalPacketSize() const { return total_pkt_size_; }

private:
  std::queue<AVPacket *> packet_que_;
  mutable std::mutex mut_;
  size_t total_pkt_size_{0};
};
} // namespace ffmpeg_utils

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_PACKET_QUEUE_H
