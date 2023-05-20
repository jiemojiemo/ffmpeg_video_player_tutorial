//
// Created by user on 11/11/22.
//

#ifndef AUDIOEFFECT_AE_SIMPLE_FIFO_H
#define AUDIOEFFECT_AE_SIMPLE_FIFO_H
#pragma once
#include <vector>
namespace utils {
template <typename T> class SimpleFIFO {
public:
  SimpleFIFO(int capacity) { setCapacity(capacity); }

  size_t capacity() const { return (buf_.size() - 1); }

  size_t size() const {
    return write_pos_ >= read_pos_ ? (write_pos_ - read_pos_)
                                   : (write_pos_ + capacity() - read_pos_ + 1);
  }

  bool full() const { return size() == capacity(); }

  bool empty() const { return size() == 0; }

  bool push(T &&value) {
    if (full()) {
      return false;
    }

    buf_[write_pos_] = std::move(value);
    write_pos_ = nextPost(write_pos_);
    return true;
  }

  bool pop(T &value) {
    if (empty()) {
      return false;
    }

    value = std::move(buf_[read_pos_]);
    read_pos_ = nextPost(read_pos_);
    return true;
  }

  void clear() {
    T v;
    for (; pop(v);)
      ;
  }

  void setCapacity(int new_capacity) {
    buf_.resize(new_capacity + 1);

    read_pos_ = 0;
    write_pos_ = 0;
  }

private:
  size_t nextPost(size_t current_pos) {
    return current_pos >= capacity() ? 0 : current_pos + 1;
  }
  std::vector<T> buf_;
  size_t read_pos_{0};
  size_t write_pos_{0};
};

} // namespace utils

#endif // AUDIOEFFECT_AE_SIMPLE_FIFO_H
