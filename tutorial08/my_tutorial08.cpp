//
// Created by user on 11/13/23.
//
#include "j_video_player/modules/j_ffmpeg_av_decoder.h"
#include "j_video_player/modules/j_sdl2_audio_output.h"
#include "j_video_player/modules/j_sdl2_video_output.h"
#include "j_video_player/modules/j_simple_player.h"
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

  auto video_source = std::make_shared<SimpleVideoSource>();
  video_source->prepare(video_decoder);

  auto audio_source = std::make_shared<SimpleAudioSource>();
  audio_source->prepare(audio_decoder);

  auto video_output = std::make_shared<SDL2VideoOutput>();
  auto audio_output = std::make_shared<SDL2AudioOutput>();

  auto player =
      SimplePlayer{video_source, audio_source, video_output, audio_output};

  int ret = player.open(in_file);
  RETURN_IF_ERROR_LOG(ret, "open player failed, exit");

  auto media_file_info = player.getMediaFileInfo();

  VideoOutputParameters video_output_param;
  video_output_param.width = media_file_info.width;
  video_output_param.height = media_file_info.height;
  video_output_param.pixel_format = AVPixelFormat::AV_PIX_FMT_YUV420P;

  AudioOutputParameters audio_output_param;
  audio_output_param.sample_rate = 44100;
  audio_output_param.channels = 2;
  audio_output_param.num_frames_of_buffer = 1024;

  ret = player.prepareForOutput(video_output_param, audio_output_param);
  RETURN_IF_ERROR_LOG(ret, "prepare player failed, exit");

  player.play();

  SDL_Event event;
  auto doSeekRelative = [&](float sec) {
    auto current_pos = player.getCurrentPosition();
    auto target_pos = current_pos + static_cast<int64_t>(sec * AV_TIME_BASE);
    LOGE("seek to %lf\n", double(target_pos) / AV_TIME_BASE);
    player.seek(target_pos);
  };
  auto doPauseOrPlaying = [&]() {
    auto is_playing = player.isPlaying();
    if (is_playing) {
      player.pause();
    } else {
      player.play();
    }
  };

  for (;;) {
    SDL_PollEvent(&event);
    switch (event.type) {
    case SDL_KEYDOWN: {
      switch (event.key.keysym.sym) {
      case SDLK_LEFT: {
        doSeekRelative(-5.0);
        break;
      }

      case SDLK_RIGHT: {
        doSeekRelative(5.0);
        break;
      }

      case SDLK_DOWN: {
        doSeekRelative(-60.0);
        break;
      }

      case SDLK_UP: {
        doSeekRelative(60.0);
        break;
      }
      case SDLK_SPACE: {
        doPauseOrPlaying();
        break;
      }
      default:
        break;
      }
      break;
    }
    case SDL_QUIT:
      player.stop();
      return 0;
    }
  }
}