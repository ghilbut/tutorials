#include <stdio.h>
#include <algorithm>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avstring.h>
}
#include <SDL.h>
#include <SDL_thread.h>
#ifdef main
#undef main
#endif



enum {
	kEventQuit = SDL_USEREVENT
};





bool g_quit = false;

class PacketQueue
{
public:
	PacketQueue(void)
		: first_(0), last_(0), mutex_(SDL_CreateMutex()), cond_(SDL_CreateCond())
	{
	}
	~PacketQueue(void)
	{
		Flush();
	}

	bool Put(AVPacket* packet)
	{
		if (av_dup_packet(packet) < 0) {
			return false;
		}
		AVPacketList* packet_list = static_cast<AVPacketList*>(av_malloc(sizeof(AVPacketList)));
		if (packet_list == 0) {
			return false;
		}

		packet_list->pkt = *packet;
		packet_list->next = 0;

		SDL_LockMutex(mutex_);

		if (last_ == 0) {
			first_ = packet_list;
		} else {
			last_->next = packet_list;
		}
		last_ = packet_list;

		SDL_CondSignal(cond_);
		SDL_UnlockMutex(mutex_);
		return true;
	}

	bool Get(AVPacket* packet)
	{
		SDL_LockMutex(mutex_);

		bool retVal = false;
		while (!g_quit) {
			AVPacketList* packet_list = first_;
			if (packet_list) {
				first_ = packet_list->next;
				if (first_ == 0) {
					last_ = 0;
				}
				*packet = packet_list->pkt;
				av_free(packet_list);
				retVal = true;
				break;
			} else {
				SDL_CondWait(cond_, mutex_);
			}
		}

		SDL_UnlockMutex(mutex_);
		return retVal;
	}

	void Flush(void)
	{
		AVPacketList* next = first_;
		while (next) {
			av_free_packet(&(next->pkt));
			AVPacketList* tmp = next->next;
			av_free(next);
			next = tmp;
		}
	}

	bool IsEmpty(void) const
	{
		return (first_ == 0 && last_ == 0);
	}

private:
	AVPacketList* first_;
	AVPacketList* last_;
	SDL_mutex* mutex_;
	SDL_cond* cond_;
};



class AudioDecoder
{
public:
	explicit AudioDecoder(PacketQueue& q)
		: stream_(0), q_(q), buffer_size_(0), buffer_index_(0)
	{
		memset(buffer_, 0, sizeof(buffer_));
	}

	~AudioDecoder(void)
	{
	}

	bool Open(const AVFormatContext* format_ctx, int stream_index)
	{
		SDL_PauseAudio(1);

		static const int kSDLAudioBufferSize = 1024;
		stream_ = format_ctx->streams[stream_index];
		AVCodecContext* codec_ctx = stream_->codec;
		SDL_AudioSpec spec_desired;
		spec_desired.freq = codec_ctx->sample_rate;
		spec_desired.format = AUDIO_S16SYS;
		spec_desired.channels = codec_ctx->channels;
		spec_desired.silence = 0;
		spec_desired.samples = kSDLAudioBufferSize;
		spec_desired.callback = audio_callback;
		spec_desired.userdata = this;

		SDL_AudioSpec spec_obtained;
		if (SDL_OpenAudio(&spec_desired, &spec_obtained) != 0) {
			printf("SLD_OpenAudio failed: %s\n", SDL_GetError());
			stream_ = 0;
			return false;
		}

		AVCodec* audio_decoder = avcodec_find_decoder(codec_ctx->codec_id);
		if (audio_decoder == 0) {
			printf("Unsupported audio codec\n");
			stream_ = 0;
			return false;
		}

		if (avcodec_open2(codec_ctx, audio_decoder, 0) < 0) {
			printf("Couldn't open audio codec.\n");
			stream_ = 0;
			return false;
		}

		q_.Flush();

		buffer_size_ = 0;
		buffer_index_ = 0;

		SDL_PauseAudio(0);
		return true;
	}

	void Close(void)
	{
		SDL_PauseAudio(1);

		if (stream_) {
			avcodec_close(stream_->codec);
			stream_ = 0;
		}
	}

private:
	size_t DecodeAudio(AVCodecContext* codec_ctx, uint8_t* out_buffer)
	{
		AVPacket packet;
		av_init_packet(&packet);
		packet.data = 0;
		packet.size = 0;

		while (!g_quit) {
			while(packet.data != 0 && packet.size > 0) {

				int got_frame;
				AVFrame frame;
				avcodec_get_frame_defaults(&frame);
				const int bytes_decoded = avcodec_decode_audio4(codec_ctx, &frame, &got_frame, &packet);
				av_free_packet(&packet);
				if(bytes_decoded < 0 || got_frame == 0) {
					break;
				}

				memcpy(out_buffer, frame.data[0], frame.linesize[0]);
				return frame.linesize[0];
			}

			if(!q_.Get(&packet)) {
				break;
			}
		}

		return 0;
	}

