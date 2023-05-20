//
// Created by user on 1/20/23.
//
#include <gmock/gmock.h>

#include "j_video_player/utils/waitable_queue.h"

using namespace testing;
using namespace utils;
using namespace std::literals;

class AWaitableQueue : public Test {
public:
  constexpr static int capacity = 8;

  WaitableQueue<int> int_que;
  WaitableQueue<std::vector<int>> vec_que;
  WaitableQueue<std::vector<int>> fixed_q{capacity};
};

TEST_F(AWaitableQueue, IsEmptyWhenInit) { ASSERT_TRUE(int_que.empty()); }

TEST_F(AWaitableQueue, IsNotEmptyAfterPush) {
  int_que.push(10);

  ASSERT_FALSE(int_que.empty());
}

TEST_F(AWaitableQueue, HasZeroSizeWhenInt) {
  ASSERT_THAT(int_que.size(), Eq(0));
}

TEST_F(AWaitableQueue, PushIncreaseSize) {
  int_que.push(10);

  ASSERT_THAT(int_que.size(), Eq(1));
}

TEST_F(AWaitableQueue, PushDoNotIncreaseSizeIfItIsFull) {
  std::vector<int> item{1, 2, 3};
  for (auto i = 0u; i < fixed_q.capacity(); ++i) {
    fixed_q.push(item);
  }
  ASSERT_TRUE(fixed_q.full());
  ASSERT_THAT(fixed_q.size(), Eq(capacity));

  fixed_q.push(item);
  ASSERT_THAT(fixed_q.size(), Eq(capacity));
}

TEST_F(AWaitableQueue, TryPopReturnsFalseIfIsEmpty) {
  ASSERT_TRUE(int_que.empty());

  int res = -1;
  auto ok = int_que.try_pop(res);

  ASSERT_FALSE(ok);
}

TEST_F(AWaitableQueue, TryPopGetTopValue) {
  int_que.push(1);
  int_que.push(2);

  int res = -1;
  auto ok = int_que.try_pop(res);

  ASSERT_TRUE(ok);
  ASSERT_THAT(res, Eq(1));
}

TEST_F(AWaitableQueue, TryPopAllItemsMakeQueueEmptyAgain) {
  int_que.push(1);
  ASSERT_FALSE(int_que.empty());

  int res = -1;
  int_que.try_pop(res);

  ASSERT_TRUE(int_que.empty());
}

TEST_F(AWaitableQueue, TryPopCReturnEmptyPtrIfQueueIsEmpty) {
  auto res = vec_que.try_pop();

  ASSERT_THAT(res, IsNull());
}

TEST_F(AWaitableQueue, TryPopReturnItemSharedPtr) {
  std::vector<int> item{1, 2, 3};
  vec_que.push(item);

  auto res = vec_que.try_pop();

  ASSERT_THAT(res, NotNull());
  ASSERT_THAT(*res, ContainerEq(item));
}

TEST_F(AWaitableQueue, TryPopReturnPtrMakeQueueEmptyAgain) {
  std::vector<int> item{1, 2, 3};
  vec_que.push(item);
  ASSERT_FALSE(vec_que.empty());

  auto output = vec_que.try_pop();

  ASSERT_TRUE(vec_que.empty());
}

TEST_F(AWaitableQueue, PopCanWaitUntilHasData) {
  std::vector<int> item{1, 2, 3};
  std::thread t([this, &item]() {
    std::this_thread::sleep_for(10ms);
    vec_que.push(item);
  });

  std::vector<int> res;
  vec_que.wait_and_pop(res);
  ASSERT_THAT(res, ContainerEq(item));
  t.join();
}

TEST_F(AWaitableQueue, WaitAndPopCosumeItem) {
  std::vector<int> item{1, 2, 3};
  vec_que.push(item);
  ASSERT_FALSE(vec_que.empty());

  std::vector<int> res;
  vec_que.wait_and_pop(res);

  ASSERT_TRUE(vec_que.empty());
}

