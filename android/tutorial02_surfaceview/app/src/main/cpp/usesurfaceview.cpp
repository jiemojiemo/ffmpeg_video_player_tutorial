#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/bitmap.h>
#include <algorithm>
#include <j_video_player/modules/j_simple_source.h>
#include "j_video_player/modules/j_ffmpeg_av_decoder.h"
#include "j_video_player/modules/android/j_andr_surfaceview_video_output.h"
#include "j_video_player/modules/j_simple_player.h"
#include "j_video_player/modules/j_ffmpeg_av_decoder.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_example_usesurfaceview_DisplayImageActivity_renderImage(JNIEnv *env, jobject thiz,
                                                                 jobject surface, jobject bitmap) {
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

using namespace j_video_player;

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativeCreatePlayer(JNIEnv *env, jobject thiz) {
  auto video_decoder = std::make_shared<FFmpegVideoDecoder>();
  auto audio_decoder = std::make_shared<FFmpegAudioDecoder>();
  auto video_source = std::make_shared<SimpleVideoSource>(video_decoder);
  auto audio_source = std::make_shared<SimpleAudioSource>(audio_decoder);
  auto video_output = std::make_shared<SurfaceViewVideoOutput>();

  auto *player = new SimplePlayer{video_source, audio_source, video_output, nullptr};
  return reinterpret_cast<jlong>(player);
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativeOpen(JNIEnv *env,
                                                        jobject thiz,
                                                        jlong player_handle,
                                                        jstring video_path) {
  auto *player = reinterpret_cast<SimplePlayer *>(player_handle);
  // jsring to std::string
  const char *path = env->GetStringUTFChars(video_path, nullptr);
  std::string path_str(path);
  env->ReleaseStringUTFChars(video_path, path);

  int ret = player->open(path_str);
  return (ret == 0) ? JNI_TRUE : JNI_FALSE;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativeDestroy(JNIEnv *env,
                                                           jobject thiz,
                                                           jlong _handel) {
  auto *player = reinterpret_cast<SimplePlayer *>(_handel);
  delete (player);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativeGetMediaFileWidth(JNIEnv *env,
                                                                     jobject thiz,
                                                                     jlong player_handle) {
  auto *player = reinterpret_cast<SimplePlayer *>(player_handle);
  return player->getMediaFileInfo().width;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativeGetMediaFileHeight(JNIEnv *env,
                                                                      jobject thiz,
                                                                      jlong player_handle) {
  auto *player = reinterpret_cast<SimplePlayer *>(player_handle);
  return player->getMediaFileInfo().height;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativeAttachSurface(JNIEnv *env,
                                                                 jobject thiz,
                                                                 jlong _handel,
                                                                 jobject surface) {
  auto *player = reinterpret_cast<SimplePlayer *>(_handel);
  auto video_output = std::dynamic_pointer_cast<SurfaceViewVideoOutput>(player->video_output);
  video_output->attachSurface(env, surface);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativePrepare(JNIEnv *env,
                                                           jobject thiz,
                                                           jlong _handel) {
  auto *player = reinterpret_cast<SimplePlayer *>(_handel);
  auto media_file_info = player->getMediaFileInfo();

  VideoOutputParameters video_output_param;
  video_output_param.width = media_file_info.width;
  video_output_param.height = media_file_info.height;
  video_output_param.pixel_format = AVPixelFormat::AV_PIX_FMT_RGBA;

  return player->prepare(video_output_param, {});
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativePlay(JNIEnv *env, jobject thiz, jlong _handel) {
  auto *player = reinterpret_cast<SimplePlayer *>(_handel);
  return player->play();
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativeStop(JNIEnv *env, jobject thiz, jlong _handel) {
  auto *player = reinterpret_cast<SimplePlayer *>(_handel);
  return player->stop();
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativeSeek(JNIEnv *env,
                                                        jobject thiz,
                                                        jlong _handel,
                                                        jdouble progress) {

    auto *player = reinterpret_cast<SimplePlayer *>(_handel);
    auto duration = player->getDuration();
    auto seek_pos = static_cast<int64_t>(progress * duration);
    return player->seek(seek_pos);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_usesurfaceview_SimplePlayer_nativePause(JNIEnv *env, jobject thiz, jlong _handel) {
    auto *player = reinterpret_cast<SimplePlayer *>(_handel);
    return player->pause();
}