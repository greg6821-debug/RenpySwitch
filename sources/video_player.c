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
#define MAX_AUDIO_QUEUE_SIZE (5 * AUDIO_BUFFER_SIZE) // 5 секунд аудио

typedef struct {
    uint8_t *data;
    int size;
    int write_pos;
    int read_pos;
    int available;
    SDL_mutex *mutex;
    SDL_cond *cond;
} AudioBuffer;

typedef struct {
    AVFormatContext *fmt_ctx;
    AVCodecContext *video_codec_ctx;
    AVCodecContext *audio_codec_ctx;
    SwrContext *swr_ctx;
    struct SwsContext *sws_ctx;  // Исправлено: struct SwsContext
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_AudioDeviceID audio_device;
    AudioBuffer audio_buf;
    uint8_t *video_buffer;
    AVFrame *frame;
    AVFrame *rgba_frame;
    AVFrame *audio_frame;
    int video_stream_index;
    int audio_stream_index;
    int audio_sample_rate;
    int audio_channels;
    bool audio_initialized;
} VideoPlayer;

static VideoPlayer g_player = {0};

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

// Очистка всех ресурсов видеоплеера
static void cleanup_video_player()
{
    printf("[Video] Cleaning up video player resources...\n");
    
    // Остановка и закрытие аудио устройства
    if (g_player.audio_initialized && g_player.audio_device) {
        SDL_PauseAudioDevice(g_player.audio_device, 1);
        SDL_Delay(100); // Даем время на завершение коллбэков
        SDL_CloseAudioDevice(g_player.audio_device);
        g_player.audio_device = 0;
        g_player.audio_initialized = false;
    }
    
    // Очистка аудио буфера
    cleanup_audio_buffer(&g_player.audio_buf);
    
    // Освобождение видео буфера
    if (g_player.video_buffer) {
        av_free(g_player.video_buffer);
        g_player.video_buffer = NULL;
    }
    
    // Освобождение кадров
    if (g_player.audio_frame) {
        av_frame_free(&g_player.audio_frame);
        g_player.audio_frame = NULL;
    }
    
    if (g_player.rgba_frame) {
        av_frame_free(&g_player.rgba_frame);
        g_player.rgba_frame = NULL;
    }
    
    if (g_player.frame) {
        av_frame_free(&g_player.frame);
        g_player.frame = NULL;
    }
    
    // Освобождение контекстов преобразования
    if (g_player.sws_ctx) {
        sws_freeContext(g_player.sws_ctx);
        g_player.sws_ctx = NULL;
    }
    
    if (g_player.swr_ctx) {
        swr_free(&g_player.swr_ctx);
        g_player.swr_ctx = NULL;
    }
    
    // Освобождение SDL ресурсов
    if (g_player.texture) {
        SDL_DestroyTexture(g_player.texture);
        g_player.texture = NULL;
    }
    
    if (g_player.renderer) {
        SDL_DestroyRenderer(g_player.renderer);
        g_player.renderer = NULL;
    }
    
    if (g_player.window) {
        SDL_DestroyWindow(g_player.window);
        g_player.window = NULL;
    }
    
    // Освобождение контекстов кодеков
    if (g_player.audio_codec_ctx) {
        avcodec_free_context(&g_player.audio_codec_ctx);
        g_player.audio_codec_ctx = NULL;
    }
    
    if (g_player.video_codec_ctx) {
        avcodec_free_context(&g_player.video_codec_ctx);
        g_player.video_codec_ctx = NULL;
    }
    
    // Освобождение контекста формата
    if (g_player.fmt_ctx) {
        avformat_close_input(&g_player.fmt_ctx);
        g_player.fmt_ctx = NULL;
    }
    
    // Завершение SDL
    SDL_Quit();
    
    // Обнуление всех полей
    memset(&g_player, 0, sizeof(VideoPlayer));
    
    printf("[Video] Cleanup complete\n");
}

// Функция для синхронизации видео по времени
static void sync_video_frame(AVStream *video_stream, AVFrame *frame, int64_t *start_time, double *frame_timer)
{
    if (frame->pts == AV_NOPTS_VALUE) {
        frame->pts = frame->pkt_dts;
    }
    
    double pts = frame->pts * av_q2d(video_stream->time_base);
    double current_time = av_gettime() / 1000000.0 - *start_time;
    double delay = pts - *frame_timer;
    
    // Обновляем таймер кадра
    *frame_timer = pts;
    
    // Если мы отстаем, не ждем
    if (delay <= 0 || delay > 1.0) {
        delay = 0.01; // Минимальная задержка
    }
    
    // Вычисляем сколько нужно ждать
    double diff = pts - current_time;
    
    if (diff > 0) {
        // Ждем нужное время
        av_usleep((int64_t)(diff * 1000000));
    } else if (diff < -0.1) {
        // Мы отстаем больше чем на 0.1 секунды - сбрасываем таймер
        *frame_timer = current_time;
    }
}

