//
// Created by user on 11/17/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_J_SIMPLE_PLAYER_H
#define FFMPEG_VIDEO_PLAYER_J_SIMPLE_PLAYER_H
#include "j_video_player/modules/j_i_audio_output.h"
#include "j_video_player/modules/j_i_source.h"
#include "j_video_player/modules/j_i_video_output.h"
namespace j_video_player {
class SimplePlayer {
public:
  SimplePlayer(std::shared_ptr<IVideoSource> video_source,
               std::shared_ptr<IAudioSource> audio_source,
               std::shared_ptr<IVideoOutput> video_output,
               std::shared_ptr<IAudioOutput> audio_output)
      : video_source(std::move(video_source)),
        audio_source(std::move(audio_source)),
        video_output(std::move(video_output)),
        audio_output(std::move(audio_output)),
        image_converter_(
            std::make_shared<ffmpeg_utils::FFMPEGImageConverter>()),
        resampler_(std::make_shared<ffmpeg_utils::FFmpegAudioResampler>()),
        clock_(std::make_shared<utils::ClockManager>()) {
    attachComponents();
  }

  int open(const std::string &in_file) {
    auto ret = video_source->open(in_file);
    RETURN_IF_ERROR_LOG(ret, "open source failed, exit");

    ret = audio_source->open(in_file);
    RETURN_IF_ERROR_LOG(ret, "open source failed, exit");

    media_file_info_ = video_source->getMediaFileInfo();

    return 0;
  }

  const MediaFileInfo &getMediaFileInfo() const { return media_file_info_; }

  MediaFileInfo getMediaFileInfo() { return media_file_info_; }

  int prepareForOutput(const VideoOutputParameters &v_out_params,
                       const AudioOutputParameters &a_out_params) {

    int ret = prepareImageConverter(media_file_info_, v_out_params);
    RETURN_IF_ERROR(ret);

    ret = prepareAudioResampler(media_file_info_, a_out_params);
    RETURN_IF_ERROR(ret);

    ret = prepareVideoOutput(v_out_params);
    RETURN_IF_ERROR(ret);

    ret = prepareAudioOutput(a_out_params);
    RETURN_IF_ERROR(ret);

    return 0;
  }

  int play() {
    int ret = 0;
    if (video_source) {
      ret |= video_source->play();
    }
    if (audio_source) {
      ret |= audio_source->play();
    }
    if (video_output) {
      ret |= video_output->play();
    }
    if (audio_output) {
      ret |= audio_output->play();
    }

    if (ret != 0) {
      LOGE("play failed, exit");
      return -1;
    }

    return 0;
  }

  int stop() {
    int ret = 0;
    if (video_source) {
      ret |= video_source->stop();
    }
    if (audio_source) {
      ret |= audio_source->stop();
    }
    if (video_output) {
      ret |= video_output->stop();
    }
    if (audio_output) {
      ret |= audio_output->stop();
    }
    if (ret != 0) {
      LOGE("stop failed, exit");
      return -1;
    }
    return 0;
  }

  int seek(int64_t timestamp) {
    int ret = 0;
    if (video_source) {
      ret |= video_source->seek(timestamp);
    }
    if (audio_source) {
      ret |= audio_source->seek(timestamp);
    }
    if (ret != 0) {
      LOGE("seek failed, exit");
      return -1;
    }
    return 0;
  }

  int pause() {
    int ret = 0;
    if (video_source) {
      ret |= video_source->pause();
    }
    if (audio_source) {
      ret |= audio_source->pause();
    }
    if (ret != 0) {
      LOGE("pause failed, exit");
      return -1;
    }
    return 0;
  }

  int64_t getDuration() const {
    int64_t v_duration = 0;
    int64_t a_duration = 0;
    if (video_source) {
      v_duration = video_source->getDuration();
    }
    if (audio_source) {
      a_duration = audio_source->getDuration();
    }
    return std::max(v_duration, a_duration);
  }

