#include "Tray.h"
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TRAYICON:
            // left click: display soundbite keys and names
            if (lParam == WM_LBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                
                // populate keys and names
                for (int i = 0; i < g_soundbiteKeys.size(); i++) {
                    AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 0, g_soundbiteNames[i].c_str());
                }
                
                // dismisses menu when clicking away
                SetForegroundWindow(hwnd); 
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
            }
            // right click: display exit and handle exit event
            else if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();

                AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
                
                // dismisses menu when clicking away
                SetForegroundWindow(hwnd); 
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);            
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                // remove tray icon and close app
                Shell_NotifyIconW(NIM_DELETE, &nid);
                PostQuitMessage(0);
                g_running.store(false);
            }
            break;
            
        case WM_DESTROY:
            // remove tray icon and close app
            Shell_NotifyIconW(NIM_DELETE, &nid);
            PostQuitMessage(0);
            g_running.store(false);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void TrayLoop() {
    g_trayThreadId = GetCurrentThreadId();

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = TrayWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Soundboard App", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);

    // initialize and load tray icon
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1; 
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));
    lstrcpyW(nid.szTip, L"Soundboard");
    Shell_NotifyIconW(NIM_ADD, &nid);

    // standard win32 message loop
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}