//
// Created by user on 11/12/23.
//
#include <gmock/gmock.h>

#include "j_video_player/modules/j_base_video_output.h"
#include "j_video_player/modules/j_ffmpeg_av_decoder.h"
#include "j_video_player/modules/j_simple_source.h"

using namespace testing;

using namespace j_video_player;

class MockBaseVideoOutput : public BaseVideoOutput {
public:
  MOCK_METHOD(int, prepare, (const VideoOutputParameters &parameters),
              (override));
  MOCK_METHOD(int, drawFrame, (std::shared_ptr<Frame> frame), (override));
  std::shared_ptr<ISource> getSource() const { return source_; }
  std::shared_ptr<ffmpeg_utils::FFMPEGImageConverter> getConverter() const {
    return converter_;
  }
};

class ABaseVideoOutput : public Test {
public:
  MockBaseVideoOutput output;
  std::shared_ptr<IVideoDecoder> decoder =
      std::make_shared<FFmpegVideoDecoder>();
  std::shared_ptr<SimpleVideoSource> source =
      std::make_shared<SimpleVideoSource>(decoder);
  std::shared_ptr<ffmpeg_utils::FFMPEGImageConverter> converter =
      std::make_shared<ffmpeg_utils::FFMPEGImageConverter>();
};

TEST_F(ABaseVideoOutput, AttachedSourceIsNullAfterInit) {
  ASSERT_THAT(output.getSource(), IsNull());
}

TEST_F(ABaseVideoOutput, CanAttachSource) {
  output.attachVideoSource(source);

  ASSERT_THAT(output.getSource(), NotNull());
}

TEST_F(ABaseVideoOutput, AttachedImageConverterIsNullAfterInit) {
  ASSERT_THAT(output.getConverter(), IsNull());
}

TEST_F(ABaseVideoOutput, CanAttachImageConverter) {
  output.attachImageConverter(converter);

  ASSERT_THAT(output.getConverter(), NotNull());
}

TEST_F(ABaseVideoOutput, InitStateIsIdle) {
  ASSERT_THAT(output.getState(), Eq(OutputState::kIdle));
}

TEST_F(ABaseVideoOutput, PlayFailedIfNeverAttachSource) {
  auto ret = output.play();

  ASSERT_THAT(ret, Not(0));
}

TEST_F(ABaseVideoOutput, PlaySuccessIfAttachSource) {
  output.attachVideoSource(source);
  auto ret = output.play();

  ASSERT_THAT(ret, Eq(0));
}

TEST_F(ABaseVideoOutput, PlayChangeStateToPlaying) {
  output.attachVideoSource(source);
  output.play();

  ASSERT_THAT(output.getState(), Eq(OutputState::kPlaying));
}

TEST_F(ABaseVideoOutput, StopChangeStateToStopped) {
  output.attachVideoSource(source);
  output.play();
  output.stop();

  ASSERT_THAT(output.getState(), Eq(OutputState::kStopped));
}

TEST_F(ABaseVideoOutput, PauseChangeStateToPaused) {
  output.attachVideoSource(source);
  output.play();
  output.pause();

  ASSERT_THAT(output.getState(), Eq(OutputState::kPaused));
}