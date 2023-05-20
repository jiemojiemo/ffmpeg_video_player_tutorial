//
// Created by user on 7/25/22.
//

#ifndef FFMPEG_AND_SDL_TUTORIAL_FFMPEG_FORMAT_UTILS_H
#define FFMPEG_AND_SDL_TUTORIAL_FFMPEG_FORMAT_UTILS_H

#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif
#include <algorithm>
namespace ffmpeg_utils {
class FormatUtils {
public:
    static bool isPlanarFormat(AVSampleFormat f)
    {
        static const auto planar_formats = {
            AV_SAMPLE_FMT_U8P,
            AV_SAMPLE_FMT_S16P,
            AV_SAMPLE_FMT_FLTP,
            AV_SAMPLE_FMT_DBLP,
            AV_SAMPLE_FMT_S64P,
        };

        return std::find(std::begin(planar_formats),std::end(planar_formats), f) != std::end(planar_formats);
    }

    static bool isInterleaveFormat(AVSampleFormat f)
    {
        static const auto planar_formats = {
            AV_SAMPLE_FMT_U8,
            AV_SAMPLE_FMT_S16,
            AV_SAMPLE_FMT_FLT,
            AV_SAMPLE_FMT_DBL,
            AV_SAMPLE_FMT_S64,
        };

        return std::find(std::begin(planar_formats),std::end(planar_formats), f) != std::end(planar_formats);
    }

    static size_t getAudioFormatSampleByteSize(AVSampleFormat f) {
        switch (f) {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
            return sizeof(uint8_t);
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            return sizeof(int16_t);
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
            return sizeof(int32_t);
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            return sizeof(float);
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
            return sizeof(double);
        case AV_SAMPLE_FMT_S64:
        case AV_SAMPLE_FMT_S64P:
            return sizeof(int64_t);
        default:
            return 0;
        }
    }

    static size_t getChannelDataSize(int num_channels, AVSampleFormat f)
    {
        if(isInterleaveFormat(f)){
            return 1;
        }else
        {
            return num_channels;
        }
    }
};
} // namespace ffmpeg_utils


#endif // FFMPEG_AND_SDL_TUTORIAL_FFMPEG_FORMAT_UTILS_H
