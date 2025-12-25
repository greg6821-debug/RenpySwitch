#include <switch.h>
#include <Python.h>
#include <stdio.h>

u64 cur_progid = 0;
AccountUid userID = {0};

static PyObject* commitsave(PyObject* self, PyObject* args)
{
    u64 total_size = 0;
    u64 free_size = 0;
    FsFileSystem* FsSave = fsdevGetDeviceFileSystem("save");

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
                    0x400000);
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

static PyMethodDef otrh_methods[] = {
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
    otrh_methods
};

PyMODINIT_FUNC PyInit__otrhlibnx(void)
{
    return PyModule_Create(&otrh_module);
}

/* ---- external init symbols ---- */
#define DECL(name) PyMODINIT_FUNC PyInit_##name(void)

DECL(pygame_sdl2_color);
DECL(pygame_sdl2_controller);
DECL(pygame_sdl2_display);
DECL(pygame_sdl2_draw);
DECL(pygame_sdl2_error);
DECL(pygame_sdl2_event);
DECL(pygame_sdl2_gfxdraw);
DECL(pygame_sdl2_image);
DECL(pygame_sdl2_joystick);
DECL(pygame_sdl2_key);
DECL(pygame_sdl2_locals);
DECL(pygame_sdl2_mouse);
DECL(pygame_sdl2_power);
DECL(pygame_sdl2_pygame_time);
DECL(pygame_sdl2_rect);
DECL(pygame_sdl2_render);
DECL(pygame_sdl2_rwobject);
DECL(pygame_sdl2_scrap);
DECL(pygame_sdl2_surface);
DECL(pygame_sdl2_transform);

DECL(_renpy);
DECL(_renpybidi);
DECL(renpy_audio_renpysound);
DECL(renpy_display_accelerator);
DECL(renpy_display_render);
DECL(renpy_display_matrix);
DECL(renpy_gl_gldraw);
DECL(renpy_gl_glenviron_shader);
DECL(renpy_gl_glrtt_copy);
DECL(renpy_gl_glrtt_fbo);
DECL(renpy_gl_gltexture);
DECL(renpy_pydict);
DECL(renpy_style);
DECL(renpy_styledata_style_activate_functions);
DECL(renpy_styledata_style_functions);
DECL(renpy_styledata_style_hover_functions);
DECL(renpy_styledata_style_idle_functions);
DECL(renpy_styledata_style_insensitive_functions);
DECL(renpy_styledata_style_selected_activate_functions);
DECL(renpy_styledata_style_selected_functions);
DECL(renpy_styledata_style_selected_hover_functions);
DECL(renpy_styledata_style_selected_idle_functions);
DECL(renpy_styledata_style_selected_insensitive_functions);
DECL(renpy_styledata_styleclass);
DECL(renpy_styledata_stylesets);
DECL(renpy_text_ftfont);
DECL(renpy_text_textsupport);
DECL(renpy_text_texwrap);
DECL(renpy_compat_dictviews);
DECL(renpy_gl2_gl2draw);
DECL(renpy_gl2_gl2mesh);
DECL(renpy_gl2_gl2mesh2);
DECL(renpy_gl2_gl2mesh3);
DECL(renpy_gl2_gl2model);
DECL(renpy_gl2_gl2polygon);
DECL(renpy_gl2_gl2shader);
DECL(renpy_gl2_gl2texture);
DECL(renpy_uguu_gl);
DECL(renpy_uguu_uguu);
DECL(renpy_lexersupport);
DECL(renpy_display_quaternion);

#undef DECL

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
        size = 0x2000000 * 16;

    Result rc = svcSetHeapSize(&addr, size);
    if (R_FAILED(rc) || !addr)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));

    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end = (char*)addr + size;
}

static AppletHookCookie applet_hook_cookie;

