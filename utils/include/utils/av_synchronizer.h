//
// Created by user on 2/3/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_AV_SYNCHRONIZER_H
#define FFMPEG_VIDEO_PLAYER_AV_SYNCHRONIZER_H

#pragma once
#include "utils/clock_manager.h"

#include <cmath>
namespace utils {
class AVSynchronizer {
public:
  /**
   * computes delay for sync video to audio
   * @param clock_manager to get audio/video clock
   * @return delay time, seconds
   */
  double computeTargetDelay(const ClockManager &clock_manager) {

    // video sync to audio
    auto video_pts = clock_manager.getVideoClock();
    auto video_pre_pts = clock_manager.getVideoPreClock();

    auto pts_delay = video_pts - video_pre_pts;
    if (pts_delay <= 0 || pts_delay >= 1.0) {
      // use the previously calculated delay
      pts_delay = last_duration;
    }
    last_duration = pts_delay;

    auto audio_pts = clock_manager.getAudioClock();
    auto diff = video_pts - audio_pts;
    auto sync_threshold = std::fmax(pts_delay, AV_SYNC_THRESHOLD);
    if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
      if (diff <= -sync_threshold) {
        pts_delay = std::fmax(0.0, pts_delay + diff);
      } else if (diff >= sync_threshold) {
        pts_delay = 2 * pts_delay;
      }
    }
    return pts_delay;
  }

  double getLastDuration() const { return last_duration; }

private:
  static constexpr double AV_SYNC_THRESHOLD = 0.01;
  static constexpr double AV_NOSYNC_THRESHOLD = 10.0;
  double last_duration = 0.0f;
};
} // namespace utils

#endif // FFMPEG_VIDEO_PLAYER_AV_SYNCHRONIZER_H
