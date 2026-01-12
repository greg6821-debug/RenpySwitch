#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include <switch.h>
#include <string.h>

u64 cur_progid = 0;
AccountUid userID = {0};

// Python 3.9 module initialization structure
static struct PyModuleDef otrh_libnx_module = {
    PyModuleDef_HEAD_INIT,
    "_otrhlibnx",       /* name of module */
    NULL,               /* module documentation, may be NULL */
    -1,                 /* size of per-interpreter state of the module,
                           or -1 if the module keeps state in global variables. */
    NULL                /* module methods */
};

static PyObject* commitsave(PyObject* self, PyObject* args)
{
    u64 total_size = 0;
    u64 free_size = 0;
    FsFileSystem* FsSave = fsdevGetDeviceFileSystem("save");

    if (FsSave == NULL) {
        Py_RETURN_NONE;
    }

    FsSaveDataInfoReader reader;
    FsSaveDataInfo info;
    s64 total_entries = 0;
    Result rc = 0;
    
    fsdevCommitDevice("save");
    fsFsGetTotalSpace(FsSave, "/", &total_size);
    fsFsGetFreeSpace(FsSave, "/", &free_size);
    
    if (free_size < 0x800000) {
        u64 new_size = total_size + 0x800000;

        fsdevUnmountDevice("save");
        fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);

        while(1) {
            rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &total_entries);
            if (R_FAILED(rc) || total_entries == 0) break;

            if (info.save_data_type == FsSaveDataType_Account && 
                userID.uid[0] == info.uid.uid[0] && 
                userID.uid[1] == info.uid.uid[1] && 
                info.application_id == cur_progid) {
                fsExtendSaveDataFileSystem(info.save_data_space_id, info.save_data_id, new_size, 0x400000);
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

static PyMethodDef myMethods[] = {
    { "commitsave", commitsave, METH_NOARGS, "Commit save data" },
    { "startboost", startboost, METH_NOARGS, "Start CPU boost" },
    { "disableboost", disableboost, METH_NOARGS, "Disable CPU boost" },
    { "restartprogram", restartprogram, METH_NOARGS, "Restart program" },
    { NULL, NULL, 0, NULL }
};

// Python 3.9 module initialization function
PyMODINIT_FUNC PyInit__otrhlibnx(void)
{
    return PyModule_Create(&otrh_libnx_module);
}

// Declare module initialization functions for Python 3.9
PyMODINIT_FUNC PyInit_pygame_sdl2_color(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_controller(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_display(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_draw(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_error(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_event(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_gfxdraw(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_image(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_joystick(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_key(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_locals(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_mouse(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_power(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_pygame_time(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_rect(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_render(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_rwobject(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_scrap(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_surface(void);
PyMODINIT_FUNC PyInit_pygame_sdl2_transform(void);

PyMODINIT_FUNC PyInit__renpy(void);
PyMODINIT_FUNC PyInit__renpybidi(void);
PyMODINIT_FUNC PyInit_renpy_audio_renpysound(void);
PyMODINIT_FUNC PyInit_renpy_display_accelerator(void);
PyMODINIT_FUNC PyInit_renpy_display_render(void);
PyMODINIT_FUNC PyInit_renpy_display_matrix(void);
PyMODINIT_FUNC PyInit_renpy_gl_gldraw(void);
PyMODINIT_FUNC PyInit_renpy_gl_glenviron_shader(void);
PyMODINIT_FUNC PyInit_renpy_gl_glrtt_copy(void);
PyMODINIT_FUNC PyInit_renpy_gl_glrtt_fbo(void);
PyMODINIT_FUNC PyInit_renpy_gl_gltexture(void);
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
PyMODINIT_FUNC PyInit_renpy_text_ftfont(void);
PyMODINIT_FUNC PyInit_renpy_text_textsupport(void);
PyMODINIT_FUNC PyInit_renpy_text_texwrap(void);

PyMODINIT_FUNC PyInit_renpy_compat_dictviews(void);
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

PyMODINIT_FUNC PyInit_renpy_lexersupport(void);
PyMODINIT_FUNC PyInit_renpy_display_quaternion(void);


static void register_builtin_modules(void)
{
// Python 3.9 builtin modules array
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

PyImport_AppendInittab("_renpy", PyInit__renpy);
PyImport_AppendInittab("_renpybidi", PyInit__renpybidi);
PyImport_AppendInittab("renpy.audio.renpysound", PyInit_renpy_audio_renpysound);
PyImport_AppendInittab("renpy.display.accelerator", PyInit_renpy_display_accelerator);
PyImport_AppendInittab("renpy.display.matrix", PyInit_renpy_display_matrix);
PyImport_AppendInittab("renpy.display.render", PyInit_renpy_display_render);
PyImport_AppendInittab("renpy.gl.gldraw", PyInit_renpy_gl_gldraw);
PyImport_AppendInittab("renpy.gl.glenviron_shader", PyInit_renpy_gl_glenviron_shader);
PyImport_AppendInittab("renpy.gl.glrtt_copy", PyInit_renpy_gl_glrtt_copy);
PyImport_AppendInittab("renpy.gl.glrtt_fbo", PyInit_renpy_gl_glrtt_fbo);
PyImport_AppendInittab("renpy.gl.gltexture", PyInit_renpy_gl_gltexture);
PyImport_AppendInittab("renpy.pydict", PyInit_renpy_pydict);
PyImport_AppendInittab("renpy.style", PyInit_renpy_style);
PyImport_AppendInittab("renpy.styledata.style_activate_functions", PyInit_renpy_styledata_style_activate_functions);
PyImport_AppendInittab("renpy.styledata.style_functions", PyInit_renpy_styledata_style_functions);
PyImport_AppendInittab("renpy.styledata.style_hover_functions", PyInit_renpy_styledata_style_hover_functions);
PyImport_AppendInittab("renpy.styledata.style_idle_functions", PyInit_renpy_styledata_style_idle_functions);
PyImport_AppendInittab("renpy.styledata.style_insensitive_functions", PyInit_renpy_styledata_style_insensitive_functions);
PyImport_AppendInittab("renpy.styledata.style_selected_activate_functions", PyInit_renpy_styledata_style_selected_activate_functions);
PyImport_AppendInittab("renpy.styledata.style_selected_functions", PyInit_renpy_styledata_style_selected_functions);
PyImport_AppendInittab("renpy.styledata.style_selected_hover_functions", PyInit_renpy_styledata_style_selected_hover_functions);
PyImport_AppendInittab("renpy.styledata.style_selected_idle_functions", PyInit_renpy_styledata_style_selected_idle_functions);
PyImport_AppendInittab("renpy.styledata.style_selected_insensitive_functions", PyInit_renpy_styledata_style_selected_insensitive_functions);
PyImport_AppendInittab("renpy.styledata.styleclass", PyInit_renpy_styledata_styleclass);
PyImport_AppendInittab("renpy.styledata.stylesets", PyInit_renpy_styledata_stylesets);
PyImport_AppendInittab("renpy.text.ftfont", PyInit_renpy_text_ftfont);
PyImport_AppendInittab("renpy.text.textsupport", PyInit_renpy_text_textsupport);
PyImport_AppendInittab("renpy.text.texwrap", PyInit_renpy_text_texwrap);

PyImport_AppendInittab("renpy.compat.dictviews", PyInit_renpy_compat_dictviews);
PyImport_AppendInittab("renpy.gl2.gl2draw", PyInit_renpy_gl2_gl2draw);
PyImport_AppendInittab("renpy.gl2.gl2mesh", PyInit_renpy_gl2_gl2mesh);
PyImport_AppendInittab("renpy.gl2.gl2mesh2", PyInit_renpy_gl2_gl2mesh2);
PyImport_AppendInittab("renpy.gl2.gl2mesh3", PyInit_renpy_gl2_gl2mesh3);
PyImport_AppendInittab("renpy.gl2.gl2model", PyInit_renpy_gl2_gl2model);
PyImport_AppendInittab("renpy.gl2.gl2polygon", PyInit_renpy_gl2_gl2polygon);
PyImport_AppendInittab("renpy.gl2.gl2shader", PyInit_renpy_gl2_gl2shader);
PyImport_AppendInittab("renpy.gl2.gl2texture", PyInit_renpy_gl2_gl2texture);
PyImport_AppendInittab("renpy.uguu.gl", PyInit_renpy_uguu_gl);
PyImport_AppendInittab("renpy.uguu.uguu", PyInit_renpy_uguu_uguu);
    
PyImport_AppendInittab("renpy.lexersupport", PyInit_renpy_lexersupport);
PyImport_AppendInittab("renpy.display.quaternion", PyInit_renpy_display_quaternion);
}

// Override the heap initialization function
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

    if (R_FAILED(rc) || addr == NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));

    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

Result createSaveData()
{
    NsApplicationControlData g_applicationControlData;
    size_t dummy;

    nsGetApplicationControlData(0x1, cur_progid, &g_applicationControlData, sizeof(g_applicationControlData), &dummy);

    FsSaveDataAttribute attr;
    memset(&attr, 0, sizeof(FsSaveDataAttribute));
    attr.application_id = cur_progid;
    attr.uid = userID;
    attr.system_save_data_id = 0;
    attr.save_data_type = FsSaveDataType_Account;
    attr.save_data_rank = 0;
    attr.save_data_index = 0;

    FsSaveDataCreationInfo crt;
    memset(&crt, 0, sizeof(FsSaveDataCreationInfo));
            
    crt.save_data_size = 0x800000;
    crt.journal_size = 0x400000;
    crt.available_size = 0x8000;
    crt.owner_id = g_applicationControlData.nacp.save_data_owner_id;
    crt.flags = 0;
    crt.save_data_space_id = FsSaveDataSpaceId_User;

    FsSaveDataMetaInfo meta = {0};

    return fsCreateSaveDataFileSystem(&attr, &crt, &meta);
}

void userAppInit()
{
    Result rc = 0;
    PselUserSelectionSettings settings;
    
    rc = svcGetInfo(&cur_progid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
    rc = accountInitialize(AccountServiceType_Application);
    rc = accountGetPreselectedUser(&userID);
    
    if (R_FAILED(rc)) {
        s32 count;
        accountGetUserCount(&count);

        if (count > 1) {
            pselShowUserSelector(&userID, &settings);
        } else {
            s32 loadedUsers;
            AccountUid account_ids[count];
            accountListAllUsers(account_ids, count, &loadedUsers);
            if (count > 0) {
                userID = account_ids[0];
            }
        }
    }

    if (accountUidIsValid(&userID)) {
        rc = fsdevMountSaveData("save", cur_progid, userID);
        if (R_FAILED(rc)) {
            rc = createSaveData();
            rc = fsdevMountSaveData("save", cur_progid, userID);
        }
    }

    romfsInit();
    socketInitializeDefault();
}

void userAppExit()
{
    fsdevCommitDevice("save");
    fsdevUnmountDevice("save");
    socketExit();
    romfsExit();
}

ConsoleRenderer* getDefaultConsoleRenderer(void)
{
    return NULL;
}

char python_error_buffer[0x400];

void show_error(const char* message, int exit_flag)
{
    if (exit_flag == 1) {
        Py_Finalize();
    }
    
    const char* first_line = message;
    char* end = strchr(message, '\n');
    if (end != NULL)
    {
        size_t len = (end - message) > sizeof(python_error_buffer) - 1 ? 
                     sizeof(python_error_buffer) - 1 : (end - message);
        memcpy(python_error_buffer, message, len);
        python_error_buffer[len] = '\0';
        first_line = python_error_buffer;
    }
    
    ErrorSystemConfig c;
    errorSystemCreate(&c, first_line, message);
    errorSystemShow(&c);
    
    if (exit_flag == 1) {
        Py_Exit(1);
    }
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

int main(int argc, char* argv[])
{
    setenv("MESA_NO_ERROR", "1", 1);
    setenv("PYTHONPYCACHEPREFIX", "save://__pycache__", 1);

    appletLockExit();
    appletHook(&applet_hook_cookie, on_applet_hook, NULL);

    // Python 3.9 initialization flags
    

    FILE* sysconfigdata_file = fopen("romfs:/Contents/lib.zip", "rb");
    FILE* renpy_file = fopen("romfs:/Contents/renpy.py", "rb");

    if (sysconfigdata_file == NULL)
    {
        show_error("Could not find lib.zip.\n\nPlease ensure that you have extracted the files correctly so that the \"lib.zip\" file is in the same directory as the nsp file.", 1);
    }

    if (renpy_file == NULL)
    {
        show_error("Could not find renpy.py.\n\nPlease ensure that you have extracted the files correctly so that the \"renpy.py\" file is in the same directory as the nsp file.", 1);
    }

    fclose(sysconfigdata_file);


    register_builtin_modules();
    
    
    
    // ===== Python 3.9 legacy embedding (CORRECT) =====

    // 1. Environment isolation
    setenv("PYTHONNOUSERSITE", "1", 1);
    setenv("PYTHONDONTWRITEBYTECODE", "1", 1);
    setenv("PYTHONOPTIMIZE", "2", 1);

    // 2. Program name
    wchar_t *program_name = Py_DecodeLocale("renpy-switch", NULL);
    Py_SetProgramName(program_name);

    // 3. Python home (lib.zip)
    wchar_t *python_home = Py_DecodeLocale("romfs:/Contents/lib.zip", NULL);
    Py_SetPythonHome(python_home);

    // 4. sys.path (CRITICAL)
    wchar_t *python_path = Py_DecodeLocale(
        "romfs:/Contents/lib.zip:"
        "romfs:/Contents",
        NULL
    );
    Py_SetPath(python_path);

    // 5. Initialize Python
    Py_Initialize();

    // 6. sys.argv with flags (-S -OO)
    wchar_t *argv_w[] = {
        program_name,
        L"-S",
        L"-OO",
    };
    PySys_SetArgv(3, argv_w);
    

    FILE* f = fopen("romfs:/Contents/renpy.py", "r");
    if (!f) {
        show_error("Could not open renpy.py", 1);
    }

    // Получаем длину файла
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Выделяем буфер
    char *source = malloc(fsize + 1);
    fread(source, 1, fsize, f);
    source[fsize] = '\0';
    fclose(f);

    // Компилируем код
    PyObject *compiled = Py_CompileString(source, "renpy.py", Py_file_input);
    free(source);

    if (!compiled) {
        PyErr_Print();
        show_error("Failed to compile renpy.py", 1);
    }

    // Выполняем в __main__ модуле
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *main_dict = PyModule_GetDict(main_module);

    PyObject *result = PyEval_EvalCode(compiled, main_dict, main_dict);
    Py_XDECREF(compiled);

    if (!result) {
        PyErr_Print();
        show_error("Failed to execute renpy.py", 1);
    }
    Py_XDECREF(result);


    Py_Finalize();
    PyMem_Free(program_name);
    PyMem_Free(python_home);
    PyMem_Free(python_path);
    return 0;
}
