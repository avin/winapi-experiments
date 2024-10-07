#include "ChartControl.h"

#include <cmath>
#include <functional>
#include <string>


LRESULT CALLBACK ChartWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  static int sx, sy;
  static auto zoom = 100.0;
  static POINT offsetPoint{0, 0};
  static POINT beforeDragOffsetPoint{0, 0};
  static POINT startDragPoint{0, 0};
  static BOOL isDragging = FALSE;

  static HPEN hDrawPenRed = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
  static HPEN hDrawPenGreen = CreatePen(PS_SOLID, 2, RGB(0, 200, 0));
  static HPEN hDrawPenBlue = CreatePen(PS_SOLID, 2, RGB(0, 0, 255));

  static auto px = [](double x) {
    return sx / 2 + static_cast<int>(x * zoom) + offsetPoint.x;
  };

  static auto py = [](double y) {
    return sy - (sy / 2 + static_cast<int>(y * zoom)) + offsetPoint.y;
  };

  static auto getMinX = []() -> double {
    return (0 - sx / 2 - offsetPoint.x) / zoom;
  };
  static auto getMaxX = []() -> double {
    return (sx - sx / 2 - offsetPoint.x) / zoom;
  };

  static auto getMinY = []() -> double {
    return (sy - sy - sy / 2 + offsetPoint.y) / zoom;
  };
  static auto getMaxY = []() -> double {
    return (sy - 0 - sy / 2 + offsetPoint.y) / zoom;
  };

  auto drawChartLine = [](HDC hdc, HPEN hPen, std::function<double(double)> func) {
    auto hOldPen = SelectObject(hdc, hPen);

    auto isFirstPoint = true;
    for (auto x = getMinX(); x < getMaxX(); x += (1. / zoom)) {
      auto y = func(x);

      auto rx = px(x);
      auto ry = py(y);
      if (rx > 0 && rx < sx && ry > 0 && ry < sy) {
        if (isFirstPoint) {
          MoveToEx(hdc, rx, ry, NULL);
        } else {
          LineTo(hdc, rx, ry);
        }
        isFirstPoint = false;
      } else {
        isFirstPoint = true;
      }
    }

    SelectObject(hdc, hOldPen);
  };

  switch (msg) {
  case WM_CREATE: {
    // 
    break;
  }
  case WM_LBUTTONDOWN: {
    int xPos = LOWORD(lParam);
    int yPos = HIWORD(lParam);

    startDragPoint.x = xPos;
    startDragPoint.y = yPos;
    beforeDragOffsetPoint = offsetPoint;
    isDragging = TRUE;

    break;
  }
  case WM_RBUTTONDOWN: {
    MessageBoxW(
        hWnd,
        std::to_wstring(getMinX()).c_str(),
        std::to_wstring(getMinY()).c_str(),
        MB_OK);

    break;
  }
  case WM_LBUTTONUP: {
    isDragging = FALSE;
    break;
  }
  case WM_MOUSEMOVE: {
    if (isDragging) {
      int xPos = LOWORD(lParam);
      int yPos = HIWORD(lParam);

      offsetPoint.x = beforeDragOffsetPoint.x - (startDragPoint.x - xPos);
      offsetPoint.y = beforeDragOffsetPoint.y - (startDragPoint.y - yPos);

      InvalidateRect(hWnd, NULL, TRUE);
    }
    break;
  }
  case WM_MOUSEWHEEL: {
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    if (delta > 0) {
      zoom *= 1.1;
    } else if (delta < 0) {
      zoom /= 1.1;
    }
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

    FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));

    // Рисуем оси координат
    {
      // Y
      MoveToEx(hdc, px(0), py(getMinY()), NULL);
      LineTo(hdc, px(0), py(getMaxY()));

      // X
      MoveToEx(hdc, px(getMinX()), py(0), NULL);
      LineTo(hdc, px(getMaxX()), py(0));
    }

    // Рисуем линии графика
    {
      drawChartLine(hdc, hDrawPenRed, [](double x) { return sin(x); });
      drawChartLine(hdc, hDrawPenGreen, [](double x) { return cos(x); });
      drawChartLine(hdc, hDrawPenBlue, [](double x) { return x + 2.; });
      drawChartLine(hdc, hDrawPenBlue, [](double x) { return 5./x; });
    }

    // -------------------------

    BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY); // NOLINT(readability-suspicious-call-argument)

    SelectObject(hdc, hOldBitmap);
    DeleteObject(hbm);
    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
    break;
  }

  case WM_SIZE: {
    sx = LOWORD(lParam);
    sy = HIWORD(lParam);
    break;
  }

  case WM_DESTROY: {
    DeleteObject(hDrawPenRed);
    DeleteObject(hDrawPenGreen);
    DeleteObject(hDrawPenBlue);
    break;
  }
  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}


void RegisterChartControl(HINSTANCE hInstance) {
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.lpfnWndProc = ChartWndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"ChartControl";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassEx(&wc);
}
