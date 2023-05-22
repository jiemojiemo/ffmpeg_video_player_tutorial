//
// Created by user on 5/22/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H
#define FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H

#include "j_video_player/ffmpeg_utils/ffmpeg_audio_resampler.h"
#include "j_video_player/modules/j_ffmpeg_base_decoder.h"

namespace j_video_player {
class AudioDecoder : public FFMPEGBaseDecoder {
public:
  AudioDecoder(const std::string &file_path) {
    init(file_path, AVMEDIA_TYPE_AUDIO);
  }

  ~AudioDecoder() override { uninit(); }

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
    }
  }
  void OnDecoderDone() override { audio_resampler_ = nullptr; }
  void OnFrameAvailable(AVFrame *frame) override { (void)(frame); }

  // ---- testing ----
  ffmpeg_utils::FFMPEGAudioResampler *getResampler() const {
    return audio_resampler_.get();
  }

private:
  std::unique_ptr<ffmpeg_utils::FFMPEGAudioResampler> audio_resampler_{nullptr};
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_AUDIO_DECODER_H
