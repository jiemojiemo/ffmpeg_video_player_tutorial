//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H
#define FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H

#include "j_video_player/ffmpeg_utils/ffmpeg_audio_resampler.h"
#include "j_video_player/ffmpeg_utils/ffmpeg_waitable_frame_queue.h"
#include "j_video_player/modules/j_audio_render.h"
#include "j_video_player/modules/j_audio_sample_dispatcher.h"
#include "j_video_player/modules/j_ffmpeg_base_decoder.h"
#include "j_video_player/utils/clock_manager.h"
#include "j_video_player/utils/simple_fifo.h"

namespace j_video_player {
class IAudioRender;
class AudioDecoder : public FFMPEGBaseDecoder {
public:
  AudioDecoder(const std::string &file_path) {
    init(file_path, AVMEDIA_TYPE_AUDIO);
  }

  ~AudioDecoder() override { uninit(); }

  void setRender(std::shared_ptr<IAudioRender> render) {
    audio_render_ = std::move(render);
    audio_render_->setAudioCallback(
        [this](uint8_t *stream, int len) { audioCallback(stream, len); });
  }

  void setAVSyncClock(std::shared_ptr<utils::ClockManager> clock) {
    clock_ = std::move(clock);
  }

  void audioCallback(uint8_t *stream, int len) {
    (void)(stream);
    (void)(len);
    const int num_samples_of_stream = len / sizeof(int16_t);
    auto audio_pts_int64 = dispatcher_.pullAudioSamples((int16_t *)(stream),
                                                        num_samples_of_stream);
    if (clock_) {
      clock_->setAudioClock(audio_pts_int64 / (double)(AV_TIME_BASE));
    }
  }

  void clearCache() override { dispatcher_.clearAudioCache(); }

  void onPrepareDecoder() override {
    if (isInitSucc()) {
      audio_resampler_ = std::make_unique<ffmpeg_utils::FFMPEGAudioResampler>();
      auto codec_context = getCodecContext();
      auto in_num_channels = codec_context->channels;
      auto out_num_channels = kOutNumChannels;
      auto in_sample_rate = codec_context->sample_rate;
      auto out_sample_rate = kOutSampleRate;
      auto in_channel_layout = codec_context->channel_layout;
      auto out_channel_layout = AV_CH_LAYOUT_STEREO;
      auto in_sample_format = codec_context->sample_fmt;
      auto out_sample_format = AV_SAMPLE_FMT_S16;
      auto max_frames_size = 1024;
      audio_resampler_->prepare(
          in_num_channels, out_num_channels, in_channel_layout,
          out_channel_layout, in_sample_rate, out_sample_rate, in_sample_format,
          out_sample_format, max_frames_size);

      if (audio_render_) {
        audio_render_->initAudioRender();
      }
    }
  }
  void OnDecoderDone() override { audio_resampler_ = nullptr; }
  void OnFrameAvailable(AVFrame *frame) override {
    // resample frame and push to audio render
    if (frame && audio_resampler_) {
      int num_samples_out_per_channel = audio_resampler_->convert(
          (const uint8_t **)frame->data, frame->nb_samples);
      int num_total_samples = num_samples_out_per_channel * frame->channels;
      auto *int16_resample_data =
          reinterpret_cast<int16_t *>(audio_resampler_->resample_data[0]);
      dispatcher_.waitAndPushSamples(int16_resample_data, num_total_samples,
                                     frame->pts);
    }
  }

  // ---- testing ----
  ffmpeg_utils::FFMPEGAudioResampler *getResampler() const {
    return audio_resampler_.get();
  }

private:
  std::unique_ptr<ffmpeg_utils::FFMPEGAudioResampler> audio_resampler_{nullptr};
  std::shared_ptr<IAudioRender> audio_render_{nullptr};
  std::shared_ptr<utils::ClockManager> clock_{nullptr};
  AudioSampleDispatcher dispatcher_;

  constexpr static int kMaxAudioFrameQueueSize = 3;
  constexpr static int kOutNumChannels = 2;
  constexpr static int kOutSampleRate = 44100;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H
