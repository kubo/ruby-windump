#define _WIN32_WINNT 0x0500
#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include "minidump.h"

#ifdef HAVE_DBGHELP_H
#include <dbghelp.h>
#endif

#ifndef HAVE_TYPE_PMINIDUMP_EXCEPTION_INFORMATION
typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;
#endif

#ifndef HAVE_TYPE_PMINIDUMP_USER_STREAM_INFORMATION
typedef void * PMINIDUMP_USER_STREAM_INFORMATION; /* dummy definition */
#endif

#ifndef HAVE_TYPE_PMINIDUMP_CALLBACK_INFORMATION
typedef void * PMINIDUMP_CALLBACK_INFORMATION; /* dummy definition */
#endif

typedef BOOL (WINAPI *MiniDumpWriteDump_t)(
    HANDLE hProcess,
    DWORD ProcessId,
    HANDLE hFile,
    MINIDUMP_TYPE DumpType,
    PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    PMINIDUMP_CALLBACK_INFORMATION CallbackParam
);

static MiniDumpWriteDump_t MiniDumpWriteDump_func;

#define MAX_THREADS_TO_BE_SUSPENDED 10000
static HANDLE suspended_threads[MAX_THREADS_TO_BE_SUSPENDED];

/*
 * This function is multithread unsafe. We can make it multithread safe
 * by allocating memory at runtime. But memory allocation may cause
 * another segmentation fault. Thus we use a static array to store
 * suspended thread handles.
 */
static DWORD suspend_threads(void)
{
    HANDLE hSnapshot;
    THREADENTRY32 entry;
    DWORD num_suspended = 0;
    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();
    BOOL bOk;

    fprintf(stderr, "Suspending threads... ");
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "failed.\n");
        return 0;
    }
    entry.dwSize = sizeof(THREADENTRY32);
    bOk = Thread32First(hSnapshot, &entry);
    while (bOk) {
        if (entry.th32OwnerProcessID == pid && entry.th32ThreadID != tid) {
            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
            if (hThread != NULL && SuspendThread(hThread)) {
                suspended_threads[num_suspended++] = hThread;
                if (num_suspended == MAX_THREADS_TO_BE_SUSPENDED) {
                    fprintf(stderr, "too many threads to stop... ");
                    break;
                }
            }
        }
        bOk = Thread32Next(hSnapshot, &entry);
    }
    CloseHandle(hSnapshot);
    fprintf(stderr, "done.\n");
    return num_suspended;
}

static void resume_threads(DWORD num_suspended)
{
    DWORD idx;
    fprintf(stderr, "Resuming threads... ");
    for (idx = 0; idx < num_suspended; idx++) {
        ResumeThread(suspended_threads[idx]);
    }
    fprintf(stderr, "done.\n");
}

const char *minidump_init(HMODULE hDLL)
{
    HMODULE hDbgHelp = NULL;

    if (hDLL != NULL) {
        char path[MAX_PATH];
        DWORD rv;
        char *p;

        rv = GetModuleFileName(hDLL, path, MAX_PATH);
        if (rv > 0 && (p = strrchr(path, '\\')) != NULL) {
            if ((p - path) + strlen("\\dbghelp.dll") < MAX_PATH) {
                strcpy(p, "\\dbghelp.dll");
                hDbgHelp = LoadLibrary(path);
            }
        }
    }
    if (hDbgHelp == NULL) {
        hDbgHelp = LoadLibrary("dbghelp.dll");
    }
    if (hDbgHelp == NULL) {
        return "Could not find dbghelp.dll.";
    }
    MiniDumpWriteDump_func = (MiniDumpWriteDump_t)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if (MiniDumpWriteDump_func == NULL) {
        FreeLibrary(hDbgHelp);
        return "Could not find a symbol MiniDumpWriteDump in dbghelp.dll.";
    }
    return NULL;
}

void minidump_dump(const char *filename, MINIDUMP_TYPE dumptype, PEXCEPTION_POINTERS excptr)
{
    HANDLE hDumpFile;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;
    BOOL bOk;
    DWORD num_suspended;

    hDumpFile = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE,
                FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    if (hDumpFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open file \"%s\" for dump.\n", filename);
        return;
    }
    num_suspended = suspend_threads();
    fprintf(stderr, "Generating a dump \"%s\"... ", filename);

    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = excptr;
    ExpParam.ClientPointers = TRUE;

    bOk = MiniDumpWriteDump_func(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile,
                                 dumptype, &ExpParam, NULL, NULL);
    CloseHandle(hDumpFile);
    if (bOk) {
        fprintf(stderr, "done.\n");
    } else {
        fprintf(stderr, "failed.\n");
    }
    resume_threads(num_suspended);
}
