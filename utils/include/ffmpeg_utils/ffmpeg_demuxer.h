//
// Created by user on 1/15/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_FFMPEG_DEMUXER_H
#define FFMPEG_VIDEO_PLAYER_FFMPEG_DEMUXER_H
#pragma once
#include "ffmpeg_utils/ffmpeg_common_utils.h"
#include "ffmpeg_utils/ffmpeg_headers.h"
#include <string>
namespace ffmpeg_utils {
class FFMPEGDemuxer {
public:
  ~FFMPEGDemuxer() {
    close();
  }
  int openFile(const std::string &file_path) {
    int ret =
        avformat_open_input(&format_ctx_, file_path.c_str(), NULL, NULL); // [2]
    RETURN_IF_ERROR(ret);

    ret = avformat_find_stream_info(format_ctx_, NULL);
    RETURN_IF_ERROR(ret);

    findFirstVideoStreamIndex();
    findFirstAudioStreamIndex();

    allocateInternalPacket();

    return 0;
  }

  std::pair<int, AVPacket*> readPacket()
  {
    if(!isValid()){
      return {-1, nullptr};
    }

    int ret = av_read_frame(format_ctx_, packet_);
    return {ret, packet_};
  }

  bool isValid() const { return format_ctx_ != nullptr; }

  void dumpFormat() const
  {
    if(isValid()){
      av_dump_format(format_ctx_, 0, format_ctx_->url, 0);
    }
  }

  AVFormatContext* getFormatContext() const{
    return format_ctx_;
  }

  int getStreamCount() const {
    if (isValid()) {
      return static_cast<int>(format_ctx_->nb_streams);
    }
    return 0;
  }

  int getVideoStreamIndex() const { return video_stream_index_; }

  int getAudioStreamIndex() const { return audio_stream_index_; }

private:
  void close()
  {
    if (format_ctx_) {
      avformat_close_input(&format_ctx_);
    }

    if(packet_){
      av_packet_unref(packet_);
      av_packet_free(&packet_);
    }
  }
  void findFirstVideoStreamIndex() {
    video_stream_index_ = findFirstStreamIndex(AVMEDIA_TYPE_VIDEO);
  }

  void findFirstAudioStreamIndex() {
    audio_stream_index_ = findFirstStreamIndex(AVMEDIA_TYPE_AUDIO);
  }

  int findFirstStreamIndex(AVMediaType target_type) {
    for (int i = 0; i < format_ctx_->nb_streams; ++i) {
      if (format_ctx_->streams[i]->codecpar->codec_type == target_type) {
        return i;
      }
    }
    return -1;
  }


  void allocateInternalPacket()
  {
    packet_ = av_packet_alloc();
  }

  AVFormatContext *format_ctx_{nullptr};
  AVPacket* packet_{nullptr};

  int video_stream_index_{-1};
  int audio_stream_index_{-1};

};
} // namespace ffmpeg_utils


#endif // FFMPEG_VIDEO_PLAYER_FFMPEG_DEMUXER_H
