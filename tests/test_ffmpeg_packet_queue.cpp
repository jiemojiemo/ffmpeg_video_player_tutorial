//
// Created by user on 1/19/23.
//
#include <gmock/gmock.h>

#include "ffmpeg_utils/ffmpeg_packet_queue.h"

using namespace testing;
using namespace ffmpeg_utils;

class AAudioPacketQueue : public Test {
public:
  void SetUp() override {
    pkt = av_packet_alloc();
    av_new_packet(pkt, 100);
  }

  void TearDown() override { free_packet(&pkt); }

  void free_packet(AVPacket **p) {
    av_packet_unref(*p);
    av_packet_free(p);
  }

  PacketQueue q;
  AVPacket *pkt;
};

TEST_F(AAudioPacketQueue, HasZeroSizeWhenInit) { ASSERT_THAT(q.size(), Eq(0)); }

TEST_F(AAudioPacketQueue, PushPacketIncreaseQueueSize) {
  ASSERT_THAT(q.size(), Eq(0));

  q.cloneAndPush(pkt);
  q.cloneAndPush(pkt);

  ASSERT_THAT(q.size(), Eq(2));
}

TEST_F(AAudioPacketQueue, PopNullIfQueueIsEmpty) {
  ASSERT_THAT(q.size(), Eq(0));

  ASSERT_THAT(q.pop(), IsNull());
}

TEST_F(AAudioPacketQueue, CanPopPacketFromQueue) {
  q.cloneAndPush(pkt);

  auto *new_pkt = q.pop();
  ASSERT_THAT(new_pkt->size, Eq(pkt->size));
  free_packet(&new_pkt);
}

TEST_F(AAudioPacketQueue, PopDecreaseQueueSize) {
  q.cloneAndPush(pkt);
  q.cloneAndPush(pkt);
  ASSERT_THAT(q.size(), Eq(2));

  auto *new_pkt = q.pop();
  free_packet(&new_pkt);

  ASSERT_THAT(q.size(), Eq(1));
}

TEST_F(AAudioPacketQueue, CloneAndPushIncreaseTotalPacketSize) {
  ASSERT_THAT(q.totalPacketSize(), Eq(0));

  q.cloneAndPush(pkt);

  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size));
}

TEST_F(AAudioPacketQueue, PopDecreaseTotalPacketSize) {
  q.cloneAndPush(pkt);
  q.cloneAndPush(pkt);
  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size * 2));

  q.pop();
  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size));

  q.pop();
  ASSERT_THAT(q.totalPacketSize(), Eq(0));
}