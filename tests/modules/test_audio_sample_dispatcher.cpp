//
// Created by user on 5/26/23.
//
#include "j_video_player/modules/j_audio_sample_dispatcher.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace j_video_player;
class AAudioSampleDispatcher : public Test {
public:
  AudioSampleDispatcher d;
};

TEST_F(AAudioSampleDispatcher, InitAvaibleReadSizeIsZero) {
  ASSERT_THAT(d.getAvailableReadSize(), Eq(0));
}

TEST_F(AAudioSampleDispatcher, PushAudioSampleWillIncreaseAvailableReadSize) {
  ASSERT_THAT(d.getAvailableReadSize(), Eq(0));
  const int num_samples = 1024;
  int16_t samples[num_samples];
  d.waitAndPushSamples(samples, num_samples, 0);

  ASSERT_THAT(d.getAvailableReadSize(), Eq(num_samples));
}

TEST_F(AAudioSampleDispatcher, PullAudioSamplesReturnsThePts) {
  const int num_samples = 1024;
  int pts = 1000;
  int16_t samples[num_samples];
  d.waitAndPushSamples(samples, num_samples, pts);

  int pull_nb_samples = 64;
  auto current_pts = d.pullAudioSamples(samples, pull_nb_samples);

  ASSERT_THAT(current_pts, Eq(pts));
}

TEST_F(AAudioSampleDispatcher, CanClearAudioSampleBuffer) {
  const int num_samples = 1024;
  int pts = 1000;
  int16_t samples[num_samples];
  d.waitAndPushSamples(samples, num_samples, pts);

  d.clearAudioCache();

  ASSERT_THAT(d.getAvailableReadSize(), Eq(0));
}