  int64_t getCurrentPosition() {
    if (video_source) {
      return video_source->getCurrentPosition();
    }
    if (audio_source) {
      return audio_source->getCurrentPosition();
    }
    return 0;
  }

  bool isPlaying() const {
    if (video_source) {
      return video_source->getState() == SourceState::kPlaying;
    }
    if (audio_source) {
      return audio_source->getState() == SourceState::kPlaying;
    }
    return false;
  }

  bool isStopped() const {
    if (video_source) {
      return video_source->getState() == SourceState::kStopped || video_source->getState() == SourceState::kIdle;
    }
    if (audio_source) {
      return audio_source->getState() == SourceState::kStopped || audio_source->getState() == SourceState::kIdle;
    }
    return false;
  }

  std::shared_ptr<IVideoSource> video_source;
  std::shared_ptr<IAudioSource> audio_source;
  std::shared_ptr<IVideoOutput> video_output;
  std::shared_ptr<IAudioOutput> audio_output;

private:
  int prepareImageConverter(const MediaFileInfo &src_media_file_info,
                            const VideoOutputParameters &v_out_params) {
    if (video_output == nullptr) {
      return 0;
    }

    int expected_width = v_out_params.width;
    int expected_height = v_out_params.height;
    int expected_pixel_format = v_out_params.pixel_format;
    auto ret = image_converter_->prepare(
        src_media_file_info.width, src_media_file_info.height,
        (AVPixelFormat)src_media_file_info.pixel_format, expected_width,
        expected_height, (AVPixelFormat)expected_pixel_format, 0, nullptr,
        nullptr, nullptr);
    RETURN_IF_ERROR_LOG(ret, "prepare image converter failed, exit");
    return 0;
  }

  int prepareAudioResampler(const MediaFileInfo &src_media_file_info,
                            const AudioOutputParameters &a_out_params) {
    if (audio_output == nullptr) {
      return 0;
    }

    int max_frame_size =
        a_out_params.num_frames_of_buffer * 4 * a_out_params.channels;
    int output_channel_layout =
        (a_out_params.channels == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;

    auto ret = resampler_->prepare(
        src_media_file_info.channels, a_out_params.channels,
        src_media_file_info.channel_layout, output_channel_layout,
        src_media_file_info.sample_rate, a_out_params.sample_rate,
        (AVSampleFormat)src_media_file_info.sample_format, AV_SAMPLE_FMT_S16,
        max_frame_size);
    RETURN_IF_ERROR_LOG(ret, "prepare audio resampler failed, exit");
    return 0;
  }

  int prepareVideoOutput(const VideoOutputParameters &v_out_params) {
    if (video_output) {
      auto ret = video_output->prepare(v_out_params);
      RETURN_IF_ERROR_LOG(ret, "prepare video output failed, exit");
    }
    return 0;
  }

  int prepareAudioOutput(const AudioOutputParameters &a_out_params) {
    if (audio_output) {
      auto ret = audio_output->prepare(a_out_params);
      RETURN_IF_ERROR_LOG(ret, "prepare audio output failed, exit");
    }
    return 0;
  }

  void attachComponents() {
    if (video_output) {
      video_output->attachVideoSource(video_source);
      video_output->attachImageConverter(image_converter_);
      video_output->attachAVSyncClock(clock_);
    }

    if (audio_output) {
      audio_output->attachAudioSource(audio_source);
      audio_output->attachResampler(resampler_);
      audio_output->attachAVSyncClock(clock_);
    }
  }

  std::shared_ptr<ffmpeg_utils::FFMPEGImageConverter> image_converter_;
  std::shared_ptr<ffmpeg_utils::FFmpegAudioResampler> resampler_;
  std::shared_ptr<utils::ClockManager> clock_;
  MediaFileInfo media_file_info_;
};

} // namespace j_video_player

#endif // FFMPEG_VIDEO_PLAYER_J_SIMPLE_PLAYER_H
