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

int main(int argc, char* argv[])
{
    setenv("MESA_NO_ERROR", "1", 1);
    
    // 1. Initialize ROMFS и проверка
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        char err_msg[64];
        snprintf(err_msg, sizeof(err_msg), "romfsInit failed with code: 0x%08x", rc);
        show_error(err_msg);
    }
    
    // 4. Проверка stat файла
    struct stat st;
    if (fstat(fileno(test), &st) != 0) {
        fclose(test);
        show_error("fstat failed on encodings/__init__.py");
    }
    if (st.st_size <= 0) {
        fclose(test);
        show_error("encodings/__init__.py is empty or size=0");
    }
    
    // 5. Проверка чтения содержимого файла
    char buffer[10];  // Читаем первые 10 байт для теста
    size_t read_bytes = fread(buffer, 1, sizeof(buffer), test);
    if (read_bytes == 0) {  // Если ничего не прочитано — ошибка
        fclose(test);
        show_error("Failed to read from encodings/__init__.py");
    }
    fclose(test);
    
    // Если все тесты прошли, можно добавить лог (если есть консоль)
    // printf("All file checks passed!\n");
    
    /* ---- builtin modules ---- */
    static struct _inittab builtins[] = {
        {"_nx", PyInit__nx},
        {"_otrhlibnx", PyInit__otrhlibnx},
        {NULL, NULL}
    };
    PyImport_ExtendInittab(builtins);
    
    /* ---- PyConfig ---- */
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    config.isolated = 1;
    config.use_environment = 0;
    config.site_import = 0;
    config.write_bytecode = 0;
    config.verbose = 2;  // Включаем verbose для детальных логов импортов
    
    /* PYTHONHOME */
    PyConfig_SetString(&config, &config.home, L"romfs:/Contents");
    
    /* program_name / sys.executable */
    PyConfig_SetString(&config, &config.program_name, L"python3");

    PyConfig_SetString(&config, &config.filesystem_encoding, L"utf-8");
    PyConfig_SetString(&config, &config.filesystem_errors, L"surrogatepass");
    
    /* sys.prefix / sys.exec_prefix */
    PyConfig_SetString(&config, &config.prefix, L"romfs:/Contents");
    PyConfig_SetString(&config, &config.exec_prefix, L"romfs:/Contents");
    
    /* sys.path */
    PyWideStringList_Append(
    &config.module_search_paths,
    L"romfs:/python39.zip"
    );
    PyWideStringList_Append(&config.module_search_paths,
                            L"romfs:/Contents/lib/python3.9/lib-dynload");
    PyWideStringList_Append(&config.module_search_paths,
                            L"romfs:/Contents");
    config.module_search_paths_set = 1;
    
    /* argv */
    wchar_t* pyargv[] = {
        L"renpy.py", // Имя скрипта, НЕ полный путь
        NULL
    };
    PyConfig_SetArgv(&config, 1, pyargv);
    
    /* ---- init Python ---- */
    PyStatus status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }
    
    /* ---- mark platform ---- */
    PyObject* renpy = PyImport_ImportModule("renpy");
    if (renpy) {
        PyObject* v = PyBool_FromLong(1);
        PyObject_SetAttrString(renpy, "switch", v);
        Py_DECREF(v);
        Py_DECREF(renpy);
    }
    
    // 5. Инициализация Python
    PyStatus status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }
    
    // 6. Тестовый запуск Python
    PyRun_SimpleString(
        "import sys\n"
        "print('=== Python Initialized ===')\n"
        "print('Version:', sys.version)\n"
        "print('Platform:', sys.platform)\n"
        "print('Paths:', sys.path)\n"
        "print('Encodings fs:', sys.getfilesystemencoding())\n"
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
    // romfsExit(); если нужно (обычно не обязательно)
    return 0;
}
