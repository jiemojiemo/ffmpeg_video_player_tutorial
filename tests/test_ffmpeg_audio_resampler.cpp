//
// Created by user on 8/7/22.
//
#include <gmock/gmock.h>

#include "j_video_player/ffmpeg_utils/ffmpeg_audio_resampler.h"
#include "j_video_player/ffmpeg_utils/ffmpeg_format_utils.h"

using namespace testing;
using namespace ffmpeg_utils;

class AFFmpegAudioResampler : public Test {
public:
  FFmpegAudioResampler resampler;
  int in_sample_rate = 44100;
  int in_num_channels = 2;
  int in_channel_layout = AV_CH_LAYOUT_STEREO;
  AVSampleFormat in_sample_format = AV_SAMPLE_FMT_FLTP;
  int out_sample_rate = 48000;
  int out_num_channels = 2;
  AVSampleFormat out_sample_format = AV_SAMPLE_FMT_S16P;
  int out_channel_layout = AV_CH_LAYOUT_STEREO;
  int max_frames_size = 1024;
};

TEST_F(AFFmpegAudioResampler, PrepareWillInitInternalSWR) {
  resampler.prepare(in_num_channels, out_num_channels, in_channel_layout,
                    out_channel_layout, in_sample_rate, out_sample_rate,
                    in_sample_format, out_sample_format, max_frames_size);

  ASSERT_THAT(resampler.swr, NotNull());
}

TEST_F(AFFmpegAudioResampler, PrepareWillInitChannelData) {
  resampler.prepare(in_num_channels, out_num_channels, in_channel_layout,
                    out_channel_layout, in_sample_rate, out_sample_rate,
                    in_sample_format, out_sample_format, max_frames_size);

  size_t expected =
      FormatUtils::getChannelDataSize(out_num_channels, out_sample_format);
  ASSERT_THAT(resampler.resample_data.size(), Eq(expected));
}

TEST_F(AFFmpegAudioResampler, CanConvertAudioSamples) {
  resampler.prepare(in_num_channels, out_num_channels, in_channel_layout,
                    out_channel_layout, in_sample_rate, out_sample_rate,
                    in_sample_format, out_sample_format, max_frames_size);

  int in_num_samples_per_channel = 10;
  std::vector<float> left_data(in_num_samples_per_channel, 0.5);
  std::vector<float> right_data(in_num_samples_per_channel, 0.5);
  std::vector<const uint8_t *> data_refer_to{(uint8_t *)left_data.data(),
                                             (uint8_t *)right_data.data()};

  int ret = resampler.convert(data_refer_to.data(), in_num_samples_per_channel);
  ASSERT_THAT(ret, Ge(0));
}