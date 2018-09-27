#ifndef VIDEOCAP_H
#define VIDEOCAP_H

#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavdevice/avdevice.h"
}

class videocap
{
public:
    enum url_type {
        url_type_unknow = -1,
        url_type_rtsp,
        url_type_file,
        url_type_v4l2
    };

    ~videocap();
    static videocap * create(const char * url, videocap::url_type type);
    int read_packet(AVPacket * packet);

    int get_video_stream(void) { return video_stream_; }
    int get_audio_stream(void) { return audio_stream_; }
    int get_fps(void) { return fps_; }
    int get_width(void) { return width_; }
    int get_height(void) { return height_; }
    int get_pix_fmt(void) { return pix_fmt_; }
    int get_codec_id(void) { return codec_id_; }

private:
    videocap(AVFormatContext * ptr_format_ctx, AVCodecContext * ptr_codec_ctx, AVCodec * ptr_codec, int video_stream, int audio_stream,
             int fps, int width, int height, int pix_fmt, int codec_id);

    AVFormatContext * ptr_format_ctx_;
    AVCodecContext * ptr_codec_ctx_;
    AVCodec * ptr_codec_;

    int fps_;
    int width_, height_;
    int pix_fmt_, codec_id_;
    int video_stream_, audio_stream_;
};

#endif // VIDEOCAP_H


