/*
 * LazyPeregrin.scr: the file Windows lists among its screensavers.
 *
 * Windows only lists screensavers that sit in its own system folder, and the real
 * program needs its SDL libraries beside it, which do not belong there. So this
 * small file goes in the system folder and starts the real program from where it
 * was installed, passing on whatever Windows asked for (/s run, /p preview,
 * /c settings), and waits for it to finish.
 */
#include <windows.h>
#include <string.h>

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
    char dir[MAX_PATH] = "", cmd[MAX_PATH + 1024];
    DWORD len = sizeof dir;
    (void)inst; (void)prev; (void)show;
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "Software\\LazyPeregrin", "InstallDir",
                     RRF_RT_REG_SZ | KEY_WOW64_64KEY, NULL, dir, &len) != ERROR_SUCCESS || !dir[0]) {
        MessageBoxA(NULL, "The Lazy Peregrin is not installed properly. Please run its installer again.",
                    "The Lazy Peregrin", MB_OK | MB_ICONWARNING);
        return 1;
    }
    if (strlen(dir) + strlen(cmdline) + 32 > sizeof cmd) return 1;
    wsprintfA(cmd, "\"%s\\lazy-peregrin.exe\" %s", dir, cmdline);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, dir, &si, &pi)) return 1;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)code;
}
