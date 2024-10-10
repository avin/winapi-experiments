#include "window1.h"
#include "resource.h"
#include <gdiplus.h>
#include <atlbase.h>
#include <atlcom.h>
#pragma comment(lib, "gdiplus.lib")



// Функция для загрузки PNG из ресурсов
Gdiplus::Image* LoadPNGFromResource(HINSTANCE hInstance, int resourceID) {
    // Ищем ресурс PNG в исполняемом файле по его идентификатору (resourceID) и типу "PNG"
    HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(resourceID), L"PNG");
    if (!hResource) { // Проверяем, найден ли ресурс
        return nullptr; // Если ресурс не найден, возвращаем nullptr
    }

    // Получаем размер ресурса, чтобы выделить под него память
    DWORD imageSize = SizeofResource(hInstance, hResource);

    // Загружаем ресурс в память
    HGLOBAL hGlobal = LoadResource(hInstance, hResource);
    if (!hGlobal) { // Проверяем, загружен ли ресурс
        return nullptr; // Если загрузка не удалась, возвращаем nullptr
    }

    // Получаем указатель на данные ресурса
    LPVOID pResourceData = LockResource(hGlobal);
    if (!pResourceData) { // Проверяем, удалось ли заблокировать данные ресурса
        return nullptr; // Если не удалось, возвращаем nullptr
    }

    // Выделяем глобальную память для хранения данных изображения
    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (hBuffer) { // Проверяем, удалось ли выделить память
        void* pBuffer = GlobalLock(hBuffer); // Блокируем память для записи
        if (pBuffer) {
            // Копируем данные ресурса в выделенную память
            memcpy(pBuffer, pResourceData, imageSize);
            GlobalUnlock(hBuffer); // Освобождаем блокировку памяти после копирования

            // Создаем поток IStream поверх глобальной памяти
            CComPtr<IStream> pStream;
            if (SUCCEEDED(CreateStreamOnHGlobal(hBuffer, TRUE, &pStream))) {
                // Создаем объект Image, используя созданный поток
                auto image = new Gdiplus::Image(pStream);
                return image; // Возвращаем указатель на объект Image
            }
        }
        GlobalFree(hBuffer); // Освобождаем глобальную память, если произошла ошибка
    }
    return nullptr; // Возвращаем nullptr, если на каком-то этапе произошла ошибка
}

// Функция для отображения изображения на окне с использованием UpdateLayeredWindow
void DisplayPNG(HWND hWnd, Gdiplus::Image* image) {
    // Получаем контекст устройства для экрана
    HDC hdcScreen = GetDC(NULL);

    // Создаем совместимый контекст устройства в памяти
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    HBITMAP hBitmap = NULL;

    // Получаем ширину и высоту изображения
    int width = image->GetWidth();
    int height = image->GetHeight();

    // Создаем временный GDI+ Bitmap для преобразования Image в HBITMAP
    Gdiplus::Bitmap bmp(width, height, PixelFormat32bppARGB);

    // Создаем GDI+ Graphics объект, связанный с Bitmap
    Gdiplus::Graphics graphics(&bmp);

    // Рисуем изображение на графическом объекте
    graphics.DrawImage(image, 0, 0, width, height);

    // Получаем HBITMAP из Bitmap для использования в GDI
    // Здесь передаем цвет фона (0, 0, 0, 0), чтобы установить прозрачность
    bmp.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);

    // Выбираем HBITMAP в контекст памяти
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // Определяем позицию окна, размеры и координаты источника
    POINT ptPos = {0, 0}; // Позиция верхнего левого угла окна
    SIZE sizeWnd = {width, height}; // Размеры изображения
    POINT ptSrc = {0, 0}; // Координаты начала изображения

    // Настройка параметров смешивания для прозрачности
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    // AC_SRC_OVER указывает, что результат будет поверх имеющегося фона
    // 255 – это уровень прозрачности (максимальная непрозрачность)
    // AC_SRC_ALPHA означает, что альфа-канал изображения будет использоваться

    // Обновляем окно с прозрачным слоем
    UpdateLayeredWindow(hWnd, hdcScreen, &ptPos, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
    // Используем ULW_ALPHA для поддержки альфа-канала и корректного смешивания слоев

    // Освобождаем старый HBITMAP из контекста памяти
    SelectObject(hdcMem, hOldBitmap);

    // Удаляем временный HBITMAP, так как он больше не нужен
    DeleteObject(hBitmap);

    // Удаляем созданный контекст памяти
    DeleteDC(hdcMem);

    // Освобождаем контекст экрана
    ReleaseDC(NULL, hdcScreen);
}



LRESULT CALLBACK Wnd1Proc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    static Gdiplus::Image* image = nullptr;

    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
            image = LoadPNGFromResource(hInst, IDB_PNG1);

            if (image) {
                DisplayPNG(hWnd, image);
            }

            break;
        }

        case WM_PAINT: {
            if (image) {
                DisplayPNG(hWnd, image);
            }
            break;
        }

        case WM_DESTROY: {
            delete image;
            image = nullptr;
            PostQuitMessage(0);
            break;
        }

        // case WM_KILLFOCUS:
        //     SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        // break;

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                //
            }
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


ATOM RegisterWindow1Class(HINSTANCE hInst) {
    auto const className = L"Window1Class";

    WNDCLASSEX wcgraph = {0};
    wcgraph.cbSize = sizeof(WNDCLASSEX);
    wcgraph.style = CS_HREDRAW | CS_VREDRAW;
    wcgraph.lpfnWndProc = Wnd1Proc;
    wcgraph.hInstance = hInst;
    wcgraph.hCursor = LoadCursor(NULL, IDC_CROSS);
    wcgraph.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcgraph.lpszClassName = className;
    wcgraph.hIconSm = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
    return RegisterClassEx(&wcgraph);
}
