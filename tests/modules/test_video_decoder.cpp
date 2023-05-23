//
// Created by user on 5/22/23.
//
#include "j_video_player/modules/j_video_decoder.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace j_video_player;

class MockVideoRender : public IVideoRender {
public:
  MOCK_METHOD(void, initVideoRender, (int, int), (override));
  void uninit() override {}
  void renderVideoData(AVFrame *frame) override { (void)frame; }
};

class AVideoDecoder : public Test {
public:
  std::string filepath = "/Users/user/Downloads/video_1280x720_30fps_20sec.mp4";
  VideoDecoder d{filepath};
  std::shared_ptr<MockVideoRender> render = std::make_shared<MockVideoRender>();
};

TEST_F(AVideoDecoder, CanCreateWithFilePath) {
  auto dd = VideoDecoder(filepath);

  ASSERT_THAT(dd.getURL(), Eq(filepath));
}

TEST_F(AVideoDecoder, DecodeTypeIsVideo) {
  ASSERT_THAT(d.getMediaType(), Eq(AVMEDIA_TYPE_VIDEO));
}

TEST_F(AVideoDecoder, PrepareDecoderWithVideoConvert) {
  d.onPrepareDecoder();

  ASSERT_THAT(d.getVideoConverter(), NotNull());
}

TEST_F(AVideoDecoder, DecodecDoneWillReleaseVideoConvert) {
  d.onPrepareDecoder();
  d.OnDecoderDone();

  ASSERT_THAT(d.getVideoConverter(), IsNull());
}

TEST_F(AVideoDecoder, PrepareDecoderAlsoInitRender) {
  d.setRender(render);
  auto expected_width = d.getCodecContext()->width;
  auto expected_height = d.getCodecContext()->height;
  EXPECT_CALL(*render, initVideoRender(expected_width, expected_height))
      .Times(1);
  d.onPrepareDecoder();
}
