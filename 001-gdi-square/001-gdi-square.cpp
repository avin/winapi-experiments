// 001-gdi-square.cpp : Defines the entry point for the application.
//

#include "001-gdi-square.h"
#include "framework.h"

constexpr auto MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst; // current instance
WCHAR szTitle[MAX_LOADSTRING]; // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

HBRUSH hAppBakgroundBrush = CreateSolidBrush(RGB(229, 232, 235));
HBRUSH hSquareFillBrush = CreateSolidBrush(RGB(138, 187, 255));
HPEN hSquareBorderPen = CreatePen(PS_SOLID, 4, RGB(45, 114, 210));

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY001GDISQUARE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY001GDISQUARE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY001GDISQUARE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY001GDISQUARE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    // wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hbrBackground = hAppBakgroundBrush;

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, 0, 1000, 1000, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

void invalidateDrawArea(const HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);

    rect.top = 40;

    InvalidateRect(hWnd, &rect, TRUE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int sx, sy;
    static auto sizeFactor = 1.;
    static auto isAutoResizing = FALSE;
    static auto autoResizingDirection = +1;

    switch (message)
    {
    case WM_CREATE:
    {
        CreateWindowEx(
            0,
            L"BUTTON",
            L"Start",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_FLAT,
            10, 10, 150, 30,
            hWnd,
            (HMENU)IDC_TOGGLE_BUTTON,
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            nullptr);
    }
    break;
    case WM_COMMAND:
    {
        const int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
        {
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
        }
        break;
        case IDM_EXIT:
        {
            DestroyWindow(hWnd);
        }
        break;
        case IDC_TOGGLE_BUTTON:
        {
            const wchar_t *newLabel;
            KillTimer(hWnd, IDT_AUTO_RESIZE_TIMER);
            if (isAutoResizing)
            {
                newLabel = L"Start";
                isAutoResizing = FALSE;
            }
            else
            {
                newLabel = L"Stop";
                SetTimer(hWnd, IDT_AUTO_RESIZE_TIMER, 100, nullptr);
                isAutoResizing = TRUE;
            }
            SetWindowTextW(GetDlgItem(hWnd, IDC_TOGGLE_BUTTON), newLabel);
        }
        break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_LBUTTONDOWN:
    {
        if (isAutoResizing)
        {
            autoResizingDirection *= -1;
        }
        else
        {
            sizeFactor = min(sizeFactor + .1, 2.);
            invalidateDrawArea(hWnd);
        }
    }
    break;
    case WM_RBUTTONDOWN:
    {
        if (isAutoResizing)
        {
            autoResizingDirection *= -1;
        }
        else
        {
            sizeFactor = max(sizeFactor - .1, .1);
            invalidateDrawArea(hWnd);
        }
    }
    break;
    case WM_SIZE:
    {
        sx = LOWORD(lParam);
        sy = HIWORD(lParam);
    }
    break;
    case WM_TIMER:
    {
        if (autoResizingDirection == 1)
        {
            sizeFactor = min(sizeFactor + .1, 2.);
        }
        else if (autoResizingDirection == -1)
        {
            sizeFactor = max(sizeFactor - .1, .1);
        }
        if (sizeFactor == 2. || sizeFactor == .1)
        {
            autoResizingDirection *= -1;
        }
        invalidateDrawArea(hWnd);
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        auto hOldBrush = SelectObject(hdc, hSquareFillBrush);
        auto hOldPen = SelectObject(hdc, hSquareBorderPen);

        auto halfX = sx / 2;
        auto halfY = sy / 2;

        auto k = 200 * sizeFactor;

        Rectangle(hdc, halfX - k, halfY - k, halfX + k, halfY + k);

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);

        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
    {
        DeleteObject(hAppBakgroundBrush);
        DeleteObject(hSquareFillBrush);
        DeleteObject(hSquareBorderPen);
        PostQuitMessage(0);
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    default:
        return FALSE;
    }
}
