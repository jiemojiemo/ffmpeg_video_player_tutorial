//
// Created by user on 11/9/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_I_AUDIO_DECODER_H
#define FFMPEG_VIDEO_PLAYER_J_I_AUDIO_DECODER_H
#include "j_video_player/modules/j_frame.h"
#include "j_video_player/modules/j_media_file_info.h"
namespace j_video_player {
class IAudioDecoder {
public:
  virtual ~IAudioDecoder() = default;

  /**
   * open a audio file
   * @param file_path audio file path
   * @return 0 if success, otherwise return error code
   */
  virtual int open(const std::string &file_path) = 0;

  /**
   * check if the decoder is valid
   * @return true if valid, otherwise return false
   */
  virtual bool isValid() = 0;

  /**
   * close the decoder
   */
  virtual void close() = 0;

  /**
   * decode next frame
   * @return a shared_ptr of VideoFrame if success, otherwise return nullptr
   */
  virtual std::shared_ptr<Frame> decodeNextFrame() = 0;

  /**
   * seek to a timestamp quickly and get the video frame
   *
   * @param timestamp the timestamp(us) to seek
   * @return video frame if success, otherwise return nullptr
   */
  virtual std::shared_ptr<Frame> seekFrameQuick(int64_t timestamp) = 0;

  /**
   * seek to a timestamp precisely and get the video frame
   * @param timestamp the timestamp(us) to seek
   * @return video frame if success, otherwise return nullptr
   */
  virtual std::shared_ptr<Frame> seekFramePrecise(int64_t timestamp) = 0;

  /**
   * get the current position of the decoder
   * @return the current position(us)
   */
  virtual int64_t getPosition() = 0;

  virtual MediaFileInfo getMediaFileInfo() = 0;
};
} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_I_AUDIO_DECODER_H
