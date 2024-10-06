#include "ChartControl.h"


LRESULT CALLBACK ChartWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

  switch (msg) {
  case WM_CREATE: {
    // 
    break;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdcOrig = BeginPaint(hWnd, &ps);

    RECT rect;
    GetClientRect(hWnd, &rect);

    HDC hdc = CreateCompatibleDC(hdcOrig);
    HBITMAP hbm = CreateCompatibleBitmap(hdcOrig, rect.right, rect.bottom);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hbm);

    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

    //

    BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY); // NOLINT(readability-suspicious-call-argument)

    SelectObject(hdc, hOldBitmap);
    DeleteObject(hbm);
    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
    break;
  }

  case WM_DESTROY: {
    // 
    break;
  }
  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}


void RegisterChartControl(HINSTANCE hInstance) {
  WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
  wc.lpfnWndProc = ChartWndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"ChartControl";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassEx(&wc);
}
