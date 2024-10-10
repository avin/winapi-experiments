#include <Windows.h>

#include "window1.h"

#define ID_OPEN_WINDOW1 1

LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            CreateWindow(L"BUTTON", L"Open window", WS_VISIBLE | WS_CHILD, 20, 20, 100, 30, hWnd, (HMENU)ID_OPEN_WINDOW1, NULL, NULL);

            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_OPEN_WINDOW1: {
                    RECT rect;
                    GetWindowRect(hWnd, &rect);

                    auto hWindow1 = CreateWindow(L"Window1Class",
                                                 L"Window1",
                                                 WS_SYSMENU | WS_POPUP | WS_VISIBLE | WS_THICKFRAME | WS_CAPTION,
                                                 rect.left + 40, rect.top + 40,
                                                 200, 200,
                                                 hWnd,
                                                 0,
                                                 (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                                                 NULL
                        );
                    break;
                }
            }
            break;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow
    ) {
    WNDCLASSEX wcex{};

    auto const className = L"MainWindowClass";

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = className;
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    RegisterClassExW(&wcex);

    RegisterWindow1Class(hInstance);

    const HWND hWnd = CreateWindowW(
        className,
        L"Test windows",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        0,
        800,
        800,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}
