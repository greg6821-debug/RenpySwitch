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
#include <libavutil/rational.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define SKIP_HOLD_TIME 3.0
#define DEFAULT_DELAY_SECONDS 3.0
#define AUDIO_BUFFER_SIZE (48000 * 2 * 2) // 1 секунда стерео 16-бит

typedef struct {
    uint8_t *data;
    int size;
    int write_pos;
    int read_pos;
    int available;
    SDL_mutex *mutex;
    SDL_cond *cond;
} AudioBuffer;

static AudioBuffer audio_buf = {0};
static SDL_AudioDeviceID audio_device = 0;

void audio_callback(void *userdata, Uint8 *stream, int len)
{
    AudioBuffer *buf = (AudioBuffer*)userdata;
    
    SDL_LockMutex(buf->mutex);
    
    if (buf->available == 0) {
        memset(stream, 0, len);
        SDL_UnlockMutex(buf->mutex);
        return;
    }
    
    int to_copy = len;
    if (to_copy > buf->available)
        to_copy = buf->available;
    
    // Копируем данные из кольцевого буфера
    int first_chunk = buf->size - buf->read_pos;
    if (first_chunk >= to_copy) {
        memcpy(stream, buf->data + buf->read_pos, to_copy);
        buf->read_pos = (buf->read_pos + to_copy) % buf->size;
    } else {
        memcpy(stream, buf->data + buf->read_pos, first_chunk);
        memcpy(stream + first_chunk, buf->data, to_copy - first_chunk);
        buf->read_pos = to_copy - first_chunk;
    }
    
    buf->available -= to_copy;
    
    // Если скопировали меньше запрошенного - заполняем нулями
    if (to_copy < len)
        memset(stream + to_copy, 0, len - to_copy);
    
    SDL_CondSignal(buf->cond);
    SDL_UnlockMutex(buf->mutex);
}

// Отрисовка плавной заполняемой кольцевой полосы
void draw_circle_progress(SDL_Renderer *ren, int cx, int cy, int radius, int thickness, float progress)
{
    if (progress <= 0.0f) return;
    
    int segments = (int)(360 * progress);
    float angle_step = 2.0f * M_PI / 360.0f;
    float start_angle = -M_PI / 2;
    
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    
    for (int i = 0; i < segments; i++) {
        float angle = start_angle + i * angle_step;
        
        // Рисуем несколько точек для толщины круга
        for (int t = 0; t < thickness; t++) {
            float current_radius = radius - thickness/2 + t;
            int x = cx + (int)(cosf(angle) * current_radius);
            int y = cy + (int)(sinf(angle) * current_radius);
            SDL_RenderDrawPoint(ren, x, y);
        }
    }
}

// Инициализация аудио буфера
static bool init_audio_buffer(AudioBuffer *buf, int size)
{
    buf->data = (uint8_t*)malloc(size);
    if (!buf->data) return false;
    
    buf->size = size;
    buf->write_pos = 0;
    buf->read_pos = 0;
    buf->available = 0;
    
    buf->mutex = SDL_CreateMutex();
    buf->cond = SDL_CreateCond();
    
    if (!buf->mutex || !buf->cond) {
        if (buf->data) free(buf->data);
        if (buf->mutex) SDL_DestroyMutex(buf->mutex);
        if (buf->cond) SDL_DestroyCond(buf->cond);
        return false;
    }
    
    return true;
}

// Очистка аудио буфера
static void cleanup_audio_buffer(AudioBuffer *buf)
{
    if (buf->mutex) SDL_LockMutex(buf->mutex);
    
    if (buf->data) {
        free(buf->data);
        buf->data = NULL;
    }
    
    buf->size = 0;
    buf->write_pos = 0;
    buf->read_pos = 0;
    buf->available = 0;
    
    if (buf->mutex) {
        SDL_UnlockMutex(buf->mutex);
        SDL_DestroyMutex(buf->mutex);
        buf->mutex = NULL;
    }
    
    if (buf->cond) {
        SDL_DestroyCond(buf->cond);
        buf->cond = NULL;
    }
}

// Добавление аудио данных в буфер
static bool write_audio_data(AudioBuffer *buf, const uint8_t *data, int size)
{
    SDL_LockMutex(buf->mutex);
    
    // Ждем, если буфер полный
    while (buf->available + size > buf->size) {
        SDL_CondWait(buf->cond, buf->mutex);
    }
    
    // Записываем в кольцевой буфер
    int free_space = buf->size - buf->available;
    if (free_space >= size) {
        // Хватает места от write_pos до конца буфера
        int space_to_end = buf->size - buf->write_pos;
        if (space_to_end >= size) {
            memcpy(buf->data + buf->write_pos, data, size);
            buf->write_pos = (buf->write_pos + size) % buf->size;
        } else {
            memcpy(buf->data + buf->write_pos, data, space_to_end);
            memcpy(buf->data, data + space_to_end, size - space_to_end);
            buf->write_pos = size - space_to_end;
        }
        buf->available += size;
        SDL_UnlockMutex(buf->mutex);
        return true;
    }
    
    SDL_UnlockMutex(buf->mutex);
    return false;
}

