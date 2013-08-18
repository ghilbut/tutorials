#include <cstdio>
#include <vector>
#include <queue>
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
    kEventDemuxComplete = SDL_USEREVENT,
    kEventAudioEmpty,
    kEventVideoEmpty,
    kEventQuit
};



bool g_quit = false;



class PacketQueue {
public:
    PacketQueue(void)
        : mutex_(SDL_CreateMutex()), cond_(SDL_CreateCond()) {
    }

    ~PacketQueue(void) {
        flush();
    }

    bool empty(void) {
        return packets_.empty();
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
        SDL_LockMutex(mutex_);
        while (!packets_.empty()) {
            pop();
        }
        SDL_UnlockMutex(mutex_);
    }

private:
    std::queue<AVPacket*> packets_;
    SDL_mutex* mutex_;
    SDL_cond* cond_;
};



class Surface {
public:
    Surface(void)
        : bmp_(0), width_(0), height_(0), allocated_(false) {
    }
    ~Surface(void) {
        Release();
    }

    void Alloc(const AVCodecContext* ctx) {
        SDL_Surface* display = SDL_SetVideoMode(ctx->width, ctx->height, 0, 0);
        bmp_ = SDL_CreateYUVOverlay(ctx->width, ctx->height, SDL_YV12_OVERLAY, display);
        width_ = ctx->width;
        height_ = ctx->height;
        allocated_ = true;
    }

    void Release(void) {
        if (bmp_) {
            SDL_FreeYUVOverlay(bmp_);
            allocated_ = false;
            bmp_ = 0;
        }
    }

    SDL_Overlay* bmp(void) const { return bmp_; }
    int  width(void) const       { return width_; }
    int  height(void) const      { return height_; }
    bool IsAllocated(void) const { return allocated_; }

private:
    SDL_Overlay* bmp_;
    int width_;
    int height_;
    bool allocated_;
};



AVFormatContext* format_ctx = 0;



class AudioDecoder {
public:
    AudioDecoder(void)
        : audio_stream_(0) {
    }

    ~AudioDecoder(void) {
        Close();
    }

    bool Open(int stream_index) {
        SDL_PauseAudio(1);

        static const int kSDLAudioBufferSize = 1024;
        audio_stream_ = format_ctx->streams[stream_index];
        AVCodecContext* audio_decode_ctx = audio_stream_->codec;
        SDL_AudioSpec spec_desired;
        spec_desired.freq = audio_decode_ctx->sample_rate;
        spec_desired.format = AUDIO_S16SYS;
        spec_desired.channels = audio_decode_ctx->channels;
        spec_desired.silence = 0;
        spec_desired.samples = kSDLAudioBufferSize;
        spec_desired.callback = audio_callback;
        spec_desired.userdata = this;

        SDL_AudioSpec spec_obtained;
        if (SDL_OpenAudio(&spec_desired, &spec_obtained) != 0) {
            printf("[Tutorial] SLD_OpenAudio failed: %s\n", SDL_GetError());
            audio_stream_ = 0;
            return false;
        }

        AVCodec* audio_decoder = avcodec_find_decoder(audio_decode_ctx->codec_id);
        if (audio_decoder == 0) {
            printf("[Tutorial] Unsupported audio codec\n");
            audio_stream_ = 0;
            return false;
        }

        if (avcodec_open2(audio_decode_ctx, audio_decoder, 0) < 0) {
            printf("[Tutorial] Couldn't open audio codec.\n");
            audio_stream_ = 0;
            return false;
        }

        audio_q_.flush();

        SDL_PauseAudio(0);
        return true;
    }

    void Close(void) {
        SDL_PauseAudio(1);

        if (audio_stream_) {
            avcodec_close(audio_stream_->codec);
            audio_stream_ = 0;
        }
    }

