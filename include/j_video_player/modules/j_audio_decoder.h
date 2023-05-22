//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H
#define FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H

#include "j_video_player/ffmpeg_utils/ffmpeg_audio_resampler.h"
#include "j_video_player/modules/j_audio_render.h"
#include "j_video_player/modules/j_ffmpeg_base_decoder.h"

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
  }

  void onPrepareDecoder() override {
    if (isInitSucc()) {
      audio_resampler_ = std::make_unique<ffmpeg_utils::FFMPEGAudioResampler>();
      auto codec_context = getCodecContext();
      auto in_num_channels = codec_context->channels;
      auto out_num_channels = 2;
      auto in_sample_rate = codec_context->sample_rate;
      auto out_sample_rate = 44100;
      auto in_channel_layout = codec_context->channel_layout;
      auto out_channel_layout = AV_CH_LAYOUT_STEREO;
      auto in_sample_format = codec_context->sample_fmt;
      auto out_sample_format = AV_SAMPLE_FMT_S16;
      auto max_frames_size = 4096;
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
      if (audio_render_) {
        auto *int16_resample_data =
            reinterpret_cast<int16_t *>(audio_resampler_->resample_data[0]);
        audio_render_->renderAudioData(int16_resample_data, num_total_samples);
      }
    }
  }

  // ---- testing ----
  ffmpeg_utils::FFMPEGAudioResampler *getResampler() const {
    return audio_resampler_.get();
  }

private:
  std::unique_ptr<ffmpeg_utils::FFMPEGAudioResampler> audio_resampler_{nullptr};
  std::shared_ptr<IAudioRender> audio_render_{nullptr};
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H
