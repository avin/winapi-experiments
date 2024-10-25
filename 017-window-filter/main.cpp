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
        for (int x = 0; x < (int)srcheader.width - 1; x++) {
            int index = (y * srcheader.stride) + (x * 4);
            int shiftedIndex = index + 4; // Пиксель справа

            // Получаем компоненты текущего и смещённого пикселей
            BYTE blue1 = srcPixels[index];
            BYTE green1 = srcPixels[index + 1];
            BYTE red1 = srcPixels[index + 2];

            BYTE blue2 = srcPixels[shiftedIndex];
            BYTE green2 = srcPixels[shiftedIndex + 1];
            BYTE red2 = srcPixels[shiftedIndex + 2];

            // Рассчитываем яркость обоих пикселей
            BYTE brightness1 = (BYTE)(0.299 * red1 + 0.587 * green1 + 0.114 * blue1);
            BYTE brightness2 = (BYTE)(0.299 * red2 + 0.587 * green2 + 0.114 * blue2);

            // Сохраняем более тёмный пиксель в итоговый кадр
            if (brightness1 < brightness2) {
                destPixels[index] = blue1;
                destPixels[index + 1] = green1;
                destPixels[index + 2] = red1;
                destPixels[index + 3] = srcPixels[index + 3]; // Alpha
            } else {
                destPixels[index] = blue2;
                destPixels[index + 1] = green2;
                destPixels[index + 2] = red2;
                destPixels[index + 3] = srcPixels[shiftedIndex + 3]; // Alpha
            }
        }

        // Обрабатываем крайний правый пиксель, который не имеет смещённого соседа справа
        int lastIndex = (y * srcheader.stride) + ((int)srcheader.width - 1) * 4;
        destPixels[lastIndex] = srcPixels[lastIndex];
        destPixels[lastIndex + 1] = srcPixels[lastIndex + 1];
        destPixels[lastIndex + 2] = srcPixels[lastIndex + 2];
        destPixels[lastIndex + 3] = srcPixels[lastIndex + 3]; // Alpha
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
        WS_EX_TOPMOST,
        szWindowClass,
        szTitle,
        WS_POPUP | WS_THICKFRAME, // WS_OVERLAPPEDWINDOW, // WS_POPUP, // Без заголовка и границ
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
                           WS_CHILD, // | MS_SHOWMAGNIFIEDCURSOR
                           0, 0, 800, 600, hWnd, NULL, hInstance, NULL);

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

            //

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
            MoveWindow(hMagWnd, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, TRUE);
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
