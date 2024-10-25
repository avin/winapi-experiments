#include <windows.h>
#include <magnification.h>

#pragma comment(lib, "Magnification.lib")

const wchar_t szWindowClass[] = L"MyWindowClass";
const wchar_t szTitle[] = L"My Window";

HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL bDragging = FALSE;
POINT ptLast;

HWND hMagWnd;

// Функция обратного вызова для обработки изображения
BOOL CALLBACK MyMagImageScalingCallback(HWND hwnd, void* srcdata, MAGIMAGEHEADER srcheader,
                                        void* destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty) {
    // Проверяем, что размеры исходного и конечного изображения совпадают
    if (srcheader.width != destheader.width || srcheader.height != destheader.height) {
        return FALSE;
    }

    // Получаем указатели на данные пикселей
    BYTE* srcPixels = (BYTE*)srcdata;
    BYTE* destPixels = (BYTE*)destdata;

    // Проходим по каждому пикселю и преобразуем его в оттенок серого
    for (int y = 0; y < (int)srcheader.height; y++) {
        for (int x = 0; x < (int)srcheader.width; x++) {
            int index = (y * srcheader.stride) + (x * 4);

            // Получаем компоненты текущего пикселя
            BYTE blue = srcPixels[index];
            BYTE green = srcPixels[index + 1];
            BYTE red = srcPixels[index + 2];

            bool hasDarkNeighbor = false;

            // Проверяем соседние пиксели (если они в пределах изображения)
            int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto offset : offsets) {
                int nx = x + offset[0];
                int ny = y + offset[1];
                if (nx >= 0 && nx < (int)srcheader.width && ny >= 0 && ny < (int)srcheader.height) {
                    int neighborIndex = (ny * srcheader.stride) + (nx * 4);
                    BYTE nBlue = srcPixels[neighborIndex];
                    BYTE nGreen = srcPixels[neighborIndex + 1];
                    BYTE nRed = srcPixels[neighborIndex + 2];

                    // Рассчитываем яркость соседнего пикселя
                    BYTE neighborBrightness = (BYTE)(0.299 * nRed + 0.587 * nGreen + 0.114 * nBlue);

                    // Проверяем, темнее ли среднее значение
                    if (neighborBrightness < 128) {
                        hasDarkNeighbor = true;
                        break;
                    }
                }
            }

            // Если есть темный соседний пиксель, делаем текущий пиксель черным
            if (hasDarkNeighbor) {
                destPixels[index] = 0;       // Blue
                destPixels[index + 1] = 0;   // Green
                destPixels[index + 2] = 0;   // Red
            } else {
                // Иначе, сохраняем оригинальный оттенок серого
                BYTE gray = (BYTE)(0.299 * red + 0.587 * green + 0.114 * blue);
                destPixels[index] = gray;
                destPixels[index + 1] = gray;
                destPixels[index + 2] = gray;
            }
            destPixels[index + 3] = srcPixels[index + 3]; // Alpha
        }
    }


    return TRUE;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine, int nCmdShow) {
    // Инициализируем библиотеку COM
    CoInitialize(NULL);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szWindowClass;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; // Не используем фон

    RegisterClass(&wc);

    // Создаем основное окно
    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST ,
        szWindowClass,
        szTitle,
        WS_POPUP, // WS_OVERLAPPEDWINDOW, // WS_POPUP, // Без заголовка и границ
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
        );

    if (!hWnd) {
        CoUninitialize();
        return FALSE;
    }

    hInst = hInstance;

    // Инициализируем Magnification API
    if (!MagInitialize()) {
        MessageBox(NULL, L"Не удалось инициализировать Magnification API", L"Ошибка", MB_ICONERROR);
        CoUninitialize();
        return FALSE;
    }

    // Создаем дочернее окно увеличения
    hMagWnd = CreateWindow(WC_MAGNIFIER, L"MagnifierWindow",
                           WS_CHILD | MS_SHOWMAGNIFIEDCURSOR,
                           2, 2, 800-4, 600-4, hWnd, NULL, hInstance, NULL);

    if (!hMagWnd) {
        MessageBox(NULL, L"Не удалось создать окно увеличения", L"Ошибка", MB_ICONERROR);
        MagUninitialize();
        CoUninitialize();
        return FALSE;
    }

    // Устанавливаем коэффициент увеличения 1.0 (без увеличения)
    MAGTRANSFORM magTransform = {0};
    magTransform.v[0][0] = 1.0f;
    magTransform.v[1][1] = 1.0f;
    magTransform.v[2][2] = 1.0f;
    MagSetWindowTransform(hMagWnd, &magTransform);

    // Устанавливаем фильтр для исключения нашего окна из захвата
    MagSetWindowFilterList(hMagWnd, MW_FILTERMODE_EXCLUDE, 1, &hWnd);

    // Устанавливаем функцию обратного вызова для обработки изображения
    MagSetImageScalingCallback(hMagWnd, MyMagImageScalingCallback);

    // Показываем окна
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    ShowWindow(hMagWnd, SW_SHOW);
    UpdateWindow(hMagWnd);

    // Устанавливаем таймер для обновления области источника
    SetTimer(hWnd, 1, 30, NULL); // Обновление ~XX FPS

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Освобождаем ресурсы
    MagUninitialize();
    CoUninitialize();

    return (int)msg.wParam;
}

