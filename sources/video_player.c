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
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define SKIP_HOLD_TIME 3.0  // секунд удержания для пропуска

typedef struct {
    uint8_t *data;
    int size;
    int pos;
} AudioBuffer;

static AudioBuffer audio_buf = {0};

/// SDL аудио callback
void audio_callback(void *userdata, Uint8 *stream, int len)
{
    AudioBuffer *buf = (AudioBuffer*)userdata;
    
    if (!buf->data || buf->pos >= buf->size) {
        memset(stream, 0, len);
        return;
    }

    int to_copy = len;
    if (buf->pos + to_copy > buf->size)
        to_copy = buf->size - buf->pos;

    memcpy(stream, buf->data + buf->pos, to_copy);
    buf->pos += to_copy;

    if (to_copy < len)
        memset(stream + to_copy, 0, len - to_copy);
}

// ----------------- отрисовка круговой прогресс-бара -----------------
void draw_circle_progress(SDL_Renderer *ren, int cx, int cy, int radius, float progress)
{
    int segments = 64;
    float angle_step = 2.0f * M_PI / segments;

    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); // белый

    for (int i = 0; i < segments * progress; i++) {
        float angle = i * angle_step - M_PI / 2;
        int x = cx + (int)(cosf(angle) * radius);
        int y = cy + (int)(sinf(angle) * radius);
        SDL_RenderDrawPoint(ren, x, y);
    }
}