// Очистка экрана черным цветом
static void clear_screen(SDL_Renderer *renderer, int width, int height)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_Delay(1); // Даем время для отрисовки
}

void play_video_file_delay(const char *path, int skip_enabled, float delay_seconds)
{
    printf("[Video] Starting: %s (delay: %.1fs, skip: %s)\n", 
           path, delay_seconds, skip_enabled ? "enabled" : "disabled");
    
    // Инициализируем аудио буфер
    if (!init_audio_buffer(&audio_buf, AUDIO_BUFFER_SIZE)) {
        printf("[Video] Failed to init audio buffer\n");
        return;
    }
    
    // Инициализируем контроллеры
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    
    // Инициализируем SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        printf("[Video] SDL_Init failed: %s\n", SDL_GetError());
        cleanup_audio_buffer(&audio_buf);
        return;
    }
    
    // Пробуем открыть файл
    AVFormatContext *fmt_ctx = NULL;
    if (avformat_open_input(&fmt_ctx, path, NULL, NULL) < 0) {
        printf("[Video] Cannot open file: %s\n", path);
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
        return;
    }
    
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("[Video] Could not find stream info\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
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
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
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
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
        return;
    }
    
    AVCodecContext *video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!video_codec_ctx) {
        printf("[Video] Could not allocate video codec context\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
        return;
    }
    
    if (avcodec_parameters_to_context(video_codec_ctx, video_stream->codecpar) < 0) {
        printf("[Video] Could not copy video codec parameters\n");
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
        return;
    }
    
    if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
        printf("[Video] Could not open video codec\n");
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
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
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
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
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
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
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
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
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
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
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
        return;
    }
    
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, 
                                            video_codec_ctx->width, 
                                            video_codec_ctx->height, 
                                            1);
    uint8_t *buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
    if (!buffer) {
        printf("[Video] Could not allocate buffer\n");
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
        SDL_Quit();
        cleanup_audio_buffer(&audio_buf);
        return;
    }
    
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
        audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &obtained_spec, 0);
        if (audio_device > 0) {
            audio_initialized = true;
            SDL_PauseAudioDevice(audio_device, 0);
        }
    }
    
    // Задержка перед началом воспроизведения
    if (delay_seconds > 0) {
        printf("[Video] Waiting %.1f seconds before playback...\n", delay_seconds);
        
        Uint32 start_delay = SDL_GetTicks();
        float elapsed_delay = 0.0f;
        bool skip_delay = false;
        
        while (elapsed_delay < delay_seconds && !skip_delay) {
            // Обновляем состояние контроллера
            padUpdate(&pad);
            u64 kHeld = padGetButtons(&pad);
            
            // Проверяем, не нажата ли кнопка B для пропуска задержки
            if (skip_enabled && (kHeld & HidNpadButton_B)) {
                skip_delay = true;
                printf("[Video] Delay skipped\n");
                break;
            }
            
            // Проверка событий SDL
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    skip_delay = true;
                    break;
                }
            }
            
            // Очищаем экран черным цветом
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
            
            // Ждем немного
            SDL_Delay(16);
            
            // Обновляем прошедшее время
            elapsed_delay = (SDL_GetTicks() - start_delay) / 1000.0f;
        }
    }
    
    // Основной цикл воспроизведения
    AVPacket *packet = av_packet_alloc();
    bool quit = false;
    float hold_time = 0.0f;
    Uint32 last_button_check = SDL_GetTicks();
    
    // Для синхронизации видео
    int64_t start_time = av_gettime(); // Время начала воспроизведения в микросекундах
    double video_clock = 0; // Текущее время видео в секундах
    AVRational time_base = video_stream->time_base;
    double frame_delay = av_q2d(video_stream->avg_frame_rate);
    if (frame_delay > 0) {
        frame_delay = 1.0 / frame_delay;
    } else {
        // Если не можем получить fps, используем 30 fps
        frame_delay = 1.0 / 30.0;
    }
    
    while (!quit && av_read_frame(fmt_ctx, packet) >= 0) {
        // Частая проверка кнопок для пропуска
        Uint32 now = SDL_GetTicks();
        float delta_time = (now - last_button_check) / 1000.0f;
        last_button_check = now;
        
        // Обновляем состояние контроллера
        padUpdate(&pad);
        u64 kHeld = padGetButtons(&pad);
        
        if (skip_enabled && (kHeld & HidNpadButton_B)) {
            hold_time += delta_time;
            if (hold_time >= SKIP_HOLD_TIME) {
                printf("[Video] Skip triggered after %.1f seconds\n", hold_time);
                quit = true;
                av_packet_unref(packet);
                break;
            }
        } else {
            hold_time = 0.0f;
        }
        
        // Проверка событий SDL
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                av_packet_unref(packet);
                break;
            }
        }
        
        if (quit) {
            av_packet_unref(packet);
            break;
        }
        
        // Видео пакет
        if (packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(video_codec_ctx, packet) < 0) {
                break;
            }
            
            while (avcodec_receive_frame(video_codec_ctx, frame) == 0) {
                // Получаем PTS кадра (время отображения)
                if (frame->pts != AV_NOPTS_VALUE) {
                    video_clock = frame->pts * av_q2d(time_base);
                } else if (frame->pkt_dts != AV_NOPTS_VALUE) {
                    video_clock = frame->pkt_dts * av_q2d(time_base);
                }
                
                // Синхронизация по времени
                double current_time = (av_gettime() - start_time) / 1000000.0;
                double sync_delay = video_clock - current_time;
                
                // Если кадр должен быть показан в будущем, ждем
                if (sync_delay > 0) {
                    // Ждем, но не больше frame_delay * 2
                    if (sync_delay > frame_delay * 2) {
                        sync_delay = frame_delay;
                    }
                    av_usleep((int64_t)(sync_delay * 1000000));
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
                
                // Плавный индикатор пропуска
                if (hold_time > 0.0f) {
                    float progress = fminf(hold_time / SKIP_HOLD_TIME, 1.0f);
                    draw_circle_progress(renderer, 
                                       video_codec_ctx->width - 40,
                                       video_codec_ctx->height - 40,
                                       25, 6, progress);
                }
                
                SDL_RenderPresent(renderer);
                
                // Небольшая задержка для стабильности FPS
                SDL_Delay(1);
            }
        }
        
        // Аудио пакет
        if (packet->stream_index == audio_stream_index && audio_codec_ctx && swr_ctx) {
            if (avcodec_send_packet(audio_codec_ctx, packet) < 0) {
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
                        // Записываем в кольцевой буфер
                        int actual_size = av_samples_get_buffer_size(&out_linesize, audio_channels,
                                                                    converted, AV_SAMPLE_FMT_S16, 1);
                        write_audio_data(&audio_buf, converted_audio, actual_size);
                    }
                    
                    av_freep(&converted_audio);
                }
            }
        }
        
        av_packet_unref(packet);
    }
    
    // Очистка пакета
    if (packet) {
        av_packet_free(&packet);
        packet = NULL;
    }
    
    // Очищаем экран черным цветом
    printf("[Video] Clearing screen...\n");
    clear_screen(renderer, video_codec_ctx->width, video_codec_ctx->height);
    
    // Пауза перед выходом
    printf("[Video] Pausing for 1 second before cleanup...\n");
    SDL_Delay(1000);
    
    // Закрываем аудио
    if (audio_initialized) {
        // Даем немного времени на завершение воспроизведения
        SDL_Delay(100);
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
    
    // Освобождение ресурсов
    printf("[Video] Freeing resources...\n");
    
    if (buffer) {
        av_free(buffer);
        buffer = NULL;
    }
    
    if (audio_frame) {
        av_frame_free(&audio_frame);
        audio_frame = NULL;
    }
    
    if (rgba_frame) {
        av_frame_free(&rgba_frame);
        rgba_frame = NULL;
    }
    
    if (frame) {
        av_frame_free(&frame);
        frame = NULL;
    }
    
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = NULL;
    }
    
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    
    if (audio_codec_ctx) {
        avcodec_free_context(&audio_codec_ctx);
        audio_codec_ctx = NULL;
    }
    
    if (swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = NULL;
    }
    
    if (video_codec_ctx) {
        avcodec_free_context(&video_codec_ctx);
        video_codec_ctx = NULL;
    }
    
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = NULL;
    }
    
    SDL_Quit();
    cleanup_audio_buffer(&audio_buf);
    
    printf("[Video] Finished\n");
}

// Совместимость со старой функцией
void play_video_file(const char *path, int skip_enabled)
{
    play_video_file_delay(path, skip_enabled, DEFAULT_DELAY_SECONDS);
}
