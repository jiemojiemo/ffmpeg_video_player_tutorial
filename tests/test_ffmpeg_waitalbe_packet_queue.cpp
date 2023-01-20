//
// Created by user on 1/20/23.
//
#include <gmock/gmock.h>

#include "ffmpeg_utils/ffmpeg_headers.h"
#include "utils/waitable_queue.h"

using namespace testing;
using namespace utils;
using namespace std::literals;

namespace ffmpeg_utils {
class WaitablePacketQueue {
public:
  WaitablePacketQueue(size_t queue_size) : queue_size_(queue_size) {}

  ~WaitablePacketQueue() {
    std::lock_guard lg(mut_);

    for (; !packet_que_.empty();) {
      auto *pkt = packet_que_.front();
      av_packet_unref(pkt);
      av_packet_free(&pkt);

      packet_que_.pop();
    }
  }

  size_t capacity() const { return queue_size_; }
  size_t size() const {
    std::lock_guard lg(mut_);
    return packet_que_.size();
  }

  bool tryPush(AVPacket *pkt) {
    std::lock_guard lg(mut_);
    if (packet_que_.size() >= queue_size_) {
      return false;
    }

    auto new_pkt = av_packet_clone(pkt);
    packet_que_.push(new_pkt);
    total_pkt_size_ += new_pkt->size;
    data_cond_.notify_one();
    return true;
  }

  void waitAndPush(AVPacket *pkt) {
    std::unique_lock<std::mutex> lk(mut_);
    has_space_cond_.wait(lk,
                         [this]() { return packet_que_.size() < queue_size_; });
    auto new_pkt = av_packet_clone(pkt);
    packet_que_.push(new_pkt);
    total_pkt_size_ += new_pkt->size;
    data_cond_.notify_one();
  }

  AVPacket *tryPop() {
    std::lock_guard lg(mut_);
    if (!packet_que_.empty()) {
      auto *return_pkt = packet_que_.front();
      packet_que_.pop();
      total_pkt_size_ -= return_pkt->size;
      has_space_cond_.notify_one();
      return return_pkt;
    }
    return nullptr;
  }

  AVPacket *waitAndPop() {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this]() { return !packet_que_.empty(); });
    auto *return_pkt = packet_que_.front();
    packet_que_.pop();
    total_pkt_size_ -= return_pkt->size;
    has_space_cond_.notify_one();
    return return_pkt;
  }

  size_t totalPacketSize() const { return total_pkt_size_; }

private:
  mutable std::mutex mut_;
  std::condition_variable data_cond_;
  std::condition_variable has_space_cond_;
  std::queue<AVPacket *> packet_que_;
  const size_t queue_size_{0};
  std::atomic<size_t> total_pkt_size_{0};
};

} // namespace ffmpeg_utils

using namespace ffmpeg_utils;

class AWaitablePacketQueue : public Test {
public:
  void SetUp() override {
    pkt = av_packet_alloc();
    av_new_packet(pkt, 100);
  }

  void TearDown() override { freePacket(&pkt); }

  void freePacket(AVPacket **p) {
    av_packet_unref(*p);
    av_packet_free(p);
  }

  const size_t queue_size = 8;
  WaitablePacketQueue q{queue_size};
  AVPacket *pkt;
};

TEST_F(AWaitablePacketQueue, InitWithQueueSize) {

  WaitablePacketQueue packet_que(queue_size);

  ASSERT_THAT(packet_que.capacity(), Eq(queue_size));
}

TEST_F(AWaitablePacketQueue, TryPushIncreaseSize) {
  ASSERT_THAT(q.size(), Eq(0));

  q.tryPush(pkt);
  q.tryPush(pkt);

  ASSERT_THAT(q.size(), Eq(2));
}

TEST_F(AWaitablePacketQueue, TryPushFailedIfItIsFull) {
  for (auto i = 0u; i < queue_size; ++i) {
    q.tryPush(pkt);
  }

  auto ok = q.tryPush(pkt);
  ASSERT_FALSE(ok);
  ASSERT_THAT(q.size(), Eq(queue_size));
}

TEST_F(AWaitablePacketQueue, TryPopDecreaseSize) {
  q.tryPush(pkt);
  q.tryPush(pkt);
  ASSERT_THAT(q.size(), Eq(2));

  auto pop_pkt = q.tryPop();
  freePacket(&pop_pkt);
  ASSERT_THAT(q.size(), Eq(1));
}

TEST_F(AWaitablePacketQueue, TryPopReturnNullIfItIsEmpty) {
  ASSERT_THAT(q.size(), Eq(0));

  auto pop_pkt = q.tryPop();

  ASSERT_THAT(pop_pkt, IsNull());
}

TEST_F(AWaitablePacketQueue, waitAndPushIncreaseSize) {
  ASSERT_THAT(q.size(), Eq(0));

  q.waitAndPush(pkt);
  q.waitAndPush(pkt);

  ASSERT_THAT(q.size(), Eq(2));
}

TEST_F(AWaitablePacketQueue, waitAndPushCanWaitUntilThereHasSpace) {
  for (auto i = 0u; i < queue_size; ++i) {
    q.tryPush(pkt);
  }

  std::thread t([&]() {
    std::this_thread::sleep_for(1ms);
    auto *pkt = q.tryPop();
    freePacket(&pkt);
  });

  q.waitAndPush(pkt);

  ASSERT_THAT(q.size(), Eq(q.capacity()));
  t.join();
}

TEST_F(AWaitablePacketQueue, PopCanWaitUntilHasData) {
  std::thread t([this]() {
    std::this_thread::sleep_for(10ms);
    q.tryPush(pkt);
  });

  auto *new_pkt = q.waitAndPop();
  ASSERT_THAT(new_pkt->size, Eq(pkt->size));
  freePacket(&new_pkt);
  t.join();
}

TEST_F(AWaitablePacketQueue, TryPushIncreaseTotalPacketSize) {
  ASSERT_THAT(q.totalPacketSize(), Eq(0));

  q.tryPush(pkt);

  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size));
}

TEST_F(AWaitablePacketQueue, WaitAndPushIncreaseTotalPacketSize) {
  ASSERT_THAT(q.totalPacketSize(), Eq(0));

  q.waitAndPush(pkt);

  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size));
}

TEST_F(AWaitablePacketQueue, TryPopDecreaseTotalPacketSize) {
  q.tryPush(pkt);
  q.tryPush(pkt);
  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size * 2));

  auto *pop_pkt = q.tryPop();
  freePacket(&pop_pkt);
  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size));

  pop_pkt = q.tryPop();
  freePacket(&pop_pkt);
  ASSERT_THAT(q.totalPacketSize(), Eq(0));
}

TEST_F(AWaitablePacketQueue, WaitAndPopDecreaseTotalPacketSize) {
  q.tryPush(pkt);
  q.tryPush(pkt);
  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size * 2));

  auto *pop_pkt = q.waitAndPop();
  freePacket(&pop_pkt);
  ASSERT_THAT(q.totalPacketSize(), Eq(pkt->size));

  pop_pkt = q.waitAndPop();
  freePacket(&pop_pkt);
  ASSERT_THAT(q.totalPacketSize(), Eq(0));
}