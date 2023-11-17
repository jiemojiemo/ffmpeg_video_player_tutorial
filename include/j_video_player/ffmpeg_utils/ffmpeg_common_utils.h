//
// Created by user on 1/15/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_COMMON_UTILS_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_COMMON_UTILS_H
#if __ANDROID__
#include <android/log.h>
#endif

#if __ANDROID__
#define TAG "j_video_player"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

#else
#define LOGE(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)
#define LOGD(...) printf(__VA_ARGS__)

#endif

#define RETURN_IF_ERROR(ret)                                                   \
  if ((ret) < 0) {                                                             \
    return (ret);                                                              \
  }

#define RETURN_IF_ERROR_LOG(ret, ...)                                          \
  if ((ret) < 0) {                                                             \
    LOGE(__VA_ARGS__);                                                         \
    return (ret);                                                              \
  }

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_COMMON_UTILS_H
