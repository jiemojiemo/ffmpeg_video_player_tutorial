//
// Created by user on 5/22/23.
//
#include "j_video_player/modules/j_audio_decoder.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace j_video_player;

class AAudioDecoder : public Test {
public:
  std::string filepath = "/Users/user/Downloads/video_1280x720_30fps_20sec.mp4";
  AudioDecoder d{filepath};
};

TEST_F(AAudioDecoder, CanCreateAudioDecoderWithFilePath) {
  auto dd = AudioDecoder(filepath);

  ASSERT_THAT(dd.getURL(), Eq(filepath));
}

TEST_F(AAudioDecoder, DecodeTypeIsAudio) {
  ASSERT_THAT(d.getMediaType(), Eq(AVMEDIA_TYPE_AUDIO));
}

TEST_F(AAudioDecoder, PrepareDecoderWithAudioResampler) {
  d.onPrepareDecoder();

  ASSERT_THAT(d.getResampler(), NotNull());
}

TEST_F(AAudioDecoder, DecodecDoneWillReleaseAudioResampler) {
  d.onPrepareDecoder();
  d.OnDecoderDone();

  ASSERT_THAT(d.getResampler(), IsNull());
}
