//
// Created by user on 11/14/23.
//
#include "j_video_player/modules/j_sdl2_audio_output.h"
#include "j_video_player/modules/j_simple_source.h"
#include <gmock/gmock.h>

using namespace testing;

using namespace j_video_player;
class ASDL2AudioOutput : public Test {
public:
  SDL2AudioOutput output;
  AudioOutputParameters params;
  std::shared_ptr<IAudioDecoder> decoder =
      std::make_shared<FFmpegAudioDecoder>();
  std::shared_ptr<ISource> source =
      std::make_shared<SimpleAudioSource>(decoder);
};

TEST_F(ASDL2AudioOutput, prepareFailedIfParamInvalid) {
  params.sample_rate = 0;

  ASSERT_THAT(output.prepare(params), Eq(-1));
}

TEST_F(ASDL2AudioOutput, CanPrepareWithParams) {
  params.sample_rate = 44100;
  params.channels = 2;
  params.num_frames_of_buffer = 1024;

  ASSERT_THAT(output.prepare(params), Eq(0));
}

TEST_F(ASDL2AudioOutput, InitStateIsIdle) {
  ASSERT_THAT(output.getState(), Eq(AudioOutputState::kIdle));
}

TEST_F(ASDL2AudioOutput, PlayFailedIfNeverAttachSource) {
  auto ret = output.play();

  ASSERT_THAT(ret, Not(0));
}

TEST_F(ASDL2AudioOutput, PlayFailedIfPrepareFailed) {
  params.sample_rate = 0;
  output.prepare(params);

  auto ret = output.play();

  ASSERT_THAT(ret, Not(0));
}

TEST_F(ASDL2AudioOutput, PlayChangeStateToPlaying) {
  output.prepare(params);
  output.attachSource(source);
  output.play();

  ASSERT_THAT(output.getState(), Eq(AudioOutputState::kPlaying));
}

TEST_F(ASDL2AudioOutput, StopChangeStateToStopped) {
  output.prepare(params);
  output.attachSource(source);
  output.play();
  output.stop();

  ASSERT_THAT(output.getState(), Eq(AudioOutputState::kStopped));
}