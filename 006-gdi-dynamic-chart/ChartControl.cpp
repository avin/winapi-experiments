#include "ChartControl.h"

#include "Debug.h"

#include <cmath>
#include <functional>
#include <sstream>
#include <string>

enum class LineType {
  Common, Solid, Root
};

void DrawCenteredText(HDC hdc, const wchar_t* text, int centerX, int centerY) {
  SIZE textSize;

  // Получаем размеры текста для текущего шрифта и текста
  GetTextExtentPoint32(hdc, text, wcslen(text), &textSize);

  int textX = centerX - (textSize.cx / 2);
  int textY = centerY - (textSize.cy / 2);

  TextOut(hdc, textX, textY, text, wcslen(text));
}

void DrawCenteredRightText(HDC hdc, const wchar_t* text, int centerX, int centerY) {
  SIZE textSize;

  GetTextExtentPoint32(hdc, text, wcslen(text), &textSize);

  int textX = centerX - (textSize.cx);
  int textY = centerY - (textSize.cy / 2);

  TextOut(hdc, textX, textY, text, wcslen(text));
}

class ChartControl {
  HPEN m_hLinePenRoot, m_hLinePenSignificant, m_hLinePenCommon, m_hDrawPenRed, m_hDrawPenGreen, m_hDrawPenBlue;

  int Px(double x) const {
    return m_sx / 2 + static_cast<int>(x * m_zoom) + m_offsetPoint.x;
  }

  int Py(double y) const {
    return m_sy - (m_sy / 2 + static_cast<int>(y * m_zoom)) + m_offsetPoint.y;
  }

  double GetMinX() const {
    return ScreenXToWorldX(0);
  }

  double GetMaxX() const {
    return ScreenXToWorldX(m_sx);
  }

  double GetMinY() const {
    return ScreenYToWorldY(m_sy);
  }

  double GetMaxY() const {
    return ScreenYToWorldY(0);
  }

public:
  int m_sx{0}, m_sy{0};
  double m_zoom = 100.0;
  POINT m_offsetPoint{0, 0};

