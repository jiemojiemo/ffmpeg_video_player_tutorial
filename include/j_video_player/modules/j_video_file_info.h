//
// Created by user on 11/7/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_VIDEO_FILE_INFO_H
#define FFMPEG_VIDEO_PLAYER_J_VIDEO_FILE_INFO_H
#include "j_video_player/ffmpeg_utils/ffmpeg_headers.h"
#include <string>
namespace j_video_player {
class VideoFileInfo {
public:
  std::string file_path;
  int width{0};
  int height{0};
  int64_t duration{0};
  int64_t bit_rate{0};
  double fps{0};
  AVRational video_stream_timebase{AVRational{0, 0}};
  AVRational audio_stream_timebase{AVRational{0, 0}};
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_VIDEO_FILE_INFO_H
