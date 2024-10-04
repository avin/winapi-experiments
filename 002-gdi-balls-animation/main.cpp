#include <chrono>
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <random>

constexpr double M_PI = 3.14159265358979323846;

class Ball {

public:
  double Radius{0};
  double X{0};
  double Y{0};
  double Angle{0};
  double FutureAngle{0};
  double Speed{100.};

  Ball(const double x, const double y)
    : X{x},
      Y{y} {

    static std::mt19937 gen(std::random_device{}());

    std::uniform_real_distribution<double> distAngle(0.0, M_PI * 2.);
    std::uniform_real_distribution<double> distRadius(20., 50.);

    Angle = distAngle(gen);
    FutureAngle = Angle;
    Radius = distRadius(gen);
  }

  void Draw(const HDC hdc) const {
    Ellipse(
        hdc,
        static_cast<int>(X - Radius),
        static_cast<int>(Y - Radius),
        static_cast<int>(X + Radius),
        static_cast<int>(Y + Radius));
  }

  void BounceOff(const Ball& other) {
    double dx = X - other.X;
    double dy = Y - other.Y;
    FutureAngle = atan2(dy, dx); // Изменяем будущее направление
  }

  // Function for changing direction when colliding with walls
  void BounceOffWalls(int arenaWidth, int arenaHeight) {
    if (X - Radius < 0) {
      FutureAngle = M_PI - Angle;
      X = Radius; // Adjusting position
    } else if (X + Radius > arenaWidth) {
      FutureAngle = M_PI - Angle;
      X = arenaWidth - Radius; // Adjusting position
    }

    if (Y - Radius < 0) {
      FutureAngle = -Angle;
      Y = Radius; // Adjusting position
    } else if (Y + Radius > arenaHeight) {
      FutureAngle = -Angle;
      Y = arenaHeight - Radius; // Adjusting position
    }
  }

  // Apply the future direction (after all calculations)
  void ApplyNewDirection() {
    Angle = FutureAngle;
  }

  // Checking collision with another ball
  bool IsCollidingWith(const Ball& other) const {
    double dx = X - other.X;
    double dy = Y - other.Y;
    double distance = sqrt(dx * dx + dy * dy);
    return distance <= (Radius + other.Radius);
  }
};

class BallsCollection {
  std::vector<Ball> Items{};

public:
  void Update(const double deltaTime, int arenaWidth, int arenaHeight) {
    for (auto& ball : Items) {
      ball.X += deltaTime * ball.Speed * cos(ball.Angle);
      ball.Y += deltaTime * ball.Speed * sin(ball.Angle);

      ball.BounceOffWalls(arenaWidth, arenaHeight);
    }

    // Checking collisions of balls with each other
    for (size_t i = 0; i < Items.size(); ++i) {
      for (size_t j = i + 1; j < Items.size(); ++j) {
        if (Items[i].IsCollidingWith(Items[j])) {
          // Change the direction of movement for both balls
          Items[i].BounceOff(Items[j]);
          Items[j].BounceOff(Items[i]);
        }
      }
    }

    // Apply all angle changes only after all calculations
    for (auto& ball : Items) {
      ball.ApplyNewDirection();
    }
  }

  void Draw(HDC hdc) const {
    for (const auto& ball : Items) {
      ball.Draw(hdc);
    }
  }

  void Add(int x, int y) {
    Items.emplace_back(Ball{static_cast<double>(x), static_cast<double>(y)});
  }
};


LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
  static int arenaWidth, arenaHeight;

  static auto ballsCollection = BallsCollection{};

  using clock = std::chrono::high_resolution_clock;
  static clock::time_point lastTime;

  // static LARGE_INTEGER frequency;
  // static LARGE_INTEGER lastTime;
  // static LARGE_INTEGER currentTime;
  static HBRUSH hBrushBackground = CreateSolidBrush(RGB(245, 248, 250));

  switch (message) {
  case WM_CREATE: {
    lastTime = clock::now();

    HDC hdc = GetDC(NULL); // NULL means that we are requesting information for the entire screen		
    const auto refreshRate = GetDeviceCaps(hdc, VREFRESH);
    ReleaseDC(NULL, hdc);
    const auto elapse = 1000 / refreshRate;

    SetTimer(hWnd, 1, elapse, NULL);

    break;
  }

  case WM_ERASEBKGND:
    return 1;

  case WM_LBUTTONDOWN: {
    auto x = LOWORD(lParam);
    auto y = HIWORD(lParam);
    ballsCollection.Add(x, y);
    InvalidateRect(hWnd, NULL, TRUE);
    break;
  }

  case WM_SIZE: {
    arenaWidth = LOWORD(lParam);
    arenaHeight = HIWORD(lParam);
    break;
  }

  case WM_TIMER: {
    auto currentTime = clock::now();
    std::chrono::duration<double> deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    ballsCollection.Update(deltaTime.count(), arenaWidth, arenaHeight);

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

    ballsCollection.Draw(hdc);

    BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY);  // NOLINT(readability-suspicious-call-argument)

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
      L"Balls (click to add)",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      0,
      600,
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
