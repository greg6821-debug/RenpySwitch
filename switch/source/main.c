#include <switch.h>
#include <Python.h>
#include <stdio.h>
#include <switch/services/audren.h>
#include <SDL2/SDL.h>

static AudioRendererConfig g_audren_config;
u64 cur_progid = 0;
AccountUid userID = {0};

// --------------------- Python Functions ---------------------

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

    if (free_size < 0x800000) {
        u64 new_size = total_size + 0x800000;
        fsdevUnmountDevice("save");

        fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);
        while (1) {
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

// --------------------- Heap Initialization ---------------------

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

// --------------------- SaveData ---------------------

Result createSaveData()
{
    NsApplicationControlData g_applicationControlData;
    size_t dummy;
    nsGetApplicationControlData(0x1, cur_progid, &g_applicationControlData, sizeof(g_applicationControlData), &dummy);

    FsSaveDataAttribute attr = {0};
    attr.application_id = cur_progid;
    attr.uid = userID;
    attr.system_save_data_id = 0;
    attr.save_data_type = FsSaveDataType_Account;
    attr.save_data_rank = 0;
    attr.save_data_index = 0;

    FsSaveDataCreationInfo crt = {0};
    crt.save_data_size = 0x800000;
    crt.journal_size = 0x400000;
    crt.available_size = 0x8000;
    crt.owner_id = g_applicationControlData.nacp.save_data_owner_id;
    crt.flags = 0;
    crt.save_data_space_id = FsSaveDataSpaceId_User;

    FsSaveDataMetaInfo meta = {};
    return fsCreateSaveDataFileSystem(&attr, &crt, &meta);
}

// --------------------- App Init & Exit ---------------------

void userAppInit()
{
    romfsInit();

    // ----------------- SDL Setup -----------------
#ifdef __SWITCH__
    setenv("SDL_AUDIODRIVER", "audren", 1);
#else
    setenv("SDL_AUDIODRIVER", "dummy", 1); // для эмуляторов
#endif
    setenv("SDL_VIDEODRIVER", "switch", 1);
    setenv("SDL_AUDIO_FREQUENCY", "48000", 1);
    setenv("SDL_AUDIO_CHANNELS", "2", 1);
    setenv("SDL_AUDIO_SAMPLES", "1024", 1);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

#ifdef __SWITCH__
    // Audren initialization
    memset(&g_audren_config, 0, sizeof(g_audren_config));
    g_audren_config.num_voices = 32;
    g_audren_config.num_sinks = 1;
    g_audren_config.output_rate = AudioRendererOutputRate_48kHz;

    if (R_SUCCEEDED(audrenInitialize(&g_audren_config)))
        audrenStartAudioRenderer();

    audoutInitialize();
    audoutStartAudioOut();
#endif

    // ----------------- SaveData & Account -----------------
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

    socketInitializeDefault();
}

void userAppExit()
{
    fsdevCommitDevice("save");
    fsdevUnmountDevice("save");

#ifdef __SWITCH__
    audoutStopAudioOut();
    audoutExit();
    audrenStopAudioRenderer();
    audrenExit();
#endif

    SDL_Quit();
    socketExit();
    romfsExit();
}

// --------------------- Applet Hook ---------------------

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

// --------------------- Main ---------------------

int main(int argc, char* argv[])
{
    setenv("MESA_NO_ERROR", "1", 1);
    appletLockExit();
    appletHook(&applet_hook_cookie, on_applet_hook, NULL);

    Py_NoSiteFlag = 1;
    Py_IgnoreEnvironmentFlag = 1;
    Py_NoUserSiteDirectory = 1;
    Py_DontWriteBytecodeFlag = 1;
    Py_OptimizeFlag = 2;

    init_otrh_libnx();

    FILE* sysconfigdata_file = fopen("romfs:/Contents/lib.zip", "rb");
    FILE* renpy_file = fopen("romfs:/Contents/renpy.py", "rb");

    if (!sysconfigdata_file)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));

    if (!renpy_file)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));

    fclose(sysconfigdata_file);

    Py_InitializeEx(0);
    Py_SetPythonHome("romfs:/Contents");

    char* pyargs[] = { "romfs:/Contents/renpy.py", NULL };
    PySys_SetArgvEx(1, pyargs, 1);
    PyRun_SimpleString("import sys\nsys.path = ['romfs:/Contents/lib.zip']");

    int python_result = PyRun_SimpleFileEx(renpy_file, "romfs:/Contents/renpy.py", 1);
    if (python_result == -1) {
        Py_Exit(1);
    }

    Py_Exit(0);
    return 0;
}