void UpdateMagWindow(HWND hWnd) {
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    // Получаем размер окна
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    // Получаем положение окна на экране
    POINT pt;
    pt.x = rcClient.left;
    pt.y = rcClient.top;
    ClientToScreen(hWnd, &pt);

    // Определяем область источника для захвата
    RECT sourceRect;
    sourceRect.left = pt.x;
    sourceRect.top = pt.y;
    sourceRect.right = pt.x + width;
    sourceRect.bottom = pt.y + height;

    // Устанавливаем область источника для окна увеличения
    MagSetWindowSource(hMagWnd, sourceRect);

    // Обновляем окно увеличения
    InvalidateRect(hMagWnd, NULL, TRUE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HMENU hMenu;

    switch (message) {
        case WM_CREATE: {
            // Создаем контекстное меню
            hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1, L"Exit");
        }
        break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // Устанавливаем черный цвет для рамки
            HPEN hPen = CreatePen(PS_SOLID, 5, RGB(0, 0, 0)); // Толщина рамки 5 пикселей
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            // Получаем размер клиентской области
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);

            // Рисуем прямоугольную рамку вокруг окна
            Rectangle(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

            // Восстанавливаем старую кисть и удаляем созданные ресурсы
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);

            EndPaint(hWnd, &ps);
        }
        break;

        case WM_TIMER: {
            // Обновляем окно увеличения
            UpdateMagWindow(hWnd);
        }
        break;
        case WM_SIZE: {
            // Изменяем размер окна увеличения при изменении размера основного окна
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            MoveWindow(hMagWnd, 2, 2, rcClient.right - rcClient.left-4, rcClient.bottom - rcClient.top-4, TRUE);
        }
        break;
        case WM_LBUTTONDOWN: {
            bDragging = TRUE;
            SetCapture(hWnd);
            ptLast.x = LOWORD(lParam);
            ptLast.y = HIWORD(lParam);
        }
        break;
        case WM_MOUSEMOVE: {
            if (bDragging) {
                POINT pt;
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);

                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);

                int dx = pt.x - ptLast.x;
                int dy = pt.y - ptLast.y;

                MoveWindow(hWnd, rcWindow.left + dx, rcWindow.top + dy,
                           rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, TRUE);
            }
        }
        break;
        case WM_LBUTTONUP: {
            bDragging = FALSE;
            ReleaseCapture();
        }
        break;
        case WM_RBUTTONUP: {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ClientToScreen(hWnd, &pt);

            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        break;
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) // "Выйти"
            {
                PostQuitMessage(0);
            }
        }
        break;
        case WM_DESTROY: {
            KillTimer(hWnd, 1);
            DestroyMenu(hMenu);
            PostQuitMessage(0);
        }
        break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