// ----------------- основной видеоплеер -----------------
void play_video_file(const char *path, int skip_enabled)
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
    AVPacket *pkt = NULL;

    int vstream = -1, astream = -1;
    int w = 0, h = 0;
    int ret = 0;

    if (avformat_open_input(&fmt, path, NULL, NULL) < 0) {
        printf("[Video] Failed to open file: %s\n", path);
        goto cleanup;
    }
    
    if (avformat_find_stream_info(fmt, NULL) < 0) {
        printf("[Video] Failed to find stream info\n");
        goto cleanup;
    }

    for (unsigned i = 0; i < fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && vstream < 0)
            vstream = i;
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && astream < 0)
            astream = i;
    }

    if (vstream < 0) {
        printf("[Video] No video stream found\n");
        goto cleanup;
    }

    // Видео декодер
    const AVCodec *vcodec = avcodec_find_decoder(fmt->streams[vstream]->codecpar->codec_id);
    if (!vcodec) {
        printf("[Video] Video codec not found\n");
        goto cleanup;
    }
    
    vdec = avcodec_alloc_context3(vcodec);
    if (!vdec) {
        printf("[Video] Failed to alloc video context\n");
        goto cleanup;
    }
    
    if (avcodec_parameters_to_context(vdec, fmt->streams[vstream]->codecpar) < 0) {
        printf("[Video] Failed to copy video codec params\n");
        goto cleanup;
    }
    
    if (avcodec_open2(vdec, vcodec, NULL) < 0) {
        printf("[Video] Failed to open video codec\n");
        goto cleanup;
    }
    w = vdec->width;
    h = vdec->height;

    // Аудио декодер
    if (astream >= 0) {
        const AVCodec *acodec = avcodec_find_decoder(fmt->streams[astream]->codecpar->codec_id);
        if (!acodec) {
            printf("[Video] Audio codec not found\n");
        } else {
            adec = avcodec_alloc_context3(acodec);
            if (!adec) {
                printf("[Video] Failed to alloc audio context\n");
            } else if (avcodec_parameters_to_context(adec, fmt->streams[astream]->codecpar) < 0) {
                printf("[Video] Failed to copy audio codec params\n");
                avcodec_free_context(&adec);
                adec = NULL;
            } else if (avcodec_open2(adec, acodec, NULL) < 0) {
                printf("[Video] Failed to open audio codec\n");
                avcodec_free_context(&adec);
                adec = NULL;
            }
        }
    }

    frame = av_frame_alloc();
    rgb = av_frame_alloc();
    aframe = av_frame_alloc();
    
    if (!frame || !rgb || !aframe) {
        printf("[Video] Failed to alloc frames\n");
        goto cleanup;
    }

    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, w, h, 1);
    uint8_t *buffer = (uint8_t*)malloc(num_bytes);
    if (!buffer) {
        printf("[Video] Failed to alloc image buffer\n");
        goto cleanup;
    }
    
    av_image_fill_arrays(rgb->data, rgb->linesize, buffer, AV_PIX_FMT_RGBA, w, h, 1);

    sws = sws_getContext(w, h, vdec->pix_fmt, w, h, AV_PIX_FMT_RGBA, 
                         SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws) {
        printf("[Video] Failed to create sws context\n");
        goto cleanup;
    }

    // SDL окно
    SDL_Window *win = SDL_CreateWindow("Video", SDL_WINDOWPOS_CENTERED, 
                                       SDL_WINDOWPOS_CENTERED, w, h, 0);
    if (!win) {
        printf("[Video] Failed to create window: %s\n", SDL_GetError());
        goto cleanup;
    }
    
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren) {
        printf("[Video] Failed to create renderer: %s\n", SDL_GetError());
        goto cleanup;
    }
    
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, 
                                        SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!tex) {
        printf("[Video] Failed to create texture: %s\n", SDL_GetError());
        goto cleanup;
    }

    // SDL Audio
    SDL_AudioSpec spec;
    SDL_zero(spec);
    if (adec) {
        spec.freq = adec->sample_rate;
        spec.format = AUDIO_S16SYS;
        spec.channels = adec->ch_layout.nb_channels;
        spec.samples = 1024;
        spec.callback = audio_callback;
        spec.userdata = &audio_buf;
        
        if (SDL_OpenAudio(&spec, NULL) < 0) {
            printf("[Video] Failed to open audio: %s\n", SDL_GetError());
            adec = NULL; // Отключаем аудио
        } else {
            // Инициализация SwrContext для FFmpeg 6.0
            swr = swr_alloc();
            if (!swr) {
                printf("[Video] Failed to alloc swr context\n");
                SDL_CloseAudio();
                adec = NULL;
            } else {
                // Настройка входного канального лэйаута
                av_opt_set_chlayout(swr, "in_chlayout", &adec->ch_layout, 0);
                av_opt_set_int(swr, "in_sample_rate", adec->sample_rate, 0);
                av_opt_set_sample_fmt(swr, "in_sample_fmt", adec->sample_fmt, 0);
                
                // Настройка выходного канального лэйаута
                AVChannelLayout out_chlayout;
                av_channel_layout_default(&out_chlayout, spec.channels);
                
                av_opt_set_chlayout(swr, "out_chlayout", &out_chlayout, 0);
                av_opt_set_int(swr, "out_sample_rate", spec.freq, 0);
                av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                
                if (swr_init(swr) < 0) {
                    printf("[Video] Failed to init swr context\n");
                    swr_free(&swr);
                    swr = NULL;
                    SDL_CloseAudio();
                    adec = NULL;
                } else {
                    SDL_PauseAudio(0);
                    // Освобождаем out_chlayout после использования
                    av_channel_layout_uninit(&out_chlayout);
                }
            }
        }
    }

    printf("[Video] Playing: %s\n", path);

    bool quit = false;
    double hold_time = 0.0;
    const double dt = 0.016; // ~60fps
    clock_t last_time = clock();
    
    pkt = av_packet_alloc();
    if (!pkt) {
        printf("[Video] Failed to alloc packet\n");
        goto cleanup;
    }

    while (!quit && av_read_frame(fmt, pkt) >= 0) {
        // время кадра
        clock_t now = clock();
        double elapsed = (double)(now - last_time) / CLOCKS_PER_SEC;
        last_time = now;

        // Joycon check
        hidScanInput();
        u64 kHeld = hidKeysHeld(HidNpadIdType_No1);  // Используем HidNpadIdType_No1 вместо CONTROLLER_P1_AUTO
        
        if (skip_enabled && (kHeld & HidNpadButton_B)) {
            hold_time += elapsed;
            if (hold_time >= SKIP_HOLD_TIME) {
                quit = true;
            }
        } else {
            hold_time = 0.0;
        }

        // Проверка SDL событий для выхода
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Видео
        if (pkt->stream_index == vstream) {
            ret = avcodec_send_packet(vdec, pkt);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                printf("[Video] Error sending video packet: %d\n", ret);
            }
            
            while (avcodec_receive_frame(vdec, frame) == 0) {
                sws_scale(sws, (const uint8_t * const *)frame->data, 
                         frame->linesize, 0, h, rgb->data, rgb->linesize);
                
                SDL_UpdateTexture(tex, NULL, rgb->data[0], rgb->linesize[0]);
                SDL_RenderClear(ren);
                SDL_RenderCopy(ren, tex, NULL, NULL);

                // Круговой индикатор в правом нижнем углу
                if (hold_time > 0.0) {
                    float progress = fmin(hold_time / SKIP_HOLD_TIME, 1.0f);
                    int radius = 20;
                    int cx = w - 30;
                    int cy = h - 30;
                    draw_circle_progress(ren, cx, cy, radius, progress);
                }

                SDL_RenderPresent(ren);
                SDL_Delay((int)(dt * 1000));
            }
        }

        // Аудио
        if (adec && pkt->stream_index == astream) {
            ret = avcodec_send_packet(adec, pkt);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                printf("[Video] Error sending audio packet: %d\n", ret);
            }
            
            while (avcodec_receive_frame(adec, aframe) == 0) {
                // Выделяем буфер для конвертированного аудио
                int out_samples = av_rescale_rnd(swr_get_delay(swr, aframe->sample_rate) + 
                                                aframe->nb_samples, 
                                                spec.freq, aframe->sample_rate, AV_ROUND_UP);
                
                uint8_t *audio_buffer = NULL;
                int out_linesize;
                int out_count = av_samples_alloc(&audio_buffer, &out_linesize, spec.channels,
                                                out_samples, AV_SAMPLE_FMT_S16, 1);
                
                if (out_count >= 0) {
                    int converted = swr_convert(swr, &audio_buffer, out_samples,
                                              (const uint8_t**)aframe->data, aframe->nb_samples);
                    
                    if (converted > 0) {
                        // Освобождаем старый буфер
                        if (audio_buf.data) {
                            free(audio_buf.data);
                        }
                        
                        // Обновляем буфер
                        audio_buf.data = audio_buffer;
                        audio_buf.size = av_samples_get_buffer_size(&out_linesize, spec.channels,
                                                                  converted, AV_SAMPLE_FMT_S16, 1);
                        audio_buf.pos = 0;
                    } else {
                        av_freep(&audio_buffer);
                    }
                }
            }
        }

        av_packet_unref(pkt);
    }

    // Ждем окончания воспроизведения аудио
    if (adec) {
        SDL_PauseAudio(1);
        SDL_Delay(100); // Даем время завершиться коллбэкам
        SDL_CloseAudio();
    }

cleanup:
    // Освобождаем аудио буфер
    if (audio_buf.data) {
        free(audio_buf.data);
        audio_buf.data = NULL;
    }
    audio_buf.size = 0;
    audio_buf.pos = 0;
    
    if (buffer) free(buffer);
    if (frame) av_frame_free(&frame);
    if (rgb) av_frame_free(&rgb);
    if (aframe) av_frame_free(&aframe);
    if (vdec) avcodec_free_context(&vdec);
    if (adec) avcodec_free_context(&adec);
    if (fmt) avformat_close_input(&fmt);
    if (sws) sws_freeContext(sws);
    if (swr) swr_free(&swr);
    if (pkt) av_packet_free(&pkt);

    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
}
