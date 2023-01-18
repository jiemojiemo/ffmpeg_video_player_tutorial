//
// Created by user on 1/19/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_AUDIO_PACKET_QUEUE_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_AUDIO_PACKET_QUEUE_H
#pragma once
#include "ffmpeg_utils/ffmpeg_headers.h"
#include <queue>
#include <thread>
namespace ffmpeg_utils {
class AudioPacketQueue {
public:
  ~AudioPacketQueue() {
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
  }

  AVPacket *pop() {
    std::lock_guard lg(mut_);
    if (!packet_que_.empty()) {
      auto *return_pkt = packet_que_.front();
      packet_que_.pop();
      return return_pkt;
    }
    return nullptr;
  }

  size_t size() const {
    std::lock_guard lg(mut_);
    return packet_que_.size();
  }

private:
  std::queue<AVPacket *> packet_que_;
  mutable std::mutex mut_;
};
} // namespace ffmpeg_utils

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_AUDIO_PACKET_QUEUE_H
