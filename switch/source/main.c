#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <switch.h>
#include <stdio.h>

u64 cur_progid = 0;
AccountUid userID = {0};

/* ===================== PYTHON EXTENSION ===================== */

static PyObject* commitsave(PyObject* self, PyObject* args)
{
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

static PyMethodDef myMethods[] = {
    {"commitsave", commitsave, METH_NOARGS, NULL},
    {"startboost", startboost, METH_NOARGS, NULL},
    {"disableboost", disableboost, METH_NOARGS, NULL},
    {"restartprogram", restartprogram, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef otrhlibnx_module = {
    PyModuleDef_HEAD_INIT,
    "_otrhlibnx",
    NULL,
    -1,
    myMethods
};

PyMODINIT_FUNC PyInit__otrhlibnx(void)
{
    return PyModule_Create(&otrhlibnx_module);
}

/* ===================== APP INIT ===================== */

void userAppInit(void)
{
    svcGetInfo(&cur_progid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);

    accountInitialize(AccountServiceType_Application);
    accountGetPreselectedUser(&userID);

    fsdevMountSaveData("save", cur_progid, userID);
    romfsInit();
    socketInitializeDefault();
}

void userAppExit(void)
{
    fsdevCommitDevice("save");
    fsdevUnmountDevice("save");
    socketExit();
    romfsExit();
}

/* ===================== MAIN ===================== */

int main(int argc, char* argv[])
{
    appletLockExit();

    PyImport_AppendInittab("_otrhlibnx", PyInit__otrhlibnx);

    Py_PreInitialize(NULL);

    wchar_t *home = Py_DecodeLocale("romfs:/Contents/lib.zip", NULL);
    Py_SetPythonHome(home);

    Py_Initialize();

    wchar_t *py_argv[2];
    py_argv[0] = Py_DecodeLocale("romfs:/Contents/renpy.py", NULL);
    py_argv[1] = NULL;
    PySys_SetArgvEx(1, py_argv, 1);

    PyRun_SimpleString(
        "import sys\n"
        "sys.path = ['romfs:/Content]()
