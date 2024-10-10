#include <Windows.h>
#include <cmath>
#include "window1.h"
#include <gdiplus.h>
#include <atlbase.h>
#include <atlcom.h>
#pragma comment(lib, "gdiplus.lib")


#define ID_OPEN_WINDOW1 1

// Функция для инициализации GDI+
ULONG_PTR InitializeGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    return gdiplusToken;
}

// Функция для завершения работы GDI+
void ShutdownGDIPlus(ULONG_PTR gdiplusToken) {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    static HWND hChildWindow;
    static auto animationTick{0};
    static const int radiusX = 600; // Радиус эллипса по горизонтали
    static const int radiusY = 100; // Радиус эллипса по вертикали

    switch (message) {
        case WM_CREATE: {
            CreateWindow(L"BUTTON", L"Open moon window", WS_VISIBLE | WS_CHILD, 20, 20, 200, 30, hWnd, (HMENU)ID_OPEN_WINDOW1, NULL, NULL);

            break;
        }
        case WM_TIMER: {
            // Увеличиваем угол для движения по эллипсу
            animationTick += 2; // Шаг для изменения угла.
            if (animationTick >= 360) animationTick = 0; // Ограничиваем значение до 360

            // Получаем позицию родительского окна
            RECT rect;
            GetWindowRect(hWnd, &rect);
            int centerX = rect.left + (rect.right - rect.left) / 2;
            int centerY = rect.top + (rect.bottom - rect.top) / 2;

            // Рассчитываем координаты дочернего окна по эллипсу
            int x = centerX + (int)(radiusX * cos(animationTick * 3.14159 / 180.0)) - 100; // -100 чтобы сдвинуть в центр
            int y = centerY + (int)(radiusY * sin(animationTick * 3.14159 / 180.0)) - 100; // -100 чтобы сдвинуть в центр

            // Определяем Z-порядок в зависимости от угла: верхняя часть - за окном, нижняя - перед окном
            HWND zOrder = (sin(animationTick * 3.14159 / 180.0) < 0) ? hWnd : HWND_TOP;
            SetWindowPos(hChildWindow, zOrder, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

            ShowWindow(hChildWindow, SW_SHOW); // Теперь можно показать окно

            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_OPEN_WINDOW1: {
                    if (IsWindow(hChildWindow)) {
                        break;
                    }

                    RECT rect;
                    GetWindowRect(hWnd, &rect);

                    // WS_EX_LAYERED | WS_EX_TRANSPARENT,
                    hChildWindow = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT,
                                                  L"Window1Class",
                                                  L"Window1",
                                                  // без WS_VISIBLE, окно покажем после просчета позиции в таймере
                                                  WS_DISABLED | WS_SYSMENU | WS_THICKFRAME | WS_CAPTION,
                                                  1000, 1000,
                                                  200, 200,
                                                  NULL, //hWnd,
                                                  0,
                                                  (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                                                  NULL
                        );

                    SetTimer(hWnd, 1, 10, NULL);
                    break;
                }
            }
            break;
        }

        case WM_DESTROY: {
            KillTimer(hWnd, 1);
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
    ULONG_PTR gdiplusToken = InitializeGDIPlus();

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

    ShutdownGDIPlus(gdiplusToken);

    return static_cast<int>(msg.wParam);
}
