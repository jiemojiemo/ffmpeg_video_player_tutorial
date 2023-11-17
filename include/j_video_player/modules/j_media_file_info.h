//
// Created by user on 11/7/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_MEDIA_FILE_INFO_H
#define FFMPEG_VIDEO_PLAYER_J_MEDIA_FILE_INFO_H
#include "j_video_player/ffmpeg_utils/ffmpeg_headers.h"
#include <string>
namespace j_video_player {
class MediaFileInfo {
public:
  std::string file_path;
  int width{0};
  int height{0};
  int64_t duration{0};
  int64_t bit_rate{0};
  double fps{0};
  int pixel_format{0}; // AVPixelFormat
  AVRational video_stream_timebase{AVRational{0, 0}};

  int sample_rate{0};
  int channels{0};
  int sample_format{0}; // AVSampleFormat
  int channel_layout{0};
  AVRational audio_stream_timebase{AVRational{0, 0}};
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_MEDIA_FILE_INFO_H
