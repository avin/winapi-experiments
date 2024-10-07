#include "ChartControl.h"

#include <cmath>
#include <functional>
#include <sstream>
#include <string>

enum class LineType {
  Common, Solid, Root
};

void DebugOutputImpl(const wchar_t* format, ...) {
  wchar_t msg[255] = {0};

  va_list args;
  va_start(args, format);
  vswprintf(msg, sizeof(msg) / sizeof(wchar_t), format, args);
  va_end(args);

  OutputDebugStringW(msg);
}

#ifdef _DEBUG
#define DebugOutput(format, ...) \
        DebugOutputImpl(format, __VA_ARGS__)
#else
    #define DebugOutput(format, ...) \
        do {} while (0)  // Пустой макрос, который не делает ничего
#endif

bool areAlmostEqual(double a, double b, double epsilon = 1e-6) {
  return std::fabs(a - b) < epsilon;
}

void DrawCenteredText(HDC hdc, const wchar_t* text, int centerX, int centerY) {
  // Структура для хранения размеров текста
  SIZE textSize;

  // Получаем размеры текста для текущего шрифта и текста
  GetTextExtentPoint32(hdc, text, wcslen(text), &textSize);

  // Вычисляем координаты верхнего левого угла для центрирования текста
  int textX = centerX - (textSize.cx / 2);
  int textY = centerY - (textSize.cy / 2);

  // Рисуем текст в вычисленных координатах
  TextOut(hdc, textX, textY, text, wcslen(text));
}

