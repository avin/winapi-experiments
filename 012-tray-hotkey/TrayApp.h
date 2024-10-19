#pragma once
#ifndef TRAYAPP_H
#define TRAYAPP_H

#include <windows.h>
#include <wchar.h>
#include <shellapi.h>
#include <stdexcept>

class TrayApp {
public:
    TrayApp(HINSTANCE hInstance);
    ~TrayApp();

    int Run();

private:
    static TrayApp* s_instance;
    HINSTANCE m_hInst;
    HWND m_hMainWnd;
    HHOOK m_hKeyboardHook;
    NOTIFYICONDATAW m_nid;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI ShowMessageBoxThread(LPVOID lpParam);

    void AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();
    void InstallKeyboardHook();
    void RemoveKeyboardHook();

    void OnHotkey();
    void OnExit();
};

#endif // TRAYAPP_H