static void on_applet_hook(AppletHookType hook, void* param)
{
    if (hook == AppletHookType_OnExitRequest)
    {
        fsdevCommitDevice("save");
        svcSleepThread(1500000000ULL);
        appletUnlockExit();
    }
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

void show_error(const char* message)
{
    PyErr_Print();

    ErrorSystemConfig c;
    errorSystemCreate(&c, message, message);
    errorSystemShow(&c);

    Py_Exit(1);
}

int main(int argc, char* argv[])
{
    setenv("MESA_NO_ERROR", "1", 1);

    appletLockExit();
    appletHook(&applet_hook_cookie, on_applet_hook, NULL);

    PyStatus status;
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    // ❗ ОЧЕНЬ ВАЖНО
    config.isolated = 1;
    config.use_environment = 0;
    config.site_import = 0;
    config.write_bytecode = 0;
    config.optimization_level = 2;

    status = PyConfig_SetString(&config,
                                &config.filesystem_encoding,
                                L"utf-8");
    if (PyStatus_Exception(status)) goto exception;

    status = PyConfig_SetString(&config,
                                &config.filesystem_errors,
                                L"surrogateescape");
    if (PyStatus_Exception(status)) goto exception;
    
    // 🔴 ВОТ ГДЕ ТЕПЕРЬ ЗАДАЁТСЯ PYTHONHOME
    PyConfig_SetString(&config, &config.home, L"romfs:/Contents/lib.zip");

    // argv
    PyConfig_SetString(&config, &config.program_name, L"renpy");

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

    PyImport_AppendInittab("_otrhlibnx", PyInit__otrhlibnx);
    PyImport_AppendInittab("pygame_sdl2.color", PyInit_pygame_sdl2_color);
    PyImport_AppendInittab("pygame_sdl2.controller", PyInit_pygame_sdl2_controller);
    PyImport_AppendInittab("pygame_sdl2.display", PyInit_pygame_sdl2_display);
    PyImport_AppendInittab("pygame_sdl2.draw", PyInit_pygame_sdl2_draw);
    PyImport_AppendInittab("pygame_sdl2.error", PyInit_pygame_sdl2_error);
    PyImport_AppendInittab("pygame_sdl2.event", PyInit_pygame_sdl2_event);
    PyImport_AppendInittab("pygame_sdl2.gfxdraw", PyInit_pygame_sdl2_gfxdraw);
    PyImport_AppendInittab("pygame_sdl2.image", PyInit_pygame_sdl2_image);
    PyImport_AppendInittab("pygame_sdl2.joystick", PyInit_pygame_sdl2_joystick);
    PyImport_AppendInittab("pygame_sdl2.key", PyInit_pygame_sdl2_key);
    PyImport_AppendInittab("pygame_sdl2.locals", PyInit_pygame_sdl2_locals);
    PyImport_AppendInittab("pygame_sdl2.mouse", PyInit_pygame_sdl2_mouse);
    PyImport_AppendInittab("pygame_sdl2.power", PyInit_pygame_sdl2_power);
    PyImport_AppendInittab("pygame_sdl2.pygame_time", PyInit_pygame_sdl2_pygame_time);
    PyImport_AppendInittab("pygame_sdl2.rect", PyInit_pygame_sdl2_rect);
    PyImport_AppendInittab("pygame_sdl2.render", PyInit_pygame_sdl2_render);
    PyImport_AppendInittab("pygame_sdl2.rwobject", PyInit_pygame_sdl2_rwobject);
    PyImport_AppendInittab("pygame_sdl2.scrap", PyInit_pygame_sdl2_scrap);
    PyImport_AppendInittab("pygame_sdl2.surface", PyInit_pygame_sdl2_surface);
    PyImport_AppendInittab("pygame_sdl2.transform", PyInit_pygame_sdl2_transform);

    // 🔥 СТАРТ PYTHON
    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    PyConfig_Clear(&config);

    PyRun_SimpleString(
        "import sys\n"
        "print(sys.prefix)\n"
        "print(sys.path)\n"
        "import encodings\n"
        "print('encodings OK')\n"
    );

    FILE* f = fopen("romfs:/Contents/renpy.py", "rb");
    if (!f)
        Py_Exit(1);

    PyRun_SimpleFile(f, "renpy.py");
    fclose(f);

    Py_Exit(0);
    return 0;
    exception:
    PyConfig_Clear(&config);
    if (PyStatus_IsExit(status)) {
        return status.exitcode;
    }
    show_error(status.err_msg);
    Py_ExitStatusException(status);
}
