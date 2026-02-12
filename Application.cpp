#include "Application.h"

Application* Application::instancePointer = nullptr;

Application::Application(HINSTANCE instance)
	: instance(instance) {
	trayData = {};
}

void Application::createWindow() {
	int windowWidth = 450;
	int windowHeight = 225;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	window = CreateWindowEx(
		0,
		APPLICATION_WINDOW_CLASS,
		APPLICATION_NAME,
		WS_SYSMENU | WS_CAPTION,
		(screenWidth - windowWidth) / 2,
		(screenHeight - windowHeight) / 2,
		windowWidth, windowHeight,
		NULL,
		NULL,
		instance,
		this
	);

	createTrayIcon();
}

bool Application::isDarkMode() {
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

bool Application::resolveDarkMode() {
	if (themeMode == ThemeMode::Dark)
		return true;

	if (themeMode == ThemeMode::Light)
		return false;

	return isDarkMode();
}

bool Application::isStartupEnabled() {
	HKEY key;
	if (RegOpenKeyExW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0,
		KEY_READ,
		&key) != ERROR_SUCCESS)
		return false;

	WCHAR path[MAX_PATH];
	DWORD size = sizeof(path);

	LONG result = RegGetValueW(
		key,
		NULL,
		APPLICATION_NAME,
		RRF_RT_REG_SZ,
		NULL,
		path,
		&size
	);

	RegCloseKey(key);
	return result == ERROR_SUCCESS;
}

void Application::setStartup(bool enable) {
	HKEY key;
	if (RegOpenKeyExW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0,
		KEY_WRITE,
		&key) != ERROR_SUCCESS)
		return;

	if (!enable) {
		RegDeleteValueW(key, APPLICATION_NAME);
		RegCloseKey(key);
		return;
	}

	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);

	RegSetValueExW(
		key,
		APPLICATION_NAME,
		0,
		REG_SZ,
		(BYTE*)exePath,
		(DWORD)((wcslen(exePath) + 1) * sizeof(WCHAR))
	);

	RegCloseKey(key);
}

void Application::applyTheme(HWND handleToWindow) {
	if (!IsWindowVisible(handleToWindow)) return;

	const BOOL dark = resolveDarkMode();

	const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
	if (FAILED(DwmSetWindowAttribute(handleToWindow, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark)))) {
		const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
		DwmSetWindowAttribute(handleToWindow, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &dark, sizeof(dark));
	}

	SetWindowTheme(handleToWindow, nullptr, nullptr);
}

void Application::updateAllWindows() {
	EnumWindows(
		[](HWND handleToWindow, LPARAM param) -> BOOL {
			reinterpret_cast<Application*>(param)->applyTheme(handleToWindow);
			return true;
		},
		reinterpret_cast<LPARAM>(this)
	);
}

void Application::createTrayIcon() {
	if (trayIconCreated)
		return;

	trayData.cbSize = sizeof(NOTIFYICONDATA);

	trayData.hWnd = window;
	trayData.uID = TRAY_ID;
	trayData.uCallbackMessage = WM_APP_TRAY;
	trayData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	trayData.hIcon = resolveDarkMode() ? iconDark : iconLight;

	wcscpy_s(trayData.szTip, APPLICATION_NAME);
	trayIconCreated = Shell_NotifyIcon(NIM_ADD, &trayData);
}

void Application::updateTrayIcon() {
	if (!trayIconCreated) {
		createTrayIcon();
		return;
	}

	trayData.hIcon = resolveDarkMode() ? iconDark : iconLight;

	Shell_NotifyIcon(NIM_MODIFY, &trayData);
}

void Application::showContextMenu() {
	POINT p;
	GetCursorPos(&p);

	HMENU menu = CreatePopupMenu();
	HMENU themeMenu = CreatePopupMenu();

	bool startup = isStartupEnabled();

	AppendMenu(
		themeMenu,
		MF_STRING | (themeMode == ThemeMode::System ? MF_CHECKED : 0),
		10,
		L"Use system setting"
	);

	AppendMenu(
		themeMenu,
		MF_STRING | (themeMode == ThemeMode::Dark ? MF_CHECKED : 0),
		11,
		L"Dark"
	);

	AppendMenu(
		themeMenu,
		MF_STRING | (themeMode == ThemeMode::Light ? MF_CHECKED : 0),
		12,
		L"Light"
	);

	AppendMenu(menu, MF_STRING | (startup ? MF_CHECKED : 0), 3, L"Run On Startup");
	AppendMenu(menu, MF_SEPARATOR, 0, NULL);
	AppendMenu(menu, MF_POPUP, (UINT_PTR)themeMenu, L"Theme");
	AppendMenu(menu, MF_SEPARATOR, 0, NULL);
	AppendMenu(menu, MF_STRING, 4, L"Help");
	AppendMenu(menu, MF_STRING, 2, L"About");
	AppendMenu(menu, MF_SEPARATOR, 0, NULL);
	AppendMenu(menu, MF_STRING, 1, L"Exit");

	SetForegroundWindow(window);
	int cmd = TrackPopupMenu(menu, TPM_RETURNCMD, p.x, p.y, 0, window, NULL);

	if (cmd == 3)
		setStartup(!startup);

	if (cmd == 10)
		themeMode = ThemeMode::System;

	if (cmd == 11)
		themeMode = ThemeMode::Dark;

	if (cmd == 12)
		themeMode = ThemeMode::Light;

	if (cmd >= 10 && cmd <= 12) {
		saveSettings();
		updateTrayIcon();
		updateAllWindows();
	}

	if (cmd == 2)
		MessageBox(
			window,
			ABOUT_APPLICATION,
			L"About",
			MB_OK | MB_ICONINFORMATION
		);

	if (cmd == 4)
		ShellExecute(NULL, L"open", SUPPORT_URL, NULL, NULL, SW_SHOWNORMAL);

	if (cmd == 1) {
		DestroyMenu(menu);
		PostQuitMessage(0);
		return;
	}

	DestroyMenu(themeMenu);
	DestroyMenu(menu);
}

