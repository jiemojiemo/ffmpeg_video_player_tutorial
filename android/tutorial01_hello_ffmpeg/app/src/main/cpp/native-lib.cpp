#include <jni.h>
#include <string>
extern "C"
{
#include "libavformat/version.h"
#include "libavcodec/codec.h"
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_test_tutorial01_MainActivity_stringFromFFMPEG(
        JNIEnv* env,
        jobject /* this */) {

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    std::string hello = "Hello from ffmpeg: " + std::string(codec->long_name);
    return env->NewStringUTF(hello.c_str());
}