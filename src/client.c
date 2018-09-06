#include <stdio.h>
#include <unistd.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"



static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    fprintf(f, "P6\n%d %d\n255\n", xsize, ysize);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize * 3, f);
    fclose(f);
}

int main(int argc, char * argv[])
{
	AVFormatContext *pFormatCtx = NULL;
	AVCodecContext *pCodecCtx = NULL;
	AVCodec *pCodec = NULL;
	AVPacket packet;
	AVFrame *frame;
	struct SwsContext *sws_ctx;
	AVDictionary * opts = NULL;
    uint8_t *dst_data[4];
    int dst_linesize[4];
    int dst_w, dst_h;
	int ret, i, videoStream, frame_count;
	char buf[1024];

	if(NULL != argv[2]) {
		sscanf(argv[2], "%d", &frame_count);
	} else {
		frame_count = 10;
	}

	av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	if((ret = avformat_open_input(&pFormatCtx, argv[1], NULL, &opts))) {
		printf("avformat_open_input error %d\n", ret);
		return ret;
	}

	if((ret = avformat_find_stream_info(pFormatCtx, NULL)) < 0) {
		printf("avformat_find_stream_info error %d\n", ret);
		goto err0;
	}
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	videoStream = -1;
	for(i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if(videoStream == -1)
		goto err0;

	pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
	if(NULL == pCodec) {
		printf("avcodec_find_decoder error\n");
		goto err0;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
	if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("avcodec_open2 error\n");
		goto err0;
	}
	printf("video: %d x %d, pix_fmt: %d\n", pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt);
	dst_w = pCodecCtx->width;
	dst_h = pCodecCtx->height;

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
    		dst_w, dst_h, AV_PIX_FMT_RGB24,
			SWS_BILINEAR, NULL, NULL, NULL);
    if(NULL == sws_ctx) {
    	printf("sws_getContext error\n");
    	goto err1;
    }

    ret = av_image_alloc(dst_data, dst_linesize, dst_w, dst_h, AV_PIX_FMT_RGB24, 1);
    if(ret < 0) {
    	printf("av_image_alloc error\n");
    	goto err1;
    }

	frame = av_frame_alloc();
	while(av_read_frame(pFormatCtx, &packet) >= 0 && frame_count > 0) {
		if(packet.stream_index == videoStream) {
			ret = avcodec_send_packet(pCodecCtx, &packet);
			if(ret < 0) {
				printf("avcodec_send_packet error %d\n", ret);
				goto err1;
			}
			ret = avcodec_receive_frame(pCodecCtx, frame);
			if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				continue;
			else if(ret < 0) {
				printf("avcodec_receive_frame error %d\n", ret);
				goto err1;
			}

			ret = sws_scale(sws_ctx, (const uint8_t * const *)frame->data, frame->linesize, 0, frame->height,
					dst_data, dst_linesize);

	        printf("saving frame %3d\n", pCodecCtx->frame_number);
	        fflush(stdout);
	        snprintf(buf, sizeof(buf), "%s-%d.ppm", "pic", pCodecCtx->frame_number);
	        pgm_save(dst_data[0], dst_linesize[0],
	        		dst_w, dst_h, buf);
	        frame_count--;
		}
	}

err1:
	av_freep(&dst_data[0]);
	av_frame_free(&frame);
	avcodec_close(pCodecCtx);
err0:
	avformat_close_input(&pFormatCtx);
	return 0;
}























