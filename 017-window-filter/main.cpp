#include <windows.h>

const wchar_t szWindowClass[] = L"MyWindowClass";
const wchar_t szTitle[] = L"My Window";

HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL CaptureScreen(HDC hdcDest, RECT rcDest);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szWindowClass;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST,
        szWindowClass,
        szTitle,
        WS_POPUP, // Без заголовочной полосы
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
        );

    if (!hWnd) {
        return FALSE;
    }

    hInst = hInstance;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

BOOL CaptureScreen(HDC hdcDest, RECT rcDest) {
    HDC hdcScreen = GetDC(NULL); // DC всего экрана

    // Создаем регион, исключающий наше окно
    HRGN hRgnScreen = CreateRectRgn(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    HRGN hRgnWindow = CreateRectRgn(rcDest.left, rcDest.top, rcDest.right, rcDest.bottom);
    CombineRgn(hRgnScreen, hRgnScreen, hRgnWindow, RGN_DIFF);

    SelectClipRgn(hdcScreen, hRgnScreen);

    BOOL bRet = BitBlt(hdcDest, 0, 0, rcDest.right - rcDest.left, rcDest.bottom - rcDest.top,
                       hdcScreen, rcDest.left, rcDest.top, SRCCOPY);

    // Освобождаем ресурсы
    SelectClipRgn(hdcScreen, NULL);
    DeleteObject(hRgnScreen);
    DeleteObject(hRgnWindow);
    ReleaseDC(NULL, hdcScreen);

    return bRet;
}

BOOL bDragging = FALSE;
POINT ptLast;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HMENU hMenu;

    switch (message) {
        case WM_CREATE: {
            // Создаем меню
            hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1, L"Выйти");

            // Устанавливаем таймер для регулярного обновления окна
            SetTimer(hWnd, 1, 30, NULL); // Обновление каждые 30 мс (~33 кадра в секунду)
        }
        break;
        case WM_TIMER: {
            // Перерисовываем окно
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            RECT rc;
            GetClientRect(hWnd, &rc);
            HDC hdc = BeginPaint(hWnd, &ps);

            POINT ptWindow = {0, 0};
            ClientToScreen(hWnd, &ptWindow);
            RECT rcWindow = {ptWindow.x, ptWindow.y, ptWindow.x + rc.right, ptWindow.y + rc.bottom};

            CaptureScreen(hdc, rcWindow);

            EndPaint(hWnd, &ps);
        }
        break;
        case WM_LBUTTONDOWN: {
            bDragging = TRUE;
            SetCapture(hWnd);
            ptLast.x = LOWORD(lParam);
            ptLast.y = HIWORD(lParam);
        }
        break;
        case WM_MOUSEMOVE: {
            if (bDragging) {
                POINT pt;
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);

                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);

                int dx = pt.x - ptLast.x;
                int dy = pt.y - ptLast.y;

                MoveWindow(hWnd, rcWindow.left + dx, rcWindow.top + dy,
                           rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, TRUE);
            }
        }
        break;
        case WM_LBUTTONUP: {
            bDragging = FALSE;
            ReleaseCapture();
        }
        break;
        case WM_RBUTTONUP: {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ClientToScreen(hWnd, &pt);

            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        break;
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) // "Выйти"
            {
                PostQuitMessage(0);
            }
        }
        break;
        case WM_DESTROY:
            KillTimer(hWnd, 1); // Останавливаем таймер
            DestroyMenu(hMenu);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
