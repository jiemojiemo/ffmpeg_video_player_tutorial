//
// Created by user on 1/23/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_CLOCK_H
#define FFMPEG_VIDEO_PLAYER_CLOCK_H
#include <atomic>
namespace utils {
class Clock {
public:
  std::atomic<double> pts{0};          // clock base, seconds
  std::atomic<double> last_updated{0}; // last pts updated time
  std::atomic<double> pre_pts{0};
  std::atomic<double> pre_frame_delay{0};
};
} // namespace utils

#endif // FFMPEG_VIDEO_PLAYER_CLOCK_H
