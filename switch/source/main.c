#include <switch.h>
#include <Python.h>
#include <stdio.h>


u32 __nx_applet_exit_mode = 1;
extern u32 __nx_applet_type;
extern size_t __nx_heap_size;

#if 0
PyMODINIT_FUNC init_libnx();
#endif

#if 0
PyMODINIT_FUNC initpygame_sdl2_font();
#endif


#if 0
PyMODINIT_FUNC initpygame_sdl2_mixer();
PyMODINIT_FUNC initpygame_sdl2_mixer_music();
#endif

#if 0
PyMODINIT_FUNC initrenpy_gl2_gl2draw();
PyMODINIT_FUNC initrenpy_gl2_gl2environ_shader();
PyMODINIT_FUNC initrenpy_gl2_gl2geometry();
PyMODINIT_FUNC initrenpy_gl2_gl2rtt_fbo();
PyMODINIT_FUNC initrenpy_gl2_gl2shader();
PyMODINIT_FUNC initrenpy_gl2_gl2texture();
PyMODINIT_FUNC initrenpy_gl2_uguu();
PyMODINIT_FUNC initrenpy_gl2_uguugl();
#endif

u64 cur_progid = 0;
AccountUid userID={0};

static PyObject* commitsave(PyObject* self, PyObject* args)
{
    u64 total_size = 0;
    u64 free_size = 0;
    FsFileSystem* FsSave = fsdevGetDeviceFileSystem("save");

    FsSaveDataInfoReader reader;
    FsSaveDataInfo info;
    s64 total_entries=0;
    Result rc=0;
    
    fsdevCommitDevice("save");
    fsFsGetTotalSpace(FsSave, "/", &total_size);
    fsFsGetFreeSpace(FsSave, "/", &free_size);
    if (free_size < 0x800000) {
        u64 new_size = total_size + 0x800000;

        fsdevUnmountDevice("save");
        fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);

        while(1) {
            rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &total_entries);
            if (R_FAILED(rc) || total_entries==0) break;

            if (info.save_data_type == FsSaveDataType_Account && userID.uid[0] == info.uid.uid[0] && userID.uid[1] == info.uid.uid[1] && info.application_id == cur_progid) {
                fsExtendSaveDataFileSystem(info.save_data_space_id, info.save_data_id, new_size, 0x400000);
                break;
            }
        }

        fsSaveDataInfoReaderClose(&reader);
        fsdevMountSaveData("save", cur_progid, userID);

    }
    return Py_None;
}

static PyObject* startboost(PyObject* self, PyObject* args)
{
    appletSetCpuBoostMode(ApmPerformanceMode_Boost);
    return Py_None;
}

static PyObject* disableboost(PyObject* self, PyObject* args)
{
    appletSetCpuBoostMode(ApmPerformanceMode_Normal);
    return Py_None;
}

static PyObject* restartprogram(PyObject* self, PyObject* args)
{
    appletRestartProgram(NULL, 0);
    return Py_None;
}

