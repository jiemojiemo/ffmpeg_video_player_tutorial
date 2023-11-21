//
// Created by user on 11/22/23.
//

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/bitmap.h>
#include <algorithm>


extern "C"
JNIEXPORT void JNICALL
Java_com_example_videoplayertutorials_T02DisplayImageActivity_renderImage(JNIEnv *env,
                                                                          jobject thiz,
                                                                          jobject surface,
                                                                          jobject bitmap) {
  AndroidBitmapInfo info;
  AndroidBitmap_getInfo(env, bitmap, &info);

  char *data = NULL;
  AndroidBitmap_lockPixels(env, bitmap, (void **) &data);

  ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
  ANativeWindow_setBuffersGeometry(nativeWindow, info.width, info.height,
                                   WINDOW_FORMAT_RGBA_8888);

  ANativeWindow_Buffer buffer;
  ANativeWindow_lock(nativeWindow, &buffer, NULL);

  auto *data_src_line = (int32_t *) data;
  const auto src_line_stride = info.stride / sizeof(int32_t);

  auto *data_dst_line = (uint32_t *) buffer.bits;
  for (int y = 0; y < buffer.height; y++) {
    std::copy_n(data_src_line, buffer.width, data_dst_line);

    data_src_line += src_line_stride;

    data_dst_line += buffer.stride;
  }

  ANativeWindow_unlockAndPost(nativeWindow);
  AndroidBitmap_unlockPixels(env, bitmap);

  ANativeWindow_release(nativeWindow);
}