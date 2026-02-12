#pragma once
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include <iostream>
#include <dwmapi.h>
#include <windows.h>
#include <shellapi.h>
#include "resource.h"

#define TRAY_ID (WM_APP + 2)
#define WM_APP_TRAY (WM_APP + 1)

#define APPLICATION_VERSION L"1.2"
#define APPLICATION_AUTHOR L"Kh4l1l"
#define APPLICATION_FILE_VERSION 1,2,0,0
#define APPLICATION_PRODUCT_VERSION 1,2,0,0
#define APPLICATION_AUTHOR_HANDLE L"@xh4l1l"
#define SUPPORT_URL L"https://www.discord.gg/Wf3VPU7Zez"
#define APPLICATION_COPYRIGHT L"© 2026 " APPLICATION_AUTHOR L". All rights reserved."
#define APPLICATION_DESCRIPTIION L"Automatically applies custom Title Bar settings to classic Win32 hWnd(s) via DWM API."

#define APPLICATION_NAME L"DynamicTitleBar"
#define APPLICATION_TITLE L"Dynamic Title Bar"
#define APPLICATION_WINDOW_CLASS L"DynamicTitleBarWindow"
#define APPLICATION_MUTUAL_EXCLUSION L"DynamicTitleBarMutualExclusion"

#define ABOUT_APPLICATION \
    APPLICATION_TITLE L" " APPLICATION_VERSION L"\n" \
    L"By " APPLICATION_AUTHOR L"(" APPLICATION_AUTHOR_HANDLE L")\n\n" \
    APPLICATION_DESCRIPTIION

constexpr LPCWSTR REG_THEME = L"ThemeMode";
constexpr LPCWSTR REG_PATH = L"Software\\DynamicTitleBar";

class Application {
private:
	enum class ThemeMode {
		Dark,
		Light,
		System
	};
public:
	explicit Application(HINSTANCE instance);

	int run();
private:
	static Application* instancePointer;
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
	static LRESULT CALLBACK wndProc(HWND handleToWindow, UINT msg, WPARAM w, LPARAM l);
private:
	bool isDarkMode();
	void createWindow();
	bool resolveDarkMode();
	bool isStartupEnabled();
	void updateAllWindows();
	void applyTheme(HWND handleToWindow);
	void setStartup(bool enable);

	void saveSettings();
	void loadSettings();
	void createTrayIcon();
	void updateTrayIcon();
	void showContextMenu();
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