TEST_F(AWaitableQueue, PopCanWaitAndReturnPtrUntilHasData) {
  std::vector<int> item{1, 2, 3};
  std::thread t([this, &item]() {
    std::this_thread::sleep_for(1ms);
    vec_que.push(item);
  });

  auto res = vec_que.wait_and_pop();
  ASSERT_THAT(*res, ContainerEq(item));
  t.join();
}

TEST_F(AWaitableQueue, PopCanWaitAndReturnPtrComsumeItem) {
  std::vector<int> item{1, 2, 3};
  vec_que.push(item);
  ASSERT_FALSE(vec_que.empty());

  vec_que.wait_and_pop();
  ASSERT_TRUE(vec_que.empty());
}

TEST_F(AWaitableQueue, CanMultiThreadPush) {
  size_t num_item = 100;
  std::thread t0([this, &num_item]() {
    for (auto i = 0u; i < num_item; ++i) {
      int_que.push(i);
    }
  });

  std::thread t1([this, &num_item]() {
    for (auto i = 0u; i < num_item; ++i) {
      int_que.wait_and_pop();
    }
  });

  t0.join();
  t1.join();
  ASSERT_TRUE(int_que.empty());
}

TEST_F(AWaitableQueue, CanInitWithCapacity) {
  WaitableQueue<std::vector<int>> vec_fixed_q{capacity};

  ASSERT_THAT(vec_fixed_q.capacity(), Eq(capacity));
}

TEST_F(AWaitableQueue, WaitAndPushIncreaseSize) {
  fixed_q.wait_and_push({1, 2, 3});

  ASSERT_THAT(fixed_q.size(), Eq(1));
}

TEST_F(AWaitableQueue, WaitAndPushCanWaitUntilThereHasSpace) {
  for (auto i = 0u; i < fixed_q.capacity(); ++i) {
    fixed_q.push({1, 2, 3});
  }

  std::thread t([&]() {
    std::this_thread::sleep_for(1ms);
    fixed_q.try_pop();
  });

  fixed_q.wait_and_push({1, 2, 3});

  ASSERT_THAT(fixed_q.size(), Eq(fixed_q.capacity()));
  t.join();
}

TEST_F(AWaitableQueue, TryPushIncraeseSizeIfThereHasSpace) {
  std::vector<int> item{1, 2, 3};

  fixed_q.try_push(item);

  ASSERT_THAT(fixed_q.size(), Eq(1));
}

TEST_F(AWaitableQueue, TryPushReturnTrueIfPushSuccessfully) {
  std::vector<int> item{1, 2, 3};

  auto succ = fixed_q.try_push(item);

  ASSERT_TRUE(succ);
}

TEST_F(AWaitableQueue, TryPushFailedIfThereNoSpace) {
  std::vector<int> item{1, 2, 3};
  for (auto i = 0u; i < fixed_q.capacity(); ++i) {
    fixed_q.push(item);
  }
  ASSERT_THAT(fixed_q.size(), Eq(capacity));

  auto succ = fixed_q.try_push(item);

  ASSERT_FALSE(succ);
  ASSERT_THAT(fixed_q.size(), Eq(capacity));
}

TEST_F(AWaitableQueue, CanCheckIsFullOrNot) {
  ASSERT_FALSE(fixed_q.full());

  std::vector<int> item{1, 2, 3};
  for (auto i = 0u; i < fixed_q.capacity(); ++i) {
    fixed_q.push(item);
  }

  ASSERT_TRUE(fixed_q.full());
}

TEST_F(AWaitableQueue, FlushWillRemoveAllItems) {
  std::vector<int> item{1, 2, 3};
  fixed_q.try_push(item);
  ASSERT_THAT(fixed_q.size(), Eq(1));

  fixed_q.flush();

  ASSERT_THAT(fixed_q.size(), Eq(0));
}