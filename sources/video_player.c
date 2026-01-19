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
#include <fcntl.h>
#include <unistd.h>

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

// Функция для проверки существования файла
static bool file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

// Функция для получения размера файла
static long get_file_size(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return size;
}

// Альтернативный способ открытия файла через файловый дескриптор
static AVFormatContext* try_open_via_fd(const char *path) {
    printf("[Video] Trying to open via file descriptor: %s\n", path);
    
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("[Video] Failed to open file descriptor: errno=%d\n", errno);
        return NULL;
    }
    
    // Получаем размер файла
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    
    printf("[Video] File size via fd: %ld bytes\n", (long)size);
    
    // Пробуем открыть через avformat
    AVFormatContext *fmt_ctx = NULL;
    char fd_path[256];
    snprintf(fd_path, sizeof(fd_path), "pipe:%d", fd);
    
    AVDictionary *options = NULL;
    av_dict_set(&options, "rw_timeout", "5000000", 0); // 5 секунд timeout
    
    int ret = avformat_open_input(&fmt_ctx, fd_path, NULL, &options);
    av_dict_free(&options);
    
    if (ret < 0) {
        printf("[Video] avformat_open_input via fd failed: %d\n", ret);
        close(fd);
        return NULL;
    }
    
    printf("[Video] Successfully opened via fd\n");
    return fmt_ctx;
}

