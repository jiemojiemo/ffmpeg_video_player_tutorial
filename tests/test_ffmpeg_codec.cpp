//
// Created by user on 1/17/23.
//
#include <gmock/gmock.h>
#include "ffmpeg_utils/ffmpeg_codec.h"

using namespace testing;
using namespace ffmpeg_utils;
class AFFMEPGCodec : public Test {
public:
  FFMEPGCodec d;
};

TEST_F(AFFMEPGCodec, CanPrepareWithCodecId) {
  auto codec_id = AVCodecID::AV_CODEC_ID_H264;

  int ret = d.prepare(codec_id, nullptr);

  ASSERT_THAT(ret, Eq(0));
}

TEST_F(AFFMEPGCodec, GetCodecAfterPrepare) {
  auto codec_id = AVCodecID::AV_CODEC_ID_H264;
  d.prepare(codec_id, nullptr);

  auto *c = d.getCodec();

  ASSERT_THAT(c, NotNull());
}

TEST_F(AFFMEPGCodec, PrepareFailedIfIDInvalid)
{
  auto invalid_id = AVCodecID::AV_CODEC_ID_WRAPPED_AVFRAME + 1;

  int ret = d.prepare(AVCodecID(invalid_id), nullptr);

  ASSERT_THAT(ret, Not(0));
}

TEST_F(AFFMEPGCodec, GetCodecContextAfterPrepare) {
  auto codec_id = AVCodecID::AV_CODEC_ID_H264;
  d.prepare(codec_id, nullptr);

  auto *c = d.getCodecContext();

  ASSERT_THAT(c, NotNull());
}

TEST_F(AFFMEPGCodec, PrepareWithParametersWillCopyParametersToCodecContext)
{
  auto codec_id = AVCodecID::AV_CODEC_ID_H264;
  AVCodecParameters parameters;
  parameters.width = 10;
  parameters.height = 20;

  d.prepare(codec_id, &parameters);

  ASSERT_THAT(d.getCodecContext()->width, Eq(parameters.width));
  ASSERT_THAT(d.getCodecContext()->height, Eq(parameters.height));
}