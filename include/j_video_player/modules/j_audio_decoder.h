//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H
#define FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H

#include "j_video_player/ffmpeg_utils/ffmpeg_audio_resampler.h"
#include "j_video_player/ffmpeg_utils/ffmpeg_waitable_frame_queue.h"
#include "j_video_player/modules/j_audio_render.h"
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
    const int num_samples_per_channel = num_samples_of_stream / kOutNumChannels;

    auto [num_sample_out, last_pts] = pullAudioSamples(
        num_samples_per_channel, kOutNumChannels, (int16_t *)(stream));

    if (clock_) {
      clock_->setAudioClock(last_pts);
    }
  }

  std::pair<int, double> pullAudioSamples(size_t out_num_samples_per_channel,
                                          size_t out_num_channels,
                                          int16_t *out_buffer) {

    const auto total_need_samples =
        out_num_samples_per_channel * out_num_channels;
    auto num_samples_need = total_need_samples;
    auto *sample_fifo = audio_sample_fifo.get();
    int sample_index = 0;
    int16_t s = 0;

    auto getSampleFromFIFO = [&]() {
      for (; num_samples_need > 0;) {
        if (sample_fifo->pop(s)) {
          out_buffer[sample_index++] = s;
          --num_samples_need;
        } else {
          break;
        }
      }
    };

    auto resampleAudioAndPushToFIFO = [&](AVFrame *frame) {
      int num_samples_out_per_channel = audio_resampler_->convert(
          (const uint8_t **)frame->data, frame->nb_samples);
      int num_samples_total = num_samples_out_per_channel * frame->channels;
      auto *int16_resample_data =
          reinterpret_cast<int16_t *>(audio_resampler_->resample_data[0]);
      for (int i = 0; i < num_samples_total; ++i) {
        sample_fifo->push(std::move(int16_resample_data[i]));
      }
    };

    for (;;) {
      // try to get samples from fifo
      getSampleFromFIFO();

      if (num_samples_need <= 0) {
        break;
      } else {
        auto *frame = audio_frame_queue_.tryPop();
        ON_SCOPE_EXIT([&frame] {
          if (frame != nullptr) {
            av_frame_unref(frame);
            av_frame_free(&frame);
          }
        });

        if (frame == nullptr) {
          auto remain = total_need_samples - num_samples_need;
          std::fill_n(out_buffer + sample_index, remain, 0);
          return {remain, last_audio_frame_pts_};
        } else {
          last_audio_frame_pts_ = frame->pts / (double)AV_TIME_BASE;
          resampleAudioAndPushToFIFO(frame);
        }
      }
    }
    return {out_num_samples_per_channel, last_audio_frame_pts_};
  }

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

      audio_sample_fifo =
          std::make_unique<AudioSampleFIFO>(1024 * out_num_channels);

      if (audio_render_) {
        audio_render_->initAudioRender();
      }
    }
  }
  void OnDecoderDone() override { audio_resampler_ = nullptr; }
  void OnFrameAvailable(AVFrame *frame) override {
    audio_frame_queue_.waitAndPush(frame);
  }

  // ---- testing ----
  ffmpeg_utils::FFMPEGAudioResampler *getResampler() const {
    return audio_resampler_.get();
  }

private:
  std::unique_ptr<ffmpeg_utils::FFMPEGAudioResampler> audio_resampler_{nullptr};
  std::shared_ptr<IAudioRender> audio_render_{nullptr};
  std::shared_ptr<utils::ClockManager> clock_{nullptr};
  ffmpeg_utils::WaitableFrameQueue audio_frame_queue_{kMaxAudioFrameQueueSize};
  using AudioSampleFIFO = utils::SimpleFIFO<int16_t>;
  std::unique_ptr<AudioSampleFIFO> audio_sample_fifo;
  double last_audio_frame_pts_{0}; // based on seconds

  constexpr static int kMaxAudioFrameQueueSize = 3;
  constexpr static int kOutNumChannels = 2;
  constexpr static int kOutSampleRate = 44100;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H