  ChartControl() {
    m_hLinePenRoot = CreatePen(PS_SOLID, 2, RGB(50, 50, 50));
    m_hLinePenSignificant = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));
    m_hLinePenCommon = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));

    m_hDrawPenRed = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
    m_hDrawPenGreen = CreatePen(PS_SOLID, 2, RGB(0, 200, 0));
    m_hDrawPenBlue = CreatePen(PS_SOLID, 2, RGB(0, 0, 255));
  }

  ~ChartControl() {
    DeleteObject(m_hLinePenRoot);
    DeleteObject(m_hLinePenSignificant);
    DeleteObject(m_hLinePenCommon);
    DeleteObject(m_hDrawPenRed);
    DeleteObject(m_hDrawPenGreen);
    DeleteObject(m_hDrawPenBlue);
  }

  double ScreenXToWorldX(int x) const {
    return (static_cast<double>(x) - static_cast<double>(m_sx) / 2 - m_offsetPoint.x) / m_zoom;
  }

  double ScreenYToWorldY(int y) const {
    return (m_sy - y - static_cast<double>(m_sy) / 2 + m_offsetPoint.y) / m_zoom;
  }

  auto GetRedPen() const {
    return m_hDrawPenRed;
  }

  auto GetBluePen() const {
    return m_hDrawPenBlue;
  }

  auto GetGreenPen() const {
    return m_hDrawPenGreen;
  }

  template <typename Func>
  auto DrawChartLine(HDC hdc, HPEN hPen, Func func) const {
    const auto hOldPen = SelectObject(hdc, hPen);

    auto isFirstPoint = true;
    for (auto x = GetMinX(); x < GetMaxX(); x += (3. / m_zoom)) {
      auto y = func(x);

      auto const rx = Px(x);
      auto const ry = Py(y);
      if (rx > 0 && rx < m_sx && ry > 0 && ry < m_sy) {
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
  }

  auto DrawGrid(HDC hdc) const {

    const auto minValX = GetMinX();
    const auto maxValX = GetMaxX();
    const auto minValY = GetMinY();
    const auto maxValY = GetMaxY();

    const auto diffX = (maxValX - minValX);
    const auto diffY = (maxValY - minValY);
    const auto diff = max(diffX, diffY);
    double step = pow(10, std::floor(std::log10(diff / 20)));

    auto drawGridLines = [&](const double minVal, const double maxVal, const bool isVertical, const bool shouldDrawLines, const bool shouldDrawLabels) {
      while (static_cast<int>(step * m_zoom) <= 10) {
        step *= 5;
      }
      double bigStep = step * 5;
      auto q = std::floor(minVal / step) * step;

      for (auto i = q; i < maxVal; i += step) {
        auto lineType = LineType::Common;
        const auto z = std::abs(static_cast<int>(i / bigStep * 1000.) % 1000);

        if (z >= 999 || z <= 1) {
          if ((isVertical && Px(i) == Px(0)) || (!isVertical && Py(i) == Py(0))) {
            lineType = LineType::Root;
          } else {
            lineType = LineType::Solid;
          }
        }
        auto hLinePen = m_hLinePenCommon;
        if (lineType == LineType::Solid) {
          hLinePen = m_hLinePenSignificant;
        } else if (lineType == LineType::Root) {
          hLinePen = m_hLinePenRoot;
        }

        if(shouldDrawLines) {
          const auto hOldPen = SelectObject(hdc, hLinePen);
          if (isVertical) {
            MoveToEx(hdc, Px(i), 0, NULL);
            LineTo(hdc, Px(i), m_sy);
          } else {
            MoveToEx(hdc, 0, Py(i), NULL);
            LineTo(hdc, m_sx, Py(i));
          }
          SelectObject(hdc, hOldPen);
        }


        if(shouldDrawLabels) {
          if (lineType == LineType::Solid) {
            std::wstringstream ws{};
            ws << i;
            if (isVertical) {
              DrawCenteredText(
                  hdc,
                  ws.str().c_str(),
                  Px(i),
                  Py(-10. / m_zoom));
            } else {
              DrawCenteredRightText(
                  hdc,
                  ws.str().c_str(),
                  Px(-2. / m_zoom),
                  Py(i));
            }
          }
        }

      }
    };

    // Сетка
    drawGridLines(minValX, maxValX, true, true, false);
    drawGridLines(minValY, maxValY, false, true, false);

    // Лейблы
    drawGridLines(minValX, maxValX, true, false, true);
    drawGridLines(minValY, maxValY, false, false, true);
  }
};

LRESULT CALLBACK ChartWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

  static POINT beforeDragOffsetPoint{0, 0};
  static POINT startDragPoint{0, 0};
  static BOOL isDragging = FALSE;

  static ChartControl* pChartControl = nullptr;

  switch (msg) {
  case WM_CREATE: {
    pChartControl = new ChartControl;
    break;
  }
  case WM_LBUTTONDOWN: {
    SetCapture(hWnd);

    auto const xPos = LOWORD(lParam);
    auto const yPos = HIWORD(lParam);

    startDragPoint.x = xPos;
    startDragPoint.y = yPos;
    beforeDragOffsetPoint = pChartControl->m_offsetPoint;
    isDragging = TRUE;

    break;
  }
  case WM_LBUTTONUP: {
    ReleaseCapture();

    isDragging = FALSE;
    break;
  }
  case WM_MOUSEMOVE: {
    if (isDragging) {
      auto const xPos = LOWORD(lParam);
      auto const yPos = HIWORD(lParam);

      pChartControl->m_offsetPoint.x = beforeDragOffsetPoint.x - (startDragPoint.x - xPos);
      pChartControl->m_offsetPoint.y = beforeDragOffsetPoint.y - (startDragPoint.y - yPos);

      InvalidateRect(hWnd, NULL, TRUE);
    }
    break;
  }
  case WM_MOUSEWHEEL: {
    auto const delta = GET_WHEEL_DELTA_WPARAM(wParam);
    // Текущая позиция курсора в клиентских координатах
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(hWnd, &cursorPos);

    // Преобразуем координаты курсора в мировые координаты
    double worldX = pChartControl->ScreenXToWorldX(cursorPos.x);
    double worldY = pChartControl->ScreenYToWorldY(cursorPos.y);

    // Сохраняем текущее значение зума для корректировки offsetPoint
    auto const prevZoom = pChartControl->m_zoom;

    // Изменение масштаба
    if (delta > 0) {
      pChartControl->m_zoom *= 1.1;
    } else if (delta < 0) {
      pChartControl->m_zoom /= 1.1;
    }

    // Корректируем offsetPoint так, чтобы зумирование было относительно курсора
    pChartControl->m_offsetPoint.x += static_cast<int>((prevZoom - pChartControl->m_zoom) * worldX);
    pChartControl->m_offsetPoint.y -= static_cast<int>((prevZoom - pChartControl->m_zoom) * worldY);

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

    // -------------------------

    // Рисуем сетку
    pChartControl->DrawGrid(hdc);

    // Рисуем линии графика
    pChartControl->DrawChartLine(hdc, pChartControl->GetBluePen(), [](double x) { return sin(x); });
    pChartControl->DrawChartLine(hdc, pChartControl->GetRedPen(), [](double x) { return cos(x); });
    pChartControl->DrawChartLine(hdc, pChartControl->GetGreenPen(), [](double x) { return x + 2.; });
    pChartControl->DrawChartLine(hdc, pChartControl->GetGreenPen(), [](double x) { return 5. / x; });

    // -------------------------

    BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY); // NOLINT(readability-suspicious-call-argument)

    SelectObject(hdc, hOldBitmap);
    DeleteObject(hbm);
    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
    break;
  }

  case WM_SIZE: {
    pChartControl->m_sx = LOWORD(lParam);
    pChartControl->m_sy = HIWORD(lParam);

    break;
  }

  case WM_DESTROY: {
    delete pChartControl;
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
