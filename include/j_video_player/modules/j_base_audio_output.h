//
// Created by user on 11/24/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_BASE_AUDIO_OUTPUT_H
#define FFMPEG_VIDEO_PLAYER_J_BASE_AUDIO_OUTPUT_H
#include "j_video_player/modules/j_i_audio_output.h"
#include "j_video_player/utils/simple_fifo.h"

namespace j_video_player {
class BaseAudioOutput : public IAudioOutput {
public:
  ~BaseAudioOutput() override = default;
  void attachAudioSource(std::shared_ptr<IAudioSource> source) override {
    source_ = std::move(source);
  }

  void attachResampler(
      std::shared_ptr<ffmpeg_utils::FFmpegAudioResampler> resampler) override {
    resampler_ = std::move(resampler);
  }

  void attachAVSyncClock(std::shared_ptr<utils::ClockManager> clock) override {
    clock_ = std::move(clock);
  }

  AudioOutputState getState() const override { return state_; }

protected:
  void pullAudioSamples(int16_t *stream, int num_samples) {
    if (source_ == nullptr) {
      return;
    }

    int output_index = 0;
    int16_t sample;
    int64_t pts{-1};
    for (; output_index < num_samples;) {
      auto ok = sample_fifo_.pop(sample);
      if (ok) {
        if (pts == -1) {
          pts = last_frame_pts_;
        }
        stream[output_index++] = sample;
      } else {
        // deque a frame and resample it
        // then push to sample_fifo_
        auto frame = source_->dequeueAudioFrame();
        if (frame == nullptr) {
          break;
        } else {
          resampleAndPushToFIFO(frame);
        }
      }
    }

    // update audio clock
    if (clock_) {
      double pts_d = static_cast<double>(pts) / AV_TIME_BASE;
      clock_->setAudioClock(pts_d);
    }
  }

  void resampleAndPushToFIFO(const std::shared_ptr<Frame> &frame) {
    last_frame_pts_ = frame->pts();

    if (resampler_) {
      auto *f = frame->f;
      auto num_samples_out_per_channel =
          resampler_->convert((const uint8_t **)f->data, f->nb_samples);
      auto num_total_samples =
          num_samples_out_per_channel * resampler_->out_num_channels();
      auto *int16_resample_data =
          reinterpret_cast<int16_t *>(resampler_->resample_data[0]);
      for (auto i = 0; i < num_total_samples; ++i) {
        sample_fifo_.push(std::move(int16_resample_data[i]));
      }
    } else {
      auto *f = frame->f;
      auto num_total_samples = f->nb_samples * f->channels;
      for (auto i = 0; i < num_total_samples; ++i) {
        sample_fifo_.push(std::move(f->data[0][i]));
      }
    }
  }

  std::shared_ptr<IAudioSource> source_;
  std::atomic<AudioOutputState> state_{AudioOutputState::kIdle};
  std::shared_ptr<ffmpeg_utils::FFmpegAudioResampler> resampler_{nullptr};
  std::shared_ptr<utils::ClockManager> clock_{nullptr};
  int64_t last_frame_pts_{-1};
  constexpr static int kFIFOSize = 44100;
  utils::SimpleFIFO<int16_t> sample_fifo_{kFIFOSize};
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_BASE_AUDIO_OUTPUT_H
