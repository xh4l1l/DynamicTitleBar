#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <dwmapi.h>
#include <windows.h>
#include "resource.h"
#include <shellapi.h>

#define TRAY_ID 1001
#define WM_APP_TRAY (WM_APP + 1)
#define WM_TASKBAR_CREATED RegisterWindowMessage(L"TaskbarCreated")

HWND msgWindow;
HICON iconDark;
HICON iconLight;
HINSTANCE instance;
HWINEVENTHOOK eventHook;
NOTIFYICONDATA trayData = {};

bool trayIconCreated = false;

bool isDarkMode() {
    DWORD value = 1;
    DWORD size = sizeof(value);
    RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        NULL,
        &value,
        &size
    );
    return value == 0;
}

bool isStartupEnabled() {
    HKEY key;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &key) != ERROR_SUCCESS)
        return false;

    WCHAR path[MAX_PATH];
    DWORD size = sizeof(path);
    LONG result = RegGetValue(key, NULL, L"DynamicTitleBar", RRF_RT_REG_SZ, NULL, path, &size);

    RegCloseKey(key);

    return result == ERROR_SUCCESS;
}

void setStartup(bool enable) {
    HKEY key;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &key) != ERROR_SUCCESS)
        return;

    if (enable) {
        WCHAR exePath[MAX_PATH];
        GetModuleFileName(NULL, exePath, MAX_PATH);
        RegSetValueEx(key, L"DynamicTitleBar", 0, REG_SZ, (BYTE*)exePath, (DWORD)((wcslen(exePath) + 1) * sizeof(WCHAR)));
        RegCloseKey(key);
        return;
    }

    RegDeleteValue(key, L"DynamicTitleBar");
    RegCloseKey(key);
}

void applyTheme(HWND hwnd) {
    if (!IsWindowVisible(hwnd)) return;
    BOOL dark = isDarkMode();
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

void CALLBACK winEventCallback(
    HWINEVENTHOOK, DWORD, HWND hwnd,
    LONG idObject, LONG, DWORD, DWORD
) {
    if (idObject != OBJID_WINDOW) return;
    applyTheme(hwnd);
}

void updateAllWindows() {
    EnumWindows([](HWND hwnd, LPARAM) {
        applyTheme(hwnd);
        return TRUE;
        }, 0);
}


void createTrayIcon(HWND hwnd) {
    trayData.cbSize = sizeof(NOTIFYICONDATA);
    trayData.hWnd = hwnd;
    trayData.uID = TRAY_ID;
    trayData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    trayData.uCallbackMessage = WM_APP_TRAY;
    trayData.hIcon = isDarkMode() ? iconDark : iconLight;
    wcscpy_s(trayData.szTip, L"Dynamic Title Bar");

    trayIconCreated = Shell_NotifyIcon(NIM_ADD, &trayData);
}

void updateTrayIcon(HWND hwnd) {
    if (!trayIconCreated){
		createTrayIcon(hwnd);
		return;
    };

    trayData.hIcon = isDarkMode() ? iconDark : iconLight;
    Shell_NotifyIcon(NIM_MODIFY, &trayData);
}

void showContextMenu(HWND hwnd) {
    POINT p;
    GetCursorPos(&p);

    HMENU menu = CreatePopupMenu();
    bool startup = isStartupEnabled();
    
    AppendMenu(menu, MF_STRING | (startup ? MF_CHECKED : 0), 3, L"Run On Startup");
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, 4, L"Help");
    AppendMenu(menu, MF_STRING, 2, L"About");
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, 1, L"Exit");

    SetForegroundWindow(hwnd);

    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD, p.x, p.y, 0, hwnd, NULL);

    if (cmd == 3) {
        setStartup(!startup);
    }

    if (cmd == 2) {
        MessageBoxW(
            hwnd,
            L"Dynamic Title Bar 1.0\nBy Kh4l1l (@xh4l1l)\n\nAutomatically applies system theme to title bar mode.",
            L"About",
            MB_OK | MB_ICONINFORMATION
        );
    }

    if (cmd == 4) {
        ShellExecuteW(NULL, L"open", L"https://www.discord.gg/Wf3VPU7Zez", NULL, NULL, SW_SHOWNORMAL);
    }

    if (cmd == 1) {
        DestroyMenu(menu);
        PostQuitMessage(0);
    }

    DestroyMenu(menu);
}

DWORD WINAPI themeWatcherThread(void*) {
    HKEY key;
    RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0,
        KEY_NOTIFY,
        &key
    );
    while (true) {
        RegNotifyChangeKeyValue(key, FALSE, REG_NOTIFY_CHANGE_LAST_SET, NULL, FALSE);
        updateAllWindows();
        updateTrayIcon(msgWindow);
    }
    return 0;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    static UINT taskbarMsg = WM_TASKBAR_CREATED;

    if (msg == taskbarMsg) {
        createTrayIcon(hwnd);
        return 0;
    }

    if (msg == WM_APP_TRAY && l == WM_RBUTTONUP) {
        showContextMenu(hwnd);
        return 0;
    }

    if (msg == WM_DESTROY) {
        if (trayIconCreated) Shell_NotifyIcon(NIM_DELETE, &trayData);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    HANDLE mutualExclusion = CreateMutex(NULL, FALSE, L"DynamicTitleBarMutualExclusion");

    if (mutualExclusion == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }
    
    instance = hInstance;

    WNDCLASS wc{};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DynamicTitleBar";
    RegisterClass(&wc);

    msgWindow = CreateWindowEx(0, L"DynamicTitleBar", L"", WS_OVERLAPPED,
        0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    ShowWindow(msgWindow, SW_HIDE);

    iconDark = (HICON)LoadImage(instance, MAKEINTRESOURCE(IDI_DARK_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    iconLight = (HICON)LoadImage(instance, MAKEINTRESOURCE(IDI_LIGHT_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    createTrayIcon(msgWindow);

    eventHook = SetWinEventHook(
        EVENT_OBJECT_CREATE,
        EVENT_OBJECT_SHOW,
        NULL,
        winEventCallback,
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    CreateThread(NULL, 0, themeWatcherThread, NULL, 0, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWinEvent(eventHook);
    CloseHandle(mutualExclusion);

    return 0;
}