#include <windows.h>
#include <tchar.h>
#include <vector>

class Ball {

public:
  double Radius{0};
  double X{0};
  double Y{0};

  Ball(const double radius, const double x, const double y)
    : Radius{radius},
      X{x},
      Y{y} {
  }

  void Draw(const HDC hdc) const {
    Ellipse(
        hdc,
        static_cast<int>(X - Radius),
        static_cast<int>(Y - Radius),
        static_cast<int>(X + Radius),
        static_cast<int>(Y + Radius));
  }
};

class BallsCollection {
  std::vector<Ball> Items{};

public:
  void update(const double deltaTime, int arenaWidth, int arenaHeight) {
    for (auto& ball : Items) {
      ball.X += deltaTime * 10;
    }
  }

  void draw(HDC hdc) const {
    for (auto& ball : Items) {
      ball.Draw(hdc);
    }
  }

  void add(int x, int y) {
    Items.push_back(Ball{50.0, static_cast<double>(x), static_cast<double>(y)});
  }
};


LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
  static int arenaWidth, arenaHeight;

  static auto ballsCollection = BallsCollection{};

  static LARGE_INTEGER frequency;
  static LARGE_INTEGER lastTime;
  static LARGE_INTEGER currentTime;
  static HBRUSH hBrushBackground = CreateSolidBrush(RGB(245, 248, 250));

  switch (message) {
  case WM_CREATE: {
    HDC hdc = GetDC(NULL); // NULL means that we are requesting information for the entire screen		
    const auto refreshRate = GetDeviceCaps(hdc, VREFRESH);
    ReleaseDC(NULL, hdc);
    const auto elapse = 1000 / refreshRate;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);

    SetTimer(hWnd, 1, elapse, NULL);
    break;
  }
  case WM_ERASEBKGND:
    return 1;
  case WM_LBUTTONDOWN: {
    auto x = LOWORD(lParam);
    auto y = HIWORD(lParam);
    ballsCollection.add(x, y);
    InvalidateRect(hWnd, NULL, TRUE);
    break;
  }

  case WM_SIZE: {
    arenaWidth = LOWORD(lParam);
    arenaHeight = HIWORD(lParam);
    break;
  }

  case WM_TIMER: {

    QueryPerformanceCounter(&currentTime);

    double deltaTime = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;

    ballsCollection.update(deltaTime, arenaWidth, arenaHeight);

    lastTime = currentTime;
    InvalidateRect(hWnd, NULL, TRUE);
    break;
  }

  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdcOrig = BeginPaint(hWnd, &ps);

    auto rect = RECT{};
    GetClientRect(hWnd, &rect);

    HDC hdc = CreateCompatibleDC(hdcOrig);
    HBITMAP hbm = CreateCompatibleBitmap(hdcOrig, rect.right, rect.bottom);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hbm);

    FillRect(hdc, &rect, hBrushBackground);

    ballsCollection.draw(hdc);

    BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY);

    SelectObject(hdc, hOldBitmap);
    DeleteObject(hbm);
    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
    break;
  }
  case WM_DESTROY: {
    DeleteObject(hBrushBackground);
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

  const HWND hWnd = CreateWindowW(
      className,
      L"Balls",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      0,
      1000,
      1000,
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