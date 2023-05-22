//
// Created by user on 5/20/23.
//
#include <gmock/gmock.h>

#include "j_video_player/modules/j_ffmpeg_base_decoder.h"

using namespace j_video_player;
using namespace testing;
class MockFFBaseDecoder : public FFMPEGBaseDecoder {
public:
  ~MockFFBaseDecoder() override { uninit(); };
  MOCK_METHOD(void, onPrepareDecoder, (), (override));
  MOCK_METHOD(void, OnDecoderDone, (), (override));
  void OnFrameAvailable(AVFrame *frame) override { (void)frame; }
};

class AFFMPEGBaseDecoder : public Test {
public:
  void SetUp() override { d = std::make_unique<MockFFBaseDecoder>(); }
  void TearDown() override {
    d->stop();
    d = nullptr;
  }
  std::unique_ptr<MockFFBaseDecoder> d;
  std::string url = "/Users/user/Downloads/video_1280x720_30fps_20sec.mp4";
  AVMediaType media_type = AVMEDIA_TYPE_VIDEO;
};

TEST_F(AFFMPEGBaseDecoder, InitStateIsStopped) {
  ASSERT_THAT(d->getState(), Eq(DecoderState::kStopped));
}

TEST_F(AFFMPEGBaseDecoder, CanGetFileURLAfterInit) {
  auto ret = d->init(url, media_type);

  ASSERT_THAT(ret, Eq(0));
  ASSERT_THAT(d->getURL(), Eq(url));
}

TEST_F(AFFMPEGBaseDecoder, CanGetDecoderTypeAfterInit) {
  auto ret = d->init(url, media_type);

  ASSERT_THAT(ret, Eq(0));
  ASSERT_THAT(d->getMediaType(), Eq(media_type));
}

TEST_F(AFFMPEGBaseDecoder, InitFaildIfURLIsInvalid) {
  auto valid_url = "file://test.mp4";
  auto ret = d->init(valid_url, media_type);

  ASSERT_THAT(ret, Not(0));
}

TEST_F(AFFMPEGBaseDecoder, CanGetCodecContextAfterInit) {
  d->init(url, media_type);

  ASSERT_THAT(d->getCodecContext(), NotNull());
}

TEST_F(AFFMPEGBaseDecoder, GetNullCodecContextIfNeverInit) {
  ASSERT_THAT(d->getCodecContext(), IsNull());
}

TEST_F(AFFMPEGBaseDecoder, StartChageStateToDecoding) {
  d->init(url, media_type);
  d->start();
  ASSERT_THAT(d->getState(), Eq(DecoderState::kDecoding));
}

TEST_F(AFFMPEGBaseDecoder, StartDecodingCalloOnPrepareDecoder) {
  d->init(url, media_type);

  EXPECT_CALL(*d, onPrepareDecoder()).Times(1);
  d->start();
}

TEST_F(AFFMPEGBaseDecoder, StopCalloOnDecoderDone) {
  d->init(url, media_type);
  d->start();

  EXPECT_CALL(*d, OnDecoderDone()).Times(1);
  d->stop();
}

TEST_F(AFFMPEGBaseDecoder, StopChageStateToStopped) {
  d->init(url, media_type);
  d->start();
  d->stop();
  ASSERT_THAT(d->getState(), Eq(DecoderState::kStopped));
}

TEST_F(AFFMPEGBaseDecoder, PauseChageStateToPaused) {
  d->init(url, media_type);
  d->start();
  d->pause();
  ASSERT_THAT(d->getState(), Eq(DecoderState::kPaused));
}

TEST_F(AFFMPEGBaseDecoder, SeekUpdatePosition) {
  d->init(url, media_type);
  d->start();
  d->seek(10);
  ASSERT_THAT(d->getCurrentPosition(), Eq(10));
}

TEST_F(AFFMPEGBaseDecoder, SeekChangeStateToDecoding) {
  d->init(url, media_type);
  d->start();
  d->pause();

  d->seek(10);
  ASSERT_THAT(d->getState(), Eq(DecoderState::kDecoding));
}

TEST_F(AFFMPEGBaseDecoder, SeekFailedIfNeverStart) {
  d->init(url, media_type);
  d->seek(10);

  ASSERT_THAT(d->getCurrentPosition(), Not(10));
}