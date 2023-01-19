//
// Created by user on 1/19/23.
//
#include "ffmpeg_utils/ffmpeg_frame_queue.h"
#include <gmock/gmock.h>

using namespace testing;
using namespace ffmpeg_utils;

class AFrameQueue : public Test {
public:
  void SetUp() override {
    f = av_frame_alloc();
    f->width = 100;
    f->height = 200;
    f->format = AV_PIX_FMT_YUV420P;
    av_frame_get_buffer(f, 16);
    av_frame_make_writable(f);
  }

  void TearDown() override { freeFrame(&f); }

  void freeFrame(AVFrame **new_f) {
    av_frame_unref(*new_f);
    av_frame_free(new_f);
  }

  FrameQueue q;
  AVFrame *f;
};

TEST_F(AFrameQueue, HasZeroSizeWhenInit) { ASSERT_THAT(q.size(), Eq(0)); }

TEST_F(AFrameQueue, PushPacketIncreaseQueueSize) {
  ASSERT_THAT(q.size(), Eq(0));

  q.cloneAndPush(f);
  q.cloneAndPush(f);

  ASSERT_THAT(q.size(), Eq(2));
}

TEST_F(AFrameQueue, PopNullIfQueueIsEmpty) {
  ASSERT_THAT(q.size(), Eq(0));

  ASSERT_THAT(q.pop(), IsNull());
}

TEST_F(AFrameQueue, CanPopPacketFromQueue) {
  q.cloneAndPush(f);

  auto *new_f = q.pop();
  ASSERT_THAT(new_f->pkt_size, Eq(new_f->pkt_size));
  freeFrame(&new_f);
}

TEST_F(AFrameQueue, PopDecreaseQueueSize) {
  q.cloneAndPush(f);
  q.cloneAndPush(f);
  ASSERT_THAT(q.size(), Eq(2));

  auto *new_f = q.pop();
  freeFrame(&new_f);

  ASSERT_THAT(q.size(), Eq(1));
}