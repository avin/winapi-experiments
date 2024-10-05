#include <deque>
#include <random>
#include <windows.h>
#include <tchar.h>

class RectangleShape {
  int X, Y, Width, Height;
  HBRUSH HBrush;

public:
  RectangleShape(
      const int arenaWidth,
      const int arenaHeight,
      std::mt19937& randomGenerator) {
    std::uniform_int_distribution<int> distWidth(arenaWidth / 6, arenaWidth / 4);
    std::uniform_int_distribution<int> distHeight(arenaHeight / 6, arenaHeight / 4);

    Width = distWidth(randomGenerator);
    Height = distHeight(randomGenerator);

    std::uniform_int_distribution<int> distX(0, arenaWidth - Width);
    std::uniform_int_distribution<int> distY(0, arenaHeight - Height);

    X = distX(randomGenerator);
    Y = distY(randomGenerator);

    std::uniform_int_distribution<int> distR(0, 255);
    std::uniform_int_distribution<int> distG(0, 255);
    std::uniform_int_distribution<int> distB(0, 255);

    HBrush = CreateSolidBrush(
        RGB(distR(randomGenerator), distG(randomGenerator), distB(randomGenerator)));
  };

  ~RectangleShape() {
    DeleteObject(HBrush);
  };

  void Draw(HDC hdc) const {
    auto hOldBrush = SelectObject(hdc, HBrush);
    Rectangle(hdc, X, Y, X + Width, Y + Height);
    SelectObject(hdc, hOldBrush);
  }
};

class RectanglesCollection {
  std::deque<RectangleShape> Items;
  size_t MaxSize{};
  std::mt19937 RandomGen;

public:
  explicit RectanglesCollection(const size_t maxSize = 10)
    : MaxSize(maxSize),
      RandomGen(std::random_device{}()) {
  };

  void AddNew(int arenaWidth, int arenaHeight) {
    if (Items.size() >= MaxSize) {
      Items.pop_front();
    }
    Items.emplace_back(arenaWidth, arenaHeight, RandomGen); // Создаём объект прямо в коллекции
  }

  void Draw(HDC hdc) {
    for (auto& rect : Items) {
      rect.Draw(hdc);
    }
  }
};

LRESULT CALLBACK WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
  static int arenaWidth, arenaHeight;
  static auto rectCollection = RectanglesCollection{10};

  switch (message) {
  case WM_CREATE: {
    SetTimer(hWnd, 1, 50, NULL);
    break;
  }

  case WM_TIMER: {
    rectCollection.AddNew(arenaWidth, arenaHeight);
    InvalidateRect(hWnd, NULL, FALSE);
    break;
  }

  case WM_SIZE: {
    arenaWidth = LOWORD(lParam);
    arenaHeight = HIWORD(lParam);
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

    rectCollection.Draw(hdc);

    BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY); // NOLINT(readability-suspicious-call-argument)

    SelectObject(hdc, hOldBitmap);
    DeleteObject(hbm);
    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
    break;
  }
  case WM_DESTROY: {
    KillTimer(hWnd, 1);
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
      L"Rectangle Rush",
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