LRESULT CALLBACK ChartWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  static int sx, sy;
  static auto zoom = 100.0;
  static POINT offsetPoint{0, 0};
  static POINT beforeDragOffsetPoint{0, 0};
  static POINT startDragPoint{0, 0};
  static BOOL isDragging = FALSE;

  // Линии сетки
  static HPEN hLinePenRoot = CreatePen(PS_SOLID, 2, RGB(50, 50, 50));
  static HPEN hLinePenSignificant = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));
  static HPEN hLinePenCommon = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));

  static HPEN hDrawPenRed = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
  static HPEN hDrawPenGreen = CreatePen(PS_SOLID, 2, RGB(0, 200, 0));
  static HPEN hDrawPenBlue = CreatePen(PS_SOLID, 2, RGB(0, 0, 255));

  static auto px = [](double x) {
    return sx / 2 + static_cast<int>(x * zoom) + offsetPoint.x;
  };

  static auto py = [](double y) {
    return sy - (sy / 2 + static_cast<int>(y * zoom)) + offsetPoint.y;
  };

  static auto screenXToWorldX = [](int x) {
    return (static_cast<double>(x) - static_cast<double>(sx) / 2 - offsetPoint.x) / zoom;
  };
  static auto screenYToWorldY = [](int y) {
    return (sy - y - static_cast<double>(sy) / 2 + offsetPoint.y) / zoom;
  };

  static auto getMinX = []() -> double {
    return screenXToWorldX(0);
  };
  static auto getMaxX = []() -> double {
    return screenXToWorldX(sx);
  };

  static auto getMinY = []() -> double {
    return screenYToWorldY(sy);
  };
  static auto getMaxY = []() -> double {
    return screenYToWorldY(0);
  };

  auto drawChartLine = [](HDC hdc, HPEN hPen, std::function<double(double)> func) {
    auto hOldPen = SelectObject(hdc, hPen);

    auto isFirstPoint = true;
    for (auto x = getMinX(); x < getMaxX(); x += (3. / zoom)) {
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
    // Текущая позиция курсора в клиентских координатах
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(hWnd, &cursorPos);

    // Преобразуем координаты курсора в мировые координаты
    double worldX = screenXToWorldX(cursorPos.x);
    double worldY = screenYToWorldY(cursorPos.y);

    // Сохраняем текущее значение зума для корректировки offsetPoint
    auto prevZoom = zoom;

    // Изменение масштаба
    if (delta > 0) {
      zoom *= 1.1;
    } else if (delta < 0) {
      zoom /= 1.1;
    }

    // Корректируем offsetPoint так, чтобы зумирование было относительно курсора
    offsetPoint.x += static_cast<int>((prevZoom - zoom) * worldX);
    offsetPoint.y -= static_cast<int>((prevZoom - zoom) * worldY);

    // Перерисовываем окно
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
      // Градация Y оси
      // {
      //   auto diff = (getMaxY() - getMinY());
      //   double step = pow(10, std::floor(std::log10(diff / 20)));
      //   while (static_cast<int>(step * zoom) <= 10) {
      //     step *= 5;
      //
      //   }
      //   double bigStep = step * 5;
      //   auto q = std::floor(getMinY() / step) * step;
      //
      //   for (auto i = q; i < getMaxY(); i += step) {
      //     auto w = 5;
      //     auto z = std::abs(static_cast<int>(i / bigStep * 1000.) % 1000);
      //
      //     if (z >= 999 || z <= 1 && (py(i) != py(0))) {
      //       w = 15;
      //       std::wstringstream ws{};
      //       ws << i;
      //       DrawCenteredText(
      //           hdc,
      //           ws.str().c_str(),
      //           px(-40. / zoom),
      //           py(i));
      //     }
      //     MoveToEx(hdc, 0, py(i), NULL);
      //     LineTo(hdc, sx, py(i));
      //   }
      // }

      // Градация X оси

      auto drawGridLines = [&](double min, double max, bool isVertical) {
        auto diff = (max - min);
        double step = pow(10, std::floor(std::log10(diff / 20)));
        while (static_cast<int>(step * zoom) <= 10) {
          step *= 5;

        }
        double bigStep = step * 5;
        auto q = std::floor(min / step) * step;

        for (auto i = q; i < max; i += step) {
          auto lineType = LineType::Common;
          auto z = std::abs(static_cast<int>(i / bigStep * 1000.) % 1000);

          if (z >= 999 || z <= 1) {
            if ((isVertical && px(i) == px(0)) || (!isVertical && py(i) == py(0))) {
              lineType = LineType::Root;
            } else {
              lineType = LineType::Solid;
            }
          }
          auto hLinePen = hLinePenCommon;
          if (lineType == LineType::Solid) {
            hLinePen = hLinePenSignificant;
          } else if (lineType == LineType::Root) {
            hLinePen = hLinePenRoot;
          }
          auto hOldPen = SelectObject(hdc, hLinePen);
          if (isVertical) {
            MoveToEx(hdc, px(i), 0, NULL);
            LineTo(hdc, px(i), sy);
          } else {
            MoveToEx(hdc, 0, py(i), NULL);
            LineTo(hdc, sx, py(i));
          }

          SelectObject(hdc, hOldPen);

          if (lineType == LineType::Solid) {
            std::wstringstream ws{};
            ws << i;
            if (isVertical) {
              DrawCenteredText(
                  hdc,
                  ws.str().c_str(),
                  px(i),
                  py(-40. / zoom));
            } else {
              DrawCenteredText(
                  hdc,
                  ws.str().c_str(),
                  px(-40. / zoom),
                  py(i));
            }

          }
        }
      };

      drawGridLines(getMinX(), getMaxX(), true);
      drawGridLines(getMinY(), getMaxY(), false);

      // {
      //   auto diff = (getMaxX() - getMinX());
      //   double step = pow(10, std::floor(std::log10(diff / 20)));
      //   while (static_cast<int>(step * zoom) <= 10) {
      //     step *= 5;
      //
      //   }
      //   double bigStep = step * 5;
      //   auto q = std::floor(getMinX() / step) * step;
      //
      //   for (auto i = q; i < getMaxX(); i += step) {
      //     auto lineType = LineType::Common;
      //     auto z = std::abs(static_cast<int>(i / bigStep * 1000.) % 1000);
      //
      //     if (z >= 999 || z <= 1) {
      //
      //       if (px(i) == px(0)) {
      //         lineType = LineType::Root;
      //       } else {
      //         lineType = LineType::Solid;
      //       }
      //
      //     }
      //     auto hLinePen = hLinePenCommon;
      //     if (lineType == LineType::Solid) {
      //       hLinePen = hLinePenSignificant;
      //     } else if (lineType == LineType::Root) {
      //       hLinePen = hLinePenRoot;
      //     }
      //     auto hOldPen = SelectObject(hdc, hLinePen);
      //     MoveToEx(hdc, px(i), 0, NULL);
      //     LineTo(hdc, px(i), sy);
      //     SelectObject(hdc, hOldPen);
      //
      //     if (lineType == LineType::Solid) {
      //       std::wstringstream ws{};
      //       ws << i;
      //       DrawCenteredText(
      //           hdc,
      //           ws.str().c_str(),
      //           px(i),
      //           py(-40. / zoom));
      //     }
      //   }
      // }
    }

    // Рисуем линии графика
    {
      drawChartLine(hdc, hDrawPenRed, [](double x) { return sin(x); });
      drawChartLine(hdc, hDrawPenGreen, [](double x) { return cos(x); });
      drawChartLine(hdc, hDrawPenBlue, [](double x) { return x + 2.; });
      drawChartLine(hdc, hDrawPenBlue, [](double x) { return 5. / x; });
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
    DeleteObject(hLinePenRoot);
    DeleteObject(hLinePenSignificant);
    DeleteObject(hLinePenCommon);

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
