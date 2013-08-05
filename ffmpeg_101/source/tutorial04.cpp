#include <stdio.h>
#include <vector>
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






static const size_t kMaxAudioDataSize = (5 * 16 * 1024);
static const size_t kMaxVideoDataSize = (5 * 256 * 1024);

enum {
    kEventAlloc = SDL_USEREVENT,
    kEventRefresh,
    kEventQuit
};


bool g_quit = false;


class PacketQueue
{
public:
    PacketQueue(void)
        : first_(0), last_(0), data_size_(0),
        mutex_(SDL_CreateMutex()), cond_(SDL_CreateCond())
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
        data_size_ += packet_list->pkt.size;

        SDL_CondSignal(cond_);
        SDL_UnlockMutex(mutex_);
        return true;
    }

    bool Get(AVPacket* packet)
    {
        bool retVal = false;
        while (!g_quit) {
            AVPacketList* packet_list = first_;
            if (packet_list) {

                SDL_LockMutex(mutex_);
                first_ = packet_list->next;
                if (first_ == 0) {
                    last_ = 0;
                }
                data_size_ -= packet_list->pkt.size;
                SDL_UnlockMutex(mutex_);

                *packet = packet_list->pkt;
                av_free(packet_list);
                retVal = true;
                break;
            } else {
                SDL_CondWait(cond_, mutex_);
            }
        }
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

    size_t data_size(void) const
    {
        return data_size_;
    }

private:
    AVPacketList* first_;
    AVPacketList* last_;
    size_t data_size_;
    SDL_mutex* mutex_;
    SDL_cond* cond_;
};





class Surface
{
public:
    Surface(void)
        : bmp_(0), width_(0), height_(0), allocated_(false)
    {
    }
    ~Surface(void)
    {
        if (bmp_) {
            SDL_FreeYUVOverlay(bmp_);
        }
    }

    void Alloc(const AVCodecContext* ctx)
    {
        SDL_Surface* display = SDL_SetVideoMode(ctx->width, ctx->height, 0, 0);
        bmp_ = SDL_CreateYUVOverlay(ctx->width, ctx->height, SDL_YV12_OVERLAY, display);
        width_ = ctx->width;
        height_ = ctx->height;
        allocated_ = true;
    }