    void Push(AVPacket* packet) {
        audio_q_.push(packet);
    }

private:
    void DoCallback(Uint8* stream, int len) {
        SDL_LockAudio();

        if (len > 0) {
            while (audio_buffer_.size() < static_cast<size_t>(len)) {
                if(g_quit) {
                    return;
                }

                if (audio_q_.empty()) {
                    SDL_Event event;
                    event.type = kEventAudioEmpty;
                    event.user.data1 = 0;
                    SDL_PushEvent(&event);

                    continue;
                }

                AVPacket packet, *p = audio_q_.peek();
                if (p) {
                    memcpy(&packet, p, sizeof(packet));
                } else {
                    av_init_packet(&packet);
                }

                AVCodecContext* codec_ctx = audio_stream_->codec;

                int got_frame;
                AVFrame frame;
                avcodec_get_frame_defaults(&frame);
                const int bytes_decoded = avcodec_decode_audio4(codec_ctx, &frame, &got_frame, &packet);
                if (p) {
                    audio_q_.pop();
                }
                if(bytes_decoded < 0 || got_frame == 0) {
                    continue;
                }

                audio_buffer_.insert(audio_buffer_.end(), frame.data[0], frame.data[0] + frame.linesize[0]);
            }
        
            const int size = std::min((int)audio_buffer_.size(), len);
            std::vector<uint8_t>::const_iterator begin = audio_buffer_.begin();
            std::vector<uint8_t>::const_iterator end = begin + size;
            std::copy(begin, end, stream);
            audio_buffer_.erase(begin, end);
        }

        SDL_UnlockAudio();
    }

    static void SDLCALL audio_callback(void* userdata, Uint8* stream, int len) {
        AudioDecoder* pThis = static_cast<AudioDecoder*>(userdata);
        pThis->DoCallback(stream, len);
    }


private:
    AVStream* audio_stream_;
    PacketQueue audio_q_;
    std::vector<uint8_t> audio_buffer_;
};



static const AVPixelFormat kPixFmt = AV_PIX_FMT_YUV420P;

void Display(const AVCodecContext* decode_ctx, const AVFrame* frame, SDL_Overlay* bmp) {
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
    SwsContext* swsCtx = sws_getContext(w, h, decode_ctx->pix_fmt, w, h, kPixFmt, SWS_FAST_BILINEAR, 0, 0, 0);
    if (swsCtx == 0) {
        return;
    }

    sws_scale(swsCtx, frame->data, frame->linesize, 0, h, pic.data, pic.linesize);
    sws_freeContext(swsCtx);

    SDL_UnlockYUVOverlay(bmp);

    SDL_Rect rc = {0, 0, w, h};
    SDL_DisplayYUVOverlay(bmp, &rc);
}



class VideoDecoder {
public:
    VideoDecoder(void)
        : video_stream_(0), video_tid_(0) {
    }
    ~VideoDecoder(void) {
        Close();
    }

    bool Open(int stream_index) {
        video_stream_ = format_ctx->streams[stream_index];
        AVCodecContext* video_decode_ctx = video_stream_->codec;
        AVCodec* decoder = avcodec_find_decoder(video_decode_ctx->codec_id);
        if (decoder == 0) {
            printf("[Tutorial] Unsupported video codec!\n");
            video_stream_ = 0;
            return false;
        }

        if (avcodec_open2(video_decode_ctx, decoder, 0) < 0) {
            printf("[Tutorial] Couldn't open video codec.\n");
            video_stream_ = 0;
            return false;
        }

        video_tid_ = SDL_CreateThread(decode_thread, this);
        if (video_tid_ == 0) {
            avcodec_close(video_stream_->codec);
            video_stream_ = 0;
            return false;
        }

        return true;
    }

    void Close(void) {
        if (video_stream_) {
            avcodec_close(video_stream_->codec);
            video_stream_ = 0;
        }
    }

    void Push(AVPacket* packet) {
        video_q_.push(packet);
    }

private:
    static int SDLCALL decode_thread(void* param) {
        VideoDecoder* pThis = static_cast<VideoDecoder*>(param);
        return pThis->DoDecode();
    }
    int DoDecode(void) {
        picture_.Alloc(video_stream_->codec);

        AVFrame* frame = avcodec_alloc_frame();
        while (!g_quit) {
            AVPacket packet, *p = video_q_.peek();
            if (p) {
                memcpy(&packet, p, sizeof(packet));

                int got_picture = 0;
                if (avcodec_decode_video2(video_stream_->codec, frame, &got_picture, &packet) >= 0) {
                    if (got_picture != 0) {
                        Display(video_stream_->codec, frame, picture_.bmp());
                    }
                }

                video_q_.pop();
            } else {
                SDL_Event event;
                event.type = kEventVideoEmpty;
                event.user.data1 = 0;
                SDL_PushEvent(&event);

                av_init_packet(&packet);
            }
        }
        av_free(frame);

        picture_.Release();
        return 0;
    }

private:
    AVStream* video_stream_;
    PacketQueue video_q_;
    SDL_Thread* video_tid_;

