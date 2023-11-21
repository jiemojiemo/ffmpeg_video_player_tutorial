#include <jni.h>
#include <string>

extern "C"
{
#include <libavutil/avutil.h>
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_videoplayertutorials_Tutorial01_stringFromFFMPEG(JNIEnv *env, jobject thiz) {
  std::string hello = "Hello from ffmpeg: " + std::string(av_version_info());
  return env->NewStringUTF(hello.c_str());
}