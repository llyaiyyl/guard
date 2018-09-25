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

private:
    videocap(AVFormatContext * ptr_format_ctx, AVCodecContext * ptr_codec_ctx, AVCodec * ptr_codec);

    AVFormatContext * ptr_format_ctx_;
    AVCodecContext * ptr_codec_ctx_;
    AVCodec * ptr_codec_;

    enum AVCodecID codec_id_;
    int fps_;
    int width_, height_;
};

#endif // VIDEOCAP_H