    Surface picture_;
};



struct AVInfo {
    int audio_index;
    int video_index;
    AudioDecoder audio_decoder;
    VideoDecoder video_decoder;
};



int demux_interrupt_callback(void* param) {
    return (g_quit ? 1 : 0);
}

int thread_demux(void *arg) {
    AVInfo* info = (AVInfo*)arg;
    const int audio_index = info->audio_index;
    const int video_index = info->video_index;
    AudioDecoder& audio_decoder = info->audio_decoder;
    VideoDecoder& video_decoder = info->video_decoder;

    int result = -1;

    if (!audio_decoder.Open(audio_index)) {
        goto DEMUX_COMPLETE;
    }

    if (!video_decoder.Open(video_index)) {
        goto DEMUX_COMPLETE;
    }

    while (!g_quit) {

        AVPacket* packet(new AVPacket());
        if (av_read_frame(format_ctx, packet) < 0) {
            if (format_ctx->pb && format_ctx->pb->error) {
                SDL_Delay(100);
                continue;
            } else {
                break;
            }
        }

        if (packet->stream_index == audio_index) {
            audio_decoder.Push(packet);
        } else if (packet->stream_index == video_index) {
            video_decoder.Push(packet);
        } else {
            delete packet;
        }
    }

    result = 0;

DEMUX_COMPLETE:
    SDL_Event event;
    event.type = kEventDemuxComplete;
    event.user.data1 = 0;
    SDL_PushEvent(&event);

    return result;
}



int main(int argc, char** argv) {
    if (argc < 2) {
        printf("[Tutorial] Not enough arguements.\n");
        return -1;
    }

    av_register_all();

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        return -1;
    }
    atexit(SDL_Quit);

    const char* filepath = argv[1];
    if (avformat_open_input(&format_ctx, filepath, 0, 0) != 0) {
        printf("[Tutorial] Couldn't open file. : %s\n", filepath);
        return -1;
    }

    static const AVIOInterruptCB cb = {demux_interrupt_callback, NULL};
    format_ctx->interrupt_callback = cb;

    if (avformat_find_stream_info(format_ctx, 0) < 0) {
        printf("[Tutorial] Couldn't find stream infomation.\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    av_dump_format(format_ctx, 0, filepath, 0);

    int audio_stream_index = -1;
    int video_stream_index = -1;
    const int count = format_ctx->nb_streams;
    for (int i = 0 ; i < count ; ++i) {

        const AVStream* stream = format_ctx->streams[i];
        const AVMediaType type = stream->codec->codec_type;
        if (type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
        }
        if (type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        }
    }

    if (audio_stream_index == -1 || video_stream_index == -1) {
        printf("[Tutorial] Couldn't find audio or video stream, or both of them.\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    bool demux_complete = false;
    bool audio_complete = false;
    bool video_complete = false;

    AVInfo info;
    info.audio_index = audio_stream_index;
    info.video_index = video_stream_index;
    const SDL_Thread* demux_tid = SDL_CreateThread(thread_demux, &info);
    if(demux_tid == 0) {
        return -1;
    }

    while (!g_quit) {
        if (audio_complete || video_complete) {
            SDL_Event event;
            event.type = kEventQuit;
            event.user.data1 = 0;
            SDL_PushEvent(&event);
        }

        SDL_Event event;
        SDL_WaitEvent(&event);
        switch (event.type) {
        case kEventDemuxComplete:
            demux_complete = true;
            break;
        case kEventAudioEmpty:
            if (demux_complete) {
                audio_complete = true;
            }
            break;
        case kEventVideoEmpty:
            if (demux_complete) {
                video_complete = true;
            }
            break;
        case kEventQuit:
            // go through
        case SDL_QUIT:
            g_quit = true;
            SDL_Quit();
            break;
        default:
            //nothing
            break;
        }
    }

    info.audio_decoder.Close();
    info.video_decoder.Close();
    avformat_close_input(&format_ctx);

    return 0;
}