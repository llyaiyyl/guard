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
    int read_rgbpic(const AVPacket * pkt, uint8_t ** ppdata, int * psize);

    int get_video_stream(void) { return video_stream_; }
    int get_audio_stream(void) { return audio_stream_; }
    int get_fps(void) { return fps_; }
    int get_width(void) { return width_; }
    int get_height(void) { return height_; }
    int get_pix_fmt(void) { return pix_fmt_; }
    int get_codec_id(void) { return codec_id_; }
    videocap::url_type get_url_type(void) { return type_; }

private:
    videocap(AVFormatContext * ptr_format_ctx, AVCodecContext * ptr_codec_ctx, AVCodec * ptr_codec, int video_stream, int audio_stream,
             int fps, int width, int height, int pix_fmt, int codec_id, videocap::url_type type);

    AVFormatContext * ptr_format_ctx_;
    AVCodecContext * ptr_codec_ctx_;
    AVCodec * ptr_codec_;
    struct SwsContext * sws_ctx_;
    AVFrame * frame_;
    uint8_t * dst_data_[4];
    int dst_linesize_[4];

    int fps_;
    int width_, height_;
    int pix_fmt_, codec_id_;
    int video_stream_, audio_stream_;
    videocap::url_type type_;
};

#endif // VIDEOCAP_H


