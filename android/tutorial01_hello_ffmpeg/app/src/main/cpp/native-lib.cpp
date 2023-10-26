#include <jni.h>
#include <string>
extern "C"
{
#include <libavutil/avutil.h>
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_test_tutorial01_MainActivity_stringFromFFMPEG(
        JNIEnv* env,
        jobject /* this */) {

    std::string hello = "Hello from ffmpeg: " + std::string(av_version_info());
    return env->NewStringUTF(hello.c_str());
}