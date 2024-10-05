#include <windows.h>
#include <tchar.h>
#include <cstdarg>
#include <cmath>
#include <string>

HHOOK hHook;
HWND hWndParent;

void DebugOutput(const wchar_t* format, ...) {
  wchar_t msg[255] = {0};

  va_list args;
  va_start(args, format);
  vswprintf(msg, sizeof(msg) / sizeof(wchar_t), format, args);
  va_end(args);

  OutputDebugStringW(msg);
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

struct FieldParams {
  int fieldSize;
  int squareSize;
  int minPadding;
  int paddingLeft;
  int paddingTop;
};

// Получение основных параметров поля
void GetFieldParams(HWND hWnd, FieldParams* fp) {
  RECT clientRect;
  GetClientRect(hWnd, &clientRect);

  auto clientRectHeight = clientRect.bottom - clientRect.top;
  auto clientRectWidth = clientRect.right - clientRect.left;

  auto minSide = min(clientRectHeight, clientRectWidth);

  auto minPadding = minSide / 10;
  auto fieldSize = (minSide - minPadding * 2) / 8 * 8;

  auto paddingTop = (clientRectHeight / 2 - fieldSize / 2);
  auto paddingLeft = (clientRectWidth / 2 - fieldSize / 2);

  auto squareSize = fieldSize / 8;

  fp->fieldSize = fieldSize;
  fp->squareSize = squareSize;
  fp->minPadding = minPadding;
  fp->paddingTop = paddingTop;
  fp->paddingLeft = paddingLeft;
}

// Хук для перехвата создания MessageBox
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HCBT_ACTIVATE) {
    HWND hWndMsgBox = (HWND)wParam;

    RECT rcMsgBox;
    GetWindowRect(hWndMsgBox, &rcMsgBox);

    RECT rcParent;
    GetWindowRect(hWndParent, &rcParent);

    int msgBoxWidth = rcMsgBox.right - rcMsgBox.left;
    int msgBoxHeight = rcMsgBox.bottom - rcMsgBox.top;

    int newPosX = rcParent.left + ((rcParent.right - rcParent.left) - msgBoxWidth) / 2;
    int newPosY = rcParent.top + ((rcParent.bottom - rcParent.top) - msgBoxHeight) / 2;

    // Перемещаем MessageBox в центр окна
    SetWindowPos(hWndMsgBox, HWND_TOP, newPosX, newPosY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Снимаем хук после перемещения окна
    UnhookWindowsHookEx(hHook);
  }
  return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// Функция для показа MessageBox по центру окна
int MessageBoxCentered(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
  hWndParent = hWnd;

  // Устанавливаем хук на текущий поток для перехвата создания MessageBox
  hHook = SetWindowsHookEx(WH_CBT, CBTProc, NULL, GetCurrentThreadId());

  // Показываем MessageBox
  return MessageBoxW(hWnd, lpText, lpCaption, uType);
}

LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {

  static HPEN hFramePen = CreatePen(PS_SOLID, 2, GetSysColor(COLOR_BTNFACE));
  static FieldParams fp{};
  static int activeCell = -1;

  switch (message) {
  case WM_CREATE: {
    // GetFieldParams(hWnd, &fp);
    break;
  }

  case WM_MOUSEMOVE: {
    int mouseX = LOWORD(lParam);
    int mouseY = HIWORD(lParam);

    int cellX = std::floor(static_cast<double>(mouseX - fp.paddingLeft) / fp.squareSize);
    int cellY = std::floor(static_cast<double>(mouseY - fp.paddingTop) / fp.squareSize);

    auto prevActiveCell = activeCell;
    if (cellX >= 0 && cellX < 8 && cellY >= 0 && cellY < 8) {
      activeCell = cellY * 8 + cellX;
    } else {
      activeCell = -1;
    }
    if (activeCell != prevActiveCell) {
      InvalidateRect(hWnd, NULL, TRUE);
    }

    DebugOutput(L"x: %d; y: %d\n", activeCell, activeCell);

    break;
  }

  case WM_LBUTTONUP: {
    if (activeCell > -1) {
      int y = activeCell / 8;
      int x = activeCell % 8;

      std::wstring cellName = {static_cast<wchar_t>(L'A' + x), static_cast<wchar_t>(L'0' + 8 - y)};

      MessageBoxCentered(hWnd, cellName.c_str(), L"Clicked", MB_OK);

    }
    break;
  }

  case WM_SIZE: {
    GetFieldParams(hWnd, &fp);
    break;
  }

  case WM_ERASEBKGND:
    return 1;

  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdcOrig = BeginPaint(hWnd, &ps);

    auto rect = RECT{};
    GetClientRect(hWnd, &rect);

    HDC hdc = CreateCompatibleDC(hdcOrig);
    HBITMAP hbm = CreateCompatibleBitmap(hdcOrig, rect.right, rect.bottom);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hbm);

    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

    // Draw field frame rectangle

    auto hPrevPenObject = SelectObject(hdc, hFramePen);
    Rectangle(
        hdc,
        fp.paddingLeft - 1,
        fp.paddingTop - 1,
        fp.paddingLeft + fp.fieldSize + 1,
        fp.paddingTop + fp.fieldSize + 1);
    SelectObject(hdc, hPrevPenObject);

    for (int i = 0; i < 8; ++i) {
      for (int j = 0; j < 8; ++j) {
        auto fill = false;
        if (i % 2) {
          fill = j % 2;
        } else {
          fill = !(j % 2);
        }

        if (fill) {
          auto squareRect = RECT{};
          squareRect.top = i * fp.squareSize + fp.paddingTop;
          squareRect.bottom = (i + 1) * fp.squareSize + fp.paddingTop;

          squareRect.left = j * fp.squareSize + fp.paddingLeft;
          squareRect.right = (j + 1) * fp.squareSize + fp.paddingLeft;

          FillRect(hdc, &squareRect, (HBRUSH)(COLOR_BTNFACE + 1));
        }

      }
    }

    int fontSize = fp.squareSize / 2.5;

    HFONT hFieldFont = CreateFont(
        -fontSize,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Arial");
    auto hPrevFont = SelectObject(hdc, hFieldFont);

    for (int j = 0; j < 2; ++j) {
      for (int i = 0; i < 8; ++i) {
        std::wstring letter = {static_cast<wchar_t>(L'A' + i)};

        DrawCenteredText(
            hdc,
            letter.c_str(),
            i * fp.squareSize + fp.squareSize / 2 + fp.paddingLeft,
            fp.paddingTop - fp.minPadding / 2 + (fp.fieldSize + fp.minPadding) * j);
      }
    }

    for (int j = 0; j < 2; ++j) {
      for (int i = 0; i < 8; ++i) {
        std::wstring letter = {static_cast<wchar_t>(L'8' - i)};

        DrawCenteredText(
            hdc,
            letter.c_str(),
            fp.paddingLeft - fp.minPadding / 2 + (fp.fieldSize + fp.minPadding) * j,
            i * fp.squareSize + fp.squareSize / 2 + fp.paddingTop
            );
      }
    }

    SelectObject(hdc, hPrevFont);
    DeleteObject(hFieldFont);

    if (activeCell > -1) {
      HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
      int y = activeCell / 8;
      int x = activeCell % 8;

      Rectangle(
          hdc,
          fp.paddingLeft + x * fp.squareSize,
          fp.paddingTop + y * fp.squareSize,
          fp.paddingLeft + (x + 1) * fp.squareSize,
          fp.paddingTop + (y + 1) * fp.squareSize
          );
      SelectObject(hdc, hOldBrush);
    }

    BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY); // NOLINT(readability-suspicious-call-argument)

    SelectObject(hdc, hOldBitmap);
    DeleteObject(hbm);
    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
    break;
  }
  case WM_DESTROY: {
    DeleteObject(hFramePen);

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

  int captionHeight = GetSystemMetrics(SM_CYCAPTION);

  const HWND hWnd = CreateWindowW(
      className,
      L"ChessBoard",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      0,
      800,
      800 + captionHeight,
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