	static void SDLCALL audio_callback(void* userdata, Uint8* stream, int len) 
	{
		AudioDecoder* pThis = static_cast<AudioDecoder*>(userdata);
		pThis->DoCallback(stream, len);
	}
	void DoCallback(Uint8* stream, int len)
	{
		SDL_LockAudio();
   
		while (len > 0) {
			if (buffer_index_ >= buffer_size_) {
				AVCodecContext* codec_ctx = stream_->codec;
				buffer_size_ = DecodeAudio(codec_ctx, buffer_);
				buffer_index_ = 0;
			}

			if (buffer_size_ > 0) {
				const size_t bytes_stream = std::min(buffer_size_ - buffer_index_, static_cast<size_t>(len));
				memcpy(stream, buffer_ + buffer_index_, bytes_stream);

				len -= bytes_stream;
				stream += bytes_stream;
				buffer_index_ += bytes_stream;
			}
		}

		SDL_UnlockAudio();
	}


private:
	AVStream*    stream_;
	PacketQueue& q_;
	uint8_t      buffer_[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	size_t       buffer_size_;
	size_t       buffer_index_;
};



uint64_t g_video_pts = 0;

int do_get_buffer(struct AVCodecContext* ctx, AVFrame* frame)
{
	int retVal = avcodec_default_get_buffer(ctx, frame);
	uint64_t* pts = static_cast<uint64_t*>(av_malloc(sizeof(uint64_t)));
	*pts = g_video_pts;
	frame->opaque = pts;
	return retVal;
}
void do_release_buffer(struct AVCodecContext* ctx, AVFrame* frame)
{
	av_freep(&(frame->opaque));
	avcodec_default_release_buffer(ctx, frame);
}

double g_video_clock;

class VideoDecoder
{
public:
	explicit VideoDecoder(PacketQueue& q)
		: stream_(0), q_(q), thread_(0)
	{
	}
	~VideoDecoder(void)
	{
	}

	bool Open(const AVFormatContext* format_ctx, int stream_index)
	{
		stream_ = format_ctx->streams[stream_index];
		AVCodecContext* video_decode_ctx = stream_->codec;
		AVCodec* decoder = avcodec_find_decoder(video_decode_ctx->codec_id);
		if (decoder == 0) {
			printf("Unsupported video codec!\n");
			return false;
		}

		if (avcodec_open2(video_decode_ctx, decoder, 0) < 0) {
			printf("Couldn't open video codec.\n");
			return false;
		}

		frame_ = avcodec_alloc_frame();
		if (frame_ == 0) {
			printf("Couldn't allocate video frame.\n");
			avcodec_close(video_decode_ctx);
			return false;
		}

		SDL_Surface* screen = SDL_SetVideoMode(video_decode_ctx->width, video_decode_ctx->height, 0, 0);
		if (screen == 0) {
			printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
			avcodec_free_frame(&frame_);
			avcodec_close(video_decode_ctx);
			return false;
		}

		bmp_ = SDL_CreateYUVOverlay(video_decode_ctx->width, video_decode_ctx->height, SDL_YV12_OVERLAY, screen);
		if (bmp_ == 0) {
			printf("SDL_CreateYUVOverlay failed: %s\n", SDL_GetError());
			avcodec_free_frame(&frame_);
			avcodec_close(video_decode_ctx);
			return false;
		}

		video_decode_ctx->get_buffer = do_get_buffer;
		video_decode_ctx->release_buffer = do_release_buffer;

		thread_ = SDL_CreateThread(video_thread, this);
		return (thread_ != 0);
	}

	void Close(void)
	{
		if (stream_) {
			avcodec_close(stream_->codec);
			stream_ = 0;
		}
	}

private:
	static int SDLCALL video_thread(void* param)
	{
		VideoDecoder* pThis = static_cast<VideoDecoder*>(param);
		return pThis->DoCallback();
	}
	int DoCallback(void)
	{
		double pts = 0;

		AVPacket packet;
		while (!g_quit) {
			if (q_.Get(&packet)) {

				pts = 0;
				g_video_pts = packet.pts;

				int got_picture = 0;
				avcodec_get_frame_defaults(frame_);
				if (avcodec_decode_video2(stream_->codec, frame_, &got_picture, &packet) >= 0 && got_picture != 0) {

					if (packet.dts != AV_NOPTS_VALUE) {
						pts = (packet.dts * av_q2d(stream_->time_base));
					} else {
						pts = 0;
					}

					if (got_picture != 0) {
						pts = SynchronizeVideo(pts);
						// TODO:
					}

					Display(stream_->codec, frame_, bmp_);
				}
				av_free_packet(&packet);
			}
		}

		SDL_FreeYUVOverlay(bmp_);
		avcodec_free_frame(&frame_);
		avcodec_close(stream_->codec);

		return 0;
	}

