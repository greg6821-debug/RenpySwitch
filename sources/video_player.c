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
#define AUDIO_BUFFER_SIZE (48000 * 2 * 2)
#define MAX_PATH_LENGTH 512

// Статические переменные для хранения состояния SDL
static bool sdl_initialized = false;
static bool sdl_video_initialized = false;
static bool sdl_audio_initialized = false;
static bool window_created_by_us = false;
static bool renderer_created_by_us = false;
static SDL_Window *global_window = NULL;
static SDL_Renderer *global_renderer = NULL;

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
    uint8_t *data;
    size_t size;
    size_t pos;
} MemoryFile;

typedef struct {
    SDL_Texture *texture;
    SDL_AudioDeviceID audio_device;
    AudioBuffer audio_buf;
    
    AVFormatContext *fmt_ctx;
    AVCodecContext *video_codec_ctx;
    AVCodecContext *audio_codec_ctx;
    SwrContext *swr_ctx;
    struct SwsContext *sws_ctx;
    
    MemoryFile *memory_file;
    AVIOContext *avio_ctx;
    
    bool audio_initialized;
    bool resources_allocated;
    int video_width;
    int video_height;
} VideoState;

// Инициализация SDL (один раз)
void video_player_init()
{
    if (sdl_initialized) return;
    
    printf("[Video] Initializing SDL subsystem\n");
    
    // Инициализируем только аудио, если видео уже было инициализировано Ren'Py
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        printf("[Video] SDL_InitSubSystem audio failed: %s\n", SDL_GetError());
        return;
    }
    sdl_audio_initialized = true;
    
																					  
												
						  
																 
		
																				 
							 
													   
														   
													  
		 
		
											
											   
							
																  
										 
		 
	 
	
    // Не инициализируем видео здесь - это сделает Ren'Py
    sdl_initialized = true;
    printf("[Video] SDL subsystem initialized (audio only)\n");
}

// Деинициализация SDL
void video_player_quit()
{
    printf("[Video] Shutting down video player\n");
    
    // Если рендерер был создан нами, уничтожаем его
    if (renderer_created_by_us && global_renderer) {
        printf("[Video] Destroying our renderer\n");
        SDL_DestroyRenderer(global_renderer);
        global_renderer = NULL;
        renderer_created_by_us = false;
    }
    
    // Если окно было создано нами, уничтожаем его
    if (window_created_by_us && global_window) {
        printf("[Video] Destroying our window\n");
        SDL_DestroyWindow(global_window);
        global_window = NULL;
        window_created_by_us = false;
        
        // Если мы создавали окно, значит мы инициализировали видео подсистему
        if (sdl_video_initialized) {
            printf("[Video] Quitting SDL video subsystem\n");
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            sdl_video_initialized = false;
        }
    }
    
    // Если аудио инициализировано нами, завершаем его
    if (sdl_audio_initialized) {
        printf("[Video] Quitting SDL audio subsystem\n");
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        sdl_audio_initialized = false;
    }
    
    // Сбрасываем флаги
    sdl_initialized = false;
								  
    
    printf("[Video] Video player shutdown complete\n");
}

void audio_callback(void *userdata, Uint8 *stream, int len)
{
    AudioBuffer *buf = (AudioBuffer*)userdata;
    if (!buf || !buf->mutex) return;
    
    SDL_LockMutex(buf->mutex);
    
    if (buf->available == 0) {
        memset(stream, 0, len);
        SDL_UnlockMutex(buf->mutex);
        return;
    }
    
    int to_copy = len;
    if (to_copy > buf->available)
        to_copy = buf->available;
    
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
    
    if (to_copy < len)
        memset(stream + to_copy, 0, len - to_copy);
    
    SDL_CondSignal(buf->cond);
    SDL_UnlockMutex(buf->mutex);
}

void draw_circle_progress(SDL_Renderer *ren, int cx, int cy, int radius, int thickness, float progress)
{
    if (progress <= 0.0f) return;
    
    int segments = (int)(360 * progress);
    float angle_step = 2.0f * M_PI / 360.0f;
    float start_angle = -M_PI / 2;
    
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    
    for (int i = 0; i < segments; i++) {
        float angle = start_angle + i * angle_step;
        
        for (int t = 0; t < thickness; t++) {
            float current_radius = radius - thickness/2 + t;
            int x = cx + (int)(cosf(angle) * current_radius);
            int y = cy + (int)(sinf(angle) * current_radius);
            SDL_RenderDrawPoint(ren, x, y);
        }
    }
}

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

