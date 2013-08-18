#include <cstdio>
#include <vector>
#include <queue>
#include <algorithm>
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


bool g_quit = false;

class PacketQueue
{
public:
    PacketQueue(void)
        : mutex_(SDL_CreateMutex()), cond_(SDL_CreateCond()) {
    }
    ~PacketQueue(void) {
        flush();
    }

    bool empty(void) {
        SDL_LockMutex(mutex_);
        const bool e = packets_.empty();
        SDL_UnlockMutex(mutex_);
        return e;
    }

    AVPacket* peek(void) {
        AVPacket* packet = 0;

        SDL_LockMutex(mutex_);
        if (!packets_.empty()) {
            packet = packets_.front();
        }
        SDL_UnlockMutex(mutex_);

        return packet;
    }

    void pop(void) {
        SDL_LockMutex(mutex_);
        AVPacket* packet = packets_.front();
        packets_.pop();
        SDL_UnlockMutex(mutex_);

        av_free_packet(packet);
        delete packet;
    }

    void push(AVPacket* packet) {
        av_dup_packet(packet);

        SDL_LockMutex(mutex_);
        packets_.push(packet);
        SDL_UnlockMutex(mutex_);
    }

    void flush(void) {
        while (!empty()) {
            pop();
        }
    }

private:
    std::queue<AVPacket*> packets_;
    SDL_mutex* mutex_;
    SDL_cond* cond_;
};

PacketQueue audio_q;


void audio_callback(void* userdata, Uint8* stream, int len) 
{
    SDL_LockAudio();

    static std::vector<uint8_t> buffer;
    if (len > 0) {
        while (buffer.size() < static_cast<size_t>(len)) {
            if(g_quit) {
                return;
            }

            if (audio_q.empty()) {
                break;
            }

            AVPacket* packet = audio_q.peek();
            if (packet) {
                int got_frame;
                AVFrame frame;
                avcodec_get_frame_defaults(&frame);
                AVCodecContext* codec_ctx = static_cast<AVCodecContext*>(userdata);
                const int bytes_decoded = avcodec_decode_audio4(codec_ctx, &frame, &got_frame, packet);
                audio_q.pop();

                if(bytes_decoded < 0 || got_frame == 0) {
                    continue;
                }
                buffer.insert(buffer.end(), frame.data[0], frame.data[0] + frame.linesize[0]);
            }
        }
        
        const int size = std::min((int)buffer.size(), len);
        if (size > 0) {
            std::vector<uint8_t>::const_iterator begin = buffer.begin();
            std::vector<uint8_t>::const_iterator end = begin + size;
            std::copy(begin, end, stream);
            buffer.erase(begin, end);
        }
    }

    SDL_UnlockAudio();
}

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

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
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

    // find audio and video stream indexes
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

    if (audio_stream == -1 || video_stream == -1) {
        printf("[Tutorial] Couldn't find audio or video stream, or both of them.\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    // ready SDL audio
    static const int kSDLAudioBufferSize = 1024;
    AVCodecContext* audio_decode_ctx = format_ctx->streams[audio_stream]->codec;
    SDL_AudioSpec spec_desired;
    spec_desired.freq = audio_decode_ctx->sample_rate;
    spec_desired.format = AUDIO_S16SYS;
    spec_desired.channels = audio_decode_ctx->channels;
    spec_desired.silence = 0;
    spec_desired.samples = kSDLAudioBufferSize;
    spec_desired.callback = audio_callback;
    spec_desired.userdata = audio_decode_ctx;

    SDL_AudioSpec spec_obtained;
    if (SDL_OpenAudio(&spec_desired, &spec_obtained) != 0) {
        printf("[Tutorial] SLD_OpenAudio failed: %s\n", SDL_GetError());
        avformat_close_input(&format_ctx);
        return -1;
    }

    // open audio decode context
    AVCodec* audio_decoder = avcodec_find_decoder(audio_decode_ctx->codec_id);
    if (audio_decoder == 0) {
        printf("[Tutorial] Unsupported audio codec\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    if (avcodec_open2(audio_decode_ctx, audio_decoder, 0) < 0) {
        printf("[Tutorial] Couldn't open audio codec.\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    SDL_PauseAudio(0);

    // open video decode context
    AVCodecContext* video_decode_ctx = format_ctx->streams[video_stream]->codec;
    AVCodec* decoder = avcodec_find_decoder(video_decode_ctx->codec_id);
    if (decoder == 0) {
        printf("[Tutorial] Unsupported video codec!\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    if (avcodec_open2(video_decode_ctx, decoder, 0) < 0) {
        printf("[Tutorial] Couldn't open video codec.\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    // frame ready
    AVFrame* frame = avcodec_alloc_frame();
    if (frame == 0) {
        printf("[Tutorial] Couldn't allocate video frame.\n");
        avcodec_close(video_decode_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }

    // SDL video ready
    SDL_Surface* screen = SDL_SetVideoMode(video_decode_ctx->width, video_decode_ctx->height, 0, 0);
    if (screen == 0) {
        printf("[Tutorial] SDL_SetVideoMode failed: %s\n", SDL_GetError());
        avcodec_free_frame(&frame);
        avcodec_close(video_decode_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }

    SDL_Overlay* bmp = SDL_CreateYUVOverlay(video_decode_ctx->width, video_decode_ctx->height, SDL_YV12_OVERLAY, screen);
    if (bmp == 0) {
        printf("[Tutorial] SDL_CreateYUVOverlay failed: %s\n", SDL_GetError());
        avcodec_free_frame(&frame);
        avcodec_close(video_decode_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }

    AVPacket packet;
    while (av_read_frame(format_ctx, &packet) == 0) {

        if (packet.stream_index == audio_stream) {
            AVPacket* p = new AVPacket();
            av_copy_packet(p, &packet);
            audio_q.push(p);
        } else if (packet.stream_index == video_stream) {
            int got_picture = 0;
            if (avcodec_decode_video2(video_decode_ctx, frame, &got_picture, &packet) >= 0 && got_picture != 0) {
                Display(video_decode_ctx, frame, bmp);
            }
            av_free_packet(&packet);
        } else {
            av_free_packet(&packet);
        }

        // check user event
        SDL_Event event;
        SDL_PollEvent(&event);
        switch (event.type) {
        case SDL_QUIT:
            g_quit = true;
            SDL_Quit();
            exit(0);
            break;
        default:
            //nothing
            break;
        }
    }

    SDL_FreeYUVOverlay(bmp);
    avcodec_free_frame(&frame);
    avcodec_close(video_decode_ctx);
    avcodec_close(audio_decode_ctx);
    avformat_close_input(&format_ctx);

    return 0;
}