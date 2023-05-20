//
// Created by user on 1/15/23.
//
#include <gmock/gmock.h>

#include "j_video_player/ffmpeg_utils/ffmpeg_demuxer.h"

using namespace testing;
using namespace ffmpeg_utils;

class AFFMPEGDemuxer : public Test {
public:
  const std::string file_path = "/Users/user/Downloads/encode-v1/juren-30s.mp4";
  FFMPEGDemuxer d;
};

TEST_F(AFFMPEGDemuxer, CanOpenFile) {
  int ret = d.openFile(file_path);

  ASSERT_THAT(ret, Eq(0));
}

TEST_F(AFFMPEGDemuxer, IsValidAfterOpenSuccessfully) {
  int ret = d.openFile(file_path);

  ASSERT_THAT(ret, Eq(0));
  ASSERT_TRUE(d.isValid());
}

TEST_F(AFFMPEGDemuxer, StreamCountIsZeroWhenInit) {
  ASSERT_THAT(d.getStreamCount(), Eq(0));
}

TEST_F(AFFMPEGDemuxer, CanGetStreamCountAfterOpen) {
  d.openFile(file_path);

  ASSERT_THAT(d.getStreamCount(), Eq(2));
}

TEST_F(AFFMPEGDemuxer, CanFindVideoStreamIndexAfterOpenFile) {
  ASSERT_THAT(d.getVideoStreamIndex(), Eq(-1));

  d.openFile(file_path);

  ASSERT_THAT(d.getVideoStreamIndex(), Eq(0));
}

TEST_F(AFFMPEGDemuxer, CanFindAudioStreamIndexAfterOpenFile) {
  ASSERT_THAT(d.getAudioStreamIndex(), Eq(-1));

  d.openFile(file_path);

  ASSERT_THAT(d.getAudioStreamIndex(), Eq(1));
}

TEST_F(AFFMPEGDemuxer, CanReadNextPacket) {
  d.openFile(file_path);

  auto [ret, packet] = d.readPacket();

  ASSERT_THAT(ret, Eq(0));
  ASSERT_THAT(packet, NotNull());
}

TEST_F(AFFMPEGDemuxer, ReadPacketFailedIfNotOpenFile) {
  auto [ret, packet] = d.readPacket();

  ASSERT_THAT(ret, Eq(-1));
}