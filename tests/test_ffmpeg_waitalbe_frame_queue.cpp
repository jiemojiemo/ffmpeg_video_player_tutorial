//
// Created by user on 1/20/23.
//
#include <gmock/gmock.h>

#include "ffmpeg_utils/ffmpeg_waitable_frame_queue.h"

using namespace testing;
using namespace std::literals;
using namespace ffmpeg_utils;

class AWaitableFrameQueue : public Test {
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

  const size_t queue_size = 8;
  WaitableFrameQueue q{queue_size};
  AVFrame *f;
};

TEST_F(AWaitableFrameQueue, InitWithQueueSize) {

  WaitableFrameQueue packet_que(queue_size);

  ASSERT_THAT(packet_que.capacity(), Eq(queue_size));
}

TEST_F(AWaitableFrameQueue, TryPushIncreaseSize) {
  ASSERT_THAT(q.size(), Eq(0));

  q.tryPush(f);
  q.tryPush(f);

  ASSERT_THAT(q.size(), Eq(2));
}

TEST_F(AWaitableFrameQueue, TryPushFailedIfItIsFull) {
  for (auto i = 0u; i < queue_size; ++i) {
    q.tryPush(f);
  }

  auto ok = q.tryPush(f);
  ASSERT_FALSE(ok);
  ASSERT_THAT(q.size(), Eq(queue_size));
}

TEST_F(AWaitableFrameQueue, TryPopDecreaseSize) {
  q.tryPush(f);
  q.tryPush(f);
  ASSERT_THAT(q.size(), Eq(2));

  auto pop_frame = q.tryPop();
  freeFrame(&pop_frame);
  ASSERT_THAT(q.size(), Eq(1));
}

TEST_F(AWaitableFrameQueue, TryPopReturnNullIfItIsEmpty) {
  ASSERT_THAT(q.size(), Eq(0));

  auto pop_frame = q.tryPop();

  ASSERT_THAT(pop_frame, IsNull());
}

TEST_F(AWaitableFrameQueue, waitAndPushIncreaseSize) {
  ASSERT_THAT(q.size(), Eq(0));

  q.waitAndPush(f);
  q.waitAndPush(f);

  ASSERT_THAT(q.size(), Eq(2));
}

TEST_F(AWaitableFrameQueue, waitAndPushCanWaitUntilThereHasSpace) {
  for (auto i = 0u; i < queue_size; ++i) {
    q.tryPush(f);
  }

  std::thread t([&]() {
    std::this_thread::sleep_for(1ms);
    auto *f = q.tryPop();
    freeFrame(&f);
  });

  q.waitAndPush(f);

  ASSERT_THAT(q.size(), Eq(q.capacity()));
  t.join();
}

TEST_F(AWaitableFrameQueue, PopCanWaitUntilHasData) {
  std::thread t([this]() {
    std::this_thread::sleep_for(10ms);
    q.tryPush(f);
  });

  auto *new_frame = q.waitAndPop();
  freeFrame(&new_frame);
  t.join();
}