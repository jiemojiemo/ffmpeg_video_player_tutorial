# 基于 FFmpeg 的视频播放器教程

相关博客：

1. [基于 FFmpeg 的跨平台视频播放器简明教程（一）：FFMPEG + Conan 环境集成](https://blog.csdn.net/weiwei9363/article/details/130950529)
2. [基于 FFmpeg 的跨平台视频播放器简明教程（二）：基础知识和解封装（demux）](https://blog.csdn.net/weiwei9363/article/details/131014036)
3. [基于 FFmpeg 的跨平台视频播放器简明教程（三）：视频解码](https://blog.csdn.net/weiwei9363/article/details/131119444)
4. [基于 FFmpeg 的跨平台视频播放器简明教程（四）：像素格式与格式转换](https://blog.csdn.net/weiwei9363/article/details/131176974)
5. [基于 FFmpeg 的跨平台视频播放器简明教程（五）：使用 SDL 播放视频](https://blog.csdn.net/weiwei9363/article/details/131462303)
6. [基于 FFmpeg 的跨平台视频播放器简明教程（六）：使用 SDL 播放音频和视频](https://blog.csdn.net/weiwei9363/article/details/131567077)
7. [基于 FFmpeg 的跨平台视频播放器简明教程（七）：使用多线程解码视频和音频](https://blog.csdn.net/weiwei9363/article/details/131818080)
8. [基于 FFmpeg 的跨平台视频播放器简明教程（八）：音画同步](https://blog.csdn.net/weiwei9363/article/details/132030633)
9. [基于 FFmpeg 的跨平台视频播放器简明教程（九）：Seek 策略](https://blog.csdn.net/weiwei9363/article/details/132307253)
10. [基于 FFmpeg 的跨平台视频播放器简明教程（十）：在 Android 运行 FFmpeg](https://blog.csdn.net/weiwei9363/article/details/134027655)
11. [基于 FFmpeg 的跨平台视频播放器简明教程（十一）：一种简易播放器的架构介绍](https://blog.csdn.net/weiwei9363/article/details/134470831)

# 编译，How to build

首先下载源码：

```shell
git clone https://github.com/jiemojiemo/ffmpeg_video_player_tutorial.git
cd ffmpeg_video_player_tutorial
git submodule update --init --recursive 
```

## Mac

1. 安装 conan2，参考 [官方文档](https://docs.conan.io/2/installation.html)

```shell
pip install conan
```

2. 使用 conan 安装依赖库

```shell
mkdir build
conan install . --output-folder=build --build=missing --settings=build_type=Debug
```

3. 使用 cmake 编译

```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

## Android

使用 Android Studio 打开 `android` 目录，然后编译运行即可。