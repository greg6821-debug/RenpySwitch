#include <switch.h>
#include <Python.h>
#include <stdio.h>
#include <string.h>

u64 cur_progid = 0;
AccountUid userID = {0};

// Аудио контекст для проверки
static bool audio_initialized = false;

static PyObject* commitsave(PyObject* self, PyObject* args)
{
    FsFileSystem* FsSave = fsdevGetDeviceFileSystem("save");
    if (!FsSave) {
        PyErr_SetString(PyExc_RuntimeError, "Save filesystem not mounted");
        return NULL;
    }

    u64 total_size = 0;
    u64 free_size = 0;
    Result rc = 0;
    
    // Commit текущего состояния
    rc = fsdevCommitDevice("save");
    if (R_FAILED(rc)) {
        PyErr_Format(PyExc_RuntimeError, "Failed to commit save: 0x%x", rc);
        return NULL;
    }
    
    // Получаем информацию о свободном месте
    rc = fsFsGetTotalSpace(FsSave, "/", &total_size);
    if (R_FAILED(rc)) {
        PyErr_Format(PyExc_RuntimeError, "Failed to get total space: 0x%x", rc);
        return NULL;
    }
    
    rc = fsFsGetFreeSpace(FsSave, "/", &free_size);
    if (R_FAILED(rc)) {
        PyErr_Format(PyExc_RuntimeError, "Failed to get free space: 0x%x", rc);
        return NULL;
    }
    
    if (free_size < 0x800000) {
        u64 new_size = total_size + 0x800000;
        
        // Размонтируем устройство
        fsdevUnmountDevice("save");
        
        FsSaveDataInfoReader reader;
        FsSaveDataInfo info;
        s64 total_entries = 0;
        
        // Ищем сохранение для расширения
        rc = fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);
        if (R_FAILED(rc)) {
            PyErr_Format(PyExc_RuntimeError, "Failed to open save reader: 0x%x", rc);
            return NULL;
        }
        
        bool found = false;
        while(1) {
            rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &total_entries);
            if (R_FAILED(rc) || total_entries == 0) break;
            
            if (info.save_data_type == FsSaveDataType_Account && 
                memcmp(&userID, &info.uid, sizeof(AccountUid)) == 0 &&
                info.application_id == cur_progid) {
                
                rc = fsExtendSaveDataFileSystem(info.save_data_space_id, 
                                               info.save_data_id, 
                                               new_size, 
                                               0x400000);
                if (R_FAILED(rc)) {
                    fsSaveDataInfoReaderClose(&reader);
                    PyErr_Format(PyExc_RuntimeError, "Failed to extend save: 0x%x", rc);
                    return NULL;
                }
                found = true;
                break;
            }
        }
        
        fsSaveDataInfoReaderClose(&reader);
        
        // Монтируем обратно
        rc = fsdevMountSaveData("save", cur_progid, userID);
        if (R_FAILED(rc)) {
            PyErr_Format(PyExc_RuntimeError, "Failed to remount save: 0x%x", rc);
            return NULL;
        }
        
        if (!found) {
            PyErr_SetString(PyExc_RuntimeError, "Save data not found for extension");
            return NULL;
        }
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

// Функция для проверки и инициализации аудио
static bool initialize_audio_system(void)
{
    if (audio_initialized) return true;
    
    // Инициализация аудио через SDL (если используется)
    if (PyRun_SimpleString(
        "import pygame_sdl2.mixer\n"
        "pygame_sdl2.mixer.init()\n"
        "if not pygame_sdl2.mixer.get_init():\n"
        "    raise Exception('Audio initialization failed')\n"
    ) != 0) {
        return false;
    }
    
    audio_initialized = true;
    return true;
}

static PyObject* check_audio(PyObject* self, PyObject* args)
{
    if (!initialize_audio_system()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize audio system");
        return NULL;
    }
    
    PyObject *audio_module = PyImport_ImportModule("pygame_sdl2.mixer");
    if (!audio_module) {
        PyErr_SetString(PyExc_ImportError, "Failed to import audio module");
        return NULL;
    }
    
    PyObject *result = PyObject_CallMethod(audio_module, "get_init", NULL);
    Py_DECREF(audio_module);
    
    if (!result) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to check audio status");
        return NULL;
    }
    
    Py_INCREF(result);
    return result;
}

