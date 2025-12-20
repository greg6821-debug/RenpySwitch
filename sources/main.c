#include <switch.h>
#include <Python.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

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
    if (fsdevGetDeviceFileSystem("save"))
        fsdevCommitDevice("save");

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
        size = 0x2000000 * 8; // 256 MB fallback

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

/* -------------------------------------------------------
   Main
------------------------------------------------------- */

#include <dirent.h>  // Для opendir, closedir, readdir
#include <sys/stat.h> // Для fstat
int main(int argc, char* argv[])
{
    setenv("MESA_NO_ERROR", "1", 1);
    
    // 1. Initialize ROMFS
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        char err_msg[64];
        snprintf(err_msg, sizeof(err_msg), "romfsInit failed: 0x%08x", rc);
        show_error(err_msg);
    }
    
    // 2. Проверки ROMFS
    const char* dir_path = "romfs:/Contents/lib/python3.9/encodings";
    DIR* dir = opendir(dir_path);
    if (!dir) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "Failed to open directory %s", dir_path);
        show_error(err_msg);
    }
    struct dirent* entry = readdir(dir);
    if (!entry) {
        closedir(dir);
        show_error("Directory encodings is empty");
    }
    closedir(dir);
    
    const char* test_file = "romfs:/Contents/lib/python3.9/encodings/__init__.py";
    FILE* test = fopen(test_file, "rb");
    if (!test) {
        show_error("Cannot open __init__.py");
    }
    
    struct stat st;
    if (fstat(fileno(test), &st) != 0 || st.st_size <= 0) {
        fclose(test);
        show_error("__init__.py is empty or stat failed");
    }
    
    char buffer[10];
    if (fread(buffer, 1, sizeof(buffer), test) == 0) {
        fclose(test);
        show_error("Failed to read from __init__.py");
    }
    fclose(test);
    
    // 3. Builtin modules
    static struct _inittab builtins[] = {
        {"_nx", PyInit__nx},
        {"_otrhlibnx", PyInit__otrhlibnx},
        {NULL, NULL}
    };
    PyImport_ExtendInittab(builtins);
    
    // 4. PyConfig
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    
    config.isolated = 1;
    config.use_environment = 0;
    config.site_import = 0;
    config.write_bytecode = 0;
    config.verbose = 2;  // Оставляем verbose для отладки
    
    // Кодировки - КРИТИЧЕСКИ ВАЖНО
    PyConfig_SetString(&config, &config.filesystem_encoding, L"utf-8");
    PyConfig_SetString(&config, &config.filesystem_errors, L"surrogateescape");
    PyConfig_SetString(&config, &config.stdio_encoding, L"utf-8");
    
    // Основные пути
    PyConfig_SetString(&config, &config.home, L"romfs:/Contents");
    PyConfig_SetString(&config, &config.program_name, L"python3");
    PyConfig_SetString(&config, &config.prefix, L"romfs:/Contents");
    PyConfig_SetString(&config, &config.exec_prefix, L"romfs:/Contents");
    
    // sys.path - КРИТИЧЕСКОЕ ИЗМЕНЕНИЕ!
    // 1. Уберите python39.zip если его нет
    // PyWideStringList_Append(&config.module_search_paths, L"romfs:/python39.zip");
    
    // 2. ГЛАВНОЕ: Добавьте основной путь к стандартной библиотеке
    PyWideStringList_Append(&config.module_search_paths, L"romfs:/Contents/lib/python3.9");
    
    // 3. lib-dynload для C-расширений
    PyWideStringList_Append(&config.module_search_paths, L"romfs:/Contents/lib/python3.9/lib-dynload");
    
    // 4. Не добавляйте romfs:/Contents в sys.path - это мешает поиску пакетов
    // PyWideStringList_Append(&config.module_search_paths, L"romfs:/Contents");
    
    config.module_search_paths_set = 1;
    
    // argv
    wchar_t* pyargv[] = { L"renpy.py", NULL };
    PyConfig_SetArgv(&config, 1, pyargv);
    
    // 5. Инициализация Python
    PyStatus status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }
    
    // 6. Отладочный вывод sys.path
    PyRun_SimpleString(
        "import sys\n"
        "print('=== Python Initialized ===')\n"
        "print('Python version:', sys.version)\n"
        "print('Platform:', sys.platform)\n"
        "print('Filesystem encoding:', sys.getfilesystemencoding())\n"
        "print('\\nPython paths:')\n"
        "for p in sys.path:\n"
        "    print('  ', p)\n"
        "\n"
        "print('\\nTrying to import encodings...')\n"
        "try:\n"
        "    import encodings\n"
        "    print('SUCCESS: encodings imported from:', encodings.__file__)\n"
        "except Exception as e:\n"
        "    print('FAILED:', e)\n"
    );
    
    // 7. Запуск Ren'Py
    FILE* f = fopen("romfs:/Contents/renpy.py", "rb");
    if (!f) {
        show_error("Could not find renpy.py");
    }
    if (PyRun_SimpleFileEx(f, "renpy.py", 1) != 0) {
        show_error("Ren'Py execution failed");
    }
    
    Py_Finalize();
    return 0;
}