static void cleanup_audio_buffer(AudioBuffer *buf)
{
    if (!buf) return;
    
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

static bool write_audio_data(AudioBuffer *buf, const uint8_t *data, int size)
{
    if (!buf || !buf->mutex) return false;
    
    SDL_LockMutex(buf->mutex);
    
    while (buf->available + size > buf->size) {
        SDL_CondWait(buf->cond, buf->mutex);
    }
    
    int free_space = buf->size - buf->available;
    if (free_space >= size) {
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

static void clear_screen(SDL_Renderer *renderer)
{
    if (!renderer) return;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    MemoryFile *mf = (MemoryFile*)opaque;
    
    if (mf->pos >= mf->size)
        return AVERROR_EOF;
    
    int to_read = buf_size;
    if (mf->pos + to_read > mf->size)
        to_read = mf->size - mf->pos;
    
    memcpy(buf, mf->data + mf->pos, to_read);
    mf->pos += to_read;
    
    return to_read;
}

static int64_t seek_packet(void *opaque, int64_t offset, int whence)
{
    MemoryFile *mf = (MemoryFile*)opaque;
    
    switch (whence) {
        case SEEK_SET:
            mf->pos = offset;
            break;
        case SEEK_CUR:
            mf->pos += offset;
            break;
        case SEEK_END:
            mf->pos = mf->size + offset;
            break;
        default:
            return -1;
    }
    
    if (mf->pos < 0) mf->pos = 0;
    if (mf->pos > mf->size) mf->pos = mf->size;
    
    return mf->pos;
}

static MemoryFile* load_file_to_memory(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        printf("[Video] Cannot open file for reading: %s\n", path);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        printf("[Video] Invalid file size: %ld\n", file_size);
        fclose(file);
        return NULL;
    }
    
    MemoryFile *mf = (MemoryFile*)malloc(sizeof(MemoryFile));
    if (!mf) {
        printf("[Video] Failed to allocate MemoryFile\n");
        fclose(file);
        return NULL;
    }
    
    mf->data = (uint8_t*)malloc(file_size);
    if (!mf->data) {
        printf("[Video] Failed to allocate memory for file data\n");
        free(mf);
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(mf->data, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != file_size) {
        printf("[Video] Failed to read entire file: %zu of %ld bytes\n", bytes_read, file_size);
        free(mf->data);
        free(mf);
        return NULL;
    }
    
    mf->size = file_size;
    mf->pos = 0;
    
    printf("[Video] Loaded file into memory: %s (%ld bytes)\n", path, file_size);
    return mf;
}

static void free_memory_file(MemoryFile *mf)
{
    if (mf) {
        if (mf->data) {
            free(mf->data);
            mf->data = NULL;
        }
        free(mf);
    }
}

static const char* resolve_switch_path(const char *input_path, char *resolved_path, size_t buffer_size)
{
    const char *test_paths[5] = {NULL};
    char alt_path1[MAX_PATH_LENGTH] = {0};
    char alt_path2[MAX_PATH_LENGTH] = {0};
    char alt_path3[MAX_PATH_LENGTH] = {0};
    
    test_paths[0] = input_path;
    
    if (strncmp(input_path, "romfs:", 6) == 0) {
        snprintf(alt_path1, sizeof(alt_path1), "/%s", input_path + 6);
        test_paths[1] = alt_path1;
        
        snprintf(alt_path2, sizeof(alt_path2), "romfs:/Contents%s", input_path + 6);
        test_paths[2] = alt_path2;
        
        snprintf(alt_path3, sizeof(alt_path3), "romfs:%s", input_path + 6);
        test_paths[3] = alt_path3;
    }
    else if (strncmp(input_path, "sdmc:", 5) == 0) {
        snprintf(alt_path1, sizeof(alt_path1), "/%s", input_path + 5);
        test_paths[1] = alt_path1;
    }
    else if (input_path[0] != '/' && strncmp(input_path, "romfs:", 6) != 0 && strncmp(input_path, "sdmc:", 5) != 0) {
        snprintf(alt_path1, sizeof(alt_path1), "romfs:/Contents/game/video/%s", input_path);
        test_paths[1] = alt_path1;
        
        snprintf(alt_path2, sizeof(alt_path2), "romfs:/Contents/%s", input_path);
        test_paths[2] = alt_path2;
        
        snprintf(alt_path3, sizeof(alt_path3), "romfs:/%s", input_path);
        test_paths[3] = alt_path3;
    }
    
    for (int i = 0; i < 5; i++) {
        if (test_paths[i] == NULL) continue;
        
        printf("[Video] Testing path: %s\n", test_paths[i]);
        
        FILE *test = fopen(test_paths[i], "rb");
        if (test) {
            fclose(test);
            strncpy(resolved_path, test_paths[i], buffer_size - 1);
            resolved_path[buffer_size - 1] = '\0';
            printf("[Video] Using resolved path: %s\n", resolved_path);
            return resolved_path;
        }
    }
    
    strncpy(resolved_path, input_path, buffer_size - 1);
    resolved_path[buffer_size - 1] = '\0';
    return resolved_path;
}

static void cleanup_video_state(VideoState *state)
{
    if (!state) return;
    
    printf("[Video] Starting cleanup...\n");
    
    // Закрываем аудио устройство
    if (state->audio_initialized && state->audio_device > 0) {
        printf("[Video] Closing audio device\n");
        SDL_PauseAudioDevice(state->audio_device, 1);
        SDL_CloseAudioDevice(state->audio_device);
        state->audio_device = 0;
        state->audio_initialized = false;
    }
    
    // Освобождаем аудио буфер
    cleanup_audio_buffer(&state->audio_buf);
    
    // Освобождаем текстуру (она всегда создается нами)
    if (state->texture) {
        printf("[Video] Destroying texture\n");
        SDL_DestroyTexture(state->texture);
        state->texture = NULL;
    }
    
    // Освобождаем FFmpeg ресурсы
    if (state->sws_ctx) {
        printf("[Video] Freeing sws context\n");
        sws_freeContext(state->sws_ctx);
        state->sws_ctx = NULL;
    }
    
    if (state->swr_ctx) {
        printf("[Video] Freeing swr context\n");
        swr_free(&state->swr_ctx);
        state->swr_ctx = NULL;
    }
    
    if (state->audio_codec_ctx) {
        printf("[Video] Freeing audio codec context\n");
        avcodec_free_context(&state->audio_codec_ctx);
        state->audio_codec_ctx = NULL;
    }
    
    if (state->video_codec_ctx) {
        printf("[Video] Freeing video codec context\n");
        avcodec_free_context(&state->video_codec_ctx);
        state->video_codec_ctx = NULL;
    }
    
    if (state->fmt_ctx) {
        printf("[Video] Closing format context\n");
        avformat_close_input(&state->fmt_ctx);
        state->fmt_ctx = NULL;
    }
    
    // Освобождаем AVIO контекст
    if (state->avio_ctx) {
        printf("[Video] Freeing AVIO context\n");
        av_freep(&state->avio_ctx->buffer);
        avio_context_free(&state->avio_ctx);
        state->avio_ctx = NULL;
    }
    
    // Освобождаем файл в памяти
    if (state->memory_file) {
        printf("[Video] Freeing memory file\n");
        free_memory_file(state->memory_file);
        state->memory_file = NULL;
    }
    
    // Освобождаем состояние
    free(state);
    
    printf("[Video] Cleanup completed\n");
}

static VideoState* create_video_state()
{
    VideoState *state = (VideoState*)calloc(1, sizeof(VideoState));
    if (!state) {
        printf("[Video] Failed to allocate video state\n");
        return NULL;
    }
    
    state->resources_allocated = false;
    return state;
}

static bool create_or_reuse_window(int width, int height)
{
    // Если окно уже существует (от Ren'Py), используем его
    if (!global_window) {
        // Пробуем получить окно от Ren'Py
        SDL_Window *existing_window = NULL;
        
        // Проверяем все окна SDL
        int num_windows = SDL_GetNumVideoDisplays();
        for (int i = 0; i < num_windows; i++) {
            existing_window = SDL_GetWindowFromID(i + 1);
            if (existing_window) {
                printf("[Video] Found existing window from Ren'Py (ID: %d)\n", i + 1);
                global_window = existing_window;
                window_created_by_us = false;
                break;
            }
        }
        
        // Если не нашли существующего окна, создаем свое
        if (!global_window) {
            printf("[Video] Creating new SDL window\n");
            
            // Инициализируем видео подсистему
            if (!sdl_video_initialized) {
                if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
                    printf("[Video] SDL_InitSubSystem video failed: %s\n", SDL_GetError());
                    return false;
                }
                sdl_video_initialized = true;
            }
            
            global_window = SDL_CreateWindow("Video", 
                                           SDL_WINDOWPOS_CENTERED, 
                                           SDL_WINDOWPOS_CENTERED, 
                                           width, 
                                           height, 
                                           0);
            if (!global_window) {
                printf("[Video] Could not create window: %s\n", SDL_GetError());
                return false;
            }
            
            window_created_by_us = true;
            printf("[Video] Window created by us\n");
            
            // Скрываем курсор
            SDL_ShowCursor(SDL_DISABLE);
        } else {
            printf("[Video] Using existing window from Ren'Py\n");
																		 
															
																								 
        }
    } else {
        // Обновляем размер существующего окна
        SDL_SetWindowSize(global_window, width, height);
        SDL_SetWindowPosition(global_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    
    // Создаем или переиспользуем рендерер
    if (!global_renderer) {
        global_renderer = SDL_CreateRenderer(global_window, -1, 
                                           SDL_RENDERER_ACCELERATED | 
                                           SDL_RENDERER_PRESENTVSYNC);
        if (!global_renderer) {
            printf("[Video] Could not create renderer: %s\n", SDL_GetError());
            return false;
        }
        renderer_created_by_us = true;
        printf("[Video] Renderer created by us\n");
    }
    
    // Очищаем экран
    SDL_SetRenderDrawColor(global_renderer, 0, 0, 0, 255);
    SDL_RenderClear(global_renderer);
    SDL_RenderPresent(global_renderer);
    
    return true;
}

void play_video_file_delay(const char *input_path, int skip_enabled, float delay_seconds)
{
    char resolved_path[MAX_PATH_LENGTH] = {0};
    const char *path = resolve_switch_path(input_path, resolved_path, sizeof(resolved_path));
    
    printf("[Video] Starting: %s (original: %s)\n", path, input_path);
    printf("[Video] Delay: %.1fs, Skip: %s\n", delay_seconds, skip_enabled ? "enabled" : "disabled");
    
    // Инициализируем SDL при первом вызове
    if (!sdl_initialized) {
        video_player_init();
    }
    
    if (!sdl_audio_initialized) {
        printf("[Video] SDL audio not initialized\n");
        return;
    }
    
    // Создаем новое состояние
    VideoState *state = create_video_state();
    if (!state) {
        printf("[Video] Failed to create video state\n");
        return;
    }
    
    // Инициализируем контроллеры
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    
    // Загружаем файл в память (для romfs)
    if (strncmp(path, "romfs:", 6) == 0) {
        state->memory_file = load_file_to_memory(path);
        if (!state->memory_file) {
            printf("[Video] Failed to load file into memory\n");
            cleanup_video_state(state);
            return;
        }
        
        unsigned char *avio_buffer = (unsigned char*)av_malloc(4096);
        if (!avio_buffer) {
            printf("[Video] Failed to allocate AVIO buffer\n");
            cleanup_video_state(state);
            return;
        }
        
        state->avio_ctx = avio_alloc_context(avio_buffer, 4096, 0, state->memory_file, read_packet, NULL, seek_packet);
        if (!state->avio_ctx) {
            printf("[Video] Failed to create AVIO context\n");
            av_free(avio_buffer);
            cleanup_video_state(state);
            return;
        }
        
        state->fmt_ctx = avformat_alloc_context();
        if (!state->fmt_ctx) {
            printf("[Video] Failed to allocate format context\n");
            cleanup_video_state(state);
            return;
        }
        
        state->fmt_ctx->pb = state->avio_ctx;
        
        if (avformat_open_input(&state->fmt_ctx, NULL, NULL, NULL) < 0) {
            printf("[Video] Cannot open file from memory: %s\n", path);
            cleanup_video_state(state);
            return;
        }
    } else {
        // Обычный способ для SD карты
        if (avformat_open_input(&state->fmt_ctx, path, NULL, NULL) < 0) {
            printf("[Video] Cannot open file: %s\n", path);
            cleanup_video_state(state);
            return;
        }
    }
    
    if (avformat_find_stream_info(state->fmt_ctx, NULL) < 0) {
        printf("[Video] Could not find stream info\n");
        cleanup_video_state(state);
        return;
    }
    
    // Находим видеопоток и аудиопоток
    int video_stream_index = -1;
    int audio_stream_index = -1;
    
    for (unsigned int i = 0; i < state->fmt_ctx->nb_streams; i++) {
        if (state->fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
            video_stream_index = i;
        } else if (state->fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1) {
            audio_stream_index = i;
        }
    }
    
    if (video_stream_index == -1) {
        printf("[Video] No video stream found\n");
        cleanup_video_state(state);
        return;
    }
    
    // Видео декодер
    AVStream *video_stream = state->fmt_ctx->streams[video_stream_index];
    const AVCodec *video_codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!video_codec) {
        printf("[Video] Unsupported video codec\n");
        cleanup_video_state(state);
        return;
    }
    
    state->video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!state->video_codec_ctx) {
        printf("[Video] Could not allocate video codec context\n");
        cleanup_video_state(state);
        return;
    }
    
    if (avcodec_parameters_to_context(state->video_codec_ctx, video_stream->codecpar) < 0) {
        printf("[Video] Could not copy video codec parameters\n");
        cleanup_video_state(state);
        return;
    }
    
    if (avcodec_open2(state->video_codec_ctx, video_codec, NULL) < 0) {
        printf("[Video] Could not open video codec\n");
        cleanup_video_state(state);
        return;
    }
    
    state->video_width = state->video_codec_ctx->width;
    state->video_height = state->video_codec_ctx->height;
    
    // Аудио декодер
    if (audio_stream_index != -1) {
        AVStream *audio_stream = state->fmt_ctx->streams[audio_stream_index];
        const AVCodec *audio_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
        if (audio_codec) {
            state->audio_codec_ctx = avcodec_alloc_context3(audio_codec);
            if (state->audio_codec_ctx) {
                if (avcodec_parameters_to_context(state->audio_codec_ctx, audio_stream->codecpar) >= 0) {
                    if (avcodec_open2(state->audio_codec_ctx, audio_codec, NULL) >= 0) {
                        // Инициализируем ресемплер
                        state->swr_ctx = swr_alloc();
                        if (state->swr_ctx) {
                            int audio_sample_rate = 48000;
                            int audio_channels = 2;
                            
                            av_opt_set_chlayout(state->swr_ctx, "in_chlayout", &state->audio_codec_ctx->ch_layout, 0);
                            av_opt_set_int(state->swr_ctx, "in_sample_rate", state->audio_codec_ctx->sample_rate, 0);
                            av_opt_set_sample_fmt(state->swr_ctx, "in_sample_fmt", state->audio_codec_ctx->sample_fmt, 0);
                            
                            AVChannelLayout out_chlayout;
                            av_channel_layout_default(&out_chlayout, audio_channels);
                            
                            av_opt_set_chlayout(state->swr_ctx, "out_chlayout", &out_chlayout, 0);
                            av_opt_set_int(state->swr_ctx, "out_sample_rate", audio_sample_rate, 0);
                            av_opt_set_sample_fmt(state->swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                            
                            if (swr_init(state->swr_ctx) < 0) {
                                swr_free(&state->swr_ctx);
                                state->swr_ctx = NULL;
                            }
                            av_channel_layout_uninit(&out_chlayout);
                        }
                    } else {
                        avcodec_free_context(&state->audio_codec_ctx);
                        state->audio_codec_ctx = NULL;
                    }
                } else {
                    avcodec_free_context(&state->audio_codec_ctx);
                    state->audio_codec_ctx = NULL;
                }
            }
        }
    }
    
    // Создаем или переиспользуем окно и рендерер
    if (!create_or_reuse_window(state->video_width, state->video_height)) {
        printf("[Video] Failed to create/reuse window\n");
        cleanup_video_state(state);
        return;
    }
    
    // Создаем текстуру
    state->texture = SDL_CreateTexture(global_renderer,
                                       SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       state->video_width,
                                       state->video_height);
    if (!state->texture) {
        printf("[Video] Could not create texture: %s\n", SDL_GetError());
        cleanup_video_state(state);
        return;
    }
    
    // Контекст для преобразования цвета
    state->sws_ctx = sws_getContext(
        state->video_width,
        state->video_height,
        state->video_codec_ctx->pix_fmt,
        state->video_width,
        state->video_height,
        AV_PIX_FMT_RGBA,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
    
    if (!state->sws_ctx) {
        printf("[Video] Could not create sws context\n");
        cleanup_video_state(state);
        return;
    }
    
    // Инициализируем аудио буфер
    if (!init_audio_buffer(&state->audio_buf, AUDIO_BUFFER_SIZE)) {
        printf("[Video] Failed to init audio buffer\n");
        cleanup_video_state(state);
        return;
    }
    
    state->resources_allocated = true;
    
    // Подготавливаем кадры
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgba_frame = av_frame_alloc();
    AVFrame *audio_frame = av_frame_alloc();
    
    if (!frame || !rgba_frame || !audio_frame) {
        printf("[Video] Could not allocate frames\n");
        if (frame) av_frame_free(&frame);
        if (rgba_frame) av_frame_free(&rgba_frame);
        if (audio_frame) av_frame_free(&audio_frame);
        cleanup_video_state(state);
        return;
    }
    
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, 
                                            state->video_width, 
                                            state->video_height, 
                                            1);
    uint8_t *buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
    if (!buffer) {
        printf("[Video] Could not allocate buffer\n");
        av_frame_free(&audio_frame);
        av_frame_free(&rgba_frame);
        av_frame_free(&frame);
        cleanup_video_state(state);
        return;
    }
    
    av_image_fill_arrays(rgba_frame->data, rgba_frame->linesize, buffer,
                        AV_PIX_FMT_RGBA, state->video_width, 
                        state->video_height, 1);
    
    // Настраиваем аудио в SDL
    if (state->audio_codec_ctx && state->swr_ctx) {
        SDL_AudioSpec wanted_spec, obtained_spec;
        SDL_zero(wanted_spec);
        wanted_spec.freq = 48000;
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.channels = 2;
        wanted_spec.samples = 1024;
        wanted_spec.callback = audio_callback;
        wanted_spec.userdata = &state->audio_buf;
        
        state->audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &obtained_spec, 0);
        if (state->audio_device > 0) {
            state->audio_initialized = true;
            SDL_PauseAudioDevice(state->audio_device, 0);
            printf("[Video] Audio initialized\n");
        }
    }
    
    // Задержка перед началом воспроизведения
    if (delay_seconds > 0) {
        printf("[Video] Waiting %.1f seconds before playback...\n", delay_seconds);
        
        Uint32 start_delay = SDL_GetTicks();
        float elapsed_delay = 0.0f;
        bool skip_delay = false;
        
        while (elapsed_delay < delay_seconds && !skip_delay) {
            padUpdate(&pad);
            u64 kHeld = padGetButtons(&pad);
            
            if (skip_enabled && (kHeld & HidNpadButton_B)) {
                skip_delay = true;
                printf("[Video] Delay skipped\n");
                break;
            }
            
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    skip_delay = true;
                    break;
                }
            }
            
            clear_screen(global_renderer);
            SDL_Delay(16);
            elapsed_delay = (SDL_GetTicks() - start_delay) / 1000.0f;
        }
    }
    
    // Основной цикл воспроизведения
    AVPacket *packet = av_packet_alloc();
    bool quit = false;
    float hold_time = 0.0f;
    Uint32 last_button_check = SDL_GetTicks();
    
    // Для синхронизации видео
    int64_t start_time = av_gettime();
    double video_clock = 0;
    AVRational time_base = video_stream->time_base;
    double frame_delay = av_q2d(video_stream->avg_frame_rate);
    if (frame_delay > 0) {
        frame_delay = 1.0 / frame_delay;
    } else {
        frame_delay = 1.0 / 30.0;
    }
    
    int retry_count = 0;
    const int max_retries = 3;
    
    while (!quit) {
        int read_result = av_read_frame(state->fmt_ctx, packet);
        if (read_result < 0) {
            if (read_result == AVERROR_EOF) {
                // Конец файла
                break;
            }
            
            retry_count++;
            if (retry_count > max_retries) {
                printf("[Video] Failed to read frame after %d retries\n", max_retries);
                break;
            }
            
            printf("[Video] Read frame error, retrying... (%d/%d)\n", retry_count, max_retries);
            SDL_Delay(10);
            continue;
        }
        
        retry_count = 0;
        
        // Проверка кнопок
        Uint32 now = SDL_GetTicks();
        float delta_time = (now - last_button_check) / 1000.0f;
        last_button_check = now;
        
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
            if (avcodec_send_packet(state->video_codec_ctx, packet) < 0) {
                av_packet_unref(packet);
                continue;
            }
            
            while (avcodec_receive_frame(state->video_codec_ctx, frame) == 0) {
                if (frame->pts != AV_NOPTS_VALUE) {
                    video_clock = frame->pts * av_q2d(time_base);
                } else if (frame->pkt_dts != AV_NOPTS_VALUE) {
                    video_clock = frame->pkt_dts * av_q2d(time_base);
                }
                
                double current_time = (av_gettime() - start_time) / 1000000.0;
                double sync_delay = video_clock - current_time;
                
                if (sync_delay > 0) {
                    if (sync_delay > frame_delay * 2) {
                        sync_delay = frame_delay;
                    }
                    av_usleep((int64_t)(sync_delay * 1000000));
                }
                
                // Конвертируем кадр в RGBA
                sws_scale(state->sws_ctx, (uint8_t const * const *)frame->data,
                         frame->linesize, 0, state->video_height,
                         rgba_frame->data, rgba_frame->linesize);
                
                SDL_UpdateTexture(state->texture, NULL, rgba_frame->data[0], 
                                rgba_frame->linesize[0]);
                
                SDL_RenderClear(global_renderer);
                SDL_RenderCopy(global_renderer, state->texture, NULL, NULL);
                
                if (hold_time > 0.0f) {
                    float progress = fminf(hold_time / SKIP_HOLD_TIME, 1.0f);
                    draw_circle_progress(global_renderer, 
                                       state->video_width - 40,
                                       state->video_height - 40,
                                       25, 6, progress);
                }
                
                SDL_RenderPresent(global_renderer);
                SDL_Delay(1);
            }
        }
        
        // Аудио пакет
        if (packet->stream_index == audio_stream_index && state->audio_codec_ctx && state->swr_ctx) {
            if (avcodec_send_packet(state->audio_codec_ctx, packet) < 0) {
                av_packet_unref(packet);
                continue;
            }
            
            while (avcodec_receive_frame(state->audio_codec_ctx, audio_frame) == 0) {
                int out_samples = av_rescale_rnd(swr_get_delay(state->swr_ctx, audio_frame->sample_rate) + 
                                                audio_frame->nb_samples,
                                                48000, audio_frame->sample_rate, AV_ROUND_UP);
                
                uint8_t *converted_audio = NULL;
                int out_linesize;
                int out_count = av_samples_alloc(&converted_audio, &out_linesize, 2,
                                                out_samples, AV_SAMPLE_FMT_S16, 1);
                
                if (out_count >= 0) {
                    int converted = swr_convert(state->swr_ctx, &converted_audio, out_samples,
                                              (const uint8_t**)audio_frame->data, audio_frame->nb_samples);
                    
                    if (converted > 0) {
                        int actual_size = av_samples_get_buffer_size(&out_linesize, 2,
                                                                    converted, AV_SAMPLE_FMT_S16, 1);
                        write_audio_data(&state->audio_buf, converted_audio, actual_size);
                    }
                    
                    av_freep(&converted_audio);
                }
            }
        }
        
        av_packet_unref(packet);
    }
    
    // Освобождаем временные ресурсы
    if (packet) {
        av_packet_free(&packet);
    }
    
    if (buffer) {
        av_free(buffer);
    }
    
    if (audio_frame) {
        av_frame_free(&audio_frame);
    }
    
    if (rgba_frame) {
        av_frame_free(&rgba_frame);
    }
    
    if (frame) {
        av_frame_free(&frame);
    }
    
    // Очищаем экран
    printf("[Video] Clearing screen...\n");
    clear_screen(global_renderer);
    
    // Освобождаем основные ресурсы
    cleanup_video_state(state);
    
    // Задержка ПОСЛЕ освобождения ресурсов
    if (delay_seconds > 0) {
        printf("[Video] Pausing for %.1f seconds after cleanup...\n", delay_seconds);
        SDL_Delay((Uint32)(delay_seconds * 1000));
    }
    
    printf("[Video] Finished\n");
}

void play_video_file(const char *path, int skip_enabled)
{
    play_video_file_delay(path, skip_enabled, DEFAULT_DELAY_SECONDS);
}