    void Release(void)
    {
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



class AudioDecoder
{
public:
    explicit AudioDecoder(PacketQueue& q)
        : audio_stream_(0), audio_q_(q)
    {
        av_init_packet(&audio_packet_);
        audio_packet_.data = 0;
        audio_packet_.size = 0;
    }

    ~AudioDecoder(void)
    {
    }

    bool Open(int stream_index)
    {
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
            printf("SLD_OpenAudio failed: %s\n", SDL_GetError());
            audio_stream_ = 0;
            return false;
        }

        AVCodec* audio_decoder = avcodec_find_decoder(audio_decode_ctx->codec_id);
        if (audio_decoder == 0) {
            printf("Unsupported audio codec\n");
            audio_stream_ = 0;
            return false;
        }

        if (avcodec_open2(audio_decode_ctx, audio_decoder, 0) < 0) {
            printf("Couldn't open audio codec.\n");
            audio_stream_ = 0;
            return false;
        }

        audio_q_.Flush();

        av_init_packet(&audio_packet_);
        audio_packet_.data = 0;
        audio_packet_.size = 0;

        SDL_PauseAudio(0);
        return true;
    }

    void Close(void)
    {
        if (audio_stream_) {
            avcodec_close(audio_stream_->codec);
            audio_stream_ = 0;
        }
    }

private:
    void DoCallback(Uint8* stream, int len)
    {
        SDL_LockAudio();

        if (len > 0) {
            AVPacket packet;
            av_init_packet(&packet);
            packet.data = 0;
            packet.size = 0;

            while (audio_buffer_.size() < static_cast<size_t>(len) && audio_q_.Get(&packet)) {
                if(g_quit) {
                    return;
                }

                AVCodecContext* codec_ctx = audio_stream_->codec;

                int got_frame;
                AVFrame frame;
                avcodec_get_frame_defaults(&frame);
                const int bytes_decoded = avcodec_decode_audio4(codec_ctx, &frame, &got_frame, &packet);
                av_free_packet(&packet);
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

    static void SDLCALL audio_callback(void* userdata, Uint8* stream, int len) 
    {
        AudioDecoder* pThis = static_cast<AudioDecoder*>(userdata);
        pThis->DoCallback(stream, len);
    }


private:
    AVStream*   audio_stream_;
    PacketQueue& audio_q_;
    std::vector<uint8_t> audio_buffer_;
    AVPacket    audio_packet_;
};





class VideoDecoder
{
public:
    explicit VideoDecoder(PacketQueue& q)
        : video_stream_(0), video_q_(q),
        mutex_(SDL_CreateMutex()), cond_(SDL_CreateCond()), video_tid(0)
    {
    }
    ~VideoDecoder(void)
    {
    }

    bool Open(int stream_index)
    {
        video_stream_ = format_ctx->streams[stream_index];
        AVCodecContext* video_decode_ctx = video_stream_->codec;
        AVCodec* decoder = avcodec_find_decoder(video_decode_ctx->codec_id);
        if (decoder == 0) {
            printf("Unsupported video codec!\n");
            video_stream_ = 0;
            return false;
        }

        if (avcodec_open2(video_decode_ctx, decoder, 0) < 0) {
            printf("Couldn't open video codec.\n");
            video_stream_ = 0;
            return false;
        }

        video_tid = SDL_CreateThread(decode_thread, this);
        if (video_tid == 0) {
            avcodec_close(video_stream_->codec);
            video_stream_ = 0;
            return false;
        }

        return true;
    }

    void Close(void)
    {
        if (video_stream_) {
            avcodec_close(video_stream_->codec);
            video_stream_ = 0;
        }
    }

private:
    static int SDLCALL decode_thread(void* param)
    {
        VideoDecoder* pThis = static_cast<VideoDecoder*>(param);
        return pThis->DoDecode();
    }
    int DoDecode(void)
    {
        AVFrame* frame = avcodec_alloc_frame();
        while (!g_quit) {
            AVPacket packet;
            if (!video_q_.Get(&packet)) {
                break;
            }

            int got_picture = 0;
            if (avcodec_decode_video2(video_stream_->codec, frame, &got_picture, &packet) >= 0 && got_picture != 0) {
                if (got_picture != 0) {
                    //Display(video_decode_ctx, frame, bmp);
                    /*if (!SavePicture(frame)) {
                        break;
                    }*/
                }
            }
            av_free_packet(&packet);
        }
        av_free(frame);
        return 0;
    }

private:
    AVStream*   video_stream_;
    PacketQueue& video_q_;
    Surface        picture_;
    SDL_mutex*  mutex_;
    SDL_cond*   cond_;
    SDL_Thread* video_tid;
};





int audio_stream_index;
PacketQueue audio_q;
AudioDecoder audio_decoder(audio_q);

int         video_stream_index;
PacketQueue    video_q;
VideoDecoder video_decoder(video_q);

//Surface        picture;
//SDL_mutex*  video_mutex;
//SDL_cond*   video_cond;

SDL_Thread* demux_tid;
//SDL_Thread* video_tid;

char filepath[260];





void CloseStream(void)
{
    if (format_ctx) {
        avformat_close_input(&format_ctx);
    }
}









/*
static const AVPixelFormat kPixFmt = AV_PIX_FMT_YUV420P;

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



void MakePicture(void)
{
    SDL_LockMutex(video_mutex);

    if (picture.IsAllocated()) {
        picture.Release();
    }
    picture.Alloc(video_stream->codec);

    SDL_CondSignal(video_cond);
    SDL_UnlockMutex(video_mutex);
}
    

bool SavePicture(AVFrame* frame)
{
    SDL_LockMutex(video_mutex);
    while(picture.IsAllocated() && !g_quit) {
        SDL_CondWait(video_cond, video_mutex);
    }
    SDL_UnlockMutex(video_mutex);

    if (g_quit) {
        return false;
    }

    if (picture.IsAllocated() == false ||
        picture.width() != video_stream->codec->width ||
        picture.height() != video_stream->codec->height) {

        picture.Release();

        SDL_Event event;
        event.type = kEventAlloc;
        event.user.data1 = 0;
        SDL_PushEvent(&event);

        // wait until we have a picture allocated
        SDL_LockMutex(video_mutex);
        while(!picture.IsAllocated() && !g_quit) {
          SDL_CondWait(video_cond, video_mutex);
        }
        SDL_UnlockMutex(video_mutex);

        if (g_quit) {
            return false;
        }
    }
    
    Display(video_stream->codec, frame, picture.bmp());

    return true;
}

int DoThread_DecodeVideo(void* param)
{
    AVFrame* frame = avcodec_alloc_frame();
    while (1) {
        AVPacket packet;
        if (!video_q.Get(&packet)) {
            break;
        }

        int got_picture = 0;
        if (avcodec_decode_video2(video_stream->codec, frame, &got_picture, &packet) >= 0 && got_picture != 0) {
            if (got_picture != 0) {
                //Display(video_decode_ctx, frame, bmp);
                if (!SavePicture(frame)) {
                    break;
                }
            }
        }
        av_free_packet(&packet);
    }
    av_free(frame);
    return 0;
}

bool OpenVideoStream(void)
{
    video_stream = format_ctx->streams[video_stream_index];
    AVCodecContext* video_decode_ctx = video_stream->codec;
    AVCodec* decoder = avcodec_find_decoder(video_decode_ctx->codec_id);
    if (decoder == 0) {
        printf("Unsupported video codec!\n");
        video_stream_index = 0;
        video_stream = 0;
        return false;
    }

    if (avcodec_open2(video_decode_ctx, decoder, 0) < 0) {
        printf("Couldn't open video codec.\n");
        video_stream_index = 0;
        video_stream = 0;
        return false;
    }

    video_tid = SDL_CreateThread(DoThread_DecodeVideo, 0);;
    return true;
}
*/




int demux_interrupt_callback(void* param)
{
    return (g_quit ? 1 : 0);
}

bool OpenAVStream(void)
{
    if (avformat_open_input(&format_ctx, filepath, 0, 0) != 0) {
        printf("Couldn't open file. : %s\n", filepath);
        return false;
    }

    static const AVIOInterruptCB cb = {demux_interrupt_callback, NULL};
    format_ctx->interrupt_callback = cb;

    if (avformat_find_stream_info(format_ctx, 0) < 0) {
        printf("Couldn't find stream infomation.\n");
        avformat_close_input(&format_ctx);
        return false;
    }

    av_dump_format(format_ctx, 0, filepath, 0);

    audio_stream_index = -1;
    video_stream_index = -1;
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

    if (audio_stream_index != -1) {
        if (!audio_decoder.Open(audio_stream_index)) {
            audio_stream_index = -1;
        }
    }
    if (video_stream_index != -1) {
        if (!video_decoder.Open(video_stream_index)) {
            video_stream_index = -1;
        }
    }

    if (audio_stream_index == -1 || video_stream_index == -1) {
        printf("Couldn't find audio or video stream, or both of them.\n");
        avformat_close_input(&format_ctx);

        SDL_Event event;
        event.type = kEventQuit;
        event.user.data1 = 0;
        SDL_PushEvent(&event);
        
        return false;
    }

    return true;
}

int DoThread_Demux(void *arg)
{
    if (!OpenAVStream()) {
        return -1;
    }

    while (!g_quit) {

        //if (audio_q.data_size() >= kMaxAudioDataSize) {
        if (audio_q.data_size() >= kMaxAudioDataSize || 
            video_q.data_size() >= kMaxVideoDataSize) {
            SDL_Delay(10);
            continue;
        }

        AVPacket packet;
        if (av_read_frame(format_ctx, &packet) < 0) {
            if (format_ctx->pb && format_ctx->pb->error) {
                SDL_Delay(100);
                continue;
            } else {
                break;
            }
        }

        if (packet.stream_index == audio_stream_index) {
            audio_q.Put(&packet);
        } else if (packet.stream_index == video_stream_index) {
            video_q.Put(&packet);
            //av_free_packet(&packet);
        } else {
            av_free_packet(&packet);
        }
    }

    SDL_Event event;
    event.type = kEventQuit;
    event.user.data1 = 0;
    SDL_PushEvent(&event);

    while (!g_quit) {
        SDL_Delay(100);
    }

    return 0;
}

void RefreshVideoTimer(void)
{
    // TODO:
}





Uint32 SDLCALL SDL_timer_callback(Uint32 interval, void *param) {
    SDL_Event event;
    event.type = kEventRefresh;
    event.user.data1 = param;
    SDL_PushEvent(&event);
    return 0; // 0 means stop timer
}

void RefreshSchedule(Uint32 interval)
{
    SDL_AddTimer(interval, SDL_timer_callback, 0);
}



int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("not enough arguements.\n");
        return -1;
    }

    av_register_all();

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        return -1;
    }
    atexit(SDL_Quit);



    av_strlcpy(filepath, argv[1], strlen(argv[1]) + 1);

    RefreshSchedule(40);

    demux_tid = SDL_CreateThread(DoThread_Demux, 0);
    if(demux_tid == 0) {
        return -1;
    }



    /*AVFrame* frame = avcodec_alloc_frame();
    if (frame == 0) {
        printf("Couldn't allocate video frame.\n");
        avcodec_close(video_decode_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }

    SDL_Surface* screen = SDL_SetVideoMode(video_decode_ctx->width, video_decode_ctx->height, 0, 0);
    if (screen == 0) {
        printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
        avcodec_free_frame(&frame);
        avcodec_close(video_decode_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }

    SDL_Overlay* bmp = SDL_CreateYUVOverlay(video_decode_ctx->width, video_decode_ctx->height, SDL_YV12_OVERLAY, screen);
    if (bmp == 0) {
        printf("SDL_CreateYUVOverlay failed: %s\n", SDL_GetError());
        avcodec_free_frame(&frame);
        avcodec_close(video_decode_ctx);
        avformat_close_input(&format_ctx);
        return -1;
    }*/

    SDL_Event event;
    
    while (1) {
        SDL_WaitEvent(&event);
        switch (event.type) {
        case kEventQuit:
            // go through
        case SDL_QUIT:
            g_quit = true;
            SDL_Quit();
            break;
        case kEventAlloc:
            //MakePicture();
            break;
        case kEventRefresh:
            RefreshVideoTimer();
            break;
        default:
            //nothing
            break;
        }
    }

    //SDL_FreeYUVOverlay(bmp);
    //avcodec_free_frame(&frame);

    return 0;
}