void CALLBACK Application::eventCallback(
	HWINEVENTHOOK,
	DWORD,
	HWND handleToWindow,
	LONG idObject,
	LONG,
	DWORD,
	DWORD
) {
	if (idObject != OBJID_WINDOW)
		return;

	if (!instancePointer)
		return;

	instancePointer->applyTheme(handleToWindow);
}

DWORD WINAPI Application::themeWatcher(void* param) {
	HKEY key;
	RegOpenKeyExW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		0,
		KEY_NOTIFY,
		&key
	);

	for (;;) {
		RegNotifyChangeKeyValue(key, FALSE, REG_NOTIFY_CHANGE_LAST_SET, NULL, FALSE);

		if (!instancePointer)
			continue;

		if (instancePointer->themeMode != ThemeMode::System)
			continue;

		instancePointer->updateTrayIcon();
		instancePointer->updateAllWindows();
	}
}

LRESULT CALLBACK Application::wndProc(HWND handleToWindow, UINT msg, WPARAM w, LPARAM l) {
	static UINT taskbarCreated;

	switch (msg) {
	case WM_CREATE:
		taskbarCreated = RegisterWindowMessage(L"TaskbarCreated");
		return 0;
	case WM_CLOSE:
#if !_DEBUG
		ShowWindow(handleToWindow, SW_HIDE);
		return 0;
#endif
	case WM_DESTROY:
		if (instancePointer)
			Shell_NotifyIcon(NIM_DELETE, &instancePointer->trayData);

		PostQuitMessage(0);
		return 0;
	default:
		if (!instancePointer)
			return DefWindowProc(handleToWindow, msg, w, l);

		if (msg == taskbarCreated) {
			instancePointer->createTrayIcon();
			return 0;
		}

		if (msg == WM_APP_TRAY) {
			if (l == WM_RBUTTONUP)
				instancePointer->showContextMenu();
			return 0;
		}

		break;
	}

	return DefWindowProc(handleToWindow, msg, w, l);
}

void Application::saveSettings() {
	HKEY key;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, nullptr,
		0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
		return;

	DWORD themeValue = static_cast<DWORD>(themeMode);

	RegSetValueExW(key, REG_THEME, 0, REG_DWORD,
		reinterpret_cast<BYTE*>(&themeValue), sizeof(themeValue));

	RegCloseKey(key);
}

void Application::loadSettings() {
	HKEY key;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &key) != ERROR_SUCCESS)
		return;

	DWORD size = sizeof(DWORD);
	DWORD themeValue = 0;

	RegGetValueW(key, nullptr, REG_THEME, RRF_RT_REG_DWORD, nullptr, &themeValue, &size);

	themeMode = static_cast<ThemeMode>(themeValue);

	RegCloseKey(key);
}

int Application::run() {
	HANDLE mutualExclusion = CreateMutexW(NULL, FALSE, APPLICATION_MUTUAL_EXCLUSION);
	if (!mutualExclusion || GetLastError() == ERROR_ALREADY_EXISTS)
		return 0;

	WNDCLASSW wc{};
	wc.lpfnWndProc = wndProc;
	wc.hInstance = instance;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpszClassName = APPLICATION_WINDOW_CLASS;
	wc.hIcon = LoadIconW(instance, MAKEINTRESOURCE(IDI_APPLICATION_ICON));

	RegisterClass(&wc);

	iconDark = (HICON)LoadImage(
		instance,
		MAKEINTRESOURCE(IDI_DARK_ICON),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR
	);

	iconLight = (HICON)LoadImage(
		instance,
		MAKEINTRESOURCE(IDI_LIGHT_ICON),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR
	);

	Application::instancePointer = this;

	eventHook = SetWinEventHook(
		EVENT_OBJECT_CREATE,
		EVENT_OBJECT_SHOW,
		NULL,
		eventCallback,
		0,
		0,
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
	);

	loadSettings();
	createWindow();

#if _DEBUG
	ShowWindow(window, SW_SHOW);
#else
	ShowWindow(window, SW_HIDE);
#endif

	applyTheme(window);
	CreateThread(NULL, 0, themeWatcher, this, 0, NULL);

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	UnhookWinEvent(eventHook);
	CloseHandle(mutualExclusion);

	return 0;
}