//
// Created by user on 2/3/23.
//
#include <gmock/gmock.h>

#include "j_video_player/utils/av_synchronizer.h"

using namespace testing;
using namespace utils;
class AAVSynchronizer : public Test {
public:
  AVSynchronizer s;
};

TEST_F(AAVSynchronizer, ComputeDelayForSycnVideoToAudio) {
  ClockManager m;
  double time = (double)av_gettime() / 1000000.0;
  m.setAudioClockAt(0.1, time);
  m.setVideoClockAt(0.1, time);

  auto delay = s.computeTargetDelay(m);
  auto expected = s.getLastDuration();

  ASSERT_THAT(delay, DoubleNear(expected, 1e-6));
}

TEST_F(AAVSynchronizer, ComputeDelayUpdateLastDuration) {
  ClockManager m;
  double time = (double)av_gettime() / 1000000.0;
  double time_drift = 0.5;
  double time1 = time + time_drift;
  double audio_pts = 0.1;
  double video_pts = 0.15;
  double audio_pts1 = 0.1 + time_drift;
  double video_pts1 = 0.15 + time_drift;
  m.setAudioClockAt(audio_pts, time);
  m.setVideoClockAt(video_pts, time);
  m.setAudioClockAt(audio_pts1, time1);
  m.setVideoClockAt(video_pts1, time1);

  s.computeTargetDelay(m);
  auto delay1 = s.computeTargetDelay(m);

  ASSERT_THAT(s.getLastDuration(), DoubleNear(delay1, 1e-4));
}