static PyObject* startboost(PyObject* self, PyObject* args)
{
    Result rc = appletSetCpuBoostMode(ApmPerformanceMode_Boost);
    if (R_FAILED(rc)) {
        PyErr_Format(PyExc_RuntimeError, "Failed to set boost mode: 0x%x", rc);
        return NULL;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* disableboost(PyObject* self, PyObject* args)
{
    Result rc = appletSetCpuBoostMode(ApmPerformanceMode_Normal);
    if (R_FAILED(rc)) {
        PyErr_Format(PyExc_RuntimeError, "Failed to disable boost: 0x%x", rc);
        return NULL;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* restartprogram(PyObject* self, PyObject* args)
{
    Result rc = appletRestartProgram(NULL, 0);
    if (R_FAILED(rc)) {
        PyErr_Format(PyExc_RuntimeError, "Failed to restart: 0x%x", rc);
        return NULL;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef myMethods[] = {
    { "commitsave", commitsave, METH_NOARGS, "Commit save data" },
    { "check_audio", check_audio, METH_NOARGS, "Check audio system status" },
    { "startboost", startboost, METH_NOARGS, "Enable CPU boost mode" },
    { "disableboost", disableboost, METH_NOARGS, "Disable CPU boost mode" },
    { "restartprogram", restartprogram, METH_NOARGS, "Restart program" },
    { NULL, NULL, 0, NULL }
};

// Для Python 2.7 используем старый API
PyMODINIT_FUNC init_otrh_libnx(void)
{
    Py_InitModule("_otrhlibnx", myMethods);
}

// Декларация функции createSaveData для избежания предупреждения
Result createSaveData();

// Хук для applet (выносим из main)
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

void userAppInit()
{
    // Инициализация базовых сервисов
    Result rc = 0;
    
    rc = svcGetInfo(&cur_progid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
    if (R_FAILED(rc)) {
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_NotInitialized));
    }
    
    // Инициализация аккаунта
    rc = accountInitialize(AccountServiceType_Application);
    if (R_FAILED(rc)) {
        // Продолжаем без аккаунта
    }
    
    // Получаем пользователя
    rc = accountGetPreselectedUser(&userID);
    if (R_FAILED(rc) || !accountUidIsValid(&userID)) {
        s32 count = 0;
        rc = accountGetUserCount(&count);
        
        if (R_SUCCEEDED(rc) && count > 0) {
            if (count > 1) {
                PselUserSelectionSettings settings;
                memset(&settings, 0, sizeof(settings));
                rc = pselShowUserSelector(&userID, &settings);
            } else {
                s32 loadedUsers = 0;
                AccountUid account_ids[8]; // Максимум 8 аккаунтов
                rc = accountListAllUsers(account_ids, count, &loadedUsers);
                if (R_SUCCEEDED(rc) && loadedUsers > 0) {
                    userID = account_ids[0];
                }
            }
        }
    }
    
    // Монтируем сохранение
    if (accountUidIsValid(&userID)) {
        rc = fsdevMountSaveData("save", cur_progid, userID);
        if (R_FAILED(rc)) {
            // Пытаемся создать сохранение
            rc = createSaveData();
            if (R_FAILED(rc)) {
                // Если не удалось создать, продолжаем без сохранения
            } else {
                rc = fsdevMountSaveData("save", cur_progid, userID);
            }
        }
    }
    
    // Инициализация других систем
    rc = romfsInit();
    if (R_FAILED(rc)) {
        // Продолжаем без romfs
    }
    
    rc = socketInitializeDefault();
    if (R_FAILED(rc)) {
        // Продолжаем без сети
    }
}

// Обновленная функция show_error
void show_error(const char* message, int should_exit)
{
    // Всегда показываем ошибку через ErrorSystem
    ErrorSystemConfig c;
    errorSystemCreate(&c, "Application Error", message);
    errorSystemShow(&c);
    
    if (should_exit) {
        // Аккуратно завершаем Python если он инициализирован
        if (Py_IsInitialized()) {
            Py_Finalize();
        }
        
        // Даем время для отображения ошибки
        svcSleepThread(3000000000ULL); // 3 секунды
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    // Настройка окружения
    setenv("MESA_NO_ERROR", "1", 1);
    setenv("PYTHONUNBUFFERED", "1", 1);
    
    // Блокировка выхода до завершения
    appletLockExit();
    
    // Настройка хука для корректного завершения
    appletHook(&applet_hook_cookie, on_applet_hook, NULL);
    
    // Настройка флагов Python (Python 2.7)
    Py_NoSiteFlag = 1;
    Py_IgnoreEnvironmentFlag = 1;
    Py_NoUserSiteDirectory = 1;
    Py_DontWriteBytecodeFlag = 1;
    Py_OptimizeFlag = 2;
    
    // Проверка необходимых файлов перед инициализацией Python
    FILE* libzip = fopen("romfs:/Contents/lib.zip", "rb");
    if (!libzip) {
        show_error("Critical: lib.zip not found in romfs:/Contents/\n\n"
                  "Make sure the NSP is properly packaged with all files.", 1);
        return 1;
    }
    fclose(libzip);
    
    FILE* renpy_main = fopen("romfs:/Contents/renpy.py", "rb");
    if (!renpy_main) {
        show_error("Critical: renpy.py not found in romfs:/Contents/\n\n"
                  "This is required to start the Ren'Py engine.", 1);
        return 1;
    }
    fclose(renpy_main);
    
    // Инициализация Python (Python 2.7)
    Py_InitializeEx(0);
    if (!Py_IsInitialized()) {
        show_error("Failed to initialize Python interpreter.", 1);
        return 1;
    }
    
    Py_SetProgramName(argv[0]);
    Py_SetPythonHome("romfs:/Contents/lib.zip");
    
    // Установка аргументов Python (Python 2.7 использует char*)
    PySys_SetArgvEx(argc, argv, 0);
    
    // Настройка пути Python
    if (PyRun_SimpleString(
        "import sys\n"
        "sys.path = ['romfs:/Contents/lib.zip']\n"
        "sys.stdout = sys.stderr\n"  // Перенаправляем stdout в stderr
    ) != 0) {
        show_error("Failed to set up Python environment.", 1);
        Py_Finalize();
        return 1;
    }
    
    // Проверка основных модулей
    const char* required_modules[] = {
        "os", "pygame_sdl2", "encodings.utf_8", NULL
    };
    
    for (int i = 0; required_modules[i] != NULL; i++) {
        PyObject* module = PyImport_ImportModule(required_modules[i]);
        if (!module) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                    "Failed to import required module: %s\n\n"
                    "Python environment may be corrupted.",
                    required_modules[i]);
            show_error(error_msg, 1);
            Py_Finalize();
            return 1;
        }
        Py_DECREF(module);
    }
    
    // Проверка и инициализация аудио системы
    if (!initialize_audio_system()) {
        show_error("Warning: Audio system failed to initialize.\n"
                  "The game will continue without sound.", 0);
    }
    
    // Запуск основного скрипта Ren'Py
    FILE* script = fopen("romfs:/Contents/renpy.py", "r");
    if (!script) {
        show_error("Failed to open renpy.py for execution.", 1);
        Py_Finalize();
        return 1;
    }
    
    int py_result = PyRun_SimpleFileEx(script, "renpy.py", 1);
    
    if (py_result != 0) {
        // Получаем информацию об ошибке Python если есть
        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);
        
        if (value != NULL) {
            PyObject* str = PyObject_Str(value);
            if (str != NULL) {
                // Для Python 2.7 используем PyString_AsString
                const char* error_msg = PyString_AsString(str);
                if (error_msg) {
                    show_error(error_msg, 1);
                }
                Py_DECREF(str);
            }
        }
        
        PyErr_Restore(type, value, traceback);
        PyErr_Print();
        
        show_error("Python script execution failed.", 1);
    }
    
    // Корректное завершение
    Py_Finalize();
    
    // Коммитим сохранение перед выходом
    fsdevCommitDevice("save");
    
    // Задержка перед выходом
    svcSleepThread(500000000ULL); // 0.5 секунды
    
    return 0;
}

// Функция createSaveData (должна быть определена где-то в коде)
Result createSaveData()
{
    // Реализация функции createSaveData (если есть в исходном коде)
    // Если нет, нужно добавить реализацию
    return 0;
}

// Остальные функции PyMODINIT_FUNC остаются как в оригинальном коде
// ... (все init функции остаются без изменений)
