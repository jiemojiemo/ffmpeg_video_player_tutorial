//
// Created by user on 11/11/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_WAITABLE_EVENT_H
#define FFMPEG_VIDEO_PLAYER_WAITABLE_EVENT_H
#include <mutex>
namespace j_video_player {
class WaitableEvent {
public:
  explicit WaitableEvent(bool manual_reset = false) noexcept
      : manual_reset_(manual_reset) {}

  bool wait(int time_out_ms) {
    std::unique_lock<std::mutex> lock(mut_);

    if (!triggered_) {
      if (time_out_ms < 0) {
        condition_.wait(lock, [this] { return triggered_.load(); });
      } else {
        if (!condition_.wait_for(lock, std::chrono::milliseconds(time_out_ms),
                                 [this] { return triggered_.load(); })) {
          return false;
        }
      }
    }

    if (!manual_reset_)
      reset();

    return true;
  }

  void signal() {
    std::unique_lock<std::mutex> lock(mut_);

    triggered_ = true;
    condition_.notify_all();
  }

  void reset() { triggered_ = false; }
  void setManualReset(bool manual_reset) { manual_reset_ = manual_reset; }
  bool isTriggered() const { return triggered_.load(); }

private:
  bool manual_reset_{false};
  std::mutex mut_;
  std::condition_variable condition_;
  std::atomic<bool> triggered_{false};
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_WAITABLE_EVENT_H
