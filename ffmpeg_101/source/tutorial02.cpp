#include <cstdio>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include <SDL.h>
#include <SDL_thread.h>
#ifdef main
#undef main
#endif



void Display(const AVCodecContext* decode_ctx, const AVFrame* frame, SDL_Overlay* bmp)
{
	SDL_LockYUVOverlay(bmp);

	AVPicture pic;
	pic.data[0] = bmp->pixels[0];
	pic.data[1] = bmp->pixels[2];
	pic.data[2] = bmp->pixels[1];

	pic.linesize[0] = bmp->pitches[0];
	pic.linesize[1] = bmp->pitches[2];
	pic.linesize[2] = bmp->pitches[1];

	const int w = decode_ctx->width;
	const int h = decode_ctx->height;
    const AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
	SwsContext* swsCtx = sws_getContext(w, h, decode_ctx->pix_fmt, w, h, pix_fmt, SWS_FAST_BILINEAR, 0, 0, 0);
	if (swsCtx == 0) {
		return;
	}

	sws_scale(swsCtx, frame->data, frame->linesize, 0, h, pic.data, pic.linesize);
	sws_freeContext(swsCtx);

	SDL_UnlockYUVOverlay(bmp);

	SDL_Rect rc = {0, 0, w, h};
	SDL_DisplayYUVOverlay(bmp, &rc);
}



int main(int argc, char** argv)
{
	if (argc < 2) {
		printf("[Tutorial] Not enough arguements.\n");
		return -1;
	}

	av_register_all();

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("[Tutorial] Couldn't initialize SDL.\n");
		return -1;
	}
	atexit(SDL_Quit);

    // open file
	AVFormatContext* format_ctx = 0;
	const char* filepath = argv[1];
	if (avformat_open_input(&format_ctx, filepath, 0, 0) != 0) {
		printf("[Tutorial] Couldn't open file. : %s\n", filepath);
		return -1;
	}

	if (avformat_find_stream_info(format_ctx, 0) < 0) {
		printf("[Tutorial] Couldn't find stream infomation.\n");
		avformat_close_input(&format_ctx);
		return -1;
	}

	av_dump_format(format_ctx, 0, filepath, 0);

    // find video stream index
	int video_stream = -1;
	for (unsigned int i = 0 ; i < format_ctx->nb_streams ; ++i) {
		if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
			break;
		}
	}

	if (video_stream == -1) {
		printf("[Tutorial] Couldn't find video stream\n");
		avformat_close_input(&format_ctx);
		return -1;
	}

    // open decode context
	AVCodecContext* decode_ctx = format_ctx->streams[video_stream]->codec;
	AVCodec* decoder = avcodec_find_decoder(decode_ctx->codec_id);
	if (decoder == 0) {
		printf("[Tutorial] Unsupported codec!\n");
		avformat_close_input(&format_ctx);
		return -1;
	}

	if (avcodec_open2(decode_ctx, decoder, 0) < 0) {
		printf("[Tutorial] Couldn't open codec.\n");
		avformat_close_input(&format_ctx);
		return -1;
	}

    // frame ready
	AVFrame* frame = avcodec_alloc_frame();
	if (frame == 0) {
		printf("[Tutorial] Couldn't allocate frame.\n");
		avcodec_close(decode_ctx);
		avformat_close_input(&format_ctx);
		return -1;
	}

    // SDL video ready
	SDL_Surface* screen = SDL_SetVideoMode(decode_ctx->width, decode_ctx->height, 0, 0);
	if (screen == 0) {
		printf("[Tutorial] SDL_SetVideoMode failed: %s\n", SDL_GetError());
		avcodec_free_frame(&frame);
		avcodec_close(decode_ctx);
		avformat_close_input(&format_ctx);
		return -1;
	}

	SDL_Overlay* bmp = SDL_CreateYUVOverlay(decode_ctx->width, decode_ctx->height, SDL_YV12_OVERLAY, screen);
	if (bmp == 0) {
		printf("[Tutorial] SDL_CreateYUVOverlay failed: %s\n", SDL_GetError());
		avcodec_free_frame(&frame);
		avcodec_close(decode_ctx);
		avformat_close_input(&format_ctx);
		return -1;
	}

	AVPacket packet;
    // read frame and dispaly
	while (av_read_frame(format_ctx, &packet) == 0) {
		if (packet.stream_index == video_stream) {
			int got_picture = 0;
			if (avcodec_decode_video2(decode_ctx, frame, &got_picture, &packet) >= 0 && got_picture != 0) {
				Display(decode_ctx, frame, bmp);
			}
		}
		av_free_packet(&packet);
	}

	SDL_FreeYUVOverlay(bmp);
	avcodec_free_frame(&frame);
	avcodec_close(decode_ctx);
	avformat_close_input(&format_ctx);

	return 0;
}