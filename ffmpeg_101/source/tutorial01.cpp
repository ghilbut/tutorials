#include <stdio.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


AVFrame* GetImageFrame(const AVCodecContext* decode_ctx, AVFrame* frame);
bool Encode(const AVCodecContext* decode_ctx, AVFrame* frame, AVPacket& out);


int main(int argc, char** argv)
{
	if (argc < 3) {
		printf("not enough arguements.\n");
		return -1;
	}

	av_register_all();

    // open file
	AVFormatContext* format_ctx = 0;
	const char* filepath = argv[1];
	if (avformat_open_input(&format_ctx, filepath, 0, 0) != 0) {
		printf("Couldn't open file. : %s\n", filepath);
		return -1;
	}

	if (avformat_find_stream_info(format_ctx, 0) < 0) {
		printf("Couldn't find stream infomation.\n");
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
		printf("Couldn't find video stream\n");
		avformat_close_input(&format_ctx);
		return -1;
	}

    // open decode context
	AVCodecContext* decode_ctx = format_ctx->streams[video_stream]->codec;
	AVCodec* decoder = avcodec_find_decoder(decode_ctx->codec_id);
	if (decoder == 0) {
		printf("Unsupported codec!\n");
		avformat_close_input(&format_ctx);
		return -1;
	}

	if (avcodec_open2(decode_ctx, decoder, 0) < 0) {
		printf("Couldn't open codec.\n");
		avformat_close_input(&format_ctx);
		return -1;
	}

    // frame ready
	AVFrame* frame = avcodec_alloc_frame();
	if (frame == 0) {
		printf("Couldn't allocate frame.\n");
        avcodec_close(decode_ctx);
		avformat_close_input(&format_ctx);
		return -1;
	}

	AVPacket packet;
    // write imgae file from read frame
	for (int count = 0; count < 5 && av_read_frame(format_ctx, &packet) == 0; ) {
		if (packet.stream_index == video_stream) {
			int got_picture = 0;
			if (avcodec_decode_video2(decode_ctx, frame, &got_picture, &packet) >= 0 && got_picture != 0) {
				if (count++ < 5) {

					char outpath[260];
					static const char* outdir = argv[2];
					sprintf(outpath, "%s\\out%d.png", outdir, count);
					printf("convert and save file: %s\n", outpath);

					AVPacket out;
					av_init_packet(&out);
                    AVFrame* img_frame = GetImageFrame(decode_ctx, frame);
	                if (img_frame != 0) {
		                if (Encode(decode_ctx, img_frame, out)) {
						    FILE* f = fopen(outpath, "wb");
						    fwrite(out.data, 1, out.size, f);
						    fclose(f);
						    av_free_packet(&out);
					    }
	                }
				}
			}
		}
		av_free_packet(&packet);
	}

	avcodec_free_frame(&frame);
	avcodec_close(decode_ctx);
	avformat_close_input(&format_ctx);

	return 0;
}


static const AVPixelFormat kPixFmt = AV_PIX_FMT_RGBA;

AVFrame* GetImageFrame(const AVCodecContext* decode_ctx, AVFrame* frame)
{
	AVFrame* img_frame = avcodec_alloc_frame();
	if (img_frame == 0) {
		return 0;
	}
					
	const int w = decode_ctx->width;
	const int h = decode_ctx->height;
	SwsContext* swsCtx = sws_getContext(w, h, decode_ctx->pix_fmt, w, h, kPixFmt, SWS_BILINEAR, 0, 0, 0);
	if (swsCtx == 0) {
		avcodec_free_frame(&img_frame);
		return 0;
	}

	const int buffer_size = avpicture_get_size(kPixFmt, w, h);
	uint8_t* buffer = (uint8_t*)av_malloc(buffer_size);
	if (buffer == 0) {
		avcodec_free_frame(&img_frame);
		sws_freeContext(swsCtx);
		return 0;
	}

	avpicture_fill(reinterpret_cast<AVPicture*>(img_frame), buffer, kPixFmt, w, h);
	if (h != sws_scale(swsCtx, frame->data, frame->linesize, 0, h, img_frame->data, img_frame->linesize)) {
		avcodec_free_frame(&img_frame);
		av_free(buffer);
	}

	sws_freeContext(swsCtx);
	return img_frame;
}

bool Encode(const AVCodecContext* decode_ctx, AVFrame* frame, AVPacket& out)
{
    // open encode context
	const AVCodecID codec_id = CODEC_ID_PNG;
	AVCodec* encoder = avcodec_find_encoder(codec_id);
	if (encoder == 0) {
		return false;
	}

	AVCodecContext* encode_ctx = avcodec_alloc_context3(encoder);
	if (encode_ctx == 0) {
		return false;
	}

	encode_ctx->bit_rate = decode_ctx->bit_rate;
	encode_ctx->width = decode_ctx->width;
	encode_ctx->height = decode_ctx->height;
	encode_ctx->pix_fmt = kPixFmt;
	encode_ctx->codec_id = codec_id;
	encode_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	encode_ctx->time_base.num = decode_ctx->time_base.num;
	encode_ctx->time_base.den = decode_ctx->time_base.den;
 
	if (avcodec_open2(encode_ctx, encoder, 0) < 0) {
		av_free(encode_ctx);
		return false;
	}

    // encode to png from video frame
	encode_ctx->mb_lmin = encode_ctx->lmin = encode_ctx->qmin * FF_QP2LAMBDA;
	encode_ctx->mb_lmax = encode_ctx->lmax = encode_ctx->qmax * FF_QP2LAMBDA;
	encode_ctx->flags = CODEC_FLAG_QSCALE;
	encode_ctx->global_quality = encode_ctx->qmin * FF_QP2LAMBDA;
	frame->pts = 1;
	frame->quality = encode_ctx->global_quality;

	out.data = 0;
	out.size = 0;
	int got_packet = 0;
	bool retVal = (avcodec_encode_video2(encode_ctx, &out, frame, &got_packet) == 0 && got_packet == 1);
	if (!retVal) {
		av_free(frame->data[0]);
	}
	avcodec_free_frame(&frame);
	avcodec_close(encode_ctx);
	av_free(encode_ctx);

	return retVal;
}
