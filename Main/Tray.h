#pragma once
#include <windows.h>
#include <shellapi.h>
#include <atomic>
#include <vector>
#include <string>
#include <array>
#include <functional>
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define IDI_MYICON 1

inline std::function<void()> g_OnTrayDoubleClick = nullptr;
inline std::vector<std::wstring> g_soundbiteKeys = {
    L"1 : ", L"2 : ", L"3 : ", L"4 : ", L"5 : ",
    L"Q : ", L"W : ", L"E : ", L"R : ", L"T : ",
    L"A : ", L"S : ", L"D : ", L"F : ", L"G : ",
    L"Z : ", L"X : ", L"C : ", L"V : ", L"B : "
};
inline std::vector<std::wstring> g_soundbiteNames;
inline NOTIFYICONDATAW nid = { 0 };
inline const wchar_t CLASS_NAME[] = L"TrayIconWindowClass";
inline std::atomic<bool> g_running{ true };
inline DWORD g_trayThreadId = 0;
inline LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void TrayLoop();