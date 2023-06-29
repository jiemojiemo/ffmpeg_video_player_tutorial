//
// Created by user on 1/19/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_DECODER_CONTEXT_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_DECODER_CONTEXT_H

#pragma once
#include "../../j_video_player/utils/simple_fifo.h"
#include "../../j_video_player/utils/waitable_queue.h"
#include "ffmpeg_audio_resampler.h"
#include "ffmpeg_codec.h"
#include "ffmpeg_demuxer.h"
#include "ffmpeg_frame_queue.h"
#include "ffmpeg_headers.h"
#include "ffmpeg_image_converter.h"
#include "ffmpeg_packet_queue.h"
#include "ffmpeg_waitable_frame_queue.h"
#include "ffmpeg_waitable_packet_queue.h"
#include "ringbuffer.hpp"

#include <string>

namespace ffmpeg_utils {
class DecoderContext {
public:
  int prepare(const std::string &infile) {
    int ret = demuxer.openFile(infile);
    RETURN_IF_ERROR_LOG(ret, "Could not open file %s\n", infile.c_str());
    demuxer.dumpFormat();

    video_stream_index = demuxer.getVideoStreamIndex();
    RETURN_IF_ERROR_LOG(ret, "Could not find video stream")
    audio_stream_index = demuxer.getAudioStreamIndex();
    RETURN_IF_ERROR_LOG(ret, "Could not find audio stream")

    video_stream = demuxer.getStream(video_stream_index);
    audio_stream = demuxer.getStream(audio_stream_index);

    ret = video_codec.prepare(video_stream->codecpar->codec_id,
                              video_stream->codecpar);
    RETURN_IF_ERROR_LOG(ret, "Prepare video codec failed\n");

    ret = audio_codec.prepare(audio_stream->codecpar->codec_id,
                              audio_stream->codecpar);
    RETURN_IF_ERROR_LOG(ret, "Prepare audio codec failed\n");

    auto dst_format = AVPixelFormat::AV_PIX_FMT_YUV420P;
    video_codec_ctx = video_codec.getCodecContext();
    ret = img_conv.prepare(video_codec_ctx->width, video_codec_ctx->height,
                           video_codec_ctx->pix_fmt, video_codec_ctx->width,
                           video_codec_ctx->height, dst_format, SWS_BILINEAR,
                           nullptr, nullptr, nullptr);
    RETURN_IF_ERROR_LOG(ret, "Prepare image converter failed\n");

    audio_codec_ctx = audio_codec.getCodecContext();
    int max_frames_size = audio_codec_ctx->sample_rate * 3; // 3s samples
    ret = audio_resampler.prepare(
        audio_codec_ctx->channels, audio_codec_ctx->channels,
        audio_codec_ctx->channel_layout, audio_codec_ctx->channel_layout,
        audio_codec_ctx->sample_rate, audio_codec_ctx->sample_rate,
        audio_codec_ctx->sample_fmt, AVSampleFormat::AV_SAMPLE_FMT_S16,
        max_frames_size);
    RETURN_IF_ERROR_LOG(ret, "Prepare audio resampler failed\n");

    audio_sample_fifo = std::make_unique<AudioSampleFIFO>(
        SDL_AUDIO_BUFFER_SIZE * audio_codec_ctx->channels);

    return 0;
  }

  static const size_t SDL_AUDIO_BUFFER_SIZE = 1024;
  static const size_t MAX_AUDIOQ_SIZE = (5 * 16 * 1024);
  static const size_t MAX_VIDEOQ_SIZE = (5 * 256 * 1024);
  static const size_t VIDEO_PACKET_QUEUE_SIZE = 10;
  static const size_t AUDIO_PACKET_QUEUE_SIZE = 10;
  static const size_t VIDEO_PICTURE_QUEUE_SIZE = 2;
  static const size_t AUDIO_FRAME_QUEUE_SIZE = 10;

  FFmpegDmuxer demuxer;
  FFmpegCodec video_codec;
  FFmpegCodec audio_codec;
  FFMPEGImageConverter img_conv;
  FFmpegAudioResampler audio_resampler;

  int video_stream_index{-1};
  int audio_stream_index{-1};

  AVStream *video_stream{nullptr};
  AVStream *audio_stream{nullptr};
  AVCodecContext *audio_codec_ctx{nullptr};
  AVCodecContext *video_codec_ctx{nullptr};

  PacketQueue audio_packet_queue;
  WaitablePacketQueue audio_packet_sync_que;
  WaitableFrameQueue audio_frame_sync_que{AUDIO_FRAME_QUEUE_SIZE};
  FrameQueue audio_frame_queue;
  PacketQueue video_packet_queue;
  WaitablePacketQueue video_packet_sync_que;
  WaitableFrameQueue video_frame_sync_que{VIDEO_PICTURE_QUEUE_SIZE};
  FrameQueue video_frame_queue;

  using AudioSampleFIFO = utils::SimpleFIFO<int16_t>;
  std::unique_ptr<AudioSampleFIFO> audio_sample_fifo;
};

} // namespace ffmpeg_utils

#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_DECODER_CONTEXT_H