static PyMethodDef myMethods[] = {
    { "commitsave", commitsave, METH_NOARGS, "commitsave" },
    { "startboost", startboost, METH_NOARGS, "startboost" },
    { "disableboost", disableboost, METH_NOARGS, "disableboost" },
    { "restartprogram", restartprogram, METH_NOARGS, "restartprogram" },
    { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC init_otrh_libnx(void)
{
    Py_InitModule("_otrhlibnx", myMethods);
}

PyMODINIT_FUNC initpygame_sdl2_color();
PyMODINIT_FUNC initpygame_sdl2_controller();
PyMODINIT_FUNC initpygame_sdl2_display();
PyMODINIT_FUNC initpygame_sdl2_draw();
PyMODINIT_FUNC initpygame_sdl2_error();
PyMODINIT_FUNC initpygame_sdl2_event();
PyMODINIT_FUNC initpygame_sdl2_gfxdraw();
PyMODINIT_FUNC initpygame_sdl2_image();
PyMODINIT_FUNC initpygame_sdl2_joystick();
PyMODINIT_FUNC initpygame_sdl2_key();
PyMODINIT_FUNC initpygame_sdl2_locals();
PyMODINIT_FUNC initpygame_sdl2_mouse();
PyMODINIT_FUNC initpygame_sdl2_power();
PyMODINIT_FUNC initpygame_sdl2_pygame_time();
PyMODINIT_FUNC initpygame_sdl2_rect();
PyMODINIT_FUNC initpygame_sdl2_render();
PyMODINIT_FUNC initpygame_sdl2_rwobject();
PyMODINIT_FUNC initpygame_sdl2_scrap();
PyMODINIT_FUNC initpygame_sdl2_surface();
PyMODINIT_FUNC initpygame_sdl2_transform();

PyMODINIT_FUNC init_renpy();
PyMODINIT_FUNC init_renpybidi();
PyMODINIT_FUNC initrenpy_audio_renpysound();
PyMODINIT_FUNC initrenpy_display_accelerator();
PyMODINIT_FUNC initrenpy_display_render();
PyMODINIT_FUNC initrenpy_display_matrix();
PyMODINIT_FUNC initrenpy_gl_gl();
PyMODINIT_FUNC initrenpy_gl_gldraw();
PyMODINIT_FUNC initrenpy_gl_glenviron_shader();
PyMODINIT_FUNC initrenpy_gl_glrtt_copy();
PyMODINIT_FUNC initrenpy_gl_glrtt_fbo();
PyMODINIT_FUNC initrenpy_gl_gltexture();
PyMODINIT_FUNC initrenpy_pydict();
PyMODINIT_FUNC initrenpy_parsersupport();
PyMODINIT_FUNC initrenpy_style();
PyMODINIT_FUNC initrenpy_styledata_style_activate_functions();
PyMODINIT_FUNC initrenpy_styledata_style_functions();
PyMODINIT_FUNC initrenpy_styledata_style_hover_functions();
PyMODINIT_FUNC initrenpy_styledata_style_idle_functions();
PyMODINIT_FUNC initrenpy_styledata_style_insensitive_functions();
PyMODINIT_FUNC initrenpy_styledata_style_selected_activate_functions();
PyMODINIT_FUNC initrenpy_styledata_style_selected_functions();
PyMODINIT_FUNC initrenpy_styledata_style_selected_hover_functions();
PyMODINIT_FUNC initrenpy_styledata_style_selected_idle_functions();
PyMODINIT_FUNC initrenpy_styledata_style_selected_insensitive_functions();
PyMODINIT_FUNC initrenpy_styledata_styleclass();
PyMODINIT_FUNC initrenpy_styledata_stylesets();
PyMODINIT_FUNC initrenpy_text_ftfont();
PyMODINIT_FUNC initrenpy_text_textsupport();
PyMODINIT_FUNC initrenpy_text_texwrap();

PyMODINIT_FUNC initrenpy_compat_dictviews();
PyMODINIT_FUNC initrenpy_gl2_gl2draw();
PyMODINIT_FUNC initrenpy_gl2_gl2mesh();
PyMODINIT_FUNC initrenpy_gl2_gl2mesh2();
PyMODINIT_FUNC initrenpy_gl2_gl2mesh3();
PyMODINIT_FUNC initrenpy_gl2_gl2model();
PyMODINIT_FUNC initrenpy_gl2_gl2polygon();
PyMODINIT_FUNC initrenpy_gl2_gl2shader();
PyMODINIT_FUNC initrenpy_gl2_gl2texture();
PyMODINIT_FUNC initrenpy_uguu_gl();
PyMODINIT_FUNC initrenpy_uguu_uguu();

// Overide the heap initialization function.
void __libnx_initheap(void)
{
    //void* addr = NULL;
    //u64 size = 0;
    //u64 mem_available = 0, mem_used = 0;
    void*  addr;
    size_t size = 0;
    size_t mem_available = 0, mem_used = 0;
    const size_t max_mem = 0x18000000;

    if (envHasHeapOverride()) {
        addr = envGetHeapOverrideAddr();
        size = envGetHeapOverrideSize();
    }
    else {
        if (__nx_heap_size==0) {
            svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
            svcGetInfo(&mem_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
            if (mem_available > mem_used+0x200000)
                size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;
            if (size==0)
                size = 0x2000000*16;
        }
        else {
            size = __nx_heap_size;
        }

        if (size > max_mem)
        {
            size = max_mem;
        }
    }

    svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&mem_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);

    if (mem_available > mem_used+0x200000)
        size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;
    if (size == 0)
        size = 0x2000000*16;

    Result rc = svcSetHeapSize(&addr, size);
    if (R_FAILED(rc) || addr==NULL)
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

    FsSaveDataMetaInfo meta={};

    return fsCreateSaveDataFileSystem(&attr, &crt, &meta);
}

void userAppInit()
{
    // fsdevUnmountAll();

    Result rc=0;
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
            size_t loadedUsers;
            AccountUid account_ids[count];
            accountListAllUsers(account_ids, count, &loadedUsers);
            userID = account_ids[0];
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

    nxlinkStdio();
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


char relative_dir_path[0x400];
char sysconfigdata_file_path[0x400];
char python_home_buffer[0x400];
char python_snprintf_buffer[0x400];
char python_script_buffer[0x400];

char python_error_buffer[0x400];

void show_error(const char* message, int exit)
{
    if (exit == 1) {
        Py_Finalize();
    }
    char* first_line = (char*)message;
    char* end = strchr(message, '\n');
    if (end != NULL)
    {
        first_line = python_error_buffer;
        memcpy(first_line, message, (end - message) > sizeof(python_error_buffer) ? sizeof(python_error_buffer) : (end - message));
        first_line[end - message] = '\0';
    }
    ErrorSystemConfig c;
    errorSystemCreate(&c, (const char*)first_line, message);
    errorSystemShow(&c);
    if (exit == 1) {
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


    if (__nx_applet_type != AppletType_Application)
    {
#if 0
        show_error("Only application override is supported by this program.\n\nTo run this program as application override, hold down the R button while launching an application on the menu.",1);
#endif
#if 1
        setenv("RENPY_LESS_MEMORY", "1", 1);
#endif
    }
    
    
    appletLockExit();
    appletHook(&applet_hook_cookie, on_applet_hook, NULL);

    Py_NoSiteFlag = 1;
    Py_IgnoreEnvironmentFlag = 1;
    Py_NoUserSiteDirectory = 1;
    Py_DontWriteBytecodeFlag = 1;
    Py_OptimizeFlag = 2;

    static struct _inittab builtins[] = {

#if 0
        {"_libnx", init_libnx},
#endif
#if 0
        {"pygame_sdl2.font", initpygame_sdl2_font},
#endif

#if 0
        {"pygame_sdl2.mixer", initpygame_sdl2_mixer},
        {"pygame_sdl2.mixer_music", initpygame_sdl2_mixer_music},
#endif
#if 0
        {"renpy.gl2.gl2draw", initrenpy_gl2_gl2draw},
        {"renpy.gl2.gl2geometry", initrenpy_gl2_gl2geometry},
        {"renpy.gl2.gl2shader", initrenpy_gl2_gl2shader},
        {"renpy.gl2.gl2texture", initrenpy_gl2_gl2texture},
        {"renpy.gl2.uguu", initrenpy_gl2_uguu},
        {"renpy.gl2.uguugl", initrenpy_gl2_uguugl},
#endif
    
        {"_otrhlibnx", init_otrh_libnx},

        {"pygame_sdl2.color", initpygame_sdl2_color},
        {"pygame_sdl2.controller", initpygame_sdl2_controller},
        {"pygame_sdl2.display", initpygame_sdl2_display},
        {"pygame_sdl2.draw", initpygame_sdl2_draw},
        {"pygame_sdl2.error", initpygame_sdl2_error},
        {"pygame_sdl2.event", initpygame_sdl2_event},
        {"pygame_sdl2.gfxdraw", initpygame_sdl2_gfxdraw},
        {"pygame_sdl2.image", initpygame_sdl2_image},
        {"pygame_sdl2.joystick", initpygame_sdl2_joystick},
        {"pygame_sdl2.key", initpygame_sdl2_key},
        {"pygame_sdl2.locals", initpygame_sdl2_locals},
        {"pygame_sdl2.mouse", initpygame_sdl2_mouse},
        {"pygame_sdl2.power", initpygame_sdl2_power},
        {"pygame_sdl2.pygame_time", initpygame_sdl2_pygame_time},
        {"pygame_sdl2.rect", initpygame_sdl2_rect},
        {"pygame_sdl2.render", initpygame_sdl2_render},
        {"pygame_sdl2.rwobject", initpygame_sdl2_rwobject},
        {"pygame_sdl2.scrap", initpygame_sdl2_scrap},
        {"pygame_sdl2.surface", initpygame_sdl2_surface},
        {"pygame_sdl2.transform", initpygame_sdl2_transform},

        {"_renpy", init_renpy},
        {"_renpybidi", init_renpybidi},
        {"renpy.audio.renpysound", initrenpy_audio_renpysound},
        {"renpy.display.accelerator", initrenpy_display_accelerator},
        {"renpy.display.matrix", initrenpy_display_matrix},
        {"renpy.display.render", initrenpy_display_render},
        {"renpy.gl.gldraw", initrenpy_gl_gldraw},
        {"renpy.gl.glenviron_shader", initrenpy_gl_glenviron_shader},
        {"renpy.gl.glrtt_copy", initrenpy_gl_glrtt_copy},
        {"renpy.gl.glrtt_fbo", initrenpy_gl_glrtt_fbo},
        {"renpy.gl.gltexture", initrenpy_gl_gltexture},
        {"renpy.pydict", initrenpy_pydict},
        {"renpy.arsersupport", initrenpy_parsersupport},
        {"renpy.style", initrenpy_style},
        {"renpy.styledata.style_activate_functions", initrenpy_styledata_style_activate_functions},
        {"renpy.styledata.style_functions", initrenpy_styledata_style_functions},
        {"renpy.styledata.style_hover_functions", initrenpy_styledata_style_hover_functions},
        {"renpy.styledata.style_idle_functions", initrenpy_styledata_style_idle_functions},
        {"renpy.styledata.style_insensitive_functions", initrenpy_styledata_style_insensitive_functions},
        {"renpy.styledata.style_selected_activate_functions", initrenpy_styledata_style_selected_activate_functions},
        {"renpy.styledata.style_selected_functions", initrenpy_styledata_style_selected_functions},
        {"renpy.styledata.style_selected_hover_functions", initrenpy_styledata_style_selected_hover_functions},
        {"renpy.styledata.style_selected_idle_functions", initrenpy_styledata_style_selected_idle_functions},
        {"renpy.styledata.style_selected_insensitive_functions", initrenpy_styledata_style_selected_insensitive_functions},
        {"renpy.styledata.styleclass", initrenpy_styledata_styleclass},
        {"renpy.styledata.stylesets", initrenpy_styledata_stylesets},
        {"renpy.text.ftfont", initrenpy_text_ftfont},
        {"renpy.text.textsupport", initrenpy_text_textsupport},
        {"renpy.text.texwrap", initrenpy_text_texwrap},

        {"renpy.compat.dictviews", initrenpy_compat_dictviews},
        {"renpy.gl2.gl2draw", initrenpy_gl2_gl2draw},
        {"renpy.gl2.gl2mesh", initrenpy_gl2_gl2mesh},
        {"renpy.gl2.gl2mesh2", initrenpy_gl2_gl2mesh2},
        {"renpy.gl2.gl2mesh3", initrenpy_gl2_gl2mesh3},
        {"renpy.gl2.gl2model", initrenpy_gl2_gl2model},
        {"renpy.gl2.gl2polygon", initrenpy_gl2_gl2polygon},
        {"renpy.gl2.gl2shader", initrenpy_gl2_gl2shader},
        {"renpy.gl2.gl2texture", initrenpy_gl2_gl2texture},
        {"renpy.uguu.gl", initrenpy_uguu_gl},
        {"renpy.uguu.uguu", initrenpy_uguu_uguu},


        {NULL, NULL}
    };



if (argc != 1)
    {
        show_error("Only one argument (the program itself) should be passed to the program.\n\nPlease use hbmenu to run this program.",1);
    }

    if (strchr(argv[0], ' '))
    {
        show_error("No spaces should be contained in the program path.\n\nPlease remove spaces from the program path.",1);
    }

    if (!strchr(argv[0], ':'))
    {
        show_error("Program path does not appear to be an absolute path.\n\nPlease use hbmenu to run this program.",1);
    }

    char* last_dir_separator = strrchr(argv[0], '/');

    if (last_dir_separator)
    {
        size_t dirpath_size = last_dir_separator - argv[0];
        memcpy(relative_dir_path, argv[0], dirpath_size);
        relative_dir_path[dirpath_size] = '\000';
    }
    else
    {
        getcwd(relative_dir_path, sizeof(relative_dir_path));
    }

    char* dir_paths[] = {
        "romfs:/Contents",
        relative_dir_path,
        NULL,
    };

    int found_sysconfigdata = 0;
    int found_renpy = 0;

    for (int i = 0; i < sizeof(dir_paths); i += 1)
        {
        if (dir_paths[i] == NULL)
        {
            break;
        }
        snprintf(sysconfigdata_file_path, sizeof(sysconfigdata_file_path), "%s/lib.zip", dir_paths[i]);
        FILE* sysconfigdata_file = fopen((const char*)sysconfigdata_file_path, "rb");
        if (sysconfigdata_file != NULL)
        {
            found_sysconfigdata = 1;
            fclose(sysconfigdata_file);
        }

        snprintf(python_script_buffer, sizeof(python_script_buffer), "%s/renpy.py", dir_paths[i]);
        FILE* renpy_file = fopen((const char*)python_script_buffer, "rb");
        if (renpy_file != NULL)
        {
            found_renpy = 1;
            fclose(renpy_file);
        }

        if (found_sysconfigdata == 1 && found_renpy == 1)
        {
            snprintf(python_home_buffer, sizeof(python_home_buffer), "%s/lib.zip", dir_paths[i]);
            snprintf(python_snprintf_buffer, sizeof(python_snprintf_buffer), "import sys\nsys.path = ['%s/lib.zip']", dir_paths[i]);
            Py_SetPythonHome(python_home_buffer);
            break;
        }
    }

    
    

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
    Py_InitializeEx(0);
    Py_SetPythonHome("romfs:/Contents/lib.zip");
    PyImport_ExtendInittab(builtins);

    char* pyargs[] = {
        "romfs:/Contents/renpy.py",
        NULL,
    };

    PySys_SetArgvEx(1, pyargs, 1);

    int python_result;

    python_result = PyRun_SimpleString("import sys\nsys.path = ['romfs:/Contents/lib.zip']");

    if (python_result == -1)
    {
        show_error("Could not set the Python path.\n\nThis is an internal error and should not occur during normal usage.", 1);
    }
    
#define x(lib) \
    { \
        if (PyRun_SimpleString("import " lib) == -1) \
        { \
            show_error("Could not import python library " lib ".\n\nPlease ensure that you have extracted the files correctly so that the \"lib\" folder is in the same directory as the nsp file, and that the \"lib\" folder contains the folder \"python2.7\". \nInside that folder, the file \"" lib ".py\" or folder \"" lib "\" needs to exist.", 1); \
        } \
    }

    x("os");
    x("pygame_sdl2");
    x("encodings");

#undef x

    python_result = PyRun_SimpleFileEx(renpy_file, "romfs:/Contents/renpy.py", 1);

    if (python_result == -1)
    {
        show_error("An uncaught Python exception occurred during renpy.py execution.\n\nPlease look in the save:// folder for more information about this exception.", 1);
    }

    Py_Exit(0);
    return 0;
}
