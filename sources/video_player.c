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
#include <libavutil/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define SKIP_HOLD_TIME 3.0

typedef struct {
    uint8_t *data;
    int size;
    int pos;
} AudioBuffer;

static AudioBuffer audio_buf = {0};
static SDL_mutex *audio_mutex = NULL;

void audio_callback(void *userdata, Uint8 *stream, int len)
{
    AudioBuffer *buf = (AudioBuffer*)userdata;
    
    SDL_LockMutex(audio_mutex);
    
    if (!buf->data || buf->pos >= buf->size) {
        memset(stream, 0, len);
        SDL_UnlockMutex(audio_mutex);
        return;
    }

    int to_copy = len;
    if (buf->pos + to_copy > buf->size)
        to_copy = buf->size - buf->pos;

    memcpy(stream, buf->data + buf->pos, to_copy);
    buf->pos += to_copy;

    if (to_copy < len)
        memset(stream + to_copy, 0, len - to_copy);
    
    SDL_UnlockMutex(audio_mutex);
}

void draw_circle_progress(SDL_Renderer *ren, int cx, int cy, int radius, float progress)
{
    int segments = 64;
    float angle_step = 2.0f * M_PI / segments;

    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);

    for (int i = 0; i < segments * progress; i++) {
        float angle = i * angle_step - M_PI / 2;
        int x = cx + (int)(cosf(angle) * radius);
        int y = cy + (int)(sinf(angle) * radius);
        SDL_RenderDrawPoint(ren, x, y);
    }
}

