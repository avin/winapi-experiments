#include "window1.h"
#include "resource.h"
#include <gdiplus.h>
#include <atlbase.h>
#include <atlcom.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Функция для загрузки PNG из ресурсов
Image* LoadPNGFromResource(HINSTANCE hInstance, int resourceID) {
    HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(resourceID), L"PNG");
    if (!hResource) return nullptr;

    DWORD imageSize = SizeofResource(hInstance, hResource);
    HGLOBAL hGlobal = LoadResource(hInstance, hResource);
    if (!hGlobal) return nullptr;

    LPVOID pResourceData = LockResource(hGlobal);
    if (!pResourceData) return nullptr;

    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (hBuffer) {
        void* pBuffer = GlobalLock(hBuffer);
        if (pBuffer) {
            memcpy(pBuffer, pResourceData, imageSize);
            GlobalUnlock(hBuffer);

            CComPtr<IStream> pStream;
            if (SUCCEEDED(CreateStreamOnHGlobal(hBuffer, TRUE, &pStream))) {
                Image* image = new Image(pStream);
                return image;
            }
        }
        GlobalFree(hBuffer);
    }
    return nullptr;
}

// Функция для отображения изображения на окне с использованием UpdateLayeredWindow
void DisplayPNG(HWND hWnd, Image* image) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = NULL;

    int width = image->GetWidth();
    int height = image->GetHeight();

    // Создаем временный HBITMAP из GDI+
    Bitmap bmp(width, height, PixelFormat32bppARGB);
    Graphics graphics(&bmp);
    graphics.DrawImage(image, 0, 0, width, height);
    bmp.GetHBITMAP(Color(0, 0, 0, 0), &hBitmap);

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // Настройка прозрачности
    POINT ptPos = {0, 0};
    SIZE sizeWnd = {width, height};
    POINT ptSrc = {0, 0};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    // Обновляем окно с прозрачным слоем
    UpdateLayeredWindow(hWnd, hdcScreen, &ptPos, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    // Очистка
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}


LRESULT CALLBACK Wnd1Proc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    static Image* image = nullptr;

    switch (message) {
        case WM_CREATE: {
            CreateWindow(L"static", L"Hello from window1", WS_VISIBLE | WS_CHILD, 10, 20, 150, 30, hWnd, 0, NULL, NULL);
            HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
            image = LoadPNGFromResource(hInst, IDB_PNG1); // Замените IDB_PNG1 на ID ресурса PNG

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
