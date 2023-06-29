//
// Created by user on 7/25/22.
//
#include "j_video_player/ffmpeg_utils/ffmpeg_format_utils.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace ffmpeg_utils;

class AFFmpegFormatUtils : public Test {
public:
};

TEST_F(AFFmpegFormatUtils, CanGetAudioUint8FormatBytesSize) {
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_U8),
              Eq(sizeof(uint8_t)));
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_U8P),
              Eq(sizeof(uint8_t)));
}

TEST_F(AFFmpegFormatUtils, CanGetAudioS16FormatBytesSize) {
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_S16),
              Eq(sizeof(int16_t)));
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_S16P),
              Eq(sizeof(int16_t)));
}

TEST_F(AFFmpegFormatUtils, CanGetAudioFloatFormatBytesSize) {
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_FLT),
              Eq(sizeof(float)));
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_FLTP),
              Eq(sizeof(float)));
}

TEST_F(AFFmpegFormatUtils, CanGetAudioDoubleFormatBytesSize) {
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_DBL),
              Eq(sizeof(double)));
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_DBL),
              Eq(sizeof(double)));
}

TEST_F(AFFmpegFormatUtils, CanGetAudioInt64FormatBytesSize) {
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_S64),
              Eq(sizeof(int64_t)));
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_S64P),
              Eq(sizeof(int64_t)));
}

TEST_F(AFFmpegFormatUtils, GetZeroSizeIfAudioFormatNotSupported) {
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_NONE),
              Eq(0));
  ASSERT_THAT(FormatUtils::getAudioFormatSampleByteSize(AV_SAMPLE_FMT_NB),
              Eq(0));
}

TEST_F(AFFmpegFormatUtils, CanCheckIsPlanerFormat) {
  auto planar_formats = {
      AV_SAMPLE_FMT_U8P,  AV_SAMPLE_FMT_S16P, AV_SAMPLE_FMT_FLTP,
      AV_SAMPLE_FMT_DBLP, AV_SAMPLE_FMT_S64P,
  };

  for (auto f : planar_formats) {
    ASSERT_TRUE(FormatUtils::isPlanarFormat(f));
  }
}

TEST_F(AFFmpegFormatUtils, CanCheckIsInterleaveFormat) {
  auto interleave_format = {
      AV_SAMPLE_FMT_U8,  AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_FLT,
      AV_SAMPLE_FMT_DBL, AV_SAMPLE_FMT_S64,
  };

  for (auto f : interleave_format) {
    ASSERT_TRUE(FormatUtils::isInterleaveFormat(f));
  }
}

TEST_F(AFFmpegFormatUtils, GetChannelDataSizeReturns1IfIsInterleave) {
  auto interleave_format = {
      AV_SAMPLE_FMT_U8,  AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_FLT,
      AV_SAMPLE_FMT_DBL, AV_SAMPLE_FMT_S64,
  };

  int num_channels = 2;
  for (auto f : interleave_format) {
    ASSERT_THAT(FormatUtils::getChannelDataSize(num_channels, f), Eq(1));
  }
}

TEST_F(AFFmpegFormatUtils, GetChannelDataSizeReturnsNumberChannelIfIsPlanar) {
  auto f = AV_SAMPLE_FMT_S16P;

  ASSERT_THAT(FormatUtils::getChannelDataSize(1, f), Eq(1));
  ASSERT_THAT(FormatUtils::getChannelDataSize(2, f), Eq(2));
}