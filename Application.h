#pragma once
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include <dwmapi.h>
#include <windows.h>
#include <shellapi.h>
#include "resource.h"

#define TRAY_ID 1001
#define WM_APP_TRAY (WM_APP + 1)
#define APPLICATION_VERSION L"1.1"
#define APPLICATION_NAME L"Dynamic Title Bar"
#define APPLICATION_AUTHOR L"By Kh4l1l (@xh4l1l)"
#define APPLICATION_DESCRIPTIION L"Automatically applies custom Title Bar settings to classic Win32 hWnd(s) via DWM API."

#define ABOUT_APPLICATION \
    APPLICATION_NAME L" " APPLICATION_VERSION L"\n" \
    APPLICATION_AUTHOR L"\n\n" \
    APPLICATION_DESCRIPTIION

constexpr LPCWSTR REG_THEME = L"ThemeMode";
constexpr LPCWSTR REG_PATH = L"Software\\DynamicTitleBar";

class Application {
public:

    explicit Application(HINSTANCE instance);
    int run();

private:
    bool isDarkMode();
    void createWindow();
    bool resolveDarkMode();
    bool isStartupEnabled();
    void setStartup(bool enable);

    void applyTheme(HWND hwnd);
    void updateAllWindows();

    void saveSettings();
    void loadSettings();
    void createTrayIcon();
    void updateTrayIcon();
    void showContextMenu();

private:
    static Application* sInstance;
    static void CALLBACK eventCallback(
        HWINEVENTHOOK,
        DWORD,
        HWND,
        LONG,
        LONG,
        DWORD,
        DWORD
    );

    static DWORD WINAPI themeWatcher(void* param);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

private:
    enum class ThemeMode {
        System,
        Dark,
        Light
    };
private:
    HWND window{};
    HICON iconDark{};
    HICON iconLight{};
    HINSTANCE instance{};
    HWINEVENTHOOK eventHook{};
    NOTIFYICONDATA trayData{};
    bool trayIconCreated{ false };
    ThemeMode themeMode{ ThemeMode::System };
};