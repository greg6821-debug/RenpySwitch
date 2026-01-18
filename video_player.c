#include "video_player.h"
#include <switch.h>
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    int size;
    int pos;
} AudioBuffer;

static AudioBuffer audio_buf = {0};

void audio_callback(void *userdata, Uint8 *stream, int len)
{
    if (!audio_buf.data || audio_buf.pos >= audio_buf.size) {
        memset(stream, 0, len);
        return;
    }

    int to_copy = len;
    if (audio_buf.pos + to_copy > audio_buf.size)
        to_copy = audio_buf.size - audio_buf.pos;

    memcpy(stream, audio_buf.data + audio_buf.pos, to_copy);
    audio_buf.pos += to_copy;

    if (to_copy < len)
        memset(stream + to_copy, 0, len - to_copy);
}

void play_video_file(const char *path)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        printf("[Video] SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    AVFormatContext *fmt = NULL;
    AVCodecContext *vdec = NULL;
    AVCodecContext *adec = NULL;
    AVFrame *frame = NULL, *rgb = NULL;
    AVFrame *aframe = NULL;
    struct SwsContext *sws = NULL;
    struct SwrContext *swr = NULL;
    AVPacket pkt;

    int vstream = -1, astream = -1;
    int w=0, h=0;

    if (avformat_open_input(&fmt, path, NULL, NULL) < 0) goto cleanup;
    avformat_find_stream_info(fmt, NULL);

    // Находим видео и аудио потоки
    for (unsigned i=0;i<fmt->nb_streams;i++){
        if(fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && vstream<0)
            vstream=i;
        if(fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && astream<0)
            astream=i;
    }

    if(vstream<0) goto cleanup;

    // Видео декодер
    const AVCodec *vcodec = avcodec_find_decoder(fmt->streams[vstream]->codecpar->codec_id);
    vdec = avcodec_alloc_context3(vcodec);
    avcodec_parameters_to_context(vdec, fmt->streams[vstream]->codecpar);
    avcodec_open2(vdec, vcodec, NULL);
    w=vdec->width; h=vdec->height;

    // Аудио декодер
    if(astream>=0){
        const AVCodec *acodec = avcodec_find_decoder(fmt->streams[astream]->codecpar->codec_id);
        adec = avcodec_alloc_context3(acodec);
        avcodec_parameters_to_context(adec, fmt->streams[astream]->codecpar);
        avcodec_open2(adec, acodec, NULL);
    }

    frame = av_frame_alloc();
    rgb   = av_frame_alloc();
    aframe = av_frame_alloc();

    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, w, h, 1);
    uint8_t *buffer = malloc(num_bytes);
    av_image_fill_arrays(rgb->data, rgb->linesize, buffer, AV_PIX_FMT_RGBA, w, h, 1);

    sws = sws_getContext(w,h,vdec->pix_fmt,w,h,AV_PIX_FMT_RGBA,SWS_BILINEAR,NULL,NULL,NULL);

    // SDL окно
    SDL_Window *win = SDL_CreateWindow("Video", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win,-1,0);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);

    // SDL Audio
    SDL_AudioSpec spec;
    SDL_zero(spec);
    if(adec){
        spec.freq = adec->sample_rate;
        spec.format = AUDIO_S16SYS;
        spec.channels = adec->channels;
        spec.samples = 1024;
        spec.callback = audio_callback;
        SDL_OpenAudio(&spec, NULL);
        SDL_PauseAudio(0);

        swr = swr_alloc_set_opts(NULL,
            av_get_default_channel_layout(spec.channels),
            AV_SAMPLE_FMT_S16,
            spec.freq,
            av_get_default_channel_layout(adec->channels),
            adec->sample_fmt,
            adec->sample_rate,
            0,NULL);
        swr_init(swr);
    }

    printf("[Video] Playing: %s\n", path);

    bool quit=false;
    while(!quit && av_read_frame(fmt, &pkt)>=0){
        if(pkt.stream_index==vstream){
            avcodec_send_packet(vdec,&pkt);
            while(avcodec_receive_frame(vdec,frame)==0){
                sws_scale(sws,(const uint8_t * const *)frame->data,frame->linesize,0,h,rgb->data,rgb->linesize);
                SDL_UpdateTexture(tex,NULL,rgb->data[0],rgb->linesize[0]);
                SDL_RenderClear(ren);
                SDL_RenderCopy(ren,tex,NULL,NULL);
                SDL_RenderPresent(ren);

                // Проверка joycon
                hidScanInput();
                u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
                if(kDown & KEY_Y) quit=true;
            }
        }
        if(adec && pkt.stream_index==astream){
            avcodec_send_packet(adec,&pkt);
            while(avcodec_receive_frame(adec,aframe)==0){
                int out_count = av_samples_get_buffer_size(NULL, spec.channels, aframe->nb_samples, AV_SAMPLE_FMT_S16,1);
                audio_buf.data = malloc(out_count);
                swr_convert(swr,&audio_buf.data,aframe->nb_samples,(const uint8_t**)aframe->data,aframe->nb_samples);
                audio_buf.size=out_count;
                audio_buf.pos=0;
            }
        }
        av_packet_unref(&pkt);
    }

    SDL_PauseAudio(1);

cleanup:
    if(buffer) free(buffer);
    if(frame) av_frame_free(&frame);
    if(rgb) av_frame_free(&rgb);
    if(aframe) av_frame_free(&aframe);
    if(vdec) avcodec_free_context(&vdec);
    if(adec) avcodec_free_context(&adec);
    if(fmt) avformat_close_input(&fmt);
    if(sws) sws_freeContext(sws);
    if(swr) swr_free(&swr);

    if(audio_buf.data) free(audio_buf.data);

    if(tex) SDL_DestroyTexture(tex);
    if(ren) SDL_DestroyRenderer(ren);
    if(win) SDL_DestroyWindow(win);
    SDL_Quit();
}
