#include "window1.h"

LRESULT CALLBACK Wnd1Proc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            CreateWindow(L"static", L"Hello from window1", WS_VISIBLE | WS_CHILD, 10, 20, 150, 30, hWnd, 0, NULL, NULL);

            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                //
            }
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


ATOM RegisterWindow1Class(HINSTANCE hInst) {
    auto const className = L"Window1Class";

    WNDCLASSEX wcgraph = {0};
    wcgraph.cbSize = sizeof(WNDCLASSEX);
    wcgraph.style = CS_HREDRAW | CS_VREDRAW;
    wcgraph.lpfnWndProc = Wnd1Proc;
    wcgraph.hInstance = hInst;
    wcgraph.hCursor = LoadCursor(NULL, IDC_CROSS);
    wcgraph.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcgraph.lpszClassName = className;
    // wcgraph.hIconSm = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
    return RegisterClassEx(&wcgraph);
}
