//
// Created by user on 11/17/23.
//

#ifndef USESURFACEVIEW_INCLUDE_J_VIDEO_PLAYER_MODULES_ANDROID_J_ANDR_SURFACEVIEW_VIDEO_OUTPUT_H
#define USESURFACEVIEW_INCLUDE_J_VIDEO_PLAYER_MODULES_ANDROID_J_ANDR_SURFACEVIEW_VIDEO_OUTPUT_H
#include "j_video_player/modules/j_base_video_output.h"

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

namespace j_video_player {
class SurfaceViewVideoOutput : public BaseVideoOutput {
public:
  ~SurfaceViewVideoOutput() override {
    detachSurface();
  }
  void attachSurface(JNIEnv *env, jobject surface) {
    if (surface == nullptr) {
      return;
    }

    detachSurface();

    nativeWindow_ = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow_ == nullptr) {
      LOGE("ANativeWindow_fromSurface failed");
      return;
    }

    return;
  }

  void detachSurface() {
    if (nativeWindow_ != nullptr) {
      ANativeWindow_release(nativeWindow_);
      nativeWindow_ = nullptr;
    }
  }

  int prepare(const VideoOutputParameters &parameters) override {
    if(nativeWindow_ == nullptr) {
      LOGE("nativeWindow_ is null, can't prepare");
      return -1;
    }
    if(parameters.pixel_format != AV_PIX_FMT_RGBA) {
      LOGE("Only support AV_PIX_FMT_RGBA pixel format");
      return -1;
    }

    auto output_pixel_format = WINDOW_FORMAT_RGBA_8888;
    int ret = ANativeWindow_setBuffersGeometry(nativeWindow_,
                                               parameters.width,
                                               parameters.height,
                                               output_pixel_format);
    RETURN_IF_ERROR_LOG(ret, "ANativeWindow_setBuffersGeometry failed");
    return 0;
  }
protected:
  int drawFrame(std::shared_ptr<Frame> frame) override {
    if(nativeWindow_ == nullptr) {
      LOGE("nativeWindow_ is null, can't drawFrame");
      return -1;
    }

    ANativeWindow_Buffer buffer;
    ANativeWindow_lock(nativeWindow_, &buffer, NULL);

    // copy frame to buffer
    auto *data_src_line = (int32_t *) frame->f->data[0];
    const auto src_line_stride = frame->f->linesize[0] / sizeof(int32_t);

    auto *data_dst_line = (uint32_t *) buffer.bits;
    auto height = std::min(buffer.height, frame->f->height);
    for (int y = 0; y < height; y++) {
      std::copy_n(data_src_line, buffer.width, data_dst_line);

      data_src_line += src_line_stride;

      data_dst_line += buffer.stride;
    }

    ANativeWindow_unlockAndPost(nativeWindow_);

    return 0;
  }

private:
  ANativeWindow *nativeWindow_ = nullptr;
};
}

#endif //USESURFACEVIEW_INCLUDE_J_VIDEO_PLAYER_MODULES_ANDROID_J_ANDR_SURFACEVIEW_VIDEO_OUTPUT_H
