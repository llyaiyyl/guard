#include <stdio.h>
#include <unistd.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"



static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}

int main(int argc, char * argv[])
{
	AVFormatContext *pFormatCtx = NULL;
	AVCodecContext *pCodecCtx = NULL;
	AVCodec *pCodec = NULL;
	AVPacket packet;
	AVFrame *frame;
	int ret, i, videoStream;
	char buf[1024];


	if((ret = avformat_open_input(&pFormatCtx, argv[1], NULL, NULL))) {
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
	printf("video: %d x %d\n", pCodecCtx->width, pCodecCtx->height);

	frame = av_frame_alloc();
	i = 0;
	while(av_read_frame(pFormatCtx, &packet) >= 0 && i < 10) {
		if(packet.stream_index == videoStream) {
			avcodec_send_packet(pCodecCtx, &packet);
			avcodec_receive_frame(pCodecCtx, frame);

	        printf("saving frame %3d\n", pCodecCtx->frame_number);
	        fflush(stdout);

	        snprintf(buf, sizeof(buf), "%s-%d", "pic", pCodecCtx->frame_number);
	        pgm_save(frame->data[0], frame->linesize[0],
	        		frame->width, frame->height, buf);
	        i++;
		}
	}

	av_frame_free(&frame);
	avcodec_close(pCodecCtx);
err0:
	avformat_close_input(&pFormatCtx);
	return 0;
}























