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
    double time = (double)av_gettime() / 1000000.0;
    setAudioClockAt(0.0, time);
    setVideoClockAt(0.0, time);
  }
  void setAudioClock(double pts) { setClock(audio_clock_, pts); }
  void setAudioClockAt(double pts, double time) {
    setClockAt(audio_clock_, pts, time);
  }
  void setVideoClock(double pts) { setClock(video_clock_, pts); }
  void setVideoClockAt(double pts, double time) {
    setClockAt(video_clock_, pts, time);
  }
  double getAudioClock() const { return getClock(audio_clock_); }
  double getAudioClockAt(double time) const {
    return getClockAt(audio_clock_, time);
  }
  double getVideoClock() const { return getClock(video_clock_); }
  double getVideoClockAt(double time) const {
    return getClockAt(video_clock_, time);
  }
  double getAudioPreClock() const { return audio_clock_.pre_pts; }
  double getVideoPreClock() const { return video_clock_.pre_pts; }

private:
  void setClock(Clock &clock, double pts) {
    setClockAt(clock, pts, (double)av_gettime() / 1000000.0);
  }

  void setClockAt(Clock &clock, double pts, double time) {
    clock.pre_pts = clock.pts.load();
    clock.pts = pts;
    clock.last_updated = time;
  }

  double getClock(const Clock &c) const {
    double time = (double)av_gettime() / 1000000.0;
    return getClockAt(c, time);
  }
  double getClockAt(const Clock &c, double time) const {
    return c.pts + time - c.last_updated;
  }

  Clock audio_clock_;
  Clock video_clock_;
};
} // namespace utils

#endif // FFMPEG_VIDEO_PLAYER_CLOCK_MANAGER_H
