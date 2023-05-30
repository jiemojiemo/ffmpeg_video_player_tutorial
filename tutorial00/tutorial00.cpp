//
// Created by user on 5/30/23.
//
#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/codec.h>
#include <libavutil/avutil.h>

#ifdef __cplusplus
}
#endif

#include <iostream>

int main() {
  printf("ffmpeg version:%s\n", av_version_info());
  return 0;
}