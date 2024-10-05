#include "StopwatchControl.h"

#include <sstream>
#include <string>
#include <windows.h>
#include <tchar.h>

LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {

  static HWND hWndStopwatch;

  switch (message) {
  case WM_CREATE: {
    hWndStopwatch = CreateWindow(
        _T("StopwatchControl"),
        nullptr,
        WS_VISIBLE | WS_CHILD,
        30,
        20,
        300,
        300,
        hWnd,
        nullptr,
        ((LPCREATESTRUCT)lParam)->hInstance,
        nullptr);

    // Buttons for timer control
    CreateWindow(_T("BUTTON"), _T("Start"), WS_VISIBLE | WS_CHILD, 20, 340, 100, 30, hWnd, (HMENU)1, NULL, NULL);
    CreateWindow(_T("BUTTON"), _T("Stop"), WS_VISIBLE | WS_CHILD, 130, 340, 100, 30, hWnd, (HMENU)2, NULL, NULL);
    CreateWindow(_T("BUTTON"), _T("Reset"), WS_VISIBLE | WS_CHILD, 240, 340, 100, 30, hWnd, (HMENU)3, NULL, NULL);
    CreateWindow(_T("BUTTON"), _T("Show Status"), WS_VISIBLE | WS_CHILD, 20, 380, 100, 30, hWnd, (HMENU)4, NULL, NULL);
    CreateWindow(_T("BUTTON"), _T("Show Time"), WS_VISIBLE | WS_CHILD, 130, 380, 100, 30, hWnd, (HMENU)5, NULL, NULL);
    break;
  }

  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case 1: {
      SendMessage(hWndStopwatch, WM_COMMAND, WM_COMMAND_TIMER_START, lParam);

      break;
    }
    case 2: {
      SendMessage(hWndStopwatch, WM_COMMAND, WM_COMMAND_TIMER_STOP, lParam);
      break;
    }
    case 3: {
      SendMessage(hWndStopwatch, WM_COMMAND, WM_COMMAND_TIMER_RESET, lParam);
      break;
    }
    case 4: {
      LRESULT isRunning = SendMessage(hWndStopwatch, WM_GET_TIMER_STATUS, 0, 0);
      MessageBox(hWnd, isRunning ? L"Timer is running" : L"Timer is stopped", L"Status", MB_OK);
      break;
    }
    case 5: {
      LRESULT timeInMilliSeconds = SendMessage(hWndStopwatch, WM_GET_TIMER_TIME, 0, 0);
      double timeInSeconds = static_cast<double>(timeInMilliSeconds) / 1000.;

      std::wstringstream wss;
      wss << L"Elapsed Time: " << timeInSeconds << L" seconds";
      MessageBox(hWnd, wss.str().c_str(), L"Time", MB_OK);
      break;
    }
    }
    break;
  }

  case WM_TIMER: {
    //
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

  RegisterStopwatchControl(hInstance);

  const HWND hWnd = CreateWindowW(
      className,
      L"Stopwatch",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT,
      0,
      380,
      470,
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