void play_video_file(const char *path, int skip_enabled)
{
    printf("[Video] ==========================================\n");
    printf("[Video] STARTING VIDEO PLAYBACK\n");
    printf("[Video] Input path: %s\n", path);
    printf("[Video] Skip enabled: %d\n", skip_enabled);
    printf("[Video] ==========================================\n");
    
    // Сначала проверяем существование файла
    printf("[Video] Checking if file exists...\n");
    
    // Пробуем разные варианты пути
    const char *paths_to_try[] = {
        path,  // Оригинальный путь
        NULL
    };
    
    char alt_path1[256], alt_path2[256], alt_path3[256];
    
    // Если путь начинается с sdmc:/, пробуем без префикса
    if (strncmp(path, "sdmc:/", 6) == 0) {
        snprintf(alt_path1, sizeof(alt_path1), "/%s", path + 6);
        paths_to_try[1] = alt_path1;
        printf("[Video] Will try alternative path 1: %s\n", alt_path1);
        
        // Также пробуем с file:// префиксом
        snprintf(alt_path2, sizeof(alt_path2), "file://%s", path + 6);
        paths_to_try[2] = alt_path2;
        printf("[Video] Will try alternative path 2: %s\n", alt_path2);
    }
    
    // Если путь начинается с romfs:/, пробуем без префикса
    if (strncmp(path, "romfs:/", 7) == 0) {
        snprintf(alt_path1, sizeof(alt_path1), "/%s", path + 7);
        paths_to_try[1] = alt_path1;
        printf("[Video] Will try alternative path 1: %s\n", alt_path1);
    }
    
    // Пробуем все пути
    const char *actual_path = NULL;
    for (int i = 0; i < 4; i++) {
        if (paths_to_try[i] == NULL) continue;
        
        printf("[Video] Trying path %d: %s\n", i, paths_to_try[i]);
        
        if (file_exists(paths_to_try[i])) {
            long size = get_file_size(paths_to_try[i]);
            printf("[Video] File exists! Size: %ld bytes\n", size);
            actual_path = paths_to_try[i];
            break;
        } else {
            printf("[Video] File does not exist or cannot be opened\n");
        }
    }
    
    if (!actual_path) {
        printf("[Video] ERROR: Cannot find file in any of the paths!\n");
        return;
    }
    
    printf("[Video] Using path: %s\n", actual_path);
    
    // Инициализируем контроллеры
    printf("[Video] Initializing controllers...\n");
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    
    // Инициализируем SDL
    printf("[Video] Initializing SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        printf("[Video] SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    printf("[Video] SDL initialized successfully\n");
    
    // Пробуем открыть файл через FFmpeg разными способами
    printf("[Video] Attempting to open video file with FFmpeg...\n");
    
    AVFormatContext *fmt_ctx = NULL;
    int ret = 0;
    
    // Способ 1: Прямое открытие
    printf("[Video] Method 1: Direct avformat_open_input\n");
    ret = avformat_open_input(&fmt_ctx, actual_path, NULL, NULL);
    if (ret < 0) {
        printf("[Video] Method 1 failed: %d\n", ret);
        
        // Способ 2: Через файловый дескриптор
        printf("[Video] Method 2: Via file descriptor\n");
        fmt_ctx = try_open_via_fd(actual_path);
        
        if (!fmt_ctx) {
            printf("[Video] Method 2 failed\n");
            
            // Способ 3: Пробуем с разными опциями
            printf("[Video] Method 3: With timeout options\n");
            AVDictionary *options = NULL;
            av_dict_set(&options, "timeout", "5000000", 0); // 5 секунд timeout
            av_dict_set(&options, "rw_timeout", "5000000", 0);
            
            ret = avformat_open_input(&fmt_ctx, actual_path, NULL, &options);
            av_dict_free(&options);
            
            if (ret < 0) {
                printf("[Video] Method 3 failed: %d\n", ret);
                SDL_Quit();
                return;
            }
        }
    }
    
    printf("[Video] Successfully opened file with FFmpeg!\n");
    
    // Получаем информацию о потоках
    printf("[Video] Getting stream info...\n");
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("[Video] Could not find stream information\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    // Выводим информацию о файле
    printf("[Video] File info:\n");
    printf("[Video]   Duration: %ld seconds\n", fmt_ctx->duration / AV_TIME_BASE);
    printf("[Video]   Bitrate: %ld bps\n", fmt_ctx->bit_rate);
    printf("[Video]   Number of streams: %d\n", fmt_ctx->nb_streams);
    
    // Находим видеопоток и аудиопоток
    int video_stream_index = -1;
    int audio_stream_index = -1;
    
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVCodecParameters *codecpar = fmt_ctx->streams[i]->codecpar;
        
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
            video_stream_index = i;
            printf("[Video]   Video stream %d: codec=%d, width=%d, height=%d\n", 
                   i, codecpar->codec_id, codecpar->width, codecpar->height);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1) {
            audio_stream_index = i;
            printf("[Video]   Audio stream %d: codec=%d, channels=%d, sample_rate=%d\n", 
                   i, codecpar->codec_id, codecpar->channels, codecpar->sample_rate);
        }
    }
    
    if (video_stream_index == -1) {
        printf("[Video] ERROR: No video stream found\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    // Получаем параметры кодека видеопотока
    AVCodecParameters *video_codec_params = fmt_ctx->streams[video_stream_index]->codecpar;
    const AVCodec *video_codec = avcodec_find_decoder(video_codec_params->codec_id);
    
    if (!video_codec) {
        printf("[Video] ERROR: Unsupported video codec: %d\n", video_codec_params->codec_id);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    printf("[Video] Found video codec: %s\n", video_codec->name);
    
    AVCodecContext *video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!video_codec_ctx) {
        printf("[Video] ERROR: Could not allocate video codec context\n");
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    if (avcodec_parameters_to_context(video_codec_ctx, video_codec_params) < 0) {
        printf("[Video] ERROR: Could not copy video codec parameters\n");
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
        printf("[Video] ERROR: Could not open video codec\n");
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    printf("[Video] Video codec opened successfully\n");
    printf("[Video] Video details: %dx%d, pix_fmt=%d, time_base=%d/%d\n", 
           video_codec_ctx->width, video_codec_ctx->height, 
           video_codec_ctx->pix_fmt,
           video_codec_ctx->time_base.num, video_codec_ctx->time_base.den);
    
    // Создаем окно SDL
    printf("[Video] Creating SDL window (%dx%d)...\n", 
           video_codec_ctx->width, video_codec_ctx->height);
    
    SDL_Window *window = SDL_CreateWindow("Video", 
                                         SDL_WINDOWPOS_CENTERED, 
                                         SDL_WINDOWPOS_CENTERED, 
                                         video_codec_ctx->width, 
                                         video_codec_ctx->height, 
                                         SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!window) {
        printf("[Video] WARNING: Could not create window: %s\n", SDL_GetError());
        printf("[Video] Trying without fullscreen...\n");
        
        window = SDL_CreateWindow("Video", 
                                 SDL_WINDOWPOS_CENTERED, 
                                 SDL_WINDOWPOS_CENTERED, 
                                 video_codec_ctx->width, 
                                 video_codec_ctx->height, 
                                 0);
        if (!window) {
            printf("[Video] ERROR: Could not create window: %s\n", SDL_GetError());
            avcodec_free_context(&video_codec_ctx);
            avformat_close_input(&fmt_ctx);
            SDL_Quit();
            return;
        }
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 
                                               SDL_RENDERER_ACCELERATED | 
                                               SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("[Video] ERROR: Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    // Устанавливаем логический размер (для масштабирования)
    SDL_RenderSetLogicalSize(renderer, video_codec_ctx->width, video_codec_ctx->height);
    
    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                            SDL_PIXELFORMAT_RGBA32,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            video_codec_ctx->width,
                                            video_codec_ctx->height);
    if (!texture) {
        printf("[Video] ERROR: Could not create texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    printf("[Video] SDL graphics initialized successfully\n");
    
    // Подготавливаем контекст для преобразования изображения
    printf("[Video] Creating sws context...\n");
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
        printf("[Video] ERROR: Could not create sws context\n");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        avcodec_free_context(&video_codec_ctx);
        avformat_close_input(&fmt_ctx);
        SDL_Quit();
        return;
    }
    
    // Выделяем память для преобразованного кадра
    printf("[Video] Allocating frames...\n");
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgba_frame = av_frame_alloc();
    if (!frame || !rgba_frame) {
        printf("[Video] ERROR: Could not allocate frames\n");
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
    printf("[Video] RGBA buffer size: %d bytes\n", num_bytes);
    
    uint8_t *buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
    
    av_image_fill_arrays(rgba_frame->data, rgba_frame->linesize, buffer,
                        AV_PIX_FMT_RGBA, video_codec_ctx->width, 
                        video_codec_ctx->height, 1);
    
    // Аудио инициализация
    printf("[Video] Initializing audio...\n");
    SDL_AudioSpec wanted_spec, obtained_spec;
    SDL_zero(wanted_spec);
    wanted_spec.freq = 48000;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 2;
    wanted_spec.samples = 1024;
    wanted_spec.callback = audio_callback;
    
    if (SDL_OpenAudio(&wanted_spec, &obtained_spec) < 0) {
        printf("[Video] WARNING: Could not open audio: %s\n", SDL_GetError());
        printf("[Video] Continuing without audio\n");
    } else {
        printf("[Video] Audio opened: %dHz, %d channels, %d samples\n",
               obtained_spec.freq, obtained_spec.channels, obtained_spec.samples);
        SDL_PauseAudio(0);
    }
    
    // Основной цикл воспроизведения
    printf("[Video] Starting main playback loop...\n");
    
    AVPacket packet;
    bool quit = false;
    double hold_time = 0.0;
    clock_t last_time = clock();
    int frame_count = 0;
    
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
                printf("[Video] Skip triggered after %.2f seconds\n", hold_time);
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
                printf("[Video] Error sending video packet\n");
                break;
            }
            
            while (avcodec_receive_frame(video_codec_ctx, frame) == 0) {
                frame_count++;
                
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
                SDL_Delay(1);
                
                // Логируем каждые 30 кадров
                if (frame_count % 30 == 0) {
                    printf("[Video] Processed %d frames\n", frame_count);
                }
            }
        }
        
        av_packet_unref(&packet);
    }
    
    // Очистка
    printf("[Video] ==========================================\n");
    printf("[Video] PLAYBACK FINISHED\n");
    printf("[Video] Total frames processed: %d\n", frame_count);
    printf("[Video] Cleaning up...\n");
    
    // Очистка аудио буфера
    if (audio_buffer) {
        free(audio_buffer);
        audio_buffer = NULL;
        audio_buffer_size = 0;
        audio_buffer_pos = 0;
    }
    
    SDL_CloseAudio();
    
    // Очистка FFmpeg ресурсов
    av_free(buffer);
    av_frame_free(&rgba_frame);
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&video_codec_ctx);
    avformat_close_input(&fmt_ctx);
    
    // Очистка SDL ресурсов
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("[Video] Cleanup complete\n");
    printf("[Video] ==========================================\n");
}
