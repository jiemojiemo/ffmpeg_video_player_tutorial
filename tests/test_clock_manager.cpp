//
// Created by user on 2/3/23.
//
#include <gmock/gmock.h>

#include "utils/clock_manager.h"

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