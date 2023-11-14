//
// Created by user on 11/13/23.
//
#include "j_video_player/modules/j_ffmpeg_av_decoder.h"
#include "j_video_player/modules/j_sdl2_audio_output.h"
#include "j_video_player/modules/j_sdl2_video_output.h"
#include "j_video_player/modules/j_simple_source.h"
void printHelpMenu() {
  printf("Invalid arguments.\n\n");
  printf("Usage: ./tutorial03 <filename> <max-frames-to-decode>\n\n");
  printf("e.g: ./tutorial03 /home/rambodrahmani/Videos/Labrinth-Jealous.mp4 "
         "200\n");
}

using namespace j_video_player;
using namespace std::chrono_literals;
int main(int argc, char *argv[]) {
  if (argc < 2) {
    printHelpMenu();
    return -1;
  }
  std::string in_file = argv[1];

  auto video_decoder = std::make_shared<FFmpegVideoDecoder>();
  auto audio_decoder = std::make_shared<FFmpegAudioDecoder>();

  auto video_source = std::make_shared<SimpleVideoSource>(video_decoder);
  auto audio_source = std::make_shared<SimpleAudioSource>(audio_decoder);

  auto video_output = std::make_shared<SDL2VideoOutput>();
  auto audio_output = std::make_shared<SDL2AudioOutput>();

  auto image_converter = std::make_shared<ffmpeg_utils::FFMPEGImageConverter>();
  auto resampler = std::make_shared<ffmpeg_utils::FFmpegAudioResampler>();
  auto av_clock = std::make_shared<utils::ClockManager>();

  // open source
  auto ret = video_source->open(in_file);
  if (ret != 0) {
    LOGE("open source failed, exit");
    return -1;
  }

  ret = audio_source->open(in_file);
  if (ret != 0) {
    LOGE("open source failed, exit");
    return -1;
  }

  auto media_file_info = video_source->getMediaFileInfo();

  // prepare image converter
  int expected_width = media_file_info.width;
  int expected_height = media_file_info.height;
  int expected_pixel_format = AV_PIX_FMT_YUV420P;
  ret = image_converter->prepare(media_file_info.width, media_file_info.height,
                                 (AVPixelFormat)media_file_info.pixel_format,
                                 expected_width, expected_height,
                                 (AVPixelFormat)expected_pixel_format, 0,
                                 nullptr, nullptr, nullptr);
  if (ret != 0) {
    LOGE("prepare image converter failed, exit");
    return -1;
  }

  // prepare audio resampler
  AudioOutputParameters audio_output_param;
  audio_output_param.sample_rate = 44100;
  audio_output_param.channels = 2;
  audio_output_param.num_frames_of_buffer = 1024;

  int max_frame_size =
      audio_output_param.num_frames_of_buffer * 4 * audio_output_param.channels;
  int output_channel_layout = (audio_output_param.channels == 1)
                                  ? AV_CH_LAYOUT_MONO
                                  : AV_CH_LAYOUT_STEREO;

  ret = resampler->prepare(
      media_file_info.channels, audio_output_param.channels,
      media_file_info.channel_layout, output_channel_layout,
      media_file_info.sample_rate, audio_output_param.sample_rate,
      (AVSampleFormat)media_file_info.sample_format, AV_SAMPLE_FMT_S16,
      max_frame_size);

  if (ret != 0) {
    LOGE("prepare audio resampler failed, exit");
    return -1;
  }

  video_output->attachSource(video_source);
  video_output->attachImageConverter(image_converter);
  video_output->attachAVSyncClock(av_clock);

  audio_output->attachSource(audio_source);
  audio_output->attachResampler(resampler);
  audio_output->attachAVSyncClock(av_clock);

  // prepare video output
  VideoOutputParameters output_param;
  output_param.width = expected_width;
  output_param.height = expected_height;
  output_param.pixel_format = SDL_PIXELFORMAT_IYUV;
  ret = video_output->prepare(output_param);
  if (ret != 0) {
    LOGE("prepare video output failed, exit");
    return -1;
  }

  // prepare audio output
  ret = audio_output->prepare(audio_output_param);
  if (ret != 0) {
    LOGE("prepare audio output failed, exit");
    return -1;
  }

  video_source->play();
  audio_source->play();

  video_output->play();
  audio_output->play();

  SDL_Event event;
  for (;;) {
    SDL_PollEvent(&event);
    switch (event.type) {
    case SDL_QUIT:
      video_source->stop();
      video_output->stop();

      audio_source->stop();
      audio_output->stop();
      return 0;
    }
  }
}