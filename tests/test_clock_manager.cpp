//
// Created by user on 2/3/23.
//
#include <gmock/gmock.h>

#include "j_video_player/utils/clock_manager.h"

using namespace testing;
using namespace utils;

class AClockManager : public Test {
public:
  ClockManager m;
};

TEST_F(AClockManager, AudioClockTimeIsZeroWhenInit) {
  ASSERT_THAT(m.getAudioClock(), DoubleNear(0.0, 1e-5));
}

TEST_F(AClockManager, VideoClockTimeIsZeroWhenInit) {
  ASSERT_THAT(m.getVideoClock(), DoubleNear(0.0, 1e-5));
}

TEST_F(AClockManager, CanSetAudioClock) {
  double t = 10;
  m.setAudioClock(t);

  ASSERT_THAT(m.getAudioClock(), DoubleNear(t, 1e-5));
}

TEST_F(AClockManager, CanSetVideoClock) {
  double t = 10;
  m.setVideoClock(t);

  ASSERT_THAT(m.getVideoClock(), DoubleNear(t, 1e-5));
}

TEST_F(AClockManager, AudioPrePtsIs0WhenInit) {
  ASSERT_THAT(m.getAudioPreClock(), DoubleNear(0.0, 1e-5));
}

TEST_F(AClockManager, VideoPrePtsIs0WhenInit) {
  ASSERT_THAT(m.getVideoPreClock(), DoubleNear(0.0, 1e-5));
}

TEST_F(AClockManager, CanGetAudioPreClock) {
  double t0 = 10;
  double t1 = 20;
  m.setAudioClock(t0);
  m.setAudioClock(t1);

  ASSERT_THAT(m.getAudioPreClock(), DoubleNear(t0, 1e-5));
}

TEST_F(AClockManager, CanGetVideoPreClock) {
  double t0 = 10;
  double t1 = 20;
  m.setVideoClock(t0);
  m.setVideoClock(t1);

  ASSERT_THAT(m.getVideoPreClock(), DoubleNear(t0, 1e-5));
}