void play_video_file(const char *path, int skip_enabled)
{
    printf("[Video] Starting: %s\n", path);
    
    // Создаем мьютекс для аудио
    audio_mutex = SDL_CreateMutex();
    if (!audio_mutex) {
        printf("[Video] Failed to create audio mutex\n");
        return;
    }
    
    // Инициализируем контроллеры
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    
    // Инициализируем SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        printf("[Video] SDL_Init failed: %s\n", SDL_GetError());
        SDL_DestroyMutex(audio_mutex);
        return;
    }
    
    // Пробуем открыть файл
    AVFormatContext *fmt_ctx = NULL;
    if (avformat_open_input(&fmt_ctx, path, NULL, NULL) < 0) {
        printf("[Video] Cannot open file: %s\n", path);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("[Video] Could not find stream info\n");
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    // Находим видеопоток и аудиопоток
    int video_stream_index = -1;
    int audio_stream_index = -1;
    
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
            video_stream_index = i;
        } else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1) {
            audio_stream_index = i;
        }
    }
    
    if (video_stream_index == -1) {
        printf("[Video] No video stream found\n");
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    // Получаем информацию о потоках
    AVStream *video_stream = fmt_ctx->streams[video_stream_index];
    AVStream *audio_stream = audio_stream_index != -1 ? fmt_ctx->streams[audio_stream_index] : NULL;
    
    // Видео декодер
    const AVCodec *video_codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!video_codec) {
        printf("[Video] Unsupported video codec\n");
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    AVCodecContext *video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!video_codec_ctx) {
        printf("[Video] Could not allocate video codec context\n");
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    if (avcodec_parameters_to_context(video_codec_ctx, video_stream->codecpar) < 0) {
        printf("[Video] Could not copy video codec parameters\n");
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
        printf("[Video] Could not open video codec\n");
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    // Аудио декодер
    AVCodecContext *audio_codec_ctx = NULL;
    SwrContext *swr_ctx = NULL;
    int audio_sample_rate = 48000;
    int audio_channels = 2;
    
    if (audio_stream_index != -1) {
        const AVCodec *audio_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
        if (audio_codec) {
            audio_codec_ctx = avcodec_alloc_context3(audio_codec);
            if (audio_codec_ctx) {
                if (avcodec_parameters_to_context(audio_codec_ctx, audio_stream->codecpar) >= 0) {
                    if (avcodec_open2(audio_codec_ctx, audio_codec, NULL) >= 0) {
                        // Инициализируем ресемплер
                        swr_ctx = swr_alloc();
                        if (swr_ctx) {
                            av_opt_set_chlayout(swr_ctx, "in_chlayout", &audio_codec_ctx->ch_layout, 0);
                            av_opt_set_int(swr_ctx, "in_sample_rate", audio_codec_ctx->sample_rate, 0);
                            av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_codec_ctx->sample_fmt, 0);
                            
                            // Выходной формат: стерео, 48kHz, S16
                            AVChannelLayout out_chlayout;
                            av_channel_layout_default(&out_chlayout, audio_channels);
                            
                            av_opt_set_chlayout(swr_ctx, "out_chlayout", &out_chlayout, 0);
                            av_opt_set_int(swr_ctx, "out_sample_rate", audio_sample_rate, 0);
                            av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                            
                            if (swr_init(swr_ctx) < 0) {
                                swr_free(&swr_ctx);
                                swr_ctx = NULL;
                            }
                            av_channel_layout_uninit(&out_chlayout);
                        }
                    } else {
                        avcodec_free_context(&audio_codec_ctx);
                        audio_codec_ctx = NULL;
                    }
                } else {
                    avcodec_free_context(&audio_codec_ctx);
                    audio_codec_ctx = NULL;
                }
            }
        }
    }
    
    // Создаем окно SDL
    SDL_Window *window = SDL_CreateWindow("Video", 
                                         SDL_WINDOWPOS_CENTERED, 
                                         SDL_WINDOWPOS_CENTERED, 
                                         video_codec_ctx->width, 
                                         video_codec_ctx->height, 
                                         0);
    if (!window) {
        printf("[Video] Could not create window: %s\n", SDL_GetError());
        if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
        if (swr_ctx) swr_free(&swr_ctx);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 
                                               SDL_RENDERER_ACCELERATED | 
                                               SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("[Video] Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
        if (swr_ctx) swr_free(&swr_ctx);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                            SDL_PIXELFORMAT_RGBA32,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            video_codec_ctx->width,
                                            video_codec_ctx->height);
    if (!texture) {
        printf("[Video] Could not create texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
        if (swr_ctx) swr_free(&swr_ctx);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    // Контекст для преобразования цвета
    struct SwsContext *sws_ctx = sws_getContext(
        video_codec_ctx->width,
        video_codec_ctx->height,
        video_codec_ctx->pix_fmt,
        video_codec_ctx->width,
        video_codec_ctx->height,
        AV_PIX_FMT_RGBA,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
    
    if (!sws_ctx) {
        printf("[Video] Could not create sws context\n");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
        if (swr_ctx) swr_free(&swr_ctx);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    // Подготавливаем кадры
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgba_frame = av_frame_alloc();
    AVFrame *audio_frame = av_frame_alloc();
    if (!frame || !rgba_frame || !audio_frame) {
        printf("[Video] Could not allocate frames\n");
        if (frame) av_frame_free(&frame);
        if (rgba_frame) av_frame_free(&rgba_frame);
        if (audio_frame) av_frame_free(&audio_frame);
        sws_freeContext(sws_ctx);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
        if (swr_ctx) swr_free(&swr_ctx);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_DestroyMutex(audio_mutex);
        SDL_Quit();
        return;
    }
    
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, 
                                            video_codec_ctx->width, 
                                            video_codec_ctx->height, 
                                            1);
    uint8_t *buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
    av_image_fill_arrays(rgba_frame->data, rgba_frame->linesize, buffer,
                        AV_PIX_FMT_RGBA, video_codec_ctx->width, 
                        video_codec_ctx->height, 1);
    
    // Настраиваем аудио в SDL
    SDL_AudioSpec wanted_spec, obtained_spec;
    SDL_zero(wanted_spec);
    wanted_spec.freq = audio_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = audio_channels;
    wanted_spec.samples = 1024;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = &audio_buf;
    
    bool audio_initialized = false;
    if (audio_codec_ctx && swr_ctx) {
        if (SDL_OpenAudio(&wanted_spec, &obtained_spec) >= 0) {
            audio_initialized = true;
            SDL_PauseAudio(0);
        }
    }
    
    // Основной цикл
    AVPacket packet;
    bool quit = false;
    double hold_time = 0.0;
    
    // Для синхронизации видео
    int64_t start_time = av_gettime();
    AVRational time_base = video_stream->time_base;
    
    while (!quit && av_read_frame(fmt_ctx, &packet) >= 0) {
        // Обновляем состояние контроллера
        padUpdate(&pad);
        u64 kHeld = padGetButtons(&pad);
        
        if (skip_enabled && (kHeld & HidNpadButton_B)) {
            hold_time += 0.016; // Примерно один кадр
            if (hold_time >= SKIP_HOLD_TIME) {
                printf("[Video] Skip triggered\n");
                quit = true;
            }
        } else {
            hold_time = 0.0;
        }
        
        // Проверка событий SDL
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }
        
        // Видео пакет
        if (packet.stream_index == video_stream_index) {
            if (avcodec_send_packet(video_codec_ctx, &packet) < 0) {
                break;
            }
            
            while (avcodec_receive_frame(video_codec_ctx, frame) == 0) {
                // Вычисляем время показа кадра
                int64_t pts = frame->pts;
                if (pts == AV_NOPTS_VALUE) {
                    pts = frame->pkt_dts;
                }
                
                if (pts != AV_NOPTS_VALUE) {
                    // Конвертируем PTS во время
                    double frame_time = pts * av_q2d(time_base);
                    
                    // Вычисляем задержку
                    double current_time = (av_gettime() - start_time) / 1000000.0;
                    double delay = frame_time - current_time;
                    
                    if (delay > 0) {
                        // Ждем нужное время
                        av_usleep((int64_t)(delay * 1000000));
                    }
                }
                
                // Конвертируем кадр в RGBA
                sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
                         frame->linesize, 0, video_codec_ctx->height,
                         rgba_frame->data, rgba_frame->linesize);
                
                // Обновляем текстуру
                SDL_UpdateTexture(texture, NULL, rgba_frame->data[0], 
                                rgba_frame->linesize[0]);
                
                // Отрисовываем
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                
                // Индикатор пропуска
                if (hold_time > 0.0) {
                    float progress = fmin(hold_time / SKIP_HOLD_TIME, 1.0f);
                    draw_circle_progress(renderer, 
                                       video_codec_ctx->width - 30,
                                       video_codec_ctx->height - 30,
                                       20, progress);
                }
                
                SDL_RenderPresent(renderer);
            }
        }
        
        // Аудио пакет
        if (packet.stream_index == audio_stream_index && audio_codec_ctx && swr_ctx) {
            if (avcodec_send_packet(audio_codec_ctx, &packet) < 0) {
                break;
            }
            
            while (avcodec_receive_frame(audio_codec_ctx, audio_frame) == 0) {
                // Вычисляем необходимое количество сэмплов на выходе
                int out_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_frame->sample_rate) + 
                                                audio_frame->nb_samples,
                                                audio_sample_rate, audio_frame->sample_rate, AV_ROUND_UP);
                
                // Выделяем буфер для конвертированного аудио
                uint8_t *converted_audio = NULL;
                int out_linesize;
                int out_count = av_samples_alloc(&converted_audio, &out_linesize, audio_channels,
                                                out_samples, AV_SAMPLE_FMT_S16, 1);
                
                if (out_count >= 0) {
                    // Конвертируем
                    int converted = swr_convert(swr_ctx, &converted_audio, out_samples,
                                              (const uint8_t**)audio_frame->data, audio_frame->nb_samples);
                    
                    if (converted > 0) {
                        SDL_LockMutex(audio_mutex);
                        
                        // Освобождаем старый буфер
                        if (audio_buf.data) {
                            free(audio_buf.data);
                        }
                        
                        // Обновляем буфер
                        audio_buf.data = converted_audio;
                        audio_buf.size = av_samples_get_buffer_size(&out_linesize, audio_channels,
                                                                  converted, AV_SAMPLE_FMT_S16, 1);
                        audio_buf.pos = 0;
                        
                        SDL_UnlockMutex(audio_mutex);
                    } else {
                        av_freep(&converted_audio);
                    }
                }
            }
        }
        
        av_packet_unref(&packet);
    }
    
    // Ждем немного, чтобы аудио закончило воспроизводиться
    if (audio_initialized) {
        SDL_Delay(100);
        SDL_PauseAudio(1);
        SDL_CloseAudio();
    }
    
    // Очистка
    SDL_LockMutex(audio_mutex);
    if (audio_buf.data) {
        free(audio_buf.data);
        audio_buf.data = NULL;
        audio_buf.size = 0;
        audio_buf.pos = 0;
    }
    SDL_UnlockMutex(audio_mutex);
    
    av_free(buffer);
    av_frame_free(&audio_frame);
    av_frame_free(&rgba_frame);
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
    if (swr_ctx) swr_free(&swr_ctx);
    avcodec_free_context(&video_codec_ctx);
    avformat_close_input(&fmt_ctx);
    
    SDL_DestroyMutex(audio_mutex);
    SDL_Quit();
    
    printf("[Video] Finished\n");
}
