#include "StopwatchControl.h"
#include <chrono>
#include <string>

constexpr double M_PI = 3.14159265358979323846;

// Оконная процедура для обработки сообщений таймера
LRESULT CALLBACK StopwatchWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  static bool running = false;

  using clock = std::chrono::high_resolution_clock;
  static clock::time_point lastTime;
  static double resultTime{0};

  static HPEN hArrowPen = CreatePen(PS_SOLID, 1, RGB(48, 64, 77));
  static HPEN hDialLinePen = CreatePen(PS_SOLID, 1, RGB(48, 64, 77));
  static HPEN hBorderPen = CreatePen(PS_SOLID, 4, RGB(48, 64, 77));
  static HBRUSH hBrushClockBackground = CreateSolidBrush(RGB(245, 248, 250));

  switch (msg) {
  case WM_CREATE: {
    // 
    break;
  }
  case WM_TIMER: {
    auto currentTime = clock::now();
    std::chrono::duration<double> deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    resultTime += deltaTime.count();

    InvalidateRect(hWnd, NULL, TRUE);
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

    auto text = std::to_wstring(resultTime);
    TextOut(hdc, 0, 0, text.c_str(), text.size());

    auto areaWidth = rect.right;
    auto mid = areaWidth / 2;
    auto padding = areaWidth / 10;
    auto clockSize = areaWidth - padding * 2;
    auto clockRadius = clockSize / 2;

    auto hOldBrush = SelectObject(hdc, hBrushClockBackground);
    Ellipse(hdc, padding, padding, padding + clockSize, padding + clockSize);
    SelectObject(hdc, hOldBrush);

    // Dial lines
    {
      auto hOldPen = SelectObject(hdc, hDialLinePen);
      for (int i = 0.; i < 60.; ++i) {
        auto angle = M_PI * 2 * (i / 60.) - M_PI / 2.;

        auto size = 5;
        if (i % 5 == 0) {
          size = 10;
        }

        auto fromX = mid + (clockRadius - size) * cos(angle);
        auto fromY = mid + (clockRadius - size) * sin(angle);

        auto toX = mid + clockRadius * cos(angle);
        auto toY = mid + clockRadius * sin(angle);

        MoveToEx(hdc, fromX, fromY, NULL);
        LineTo(hdc, toX, toY);
      }
      SelectObject(hdc, hOldPen);
    }

    // Arrow
    {
      auto angle = M_PI * 2 * (resultTime / 60.) - M_PI / 2.;

      auto fromX = mid + clockRadius / 6 * cos(angle - M_PI);
      auto fromY = mid + clockRadius / 6 * sin(angle - M_PI);

      auto toX = mid + clockRadius * cos(angle);
      auto toY = mid + clockRadius * sin(angle);

      auto hOldPen = SelectObject(hdc, hArrowPen);
      MoveToEx(hdc, fromX, fromY, NULL);
      LineTo(hdc, toX, toY);
      SelectObject(hdc, hOldPen);
    }

    auto boltRadius = clockRadius/10;
    Ellipse(hdc, mid - boltRadius, mid - boltRadius, mid + boltRadius, mid + boltRadius);

    BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY); // NOLINT(readability-suspicious-call-argument)

    SelectObject(hdc, hOldBitmap);
    DeleteObject(hbm);
    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
    break;
  }

  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case WM_COMMAND_TIMER_START:
      lastTime = clock::now();
      SetTimer(hWnd, 1, 1000 / 60, NULL);
      break;
    case WM_COMMAND_TIMER_STOP:
      KillTimer(hWnd, 1);
      break;
    case WM_COMMAND_TIMER_RESET:
      resultTime = 0.0;
      InvalidateRect(hWnd, NULL, TRUE);
      break;
    }
    break;
  }

  // Обработка кастомных сообщений
  case WM_GET_TIMER_STATUS: {
    return running;
  }
  case WM_GET_TIMER_TIME: {
    // double seconds = elapsedTime.count();
    // if (running) {
    //   seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
    // }
    // return static_cast<LRESULT>(seconds); // Возвращаем количество секунд
  }

  case WM_DESTROY: {
    DeleteObject(hArrowPen);
    DeleteObject(hDialLinePen);
    DeleteObject(hBorderPen);
    DeleteObject(hBrushClockBackground);

    KillTimer(hWnd, 1);
    break;
  }
  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}


void RegisterStopwatchControl(HINSTANCE hInstance) {
  WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
  wc.lpfnWndProc = StopwatchWndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"StopwatchControl";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassEx(&wc);
}