	void Display(const AVCodecContext* codec_ctx, const AVFrame* frame, SDL_Overlay* bmp)
	{
		SDL_LockYUVOverlay(bmp);

		AVPicture pic;
		pic.data[0] = bmp->pixels[0];
		pic.data[1] = bmp->pixels[2];
		pic.data[2] = bmp->pixels[1];

		pic.linesize[0] = bmp->pitches[0];
		pic.linesize[1] = bmp->pitches[2];
		pic.linesize[2] = bmp->pitches[1];

		const int w = codec_ctx->width;
		const int h = codec_ctx->height;
		static const AVPixelFormat kPixFmt = AV_PIX_FMT_YUV420P;
		SwsContext* swsCtx = sws_getContext(w, h, codec_ctx->pix_fmt, w, h, kPixFmt, SWS_FAST_BILINEAR, 0, 0, 0);
		if (swsCtx == 0) {
			return;
		}

		sws_scale(swsCtx, frame->data, frame->linesize, 0, h, pic.data, pic.linesize);
		sws_freeContext(swsCtx);

		SDL_UnlockYUVOverlay(bmp);

		SDL_Rect rc = {0, 0, w, h};
		SDL_DisplayYUVOverlay(bmp, &rc);
	}



	double SynchronizeVideo(double pts)
	{
		double frame_delay;
		if (pts != 0) {
			g_video_clock = pts;
		} else {
			pts = g_video_clock;
		}
		frame_delay = av_q2d(stream_->codec->time_base);
		frame_delay	+= (frame_->repeat_pict * frame_delay * 0.5);
		g_video_clock += frame_delay;
		return pts;
	}



private:
	AVStream* stream_;
	PacketQueue& q_;

	SDL_Thread* thread_;
	AVFrame* frame_;
	SDL_Overlay* bmp_;
};



PacketQueue audio_q;
AudioDecoder audio_decoder(audio_q);

PacketQueue video_q;
VideoDecoder video_decoder(video_q);

char filepath[260];

int SDLCALL demux_thread(void* param)
{
	AVFormatContext* format_ctx = 0;
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

	int audio_stream = -1;
	int video_stream = -1;
	const int count = format_ctx->nb_streams;
	for (int i = 0 ; i < count ; ++i) {

		const AVStream* stream = format_ctx->streams[i];
		const AVMediaType type = stream->codec->codec_type;
		if (type == AVMEDIA_TYPE_AUDIO && audio_stream == -1) {
			audio_stream = i;
		}
		if (type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
		}
	}

	if (audio_stream != -1) {
		if (!audio_decoder.Open(format_ctx, audio_stream)) {
			audio_stream = -1;
		}
	}

	if (video_stream != -1) {
		if (!video_decoder.Open(format_ctx, video_stream)) {
			video_stream = -1;
		}
	}

	if (audio_stream == -1 || video_stream == -1) {
		printf("Couldn't find audio or video stream, or both of them.\n");
		avformat_close_input(&format_ctx);
		return -1;
	}

	AVPacket packet;
	while (!g_quit && av_read_frame(format_ctx, &packet) == 0) {

		if (packet.stream_index == audio_stream) {
			audio_q.Put(&packet);
		} else if (packet.stream_index == video_stream) {
			video_q.Put(&packet);
		} else {
			av_free_packet(&packet);
		}
	}

	while (!g_quit && (!audio_q.IsEmpty() || !video_q.IsEmpty())) {
		SDL_Delay(10);
	}
	g_quit = true;

	if (video_stream != -1) {
		video_decoder.Close();
	}
	if (audio_stream != -1) {
		audio_decoder.Close();
	}
	avformat_close_input(&format_ctx);

	return 0;
}







int main(int argc, char** argv)
{
	if (argc < 2) {
		printf("not enough arguements.\n");
		return -1;
	}

	av_register_all();

	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
		return -1;
	}
	atexit(SDL_Quit);

	av_strlcpy(filepath, argv[1], strlen(argv[1])+1);

	SDL_Thread* demux_tid = SDL_CreateThread(demux_thread, 0);
	if(demux_tid == 0) {
		return -1;
	}

	while (!g_quit) {
		static SDL_Event event;
		SDL_WaitEvent(&event);
		switch (event.type) {
		case kEventQuit:
			// continue
		case SDL_QUIT:
			g_quit = true;
			SDL_Quit();
			break;
		default:
			//nothing
			break;
		}
	}

	SDL_WaitThread(demux_tid, 0);

	return 0;
}
