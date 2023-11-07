//
// Created by user on 1/15/23.
//
#include <gmock/gmock.h>

#include "j_video_player/ffmpeg_utils/ffmpeg_demuxer.h"

using namespace testing;
using namespace ffmpeg_utils;

class AFFmpegDemuxer : public Test {
public:
  const std::string file_path =
      "/Users/user/Documents/work/测试视频/video_1280x720_30fps_30sec.mp4";
  FFmpegDmuxer d;
};

TEST_F(AFFmpegDemuxer, CanOpenFile) {
  int ret = d.openFile(file_path);

  ASSERT_THAT(ret, Eq(0));
}

TEST_F(AFFmpegDemuxer, IsValidAfterOpenSuccessfully) {
  int ret = d.openFile(file_path);

  ASSERT_THAT(ret, Eq(0));
  ASSERT_TRUE(d.isValid());
}

TEST_F(AFFmpegDemuxer, StreamCountIsZeroWhenInit) {
  ASSERT_THAT(d.getStreamCount(), Eq(0));
}

TEST_F(AFFmpegDemuxer, CanGetStreamCountAfterOpen) {
  d.openFile(file_path);

  ASSERT_THAT(d.getStreamCount(), Eq(2));
}

TEST_F(AFFmpegDemuxer, CanFindVideoStreamIndexAfterOpenFile) {
  ASSERT_THAT(d.getVideoStreamIndex(), Eq(-1));

  d.openFile(file_path);

  ASSERT_THAT(d.getVideoStreamIndex(), Eq(0));
}

TEST_F(AFFmpegDemuxer, CanFindAudioStreamIndexAfterOpenFile) {
  ASSERT_THAT(d.getAudioStreamIndex(), Eq(-1));

  d.openFile(file_path);

  ASSERT_THAT(d.getAudioStreamIndex(), Eq(1));
}

TEST_F(AFFmpegDemuxer, CanReadNextPacket) {
  d.openFile(file_path);

  auto [ret, packet] = d.readPacket();

  ASSERT_THAT(ret, Eq(0));
  ASSERT_THAT(packet, NotNull());
}

TEST_F(AFFmpegDemuxer, ReadPacketFailedIfNotOpenFile) {
  auto [ret, packet] = d.readPacket();

  ASSERT_THAT(ret, Eq(-1));
}