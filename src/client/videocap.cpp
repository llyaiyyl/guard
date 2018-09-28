#include "videocap.h"

using namespace std;

videocap::videocap(AVFormatContext * ptr_format_ctx, AVCodecContext * ptr_codec_ctx, AVCodec * ptr_codec, int video_stream, int audio_stream,
                   int fps, int width, int height, int pix_fmt, int codec_id, url_type type)
{
    int ret;

    ptr_format_ctx_ = ptr_format_ctx;
    ptr_codec_ctx_ = ptr_codec_ctx;
    ptr_codec_ = ptr_codec;
    sws_ctx_ = NULL;
    frame_ = NULL;
    dst_data_[0] = NULL;

    video_stream_ = video_stream;
    audio_stream_ = audio_stream;
    fps_ = fps;
    width_ = width;
    height_ = height;
    pix_fmt_ = pix_fmt;
    codec_id_ = codec_id;
    type_ = type;

    sws_ctx_ = sws_getContext(ptr_codec_ctx_->width,
                              ptr_codec_ctx_->height,
                              ptr_codec_ctx_->pix_fmt,
                              ptr_codec_ctx_->width,
                              ptr_codec_ctx_->height,
                              AV_PIX_FMT_BGR24,
                              SWS_BILINEAR,
                              NULL,
                              NULL,
                              NULL);
    if(NULL == sws_ctx_) {
        cout << "sws_getContext error" << endl;
    }

    frame_ = av_frame_alloc();
    if(NULL == frame_) {
        cout << "av_frame_alloc error" << endl;
    }

    ret = av_image_alloc(dst_data_, dst_linesize_, width_, height_, AV_PIX_FMT_BGR24, 1);
    if(ret < 0) {
        cout << "av_image_alloc: " << ret << endl;
        dst_data_[0] = NULL;
    }

    if(NULL == sws_ctx_ || NULL == frame_ || NULL == dst_data_[0]) {
        if(dst_data_[0]) {
            av_freep(&dst_data_[0]);
            dst_data_[0] = NULL;
        }

        if(frame_) {
            av_frame_free(&frame_);
            frame_ = NULL;
        }

        if(sws_ctx_) {
            sws_freeContext(sws_ctx_);
            sws_ctx_ = NULL;
        }
    }
}

videocap::~videocap()
{
    if(dst_data_[0]) {
        av_freep(&dst_data_[0]);
        dst_data_[0] = NULL;
    }

    if(frame_) {
        av_frame_free(&frame_);
        frame_ = NULL;
    }

    if(sws_ctx_) {
        sws_freeContext(sws_ctx_);
    }

    if(ptr_codec_ctx_) {
        avcodec_close(ptr_codec_ctx_);
        avcodec_free_context(&ptr_codec_ctx_);
        ptr_codec_ctx_ = NULL;
    }

    if(ptr_format_ctx_) {
        avformat_close_input(&ptr_format_ctx_);
        ptr_format_ctx_ = NULL;
    }
}

videocap * videocap::create(const char *url, videocap::url_type type)
{
    AVFormatContext * ptr_format_ctx = NULL;
    AVCodecContext * ptr_codec_ctx = NULL;
    AVCodec * ptr_codec = NULL;
    AVDictionary * opts = NULL;
    int video_stream, audio_stream, fps;
    int ret;

    if(videocap::url_type_rtsp == type) {
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    } else if(videocap::url_type_v4l2 == type) {
        avdevice_register_all();
    } else if(videocap::url_type_unknow == type) {
        cout << "url_type_unknow" << endl;
        return NULL;
    }

    ret = avformat_open_input(&ptr_format_ctx, url, NULL, &opts);
    if(ret) {
        cout << "avformat_open_input: " << ret << endl;
        return NULL;
    }

    ret = avformat_find_stream_info(ptr_format_ctx, NULL);
    if(ret < 0) {
        cout << "avformat_find_stream_info: " << ret << endl;
        goto err0;
    }
    av_dump_format(ptr_format_ctx, 0, url, 0);

    // find video stream
    video_stream = -1;
    for(uint32_t i = 0; i < ptr_format_ctx->nb_streams; i++) {
        if(ptr_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            break;
        }
    }
    if(video_stream == -1) {
        cout << "can't find video stream" << endl;
        goto err0;
    }
    fps = (ptr_format_ctx->streams[video_stream]->avg_frame_rate.num) * 1.0 / ptr_format_ctx->streams[video_stream]->avg_frame_rate.den;

    // find audio stream
    audio_stream = -1;
    for(uint32_t i = 0; i < ptr_format_ctx->nb_streams; i++) {
        if(ptr_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = i;
            break;
        }
    }
    if(audio_stream == -1) {
        cout << "can't find audio stream" << endl;
    }

    ptr_codec = avcodec_find_decoder(ptr_format_ctx->streams[video_stream]->codecpar->codec_id);
    if(NULL == ptr_codec) {
        cout << "avcodec_find_decoder error" << endl;
        goto err0;
    }

    ptr_codec_ctx = avcodec_alloc_context3(ptr_codec);
    avcodec_parameters_to_context(ptr_codec_ctx, ptr_format_ctx->streams[video_stream]->codecpar);
    ret = avcodec_open2(ptr_codec_ctx, ptr_codec, NULL);
    if(ret < 0) {
        cout << "avcodec_open2: " << ret << endl;
        goto err1;
    }
    cout << ptr_codec_ctx->width << " x " << ptr_codec_ctx->height
         <<" pix_fmt: " << ptr_codec_ctx->pix_fmt
         <<" codec_id: " << ptr_format_ctx->streams[video_stream]->codecpar->codec_id
         <<" fps: " << fps << endl;

    return new videocap(ptr_format_ctx, ptr_codec_ctx, ptr_codec, video_stream, audio_stream,
                        fps, ptr_codec_ctx->width, ptr_codec_ctx->height, ptr_codec_ctx->pix_fmt, ptr_format_ctx->streams[video_stream]->codecpar->codec_id,
                        type);

err1:
    avcodec_free_context(&ptr_codec_ctx);
err0:
    avformat_close_input(&ptr_format_ctx);
    return NULL;
}

int videocap::read_packet(AVPacket *packet)
{
    return av_read_frame(ptr_format_ctx_, packet);
}

int videocap::read_rgbpic(const AVPacket * pkt, uint8_t ** ppdata, int * psize)
{
    int ret;

    if(NULL == sws_ctx_) {
        cout << "uninit sws_ctx" << endl;
        goto err;
    }

    ret = avcodec_send_packet(ptr_codec_ctx_, pkt);
    if(ret < 0) {
        cout << "avcodec_send_packet: " << ret << endl;
        goto err;
    }

    ret = avcodec_receive_frame(ptr_codec_ctx_, frame_);
    if(0 != ret) {
        cout << "avcodec_receive_frame: " << ret << endl;
        goto err;
    }

    ret = sws_scale(sws_ctx_,
              (const uint8_t * const *)frame_->data,
              frame_->linesize,
              0,
              frame_->height,
              dst_data_,
              dst_linesize_);
    if(0 == ret) {
        cout << "sws_scale error" << endl;
        goto err;
    }

    av_frame_unref(frame_);
    *ppdata = dst_data_[0];
    *psize = dst_linesize_[0];
    return ptr_codec_ctx_->frame_number;

err:
    av_frame_unref(frame_);
    *ppdata = NULL;
    *psize = 0;
    return -1;
}























