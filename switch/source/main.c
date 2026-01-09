#include <switch.h>
#include <Python.h>
#include <stdio.h>
#include <string.h>

char python_error_buffer[512];  // Буфер для короткой строки ошибки

// Функция вывода ошибок
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

// Вспомогательная функция для безопасного выполнения Python-кода
void run_python_code(const char* code)
{
    int result = PyRun_SimpleString(code);
    if (result != 0) {
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
        PyObject* py_str = PyObject_Str(pvalue);
        const char* err_msg = py_str ? PyUnicode_AsUTF8(py_str) : "Unknown Python error";
        show_error(err_msg, 0);
        Py_XDECREF(py_str);
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
    }
}

int main(int argc, char **argv)
{
    consoleInit(NULL);
    printf("Starting Python 3.9 on Switch from SD zip...\n");

    const wchar_t *python_home = L"sdmc:/python39.zip";
    const wchar_t *python_path = L"sdmc:/python39.zip";

    // Настройка Python
    Py_SetProgramName(L"python");
    Py_SetPythonHome(python_home);
    Py_SetPath(python_path);
    Py_NoSiteFlag = 1;
    Py_FrozenFlag = 1;
    Py_IgnoreEnvironmentFlag = 1;

    // Инициализация Python
    Py_Initialize();

    // Пробуем выполнить код Python
    run_python_code(
        "import sys\n"
        "print('Python version:', sys.version)\n"
        "print('Python executable:', sys.executable)\n"
        "print('Python sys.path:', sys.path)\n"
        "print('Hello from Python 3.9 in zip on Switch!')\n"
    );

    Py_Finalize();

    printf("\nPress + to exit\n");

    // Работа с контроллером
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    while (appletMainLoop())
    {
        padUpdate(&pad);
        u64 kDown = padGetButtons(&pad);

        if (kDown & HidNpadButton_Plus) break;

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
