#include <switch.h>
#include <Python.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <png.h>


#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

/* -------------------------------------------------------
   Globals
------------------------------------------------------- */

u64 cur_progid = 0;
AccountUid userID = {0};

/* -------------------------------------------------------
   _nx module (sleep)
------------------------------------------------------- */

static PyObject* py_nx_sleep(PyObject* self, PyObject* args)
{
    double seconds;
    if (!PyArg_ParseTuple(args, "d", &seconds))
        return NULL;

    uint64_t ns = (uint64_t)(seconds * 1000000000ULL);

    Py_BEGIN_ALLOW_THREADS
    svcSleepThread(ns);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

static PyMethodDef NxMethods[] = {
    {"sleep", py_nx_sleep, METH_VARARGS, "Sleep using svcSleepThread"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef nx_module = {
    PyModuleDef_HEAD_INIT,
    "_nx",
    NULL,
    -1,
    NxMethods
};

PyMODINIT_FUNC PyInit__nx(void)
{
    return PyModule_Create(&nx_module);
}

/* -------------------------------------------------------
   _otrhlibnx module
------------------------------------------------------- */

static PyObject* commitsave(PyObject* self, PyObject* args)
{
    u64 total_size = 0;
    u64 free_size = 0;
    FsFileSystem* FsSave = fsdevGetDeviceFileSystem("save");

    if (!FsSave)
        Py_RETURN_NONE;

    FsSaveDataInfoReader reader;
    FsSaveDataInfo info;
    s64 total_entries = 0;
    Result rc = 0;

    fsdevCommitDevice("save");

    fsFsGetTotalSpace(FsSave, "/", &total_size);
    fsFsGetFreeSpace(FsSave, "/", &free_size);

    if (free_size < 0x800000)
    {
        u64 new_size = total_size + 0x800000;

        fsdevUnmountDevice("save");
        fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);

        while (1)
        {
            rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &total_entries);
            if (R_FAILED(rc) || total_entries == 0)
                break;

            if (info.save_data_type == FsSaveDataType_Account &&
                userID.uid[0] == info.uid.uid[0] &&
                userID.uid[1] == info.uid.uid[1] &&
                info.application_id == cur_progid)
            {
                fsExtendSaveDataFileSystem(
                    info.save_data_space_id,
                    info.save_data_id,
                    new_size,
                    0x400000
                );
                break;
            }
        }

        fsSaveDataInfoReaderClose(&reader);
        fsdevMountSaveData("save", cur_progid, userID);
    }

    Py_RETURN_NONE;
}

static PyObject* startboost(PyObject* self, PyObject* args)
{
    appletSetCpuBoostMode(ApmPerformanceMode_Boost);
    Py_RETURN_NONE;
}

static PyObject* disableboost(PyObject* self, PyObject* args)
{
    appletSetCpuBoostMode(ApmPerformanceMode_Normal);
    Py_RETURN_NONE;
}

static PyObject* restartprogram(PyObject* self, PyObject* args)
{
    appletRestartProgram(NULL, 0);
    Py_RETURN_NONE;
}

static PyMethodDef OtrhMethods[] = {
    {"commitsave", commitsave, METH_NOARGS, NULL},
    {"startboost", startboost, METH_NOARGS, NULL},
    {"disableboost", disableboost, METH_NOARGS, NULL},
    {"restartprogram", restartprogram, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef otrh_module = {
    PyModuleDef_HEAD_INIT,
    "_otrhlibnx",
    NULL,
    -1,
    OtrhMethods
};

PyMODINIT_FUNC PyInit__otrhlibnx(void)
{
    return PyModule_Create(&otrh_module);
}


PyMODINIT_FUNC PyInit_color(void);
PyMODINIT_FUNC PyInit_controller(void);
PyMODINIT_FUNC PyInit_display(void);
PyMODINIT_FUNC PyInit_draw(void);
PyMODINIT_FUNC PyInit_error(void);
PyMODINIT_FUNC PyInit_event(void);
PyMODINIT_FUNC PyInit_font(void);
PyMODINIT_FUNC PyInit_gfxdraw(void);
PyMODINIT_FUNC PyInit_image(void);
PyMODINIT_FUNC PyInit_joystick(void);
PyMODINIT_FUNC PyInit_key(void);
PyMODINIT_FUNC PyInit_locals(void);
PyMODINIT_FUNC PyInit_mouse(void);
PyMODINIT_FUNC PyInit_power(void);
PyMODINIT_FUNC PyInit_pygame_time(void);
PyMODINIT_FUNC PyInit_rect(void);
PyMODINIT_FUNC PyInit_rwobject(void);
PyMODINIT_FUNC PyInit_scrap(void);
PyMODINIT_FUNC PyInit_transform(void);
PyMODINIT_FUNC PyInit_surface(void);

PyMODINIT_FUNC PyInit_render(void);
PyMODINIT_FUNC PyInit_mixer(void);
PyMODINIT_FUNC PyInit_mixer_music(void);


PyMODINIT_FUNC PyInit__renpy(void);
PyMODINIT_FUNC PyInit__renpybidi(void);

PyMODINIT_FUNC PyInit_renpy_audio_filter(void);
PyMODINIT_FUNC PyInit_renpy_audio_renpysound(void);
PyMODINIT_FUNC PyInit_renpy_encryption(void);

PyMODINIT_FUNC PyInit_renpy_display_accelerator(void);
PyMODINIT_FUNC PyInit_renpy_display_matrix(void);
PyMODINIT_FUNC PyInit_renpy_display_quaternion(void);
PyMODINIT_FUNC PyInit_renpy_display_render(void);

PyMODINIT_FUNC PyInit_renpy_text_ftfont(void);
PyMODINIT_FUNC PyInit_renpy_text_textsupport(void);
PyMODINIT_FUNC PyInit_renpy_text_texwrap(void);

PyMODINIT_FUNC PyInit_renpy_lexersupport(void);
PyMODINIT_FUNC PyInit_renpy_pydict(void);
PyMODINIT_FUNC PyInit_renpy_style(void);

PyMODINIT_FUNC PyInit_renpy_styledata_style_activate_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_hover_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_idle_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_insensitive_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_activate_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_hover_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_idle_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_insensitive_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_styleclass(void);
PyMODINIT_FUNC PyInit_renpy_styledata_stylesets(void);

PyMODINIT_FUNC PyInit_renpy_gl_gldraw(void);
PyMODINIT_FUNC PyInit_renpy_gl_glenviron_shader(void);
PyMODINIT_FUNC PyInit_renpy_gl_glrtt_copy(void);
PyMODINIT_FUNC PyInit_renpy_gl_glrtt_fbo(void);
PyMODINIT_FUNC PyInit_renpy_gl_gltexture(void);

PyMODINIT_FUNC PyInit_renpy_gl2_gl2draw(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2mesh(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2mesh2(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2mesh3(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2model(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2polygon(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2shader(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2texture(void);

PyMODINIT_FUNC PyInit_renpy_uguu_gl(void);
PyMODINIT_FUNC PyInit_renpy_uguu_uguu(void);


/* -------------------------------------------------------
   Heap override
------------------------------------------------------- */

void __libnx_initheap(void)
{
    void* addr = NULL;
    u64 size = 0;
    u64 mem_available = 0, mem_used = 0;

    svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&mem_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);

    if (mem_available > mem_used + 0x200000)
        size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;

    if (size == 0)
        size = 0x2000000*16; // 256 MB fallback

    Result rc = svcSetHeapSize(&addr, size);
    if (R_FAILED(rc) || addr == NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));

    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

/* -------------------------------------------------------
   Save creation
------------------------------------------------------- */

Result createSaveData(void)
{
    FsSaveDataAttribute attr = {0};
    FsSaveDataCreationInfo crt = {0};
    FsSaveDataMetaInfo meta = {0};

    attr.application_id = cur_progid;
    attr.uid = userID;
    attr.save_data_type = FsSaveDataType_Account;

    crt.save_data_size = 0x800000;   // 8 MB
    crt.journal_size   = 0x400000;   // 4 MB
    crt.available_size = 0x8000;
    crt.save_data_space_id = FsSaveDataSpaceId_User;

    return fsCreateSaveDataFileSystem(&attr, &crt, &meta);
}

/* -------------------------------------------------------
   App init / exit
------------------------------------------------------- */

void userAppInit(void)
{
    fsdevMountSdmc();

    freopen("sdmc:/renpy_switch.log", "w", stdout);
    freopen("sdmc:/renpy_switch.log", "w", stderr);

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    printf("=== Ren'Py 8 Switch launcher ===\n");

    svcGetInfo(&cur_progid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);

    accountInitialize(AccountServiceType_Application);
    accountGetPreselectedUser(&userID);

    if (accountUidIsValid(&userID)) {
        Result rc = fsdevMountSaveData("save", cur_progid, userID);
        if (R_FAILED(rc)) {
            createSaveData();
            fsdevMountSaveData("save", cur_progid, userID);
        }
    }

    romfsInit();
    socketInitializeDefault();
}

void userAppExit(void)
{
    if (fsdevGetDeviceFileSystem("save"))
        fsdevCommitDevice("save");

    fsdevUnmountDevice("save");
    socketExit();
    romfsExit();
}

/* -------------------------------------------------------
   Error helper
------------------------------------------------------- */

void show_error(const char* message)
{
    PyErr_Print();

    ErrorSystemConfig c;
    errorSystemCreate(&c, message, message);
    errorSystemShow(&c);

    Py_Exit(1);
}

static AppletHookCookie applet_hook_cookie;
static void on_applet_hook(AppletHookType hook, void *param)
{
   switch (hook)
   {
      case AppletHookType_OnExitRequest:
        fsdevCommitDevice("save");
        svcSleepThread(1500000000ULL);
        appletUnlockExit();
        break;

      default:
         break;
   }
}


void Logo_NX(const char* romfs_path, double display_seconds)
{
    // Монтируем romfs
    romfsInit();

    // Загружаем PNG из romfs
    FILE* f = fopen(romfs_path, "rb");
    if (!f) {
        printf("Cannot open %s\n", romfs_path);
        return;
    }

    // Чтение PNG через libpng
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (setjmp(png_jmpbuf(png_ptr))) {
        fclose(f);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return;
    }

    png_init_io(png_ptr, f);
    png_read_info(png_ptr, info_ptr);

    int img_width  = png_get_image_width(png_ptr, info_ptr);
    int img_height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth  = png_get_bit_depth(png_ptr, info_ptr);

    if (bit_depth == 16) png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY) png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    png_read_update_info(png_ptr, info_ptr);

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * img_height);
    for (int y = 0; y < img_height; y++)
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));

    png_read_image(png_ptr, row_pointers);
    fclose(f);

    // Получаем framebuffer
    Framebuffer fb;
    framebufferCreate(&fb, 1280, 720, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);
    framebufferBind(&fb);

    u32* pixels = (u32*)fb.framebuffers[fb.currentFramebuffer].data;

    // Пропорционально растягиваем PNG на весь экран
    for (int y = 0; y < 720; y++) {
        for (int x = 0; x < 1280; x++) {
            int src_x = x * img_width / 1280;
            int src_y = y * img_height / 720;
            png_bytep px = &(row_pointers[src_y][src_x * 4]);
            pixels[y * 1280 + x] = (px[0] << 24) | (px[1] << 16) | (px[2] << 8) | px[3]; // RGBA
        }
    }

    framebufferFlush(&fb);
    framebufferSwap(&fb);

    // Ждём display_seconds
    svcSleepThread((uint64_t)(display_seconds * 1000000000ULL));

    framebufferClose(&fb);

    for (int y = 0; y < img_height; y++) free(row_pointers[y]);
    free(row_pointers);

    romfsExit();
}

static void show_mp4_splash(const char* romfs_path, float display_time_sec)
{
    if (!romfs_path || strncmp(romfs_path, "romfs:/", 7) != 0)
        return;

    char sd_path[256];
    snprintf(sd_path, sizeof(sd_path), "sdmc:/%s", romfs_path + 7);

    // Копируем файл в SD (или можно читать напрямую из romfs)
    FILE *f_in = fopen(romfs_path, "rb");
    FILE *f_out = fopen(sd_path, "wb");
    if (!f_in || !f_out) return;
    char buf[4096]; size_t r;
    while ((r = fread(buf, 1, sizeof(buf), f_in)) > 0) fwrite(buf, 1, r, f_out);
    fclose(f_in); fclose(f_out);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow(
        "",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
    );

    SDL_Renderer* ren = SDL_CreateRenderer(
        win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);

    avformat_network_init();

    AVFormatContext* fmt_ctx = NULL;
    if (avformat_open_input(&fmt_ctx, sd_path, NULL, NULL) < 0) goto cleanup;
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) goto cleanup;

    int vstream = -1;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; i++)
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            vstream = i;
            break;
        }
    if (vstream < 0) goto cleanup;

    AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[vstream]->codecpar->codec_id);
    if (!codec) goto cleanup;

    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(ctx, fmt_ctx->streams[vstream]->codecpar);
    if (avcodec_open2(ctx, codec, NULL) < 0) goto cleanup;

    struct SwsContext* sws_ctx = sws_getContext(
        ctx->width, ctx->height, ctx->pix_fmt,
        ctx->width, ctx->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, NULL, NULL, NULL
    );
    if (!sws_ctx) goto cleanup;

    AVFrame* frame = av_frame_alloc();
    AVFrame* rgba  = av_frame_alloc();
    int bufsize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, ctx->width, ctx->height, 1);
    uint8_t* buffer = av_malloc(bufsize);
    av_image_fill_arrays(rgba->data, rgba->linesize, buffer, AV_PIX_FMT_RGBA, ctx->width, ctx->height, 1);

    SDL_Texture* tex = SDL_CreateTexture(
        ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        ctx->width, ctx->height
    );

    AVPacket pkt;
    SDL_Event e;
    uint64_t start_ticks = armGetSystemTick();
    uint64_t max_ticks = (display_time_sec > 0) ? (uint64_t)(display_time_sec * armGetSystemTickFreq()) : UINT64_MAX;

    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index != vstream) { av_packet_unref(&pkt); continue; }
        if (avcodec_send_packet(ctx, &pkt) < 0) { av_packet_unref(&pkt); continue; }

        while (avcodec_receive_frame(ctx, frame) == 0) {
            sws_scale(sws_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0, ctx->height,
                      rgba->data, rgba->linesize);

            SDL_UpdateTexture(tex, NULL, rgba->data[0], rgba->linesize[0]);

            SDL_RenderClear(ren);

            // Расчёт dst с сохранением пропорций на 1280x720
            SDL_Rect dst;
            float img_ratio = (float)ctx->width / ctx->height;
            float screen_ratio = 1280.0f / 720.0f;

            if (img_ratio > screen_ratio) {
                dst.w = 1280;
                dst.h = (int)(1280 / img_ratio);
                dst.x = 0;
                dst.y = (720 - dst.h) / 2;
            } else {
                dst.h = 720;
                dst.w = (int)(720 * img_ratio);
                dst.y = 0;
                dst.x = (1280 - dst.w) / 2;
            }

            SDL_RenderCopy(ren, tex, NULL, &dst);
            SDL_RenderPresent(ren);

            while (SDL_PollEvent(&e)) {}

            AVRational tb = fmt_ctx->streams[vstream]->time_base;
            int64_t delay_ns = av_rescale_q(frame->pkt_duration, tb, (AVRational){1, 1000000000});
            if (delay_ns <= 0) delay_ns = 16666666; // ~60 FPS
            svcSleepThread(delay_ns);

            if (armGetSystemTick() - start_ticks >= max_ticks) goto cleanup;
        }
        av_packet_unref(&pkt);
    }

