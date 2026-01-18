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

#define SKIP_HOLD_TIME 3.0

static uint8_t *audio_buffer = NULL;
static int audio_buffer_size = 0;
static int audio_buffer_pos = 0;

void audio_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;
    
    if (!audio_buffer || audio_buffer_pos >= audio_buffer_size) {
        memset(stream, 0, len);
        return;
    }

    int to_copy = len;
    if (audio_buffer_pos + to_copy > audio_buffer_size)
        to_copy = audio_buffer_size - audio_buffer_pos;

    memcpy(stream, audio_buffer + audio_buffer_pos, to_copy);
    audio_buffer_pos += to_copy;

    if (to_copy < len)
        memset(stream + to_copy, 0, len - to_copy);
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
    printf("[Video] Starting playback: %s\n", path);
    
    // Инициализируем контроллеры
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    
    // Инициализируем SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        printf("[Video] SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    
    // Сначала пробуем открыть файл напрямую
    AVFormatContext *fmt_ctx = NULL;
    
    // Пробуем разные префиксы
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "sdmc:%s", path);
    
    // Удаляем "romfs:/" если есть
    const char *actual_path = path;
    if (strncmp(path, "romfs:/", 7) == 0) {
        // Попробуем открыть без префикса
        actual_path = path + 7;
    }
    
    // Пробуем открыть файл
    if (avformat_open_input(&fmt_ctx, full_path, NULL, NULL) < 0) {
        printf("[Video] Cannot open file: %s\n", full_path);
        // Попробуем без sdmc: префикса
        if (avformat_open_input(&fmt_ctx, path, NULL, NULL) < 0) {
            printf("[Video] Cannot open file: %s\n", path);
            // Попробуем с romfs: префиксом
            if (strncmp(path, "romfs:/", 7) != 0) {
                snprintf(full_path, sizeof(full_path), "romfs:/%s", path);
                if (avformat_open_input(&fmt_ctx, full_path, NULL, NULL) < 0) {
                    printf("[Video] Cannot open any variant of the file\n");
                    SDL_Quit();
                    return;
                }
            } else {
                SDL_Quit();
                return;
            }
        }
    }
    
    printf("[Video] File opened successfully\n");
    
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("[Video] Could not find stream information\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    // Находим видеопоток
    int video_stream_index = -1;
    int audio_stream_index = -1;
    
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        } else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
        }
    }
    
    if (video_stream_index == -1) {
        printf("[Video] No video stream found\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    // Получаем параметры кодека видеопотока
    AVCodecParameters *video_codec_params = fmt_ctx->streams[video_stream_index]->codecpar;
    const AVCodec *video_codec = avcodec_find_decoder(video_codec_params->codec_id);
    
    if (!video_codec) {
        printf("[Video] Unsupported video codec\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    AVCodecContext *video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!video_codec_ctx) {
        printf("[Video] Could not allocate video codec context\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    if (avcodec_parameters_to_context(video_codec_ctx, video_codec_params) < 0) {
        printf("[Video] Could not copy video codec parameters\n");
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
        printf("[Video] Could not open video codec\n");
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
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
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 
                                               SDL_RENDERER_ACCELERATED | 
                                               SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("[Video] Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
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
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    // Подготавливаем контекст для преобразования изображения
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
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    // Выделяем память для преобразованного кадра
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgba_frame = av_frame_alloc();
    if (!frame || !rgba_frame) {
        printf("[Video] Could not allocate frames\n");
        if (frame) av_frame_free(&frame);
        if (rgba_frame) av_frame_free(&rgba_frame);
        sws_freeContext(sws_ctx);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
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
    
    // Аудио инициализация (упрощенная версия)
    SDL_AudioSpec wanted_spec, obtained_spec;
    SDL_zero(wanted_spec);
    wanted_spec.freq = 48000;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 2;
    wanted_spec.samples = 1024;
    wanted_spec.callback = audio_callback;
    
    if (SDL_OpenAudio(&wanted_spec, &obtained_spec) < 0) {
        printf("[Video] Could not open audio: %s\n", SDL_GetError());
        // Продолжаем без звука
    } else {
        SDL_PauseAudio(0);
    }
    
    // Основной цикл воспроизведения
    AVPacket packet;
    bool quit = false;
    double hold_time = 0.0;
    clock_t last_time = clock();
    
    while (!quit && av_read_frame(fmt_ctx, &packet) >= 0) {
        // Обновляем состояние контроллера
        padUpdate(&pad);
        u64 kHeld = padGetButtons(&pad);
        
        clock_t now = clock();
        double elapsed = (double)(now - last_time) / CLOCKS_PER_SEC;
        last_time = now;
        
        if (skip_enabled && (kHeld & HidNpadButton_B)) {
            hold_time += elapsed;
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
        
        // Если пакет из видеопотока
        if (packet.stream_index == video_stream_index) {
            if (avcodec_send_packet(video_codec_ctx, &packet) < 0) {
                printf("[Video] Error sending packet\n");
                break;
            }
            
            while (avcodec_receive_frame(video_codec_ctx, frame) == 0) {
                // Преобразуем кадр в RGBA
                sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
                         frame->linesize, 0, video_codec_ctx->height,
                         rgba_frame->data, rgba_frame->linesize);
                
                // Обновляем текстуру SDL
                SDL_UpdateTexture(texture, NULL, rgba_frame->data[0], 
                                rgba_frame->linesize[0]);
                
                // Отрисовываем
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                
                // Рисуем индикатор пропуска если нужно
                if (hold_time > 0.0) {
                    float progress = fmin(hold_time / SKIP_HOLD_TIME, 1.0f);
                    draw_circle_progress(renderer, 
                                       video_codec_ctx->width - 30,
                                       video_codec_ctx->height - 30,
                                       20, progress);
                }
                
                SDL_RenderPresent(renderer);
                
                // Небольшая задержка для контроля скорости
                SDL_Delay(16);
            }
        }
        
        av_packet_unref(&packet);
    }
    
    // Очистка
    printf("[Video] Playback finished, cleaning up\n");
    
    if (audio_buffer) {
        free(audio_buffer);
        audio_buffer = NULL;
    }
    
    SDL_CloseAudio();
    
    av_free(buffer);
    av_frame_free(&rgba_frame);
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    avcodec_free_context(&video_codec_ctx);
    avformat_close_input(&fmt_ctx);
    SDL_Quit();
    
    printf("[Video] Cleanup complete\n");
}
