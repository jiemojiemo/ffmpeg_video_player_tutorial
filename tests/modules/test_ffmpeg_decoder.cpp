//
// Created by user on 11/6/23.
//

#include "j_video_player/modules/j_ffmpeg_video_decoder.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace j_video_player;

class AFFmpegVideoDecoder : public Test {
public:
  void SetUp() override {}

  std::unique_ptr<FFmpegVideoDecoder> decoder =
      std::make_unique<FFmpegVideoDecoder>();
  std::string file_path =
      "/Users/user/Documents/work/测试视频/video_1280x720_30fps_30sec.mp4";
};

TEST_F(AFFmpegVideoDecoder, IsInvalidBeforeOpen) {
  ASSERT_THAT(decoder->isValid(), Eq(false));
}

TEST_F(AFFmpegVideoDecoder, OpenFileSuccessReturns0) {
  auto ret = decoder->open(file_path);

  ASSERT_THAT(ret, Eq(0));
}

TEST_F(AFFmpegVideoDecoder, IsValidAfterOpenSuccess) {
  auto ret = decoder->open(file_path);

  ASSERT_THAT(ret, Eq(0));
  ASSERT_THAT(decoder->isValid(), Eq(true));
}

TEST_F(AFFmpegVideoDecoder, IsValidAfterOpenFailed) {
  auto ret = decoder->open("invalid_file_path");

  ASSERT_THAT(ret, Ne(0));
  ASSERT_THAT(decoder->isValid(), Eq(false));
}

TEST_F(AFFmpegVideoDecoder, IsInvalidAfterClose) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));
  ASSERT_THAT(decoder->isValid(), Eq(true));

  decoder->close();
  ASSERT_THAT(decoder->isValid(), Eq(false));
}

TEST_F(AFFmpegVideoDecoder, DecodeReturnsNullIfInvalid) {
  ASSERT_THAT(decoder->isValid(), Eq(false));

  auto frame = decoder->decodeNextFrame();
  ASSERT_THAT(frame, IsNull());
}

TEST_F(AFFmpegVideoDecoder, CanDecodeNextVideoFrame) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  auto frame = decoder->decodeNextFrame();
  ASSERT_THAT(frame, NotNull());
}

TEST_F(AFFmpegVideoDecoder, PositionIsNOValueBeforeDecode) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  ASSERT_THAT(decoder->getPosition(), Eq(AV_NOPTS_VALUE));
}

TEST_F(AFFmpegVideoDecoder, PositionSameWithFramePTS) {
  decoder->open(file_path);

  for (int i = 0; i < 3; i++) {
    auto frame = decoder->decodeNextFrame();
    auto time_base = decoder->getVideoStreamTimeBase();
    auto expected_pts = av_rescale_q(frame->f->pts, time_base, AV_TIME_BASE_Q);
    ASSERT_THAT(decoder->getPosition(), Eq(expected_pts));
  }
}

TEST_F(AFFmpegVideoDecoder, SeekVideoFrameQuickCanGetVideoFrame) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  auto target_pts = 1 * 1000 * 1000; // 1s
  auto frame = decoder->seekVideoFrameQuick(target_pts);
  ASSERT_THAT(frame, NotNull());
}

TEST_F(AFFmpegVideoDecoder, SeeVideoFrameQuickFailedIfInvalid) {
  ASSERT_THAT(decoder->isValid(), Eq(false));
  ASSERT_THAT(decoder->seekVideoFrameQuick(0), IsNull());
}

TEST_F(AFFmpegVideoDecoder, SeekFramePreciseCanGetVideoFrame) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  auto target_pts = 1 * 1000 * 1000; // 1s
  auto frame = decoder->seekVideoFramePrecise(target_pts);
  ASSERT_THAT(frame, NotNull());
  ASSERT_THAT(frame->f->pts, Not(0));
}

TEST_F(AFFmpegVideoDecoder, InvalidAfterClose) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  decoder->close();
  ASSERT_THAT(decoder->isValid(), Eq(false));
}

TEST_F(AFFmpegVideoDecoder, PositionResetToNOPTSAfterClose) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  decoder->decodeNextFrame();
  ASSERT_THAT(decoder->getPosition(), Ne(AV_NOPTS_VALUE));

  decoder->close();
  ASSERT_THAT(decoder->getPosition(), Eq(AV_NOPTS_VALUE));
}

TEST_F(AFFmpegVideoDecoder, GetVideoFileInfoAsExpected) {
  auto ret = decoder->open(file_path);
  ASSERT_THAT(ret, Eq(0));

  auto info = decoder->getVideoFileInfo();
  EXPECT_THAT(info.file_path, Eq(file_path));
  EXPECT_THAT(info.width, Eq(1280));
  EXPECT_THAT(info.height, Eq(720));
  EXPECT_THAT(info.duration, Eq(30000000));
  EXPECT_THAT(info.bit_rate, Eq(304871));
  EXPECT_THAT(info.fps, Eq(30));
  EXPECT_THAT(info.video_stream_timebase.num, Eq(1));
  EXPECT_THAT(info.video_stream_timebase.den, Eq(15360));
}