#include <windows.h>
#include <tchar.h>

#include <iosfwd>
#include <sstream>

int processScrollMessage(const WPARAM wParam, const int minVal, const int maxVal, const int pageSize, const int curVal) {
    int scrollCode = LOWORD(wParam);

    switch (scrollCode) {
        case SB_LINEUP:
            return max(minVal, curVal - 1);
        case SB_LINEDOWN:
            return min(maxVal, curVal + 1);
        case SB_PAGEUP:
            return max(minVal, curVal - pageSize);
        case SB_PAGEDOWN:
            return min(maxVal, curVal + pageSize);
        case SB_THUMBTRACK:
            return HIWORD(wParam);
        default:
            return curVal;
    }
};


LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    static int arenaWidth{0}, arenaHeight{0};
    static int vScrollMin{0}, vScrollMax{200}, vScrollPos{20};
    static int hScrollMin{0}, hScrollMax{200}, hScrollPos{20};

    static int vScrollPageSize{0}, hScrollPageSize{0};

    switch (message) {
        case WM_SIZE: {
            arenaWidth = LOWORD(lParam);
            arenaHeight = HIWORD(lParam);

            vScrollPageSize = arenaHeight / 20;
            hScrollPageSize = arenaWidth / 20;

            SCROLLINFO si;
            si.cbSize = sizeof(si);
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

            // Настройка вертикального скроллбара
            si.nMin = vScrollMin;
            si.nMax = vScrollMax + vScrollPageSize - 1;
            si.nPage = vScrollPageSize;
            si.nPos = vScrollPos;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

            // Настройка горизонтального скроллбара
            si.nMin = hScrollMin;
            si.nMax = hScrollMax + hScrollPageSize - 1;
            si.nPage = hScrollPageSize;
            si.nPos = hScrollPos;
            SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

            break;
        }

        case WM_VSCROLL: {
            vScrollPos = processScrollMessage(wParam, vScrollMin, vScrollMax, vScrollPageSize, vScrollPos);
            SetScrollPos(hWnd, SB_VERT, vScrollPos, TRUE);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        case WM_HSCROLL: {
            hScrollPos = processScrollMessage(wParam, hScrollMin, hScrollMax, hScrollPageSize, hScrollPos);
            SetScrollPos(hWnd, SB_HORZ, hScrollPos, TRUE);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);

            if (delta > 0) {
                // Прокрутка вверх
                vScrollPos = max(vScrollMin, vScrollPos - 1);
            } else if (delta < 0) {
                // Прокрутка вниз
                vScrollPos = min(vScrollMax, vScrollPos + 1);
            }

            SetScrollPos(hWnd, SB_VERT, vScrollPos, TRUE);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcOrig = BeginPaint(hWnd, &ps);

            auto rect = RECT{};
            GetClientRect(hWnd, &rect);

            HDC hdc = CreateCompatibleDC(hdcOrig);
            HBITMAP hbm = CreateCompatibleBitmap(hdcOrig, rect.right, rect.bottom);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hbm);

            // --------------

            FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));

            std::wstringstream ws;
            ws << "Vert scroll [" << vScrollMin << L" - " << vScrollMax << "]: " << vScrollPos;
            TextOutW(hdc, 0, 0, ws.str().c_str(), ws.str().size());

            ws.str(L"");
            ws << "Horz scroll [" << hScrollMin << L" - " << hScrollMax << "]: " << hScrollPos;
            TextOutW(hdc, 0, 20, ws.str().c_str(), ws.str().size());

            // --------------

            BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY); // NOLINT(readability-suspicious-call-argument)

            SelectObject(hdc, hOldBitmap);
            DeleteObject(hbm);
            DeleteDC(hdc);

            EndPaint(hWnd, &ps);
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
    WNDCLASSEXW wcex = {};

    const auto className = L"MyClass";

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = className;
    // wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    RegisterClassExW(&wcex);

    const HWND hWnd = CreateWindowW(
        className,
        L"Scroll debug",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT,
        0,
        500,
        500,
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
