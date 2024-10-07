#include "ChartControl.h"

#include <windows.h>
#include <tchar.h>

LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {

  static HWND hWndChart;

  switch (message) {
  case WM_CREATE: {
    hWndChart = CreateWindow(
        L"ChartControl",
        nullptr,
        WS_VISIBLE | WS_CHILD,
        20,
        20,
        300,
        300,
        hWnd,
        nullptr,
        ((LPCREATESTRUCT)lParam)->hInstance,
        nullptr);
    break;
  }

  case WM_SIZE: {
    auto w = LOWORD(lParam);
    auto h = HIWORD(lParam);

    SetWindowPos(hWndChart, NULL, 0, 0, w, h, NULL);
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
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

  RegisterClassExW(&wcex);

  RegisterChartControl(hInstance);

  const HWND hWnd = CreateWindowW(
      className,
      L"Chart (drag/zoom with mouse)",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      0,
      1000,
      600,
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
