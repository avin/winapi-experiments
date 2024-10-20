#include <Windows.h>
#include <gdiplus.h>

#include <sstream>

#pragma comment(lib, "gdiplus.lib")


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static Gdiplus::Image textureImage(L"texture.jpg");
    static Gdiplus::Image image(L"moon.png");
    static Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(L"moon.png");

    switch (uMsg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            Gdiplus::Graphics mainGraphics(hdc);

            // Получаем размеры клиентской области окна
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // Создаем буфер в памяти (bitmap)
            Gdiplus::Bitmap buffer(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, &mainGraphics);
            Gdiplus::Graphics graphics(&buffer);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

            graphics.Clear(Gdiplus::Color(255, 240, 245, 240));

            // Circle
            {
                Gdiplus::Pen borderPen(Gdiplus::Color(255, 0, 0, 255));
                borderPen.SetWidth(3);

                Gdiplus::SolidBrush fillBrush(Gdiplus::Color(100, 255, 100, 100));

                // сначала выполняется заливка
                graphics.FillEllipse(&fillBrush, 50, 50, 200, 100);

                // потом обводка
                graphics.DrawEllipse(&borderPen, 50, 50, 200, 100);
            }

            // Rect
            {
                Gdiplus::Pen penRect(Gdiplus::Color(255, 0, 150, 0));
                graphics.DrawRectangle(&penRect, 300, 50, 200, 100);
            }

            // Line
            {
                Gdiplus::Pen penLine(Gdiplus::Color(255, 255, 0, 0));
                graphics.DrawLine(&penLine, 100, 300, 400, 300);
            }

            // Filled Rect with transparency
            {
                Gdiplus::SolidBrush solidBrush(Gdiplus::Color(100, 255, 100, 100));
                graphics.FillRectangle(&solidBrush, 320, 100, 200, 100);
            }

            // Rect with gradient
            {
                Gdiplus::PointF p1{340, 190};
                Gdiplus::PointF p2{340 + 10, 270};
                Gdiplus::Color c1{255, 255, 0, 0};
                Gdiplus::Color c2{255, 0, 255, 0};
                Gdiplus::LinearGradientBrush gradientBrush(p1, p2, c1, c2);
                graphics.FillRectangle(&gradientBrush, 340, 180, 200, 100);
            }

            // Text
            {
                Gdiplus::Font font(L"Arial", 24);
                Gdiplus::SolidBrush solidBrush(Gdiplus::Color(255, 255, 0, 0));
                Gdiplus::PointF p(100, 100);
                graphics.DrawString(L"Hello world!", -1, &font, p, &solidBrush);
            }

            // Text with format (AlignmentCenter)
            {
                Gdiplus::Font font(L"Times New Roman", 24);
                Gdiplus::SolidBrush solidBrush(Gdiplus::Color(255, 0, 0, 255));
                Gdiplus::PointF p(100, 200);
                Gdiplus::StringFormat stringFormat;
                stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
                graphics.DrawString(L"Hello world!", -1, &font, p, &stringFormat, &solidBrush);
            }

            // Styled text
            {
                // Обычный стиль шрифта
                Gdiplus::Font fontRegular(L"Arial", 20, Gdiplus::FontStyleRegular);
                Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0)); // Черный цвет
                Gdiplus::PointF pointF(550.0f, 50.0f);
                graphics.DrawString(L"FontStyleRegular", -1, &fontRegular, pointF, &brush);

                // Жирный стиль
                Gdiplus::Font fontBold(L"Arial", 20, Gdiplus::FontStyleBold);
                pointF.Y += 50.0f; // Смещаем ниже по оси Y
                graphics.DrawString(L"FontStyleBold", -1, &fontBold, pointF, &brush);

                // Курсив
                Gdiplus::Font fontItalic(L"Arial", 20, Gdiplus::FontStyleItalic);
                pointF.Y += 50.0f;
                graphics.DrawString(L"FontStyleItalic", -1, &fontItalic, pointF, &brush);

                // Подчеркнутый текст
                Gdiplus::Font fontUnderline(L"Arial", 20, Gdiplus::FontStyleUnderline);
                pointF.Y += 50.0f;
                graphics.DrawString(L"FontStyleUnderline", -1, &fontUnderline, pointF, &brush);

                // Зачеркнутый текст
                Gdiplus::Font fontStrikeout(L"Arial", 20, Gdiplus::FontStyleStrikeout);
                pointF.Y += 50.0f;
                graphics.DrawString(L"FontStyleStrikeout", -1, &fontStrikeout, pointF, &brush);

                // Bold+Underline
                Gdiplus::Font fontBoldUnderline(L"Arial", 20, Gdiplus::FontStyleBold | Gdiplus::FontStyleUnderline);
                pointF.Y += 50.0f;
                graphics.DrawString(L"Bold+Underline", -1, &fontBoldUnderline, pointF, &brush);
            }

            // Image
            {
                // Загрузка изображения из файла
                // ... уже сделано в static переменной

                // Отображение изображения на экране
                graphics.DrawImage(&image, 50, 300);

                // Рисование по заданным пропорциям
                graphics.DrawImage(&image, 200, 300, 300, 100);

                UINT width = image.GetWidth();
                UINT height = image.GetHeight();

                // Вывод размеров
                Gdiplus::Font font(L"Arial", 14);
                Gdiplus::SolidBrush solidBrush(Gdiplus::Color(255, 255, 100, 0));
                Gdiplus::PointF p(50, 300);
                std::wstringstream wss;
                wss << L"width: " << width << L"; height: " << height;
                graphics.DrawString(wss.str().c_str(), -1, &font, p, &solidBrush);
            }

            // GraphicsPath
            {
                Gdiplus::GraphicsPath path;

                path.AddLine(50, 450, 200, 450);
                path.AddArc(200, 450, 100, 100, 0, 180); // Дуга на 180 градусов

                // path.CloseFigure();

                Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
                pen.SetWidth(10);
                pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
                graphics.DrawPath(&pen, &path);

                Gdiplus::SolidBrush brush(Gdiplus::Color(128, 0, 255, 0));
                graphics.FillPath(&brush, &path);
            }

            // Transform
            {
                // Translate
                for (float i = 0; i < 5; ++i) {
                    // Применение трансформации (перемещение)
                    Gdiplus::Matrix matrix;
                    matrix.Translate(i * 10, i * 10); // Смещаем фигуру на 100 единиц вправо и 50 вниз
                    graphics.SetTransform(&matrix);

                    // Рисуем прямоугольник
                    Gdiplus::Pen pen(Gdiplus::Color(255, 255, 0, 0));
                    graphics.DrawRectangle(&pen, 400, 400, 200, 100);

                    graphics.ResetTransform();
                }

                // Transform+Scale+Rotate
                for (float i = 1; i <= 3; ++i) {
                    // Применение трансформации (перемещение)
                    Gdiplus::Matrix matrix;
                    matrix.Translate(700, 500);
                    matrix.Scale(i, i);
                    matrix.Rotate(i * 10);
                    graphics.SetTransform(&matrix);

                    // Рисуем прямоугольник
                    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 155, 0));
                    graphics.DrawRectangle(&pen, -25, -25, 50, 50);

                    graphics.ResetTransform();
                }
            }

            // Texture
            {
                Gdiplus::TextureBrush textureBrush(&textureImage);
                graphics.FillRectangle(&textureBrush, 50, 600, 300, 250);

                Gdiplus::Matrix matrix;
                matrix.Scale(0.5f, 0.5f);
                matrix.Rotate(45.0f);
                textureBrush.SetTransform(&matrix);
                graphics.FillRectangle(&textureBrush, 400, 600, 300, 250);
            }

            // Region
            {
                // Создание области на основе прямоугольника и эллипса
                Gdiplus::Rect rect(750, 50, 150, 100);
                Gdiplus::Region region(rect);

                // Добавляем эллипс в область
                Gdiplus::GraphicsPath path;
                path.AddEllipse(800, 100, 150, 100);
                region.Exclude(&path); // Union, Intersect, Exclude

                // Заполняем область красным цветом
                Gdiplus::SolidBrush brush(Gdiplus::Color(128, 255, 0, 0)); // Полупрозрачный красный
                graphics.FillRegion(&brush, &region);
            }

            // Region Clipping
            {
                Gdiplus::Rect rect(50, 50, 200, 100);
                Gdiplus::Region region(rect);
                graphics.SetClip(&region, Gdiplus::CombineModeReplace); // Устанавливаем область вырезки

                // Любое рисование теперь будет ограничено этой областью
                Gdiplus::Pen pen(Gdiplus::Color(255, 255, 0, 0));
                graphics.DrawLine(&pen, 0, 0, 300, 300);

                graphics.ResetClip();
            }

            // Filters GrayScale
            {
                Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(L"moon.png");
                for (UINT y = 0; y < bitmap->GetHeight(); ++y) {
                    for (UINT x = 0; x < bitmap->GetWidth(); ++x) {
                        Gdiplus::Color color;
                        bitmap->GetPixel(x, y, &color);

                        // Преобразуем цвет в оттенок серого
                        BYTE gray = (BYTE)(0.3 * color.GetRed() + 1.59 * color.GetGreen() + 0.11 * color.GetBlue());
                        Gdiplus::Color grayColor(color.GetA(), gray, gray, gray);

                        bitmap->SetPixel(x, y, grayColor);
                    }
                }

                graphics.DrawImage(bitmap, 700, 700);
            }

            // PathGradientBrush Circle
            {
                // Создаем путь на основе эллипса
                Gdiplus::GraphicsPath path;
                path.AddEllipse(750, 200, 200, 200); // Эллипс

                // Создаем PathGradientBrush на основе этого пути
                Gdiplus::PathGradientBrush pathBrush(&path);

                // Задаем цвет в центре градиента
                Gdiplus::Color centerColor(255, 255, 255, 255); // Белый
                pathBrush.SetCenterColor(centerColor);

                // Задаем цвета по краям фигуры
                Gdiplus::Color surroundColors[] = {Gdiplus::Color(255, 255, 0, 0)}; // Красный
                int count = 1;
                pathBrush.SetSurroundColors(surroundColors, &count);

                // Заливаем эллипс градиентом
                graphics.FillPath(&pathBrush, &path);
            }

            // PathGradientBrush Rect (like 3D button)
            {
                // Создаем путь на основе прямоугольника
                Gdiplus::GraphicsPath path;
                Gdiplus::Rect rect(750, 400, 200, 100); // Прямоугольник
                path.AddRectangle(rect);

                // Создаем PathGradientBrush на основе этого пути
                Gdiplus::PathGradientBrush pathBrush(&path);

                // Задаем цвет в центре градиента (светлый цвет для эффекта выпуклости)
                Gdiplus::Color centerColor(255, 255, 255, 255); // Белый
                pathBrush.SetCenterColor(centerColor);

                // Задаем цвет на краях градиента (темный для эффекта глубины)
                Gdiplus::Color surroundColors[] = {Gdiplus::Color(255, 100, 100, 100)}; // Темно-серый
                int count = 1;
                pathBrush.SetSurroundColors(surroundColors, &count);

                // Настройка фокуса для усиления эффекта выпуклости
                pathBrush.SetFocusScales(0.7f, 0.7f); // Центр градиента ближе к центру кнопки

                // Заливаем прямоугольник градиентом
                graphics.FillPath(&pathBrush, &path);

                // Рисуем обводку прямоугольника (темнее для границ)
                Gdiplus::Pen borderPen(Gdiplus::Color(255, 50, 50, 50), 1); // Темный серый
                graphics.DrawRectangle(&borderPen, rect);
            }

            // Rounded Rect with gradien (Windows XP like button)
            {
                static auto AddRoundedRectToPath = [](Gdiplus::GraphicsPath* path, Gdiplus::Rect rect, int cornerRadius) {
                    int diameter = cornerRadius * 2;

                    // Верхняя левая дуга
                    path->AddArc(rect.X, rect.Y, diameter, diameter, 180, 90);

                    // Верхняя прямая
                    path->AddLine(rect.X + cornerRadius, rect.Y, rect.X + rect.Width - cornerRadius, rect.Y);

                    // Верхняя правая дуга
                    path->AddArc(rect.X + rect.Width - diameter, rect.Y, diameter, diameter, 270, 90);

                    // Правая прямая
                    path->AddLine(rect.X + rect.Width, rect.Y + cornerRadius, rect.X + rect.Width, rect.Y + rect.Height - cornerRadius);

                    // Нижняя правая дуга
                    path->AddArc(rect.X + rect.Width - diameter, rect.Y + rect.Height - diameter, diameter, diameter, 0, 90);

                    // Нижняя прямая
                    path->AddLine(rect.X + rect.Width - cornerRadius, rect.Y + rect.Height, rect.X + cornerRadius, rect.Y + rect.Height);

                    // Нижняя левая дуга
                    path->AddArc(rect.X, rect.Y + rect.Height - diameter, diameter, diameter, 90, 90);

                    // Левая прямая
                    path->AddLine(rect.X, rect.Y + rect.Height - cornerRadius, rect.X, rect.Y + cornerRadius);
                };

                // Создаем путь для прямоугольника со скругленными углами
                Gdiplus::GraphicsPath path;
                Gdiplus::Rect rect(750, 600, 100, 22);
                int cornerRadius = 3;
                AddRoundedRectToPath(&path, rect, cornerRadius);

                // Заливаем прямоугольник
                Gdiplus::Color c1(255, 255, 255, 255);
                Gdiplus::Color c2(214, 208, 197);

                Gdiplus::LinearGradientBrush gradientBrush(
                    rect, c1, c2, Gdiplus::LinearGradientModeVertical);
                graphics.FillPath(&gradientBrush, &path);

                // Обводка прямоугольника
                Gdiplus::Pen borderPen(Gdiplus::Color(255, 0, 60, 116), 1);
                graphics.DrawPath(&borderPen, &path);
            }

            // Soft shadow emultation
            {

                // Рисуем несколько слоев для имитации мягкой тени
                for (int i = 0; i < 10; ++i) {
                    int alpha = 25 - i * 2; // Прозрачность уменьшается к краям
                    Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(alpha, 0, 0, 0)); // Полупрозрачный черный
                    graphics.FillEllipse(&shadowBrush, 55 - i, 155 - i, 100 + i * 2, 50 + i * 2); // Увеличение тени
                }

                // Рисуем основной объект поверх тени
                Gdiplus::SolidBrush objectBrush(Gdiplus::Color(255, 255, 0, 0)); // Красный цвет
                graphics.FillEllipse(&objectBrush, 50, 150, 100, 50);

            }

            // Копируем буфер на экран
            mainGraphics.DrawImage(&buffer, 0, 0);

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEX wcex{sizeof(WNDCLASSEX)};
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"GDIPlusExample";
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassEx(&wcex);

    // Init GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HWND hwnd = CreateWindowEx(
        0,
        wcex.lpszClassName,
        L"GDI+ Example",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 1000,
        NULL, NULL, hInstance, NULL
        );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Destroy GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return 0;
}
