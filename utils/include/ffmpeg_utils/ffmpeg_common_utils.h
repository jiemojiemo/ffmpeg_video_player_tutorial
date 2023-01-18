//
// Created by user on 1/15/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_COMMON_UTILS_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_COMMON_UTILS_H

#define RETURN_IF_ERROR(ret)                                                   \
  if ((ret) < 0) {                                                             \
    return (ret);                                                              \
  }

#define RETURN_IF_ERROR_LOG(ret, ...)                                          \
  if ((ret) < 0) {                                                             \
    printf(__VA_ARGS__);                                                       \
    return (ret);                                                              \
  }

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_COMMON_UTILS_H
