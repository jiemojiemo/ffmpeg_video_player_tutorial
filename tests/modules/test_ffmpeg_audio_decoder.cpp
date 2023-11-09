//
// Created by user on 11/9/23.
//

#include "j_video_player/modules/j_ffmpeg_av_decoder.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace j_video_player;

class AFFmpegAudioDecoder : public Test {
public:
  void SetUp() override {}

  std::unique_ptr<IAudioDecoder> decoder =
      std::make_unique<FFmpegAudioDecoder>();
  std::string file_path =
      "/Users/user/Documents/work/测试视频/video_1280x720_30fps_30sec.mp4";
};

TEST_F(AFFmpegAudioDecoder, IsInvalidBeforeOpen) {
  ASSERT_THAT(decoder->isValid(), Eq(false));
}

TEST_F(AFFmpegAudioDecoder, OpenFileSuccessReturns0) {
  auto ret = decoder->open(file_path);

  ASSERT_THAT(ret, Eq(0));
}

TEST_F(AFFmpegAudioDecoder, IsValidAfterOpenSuccess) {
  auto ret = decoder->open(file_path);

  ASSERT_THAT(ret, Eq(0));
  ASSERT_THAT(decoder->isValid(), Eq(true));
}

TEST_F(AFFmpegAudioDecoder, IsValidAfterOpenFailed) {
  auto ret = decoder->open("invalid_file_path");

  ASSERT_THAT(ret, Ne(0));
  ASSERT_THAT(decoder->isValid(), Eq(false));
}

TEST_F(AFFmpegAudioDecoder, IsInvalidAfterClose) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));
  ASSERT_THAT(decoder->isValid(), Eq(true));

  decoder->close();
  ASSERT_THAT(decoder->isValid(), Eq(false));
}

TEST_F(AFFmpegAudioDecoder, DecodeReturnsNullIfInvalid) {
  ASSERT_THAT(decoder->isValid(), Eq(false));

  auto frame = decoder->decodeNextAudioFrame();
  ASSERT_THAT(frame, IsNull());
}

TEST_F(AFFmpegAudioDecoder, CanDecodeNextAudioFrame) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  auto frame = decoder->decodeNextAudioFrame();
  ASSERT_THAT(frame, NotNull());
}

TEST_F(AFFmpegAudioDecoder, PositionIsNOValueBeforeDecode) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  ASSERT_THAT(decoder->getPosition(), Eq(AV_NOPTS_VALUE));
}

TEST_F(AFFmpegAudioDecoder, PositionSameWithFramePTS) {
  decoder->open(file_path);

  for (int i = 0; i < 3; i++) {
    auto frame = decoder->decodeNextAudioFrame();
    auto time_base = decoder->getMediaFileInfo().audio_stream_timebase;
    auto expected_pts = av_rescale_q(frame->f->pts, time_base, AV_TIME_BASE_Q);
    EXPECT_THAT(decoder->getPosition(), Eq(expected_pts));
  }
}

TEST_F(AFFmpegAudioDecoder, SeekAudioFrameQuickCanGetAudioFrame) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  auto target_pts = 1 * 1000 * 1000; // 1s
  auto frame = decoder->seekAudioFrameQuick(target_pts);
  ASSERT_THAT(frame, NotNull());
}

TEST_F(AFFmpegAudioDecoder, SeeAudioFrameQuickFailedIfInvalid) {
  ASSERT_THAT(decoder->isValid(), Eq(false));
  ASSERT_THAT(decoder->seekAudioFrameQuick(0), IsNull());
}

TEST_F(AFFmpegAudioDecoder, SeekFramePreciseCanGetAudioFrame) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  auto target_pts = 1 * 1000 * 1000; // 1s
  auto frame = decoder->seekAudioFramePrecise(target_pts);
  ASSERT_THAT(frame, NotNull());
  ASSERT_THAT(frame->f->pts, Not(0));
}

TEST_F(AFFmpegAudioDecoder, InvalidAfterClose) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  decoder->close();
  ASSERT_THAT(decoder->isValid(), Eq(false));
}

TEST_F(AFFmpegAudioDecoder, PositionResetToNOPTSAfterClose) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  decoder->decodeNextAudioFrame();
  ASSERT_THAT(decoder->getPosition(), Ne(AV_NOPTS_VALUE));

  decoder->close();
  ASSERT_THAT(decoder->getPosition(), Eq(AV_NOPTS_VALUE));
}

TEST_F(AFFmpegAudioDecoder, GetMediaFileInfoAsExpected) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  auto info = decoder->getMediaFileInfo();
  EXPECT_THAT(info.file_path, Eq(file_path));
  EXPECT_THAT(info.sample_format, Eq(AV_SAMPLE_FMT_FLTP));
  EXPECT_THAT(info.sample_rate, Eq(44100));
  EXPECT_THAT(info.channels, Eq(2));
}