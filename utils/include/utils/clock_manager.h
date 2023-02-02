//
// Created by user on 2/3/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_CLOCK_MANAGER_H
#define FFMPEG_VIDEO_PLAYER_CLOCK_MANAGER_H
#pragma once
#include "ffmpeg_utils/ffmpeg_headers.h"
#include "utils/clock.h"

namespace utils {
class ClockManager {
public:
  ClockManager() {
    setAudioClock(0.0);
    setVideoClock(0.0);
  }
  void setAudioClock(double pts) { setClock(audio_clock_, pts); }
  void setVideoClock(double pts) { setClock(video_clock_, pts); }
  double getAudioClock() const { return getClock(audio_clock_); }
  double getVideoClock() const { return getClock(video_clock_); }

private:
  void setClock(Clock &clock, double pts) {
    setClockAt(clock, pts, (double)av_gettime() / 1000000.0);
  }

  void setClockAt(Clock &clock, double pts, double time) {
    clock.pts = pts;
    clock.last_updated = time;
  }

  double getClock(const Clock &c) const {
    double time = (double)av_gettime() / 1000000.0;
    return c.pts + time - c.last_updated;
  }

  Clock audio_clock_;
  Clock video_clock_;
};
} // namespace utils


#endif // FFMPEG_VIDEO_PLAYER_CLOCK_MANAGER_H