void play_video_file_delay(const char *path, int skip_enabled, float delay_seconds)
{
    printf("[Video] Starting: %s (delay: %.1fs, skip: %s)\n", 
           path, delay_seconds, skip_enabled ? "enabled" : "disabled");
    
    // Обнуляем структуру плеера
    memset(&g_player, 0, sizeof(VideoPlayer));
    
    // Инициализируем аудио буфер
    if (!init_audio_buffer(&g_player.audio_buf, AUDIO_BUFFER_SIZE)) {
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
        cleanup_audio_buffer(&g_player.audio_buf);
        return;
    }
    
    // Пробуем открыть файл
    if (avformat_open_input(&g_player.fmt_ctx, path, NULL, NULL) < 0) {
        printf("[Video] Cannot open file: %s\n", path);
        cleanup_audio_buffer(&g_player.audio_buf);
        SDL_Quit();
        return;
    }
    
    if (avformat_find_stream_info(g_player.fmt_ctx, NULL) < 0) {
        printf("[Video] Could not find stream info\n");
        cleanup_audio_buffer(&g_player.audio_buf);
        SDL_Quit();
        avformat_close_input(&g_player.fmt_ctx);
        return;
    }
    
    // Находим видеопоток и аудиопоток
    g_player.video_stream_index = -1;
    g_player.audio_stream_index = -1;
    
    for (unsigned int i = 0; i < g_player.fmt_ctx->nb_streams; i++) {
        if (g_player.fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && g_player.video_stream_index == -1) {
            g_player.video_stream_index = i;
        } else if (g_player.fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && g_player.audio_stream_index == -1) {
            g_player.audio_stream_index = i;
        }
    }
    
    if (g_player.video_stream_index == -1) {
        printf("[Video] No video stream found\n");
        cleanup_audio_buffer(&g_player.audio_buf);
        SDL_Quit();
        avformat_close_input(&g_player.fmt_ctx);
        return;
    }
    
    // Получаем информацию о потоках
    AVStream *video_stream = g_player.fmt_ctx->streams[g_player.video_stream_index];
    AVStream *audio_stream = g_player.audio_stream_index != -1 ? 
                            g_player.fmt_ctx->streams[g_player.audio_stream_index] : NULL;
    
    // Видео декодер
    const AVCodec *video_codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!video_codec) {
        printf("[Video] Unsupported video codec\n");
        cleanup_video_player();
        return;
    }
    
    g_player.video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!g_player.video_codec_ctx) {
        printf("[Video] Could not allocate video codec context\n");
        cleanup_video_player();
        return;
    }
    
    if (avcodec_parameters_to_context(g_player.video_codec_ctx, video_stream->codecpar) < 0) {
        printf("[Video] Could not copy video codec parameters\n");
        cleanup_video_player();
        return;
    }
    
    if (avcodec_open2(g_player.video_codec_ctx, video_codec, NULL) < 0) {
        printf("[Video] Could not open video codec\n");
        cleanup_video_player();
        return;
    }
    
    // Аудио декодер
    g_player.audio_sample_rate = 48000;
    g_player.audio_channels = 2;
    
    if (g_player.audio_stream_index != -1) {
        const AVCodec *audio_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
        if (audio_codec) {
            g_player.audio_codec_ctx = avcodec_alloc_context3(audio_codec);
            if (g_player.audio_codec_ctx) {
                if (avcodec_parameters_to_context(g_player.audio_codec_ctx, audio_stream->codecpar) >= 0) {
                    if (avcodec_open2(g_player.audio_codec_ctx, audio_codec, NULL) >= 0) {
                        // Инициализируем ресемплер
                        g_player.swr_ctx = swr_alloc();
                        if (g_player.swr_ctx) {
                            av_opt_set_chlayout(g_player.swr_ctx, "in_chlayout", &g_player.audio_codec_ctx->ch_layout, 0);
                            av_opt_set_int(g_player.swr_ctx, "in_sample_rate", g_player.audio_codec_ctx->sample_rate, 0);
                            av_opt_set_sample_fmt(g_player.swr_ctx, "in_sample_fmt", g_player.audio_codec_ctx->sample_fmt, 0);
                            
                            // Выходной формат: стерео, 48kHz, S16
                            AVChannelLayout out_chlayout;
                            av_channel_layout_default(&out_chlayout, g_player.audio_channels);
                            
                            av_opt_set_chlayout(g_player.swr_ctx, "out_chlayout", &out_chlayout, 0);
                            av_opt_set_int(g_player.swr_ctx, "out_sample_rate", g_player.audio_sample_rate, 0);
                            av_opt_set_sample_fmt(g_player.swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                            
                            if (swr_init(g_player.swr_ctx) < 0) {
                                swr_free(&g_player.swr_ctx);
                                g_player.swr_ctx = NULL;
                            }
                            av_channel_layout_uninit(&out_chlayout);
                        }
                    } else {
                        avcodec_free_context(&g_player.audio_codec_ctx);
                        g_player.audio_codec_ctx = NULL;
                    }
                } else {
                    avcodec_free_context(&g_player.audio_codec_ctx);
                    g_player.audio_codec_ctx = NULL;
                }
            }
        }
    }
    
    // Создаем окно SDL
    g_player.window = SDL_CreateWindow("Video", 
                                     SDL_WINDOWPOS_CENTERED, 
                                     SDL_WINDOWPOS_CENTERED, 
                                     g_player.video_codec_ctx->width, 
                                     g_player.video_codec_ctx->height, 
                                     0);
    if (!g_player.window) {
        printf("[Video] Could not create window: %s\n", SDL_GetError());
        cleanup_video_player();
        return;
    }
    
    g_player.renderer = SDL_CreateRenderer(g_player.window, -1, 
                                           SDL_RENDERER_ACCELERATED | 
                                           SDL_RENDERER_PRESENTVSYNC);
    if (!g_player.renderer) {
        printf("[Video] Could not create renderer: %s\n", SDL_GetError());
        cleanup_video_player();
        return;
    }
    
    g_player.texture = SDL_CreateTexture(g_player.renderer,
                                        SDL_PIXELFORMAT_RGBA32,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        g_player.video_codec_ctx->width,
                                        g_player.video_codec_ctx->height);
    if (!g_player.texture) {
        printf("[Video] Could not create texture: %s\n", SDL_GetError());
        cleanup_video_player();
        return;
    }
    
    // Контекст для преобразования цвета
    g_player.sws_ctx = sws_getContext(
        g_player.video_codec_ctx->width,
        g_player.video_codec_ctx->height,
        g_player.video_codec_ctx->pix_fmt,
        g_player.video_codec_ctx->width,
        g_player.video_codec_ctx->height,
        AV_PIX_FMT_RGBA,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
    
    if (!g_player.sws_ctx) {
        printf("[Video] Could not create sws context\n");
        cleanup_video_player();
        return;
    }
    
    // Подготавливаем кадры
    g_player.frame = av_frame_alloc();
    g_player.rgba_frame = av_frame_alloc();
    g_player.audio_frame = av_frame_alloc();
    if (!g_player.frame || !g_player.rgba_frame || !g_player.audio_frame) {
        printf("[Video] Could not allocate frames\n");
        cleanup_video_player();
        return;
    }
    
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, 
                                            g_player.video_codec_ctx->width, 
                                            g_player.video_codec_ctx->height, 
                                            1);
    g_player.video_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
    if (!g_player.video_buffer) {
        printf("[Video] Could not allocate buffer\n");
        cleanup_video_player();
        return;
    }
    
    av_image_fill_arrays(g_player.rgba_frame->data, g_player.rgba_frame->linesize, 
                        g_player.video_buffer, AV_PIX_FMT_RGBA, 
                        g_player.video_codec_ctx->width, 
                        g_player.video_codec_ctx->height, 1);
    
    // Настраиваем аудио в SDL
    SDL_AudioSpec wanted_spec, obtained_spec;
    SDL_zero(wanted_spec);
    wanted_spec.freq = g_player.audio_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = g_player.audio_channels;
    wanted_spec.samples = 1024;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = &g_player.audio_buf;
    
    if (g_player.audio_codec_ctx && g_player.swr_ctx) {
        g_player.audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &obtained_spec, 0);
        if (g_player.audio_device > 0) {
            g_player.audio_initialized = true;
            SDL_PauseAudioDevice(g_player.audio_device, 0);
        } else {
            printf("[Video] Could not open audio device: %s\n", SDL_GetError());
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
            SDL_SetRenderDrawColor(g_player.renderer, 0, 0, 0, 255);
            SDL_RenderClear(g_player.renderer);
            SDL_RenderPresent(g_player.renderer);
            
            // Ждем немного
            SDL_Delay(16);
            
            // Обновляем прошедшее время
            elapsed_delay = (SDL_GetTicks() - start_delay) / 1000.0f;
        }
    }
    
    // Основной цикл воспроизведения
    AVPacket packet;
    bool quit = false;
    float hold_time = 0.0f;
    Uint32 last_button_check = SDL_GetTicks();
    
    // Для синхронизации видео
    int64_t start_time = av_gettime();
    double frame_timer = 0.0;
    AVRational time_base = video_stream->time_base;
    double fps = av_q2d(video_stream->avg_frame_rate);
    if (fps <= 0) fps = 30.0;
    double frame_duration = 1.0 / fps;
    
    printf("[Video] Video info: %dx%d, fps: %.2f, duration: %.2fs\n", 
           g_player.video_codec_ctx->width, g_player.video_codec_ctx->height,
           fps, g_player.fmt_ctx->duration / (double)AV_TIME_BASE);
    
    while (!quit && av_read_frame(g_player.fmt_ctx, &packet) >= 0) {
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
                break;
            }
        }
        
        if (quit) {
            av_packet_unref(&packet);
            break;
        }
        
        // Видео пакет
        if (packet.stream_index == g_player.video_stream_index) {
            if (avcodec_send_packet(g_player.video_codec_ctx, &packet) < 0) {
                break;
            }
            
            while (avcodec_receive_frame(g_player.video_codec_ctx, g_player.frame) == 0) {
                // Синхронизация видео
                sync_video_frame(video_stream, g_player.frame, &start_time, &frame_timer);
                
                // Конвертируем кадр в RGBA
                sws_scale(g_player.sws_ctx, (uint8_t const * const *)g_player.frame->data,
                         g_player.frame->linesize, 0, g_player.video_codec_ctx->height,
                         g_player.rgba_frame->data, g_player.rgba_frame->linesize);
                
                // Обновляем текстуру
                SDL_UpdateTexture(g_player.texture, NULL, g_player.rgba_frame->data[0], 
                                g_player.rgba_frame->linesize[0]);
                
                // Отрисовываем
                SDL_RenderClear(g_player.renderer);
                SDL_RenderCopy(g_player.renderer, g_player.texture, NULL, NULL);
                
                // Плавный индикатор пропуска
                if (hold_time > 0.0f) {
                    float progress = fminf(hold_time / SKIP_HOLD_TIME, 1.0f);
                    draw_circle_progress(g_player.renderer, 
                                       g_player.video_codec_ctx->width - 40,
                                       g_player.video_codec_ctx->height - 40,
                                       25, 6, progress);
                }
                
                SDL_RenderPresent(g_player.renderer);
            }
        }
        
        // Аудио пакет
        if (packet.stream_index == g_player.audio_stream_index && g_player.audio_codec_ctx && g_player.swr_ctx) {
            if (avcodec_send_packet(g_player.audio_codec_ctx, &packet) < 0) {
                break;
            }
            
            while (avcodec_receive_frame(g_player.audio_codec_ctx, g_player.audio_frame) == 0) {
                // Вычисляем необходимое количество сэмплов на выходе
                int out_samples = av_rescale_rnd(swr_get_delay(g_player.swr_ctx, g_player.audio_frame->sample_rate) + 
                                                g_player.audio_frame->nb_samples,
                                                g_player.audio_sample_rate, g_player.audio_frame->sample_rate, AV_ROUND_UP);
                
                // Выделяем буфер для конвертированного аудио
                uint8_t *converted_audio = NULL;
                int out_linesize;
                int out_count = av_samples_alloc(&converted_audio, &out_linesize, g_player.audio_channels,
                                                out_samples, AV_SAMPLE_FMT_S16, 1);
                
                if (out_count >= 0) {
                    // Конвертируем
                    int converted = swr_convert(g_player.swr_ctx, &converted_audio, out_samples,
                                              (const uint8_t**)g_player.audio_frame->data, g_player.audio_frame->nb_samples);
                    
                    if (converted > 0) {
                        // Записываем в кольцевой буфер
                        int actual_size = av_samples_get_buffer_size(&out_linesize, g_player.audio_channels,
                                                                    converted, AV_SAMPLE_FMT_S16, 1);
                        write_audio_data(&g_player.audio_buf, converted_audio, actual_size);
                    }
                    
                    av_freep(&converted_audio);
                }
            }
        }
        
        av_packet_unref(&packet);
    }
    
    // Очистка экрана черным цветом после завершения видео
    if (g_player.renderer) {
        SDL_SetRenderDrawColor(g_player.renderer, 0, 0, 0, 255);
        SDL_RenderClear(g_player.renderer);
        SDL_RenderPresent(g_player.renderer);
        SDL_Delay(100); // Даем время на отрисовку
    }
    
    // Освобождаем все ресурсы
    cleanup_video_player();
    
    printf("[Video] Finished\n");
}

// Совместимость со старой функцией
void play_video_file(const char *path, int skip_enabled)
{
    play_video_file_delay(path, skip_enabled, DEFAULT_DELAY_SECONDS);
}