cleanup:
    SDL_DestroyTexture(tex);
    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&rgba);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt_ctx);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}




/* -------------------------------------------------------
   Main
------------------------------------------------------- */

int main(int argc, char* argv[])
{
    Logo_NX("romfs:/nintendologo.png", 1.0); 
    // Показываем GIF из romfs:/Contents/logo.gif, 30 fps
    show_mp4_splash("romfs:/logo.mp4", 5.0f);
   
    chdir("romfs:/Contents");
    setlocale(LC_ALL, "C");
    setenv("MESA_NO_ERROR", "1", 1);

    appletLockExit();
    appletHook(&applet_hook_cookie, on_applet_hook, NULL);

    Py_NoSiteFlag = 1;
    Py_IgnoreEnvironmentFlag = 1;
    Py_NoUserSiteDirectory = 1;
    Py_DontWriteBytecodeFlag = 1;
    Py_OptimizeFlag = 2;

    PyConfig config;

    PyConfig_InitPythonConfig(&config);

     /* ---- Указываем Python'у его местоположение ---- */
    /* Это устранит ошибку "Could not find platform independent libraries" */
    PyStatus status;
    
    status = PyConfig_SetString(&config, &config.home, L"romfs:/Contents");
    if (PyStatus_Exception(status)) goto exception;

    status = PyConfig_SetString(&config, &config.prefix, L"romfs:/Contents");
    if (PyStatus_Exception(status)) goto exception;
    
    status = PyConfig_SetString(&config, &config.exec_prefix, L"romfs:/Contents");
    if (PyStatus_Exception(status)) goto exception;
   
    /* Добавляем путь к корневой папке renpy/common */
    PyWideStringList_Append(
    &config.module_search_paths,
    L"romfs:/Contents/renpy/common"
    );
   
    /* ---- Critical for Python 3.9 embedded ---- */
    config.isolated = 0;
    config.use_environment = 0;
    config.site_import = 0;
    config.user_site_directory = 0;
    config.write_bytecode = 0;
    config.optimization_level = 2;
    config.verbose = 0;

    /* Filesystem encoding */
    status = PyConfig_SetString(&config,
                                &config.filesystem_encoding,
                                L"utf-8");
    if (PyStatus_Exception(status)) goto exception;

    status = PyConfig_SetString(&config,
                                &config.filesystem_errors,
                                L"surrogateescape");
    if (PyStatus_Exception(status)) goto exception;

    /* ---- stdlib: ONLY lib.zip ---- */
    config.module_search_paths_set = 1;

    status = PyWideStringList_Append(
        &config.module_search_paths,
        L"romfs:/Contents/lib.zip"
    );
    if (PyStatus_Exception(status)) goto exception;

    /* ---- argv ---- */
    wchar_t* pyargv[] = {
        L"romfs:/Contents/renpy.py",
        NULL
    };
    status = PyConfig_SetArgv(&config, 1, pyargv);
    if (PyStatus_Exception(status)) goto exception;

   
    Py_SetProgramName(L"RenPy3.8.7");



    /* ---- Builtin modules ---- */
    static struct _inittab builtins[] = {

       //{"pygame_sdl2", PyInit_pygame_sdl2},

        {"_nx", PyInit__nx},
        {"_otrhlibnx", PyInit__otrhlibnx},

        {"pygame_sdl2.color", PyInit_color},
        {"pygame_sdl2.controller", PyInit_controller},
        {"pygame_sdl2.display", PyInit_display},
        {"pygame_sdl2.draw", PyInit_draw},
        {"pygame_sdl2.error", PyInit_error},
        {"pygame_sdl2.event", PyInit_event},
        {"pygame_sdl2.font", PyInit_font},
        {"pygame_sdl2.gfxdraw", PyInit_gfxdraw},
        {"pygame_sdl2.image", PyInit_image},
        {"pygame_sdl2.joystick", PyInit_joystick},
        {"pygame_sdl2.key", PyInit_key},
        {"pygame_sdl2.locals", PyInit_locals},
        {"pygame_sdl2.mouse", PyInit_mouse},
        {"pygame_sdl2.power", PyInit_power},
        {"pygame_sdl2.pygame_time", PyInit_pygame_time},
        {"pygame_sdl2.rect", PyInit_rect}, 
        {"pygame_sdl2.rwobject", PyInit_rwobject},
        {"pygame_sdl2.scrap", PyInit_scrap},
        {"pygame_sdl2.surface", PyInit_surface},
        {"pygame_sdl2.transform", PyInit_transform},
   
        {"pygame_sdl2.render", PyInit_render},
        {"pygame_sdl2.mixer", PyInit_mixer},
        {"pygame_sdl2.mixer_music", PyInit_mixer_music},

        {"_renpy", PyInit__renpy},
        {"_renpybidi", PyInit__renpybidi},

        {"renpy.audio.filter", PyInit_renpy_audio_filter},
        {"renpy.audio.renpysound", PyInit_renpy_audio_renpysound},
        {"renpy.display.accelerator", PyInit_renpy_display_accelerator},
        {"renpy.display.matrix", PyInit_renpy_display_matrix},
        {"renpy.display.quaternion", PyInit_renpy_display_quaternion},
        {"renpy.display.render", PyInit_renpy_display_render},
        {"renpy.encryption", PyInit_renpy_encryption},

        {"renpy.text.ftfont", PyInit_renpy_text_ftfont},
        {"renpy.text.textsupport", PyInit_renpy_text_textsupport},
        {"renpy.text.texwrap", PyInit_renpy_text_texwrap},
        {"renpy.pydict", PyInit_renpy_pydict},
        {"renpy.lexersupport", PyInit_renpy_lexersupport},
        {"renpy.style", PyInit_renpy_style},
        {"renpy.styledata.style_activate_functions", PyInit_renpy_styledata_style_activate_functions},
        {"renpy.styledata.style_functions", PyInit_renpy_styledata_style_functions},
        {"renpy.styledata.style_hover_functions", PyInit_renpy_styledata_style_hover_functions},
        {"renpy.styledata.style_idle_functions", PyInit_renpy_styledata_style_idle_functions},
        {"renpy.styledata.style_insensitive_functions", PyInit_renpy_styledata_style_insensitive_functions},
        {"renpy.styledata.style_selected_activate_functions", PyInit_renpy_styledata_style_selected_activate_functions},
        {"renpy.styledata.style_selected_functions", PyInit_renpy_styledata_style_selected_functions},
        {"renpy.styledata.style_selected_hover_functions", PyInit_renpy_styledata_style_selected_hover_functions},
        {"renpy.styledata.style_selected_idle_functions", PyInit_renpy_styledata_style_selected_idle_functions},
        {"renpy.styledata.style_selected_insensitive_functions", PyInit_renpy_styledata_style_selected_insensitive_functions},
        {"renpy.styledata.styleclass", PyInit_renpy_styledata_styleclass},
        {"renpy.styledata.stylesets", PyInit_renpy_styledata_stylesets},

        {"renpy.gl.gldraw", PyInit_renpy_gl_gldraw},
        {"renpy.gl.glenviron_shader", PyInit_renpy_gl_glenviron_shader},
        {"renpy.gl.glrtt_copy", PyInit_renpy_gl_glrtt_copy},
        {"renpy.gl.glrtt_fbo", PyInit_renpy_gl_glrtt_fbo},
        {"renpy.gl.gltexture", PyInit_renpy_gl_gltexture},

        {"renpy.gl2.gl2draw", PyInit_renpy_gl2_gl2draw},
        {"renpy.gl2.gl2mesh", PyInit_renpy_gl2_gl2mesh},
        {"renpy.gl2.gl2mesh2", PyInit_renpy_gl2_gl2mesh2},
        {"renpy.gl2.gl2mesh3", PyInit_renpy_gl2_gl2mesh3},
        {"renpy.gl2.gl2model", PyInit_renpy_gl2_gl2model},
        {"renpy.gl2.gl2polygon", PyInit_renpy_gl2_gl2polygon},
        {"renpy.gl2.gl2shader", PyInit_renpy_gl2_gl2shader},
        {"renpy.gl2.gl2texture", PyInit_renpy_gl2_gl2texture},

        {"renpy.uguu.gl", PyInit_renpy_uguu_gl},
        {"renpy.uguu.uguu", PyInit_renpy_uguu_uguu},

        {NULL, NULL}
    };

    /* ---- Sanity check ---- */
    FILE* libzip = fopen("romfs:/Contents/lib.zip", "rb");
    if (!libzip) {
        show_error("Could not find lib.zip");
    }
    fclose(libzip);

    FILE* renpy_file = fopen("romfs:/Contents/renpy.py", "rb");
    if (!renpy_file) {
        show_error("Could not find renpy.py");
    }   

    PyImport_ExtendInittab(builtins);
   
    /* ---- Initialize Python ---- */
    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) goto exception;
    PyConfig_Clear(&config);


    PyRun_SimpleString("import pygame_sdl2");
   

    int python_result;
    python_result = PyRun_SimpleString(
    "import sys\n"
    "sys.path.insert(0, 'romfs:/Contents/lib.zip')\n"
    );
    
    if (python_result == -1)
    {
        show_error("Could not set the Python path.\n\nThis is an internal error and should not occur during normal usage.");
    }
   #define x(lib) \
    { \
        if (PyRun_SimpleString("import " lib) == -1) \
        { \
            show_error("Could not import python library " lib ".\n\nPlease ensure that you have extracted the files correctly so that the \"lib\" folder is in the same directory as the nsp file, and that the \"lib\" folder contains the folder \"python3.9\". \nInside that folder, the file \"" lib ".py\" or folder \"" lib "\" needs to exist."); \
        } \
    }

    x("os");
    x("pygame_sdl2");
    x("encodings");

    #undef x

    /* ---- Run Ren'Py ---- */
    int rc = PyRun_SimpleFileEx(
        renpy_file,
        "romfs:/Contents/renpy.py",
        1
    );

    if (rc != 0) {
        show_error("Ren'Py execution failed");
    }

    Py_Finalize();
    return 0;

exception:
    PyConfig_Clear(&config);
    if (PyStatus_IsExit(status)) {
        return status.exitcode;
    }
    show_error(status.err_msg);
    Py_ExitStatusException(status);
}
