// 007-file-view.cpp : Defines the entry point for the application.
//


#include <string>
#include <fstream>
#include "007-file-view.h"

#include "Debug.h"
#include "framework.h"

#include <vector>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst; // current instance
WCHAR szTitle[MAX_LOADSTRING]; // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY007FILEVIEW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(
        hInstance,MAKEINTRESOURCE(IDC_MY007FILEVIEW));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY007FILEVIEW));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY007FILEVIEW);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
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

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT message,
                         WPARAM wParam,
                         LPARAM lParam) {
    static TCHAR fileName[256] = _T(""); // Массив для хранения имени файла
    static OPENFILENAME file; // Структура для открытия/сохранения файла
    static std::vector<std::string> linesStorage; // Вектор строк для хранения содержимого файла
    static int maxStrLength, sx, sy, vScrollPos, hScrollPos, maxScrollVertRange, maxScrollHorzRange;
    static SIZE fontSize = {8, 16}; // Размер символа: ширина и высота

    switch (message) {
        case WM_CREATE: {
            // Инициализация структуры OPENFILENAME для открытия и сохранения файла
            file.lStructSize = sizeof(OPENFILENAME);
            file.hInstance = hInst;
            file.lpstrFilter = _T("Text\0*.txt");
            file.lpstrFile = fileName;
            file.nMaxFile = 256;
            file.lpstrInitialDir = _T(".\\");
            file.lpstrDefExt = _T("txt"); // Установка расширения по умолчанию на ".txt"
            break;
        }

        case WM_SIZE: {
            // Изменение размеров окна и обновление полос прокрутки
            sx = LOWORD(lParam); // Ширина клиентской области
            sy = HIWORD(lParam); // Высота клиентской области

            // Вертикальная прокрутка: расчет и установка диапазона
            auto k = linesStorage.size() - sy / fontSize.cy;
            if (k > 0) {
                maxScrollVertRange = k;
            } else {
                maxScrollVertRange = vScrollPos = 0;
            }
            SetScrollRange(hWnd, SB_VERT, 0, maxScrollVertRange, TRUE);
            SetScrollPos(hWnd, SB_VERT, vScrollPos, TRUE);

            // Горизонтальная прокрутка: расчет и установка диапазона
            k = maxStrLength - sx / fontSize.cx;
            if (k > 0) {
                maxScrollHorzRange = k;
            } else {
                maxScrollHorzRange = hScrollPos = 0;
            }
            SetScrollRange(hWnd, SB_HORZ, 0, maxScrollHorzRange, TRUE);
            SetScrollPos(hWnd, SB_HORZ, hScrollPos, TRUE);
            break;
        }

        case WM_VSCROLL: {
            // Обработка вертикальной прокрутки
            switch (LOWORD(wParam)) {
                case SB_LINEUP: {
                    vScrollPos--; // Прокрутка на одну строку вверх
                    break;
                }
                case SB_LINEDOWN: {
                    vScrollPos++; // Прокрутка на одну строку вниз
                    break;
                }
                case SB_PAGEUP: {
                    vScrollPos -= sy / fontSize.cy; // Прокрутка на одну страницу вверх
                    break;
                }
                case SB_PAGEDOWN: {
                    vScrollPos += sy / fontSize.cy; // Прокрутка на одну страницу вниз
                    break;
                }
                case SB_THUMBTRACK: {
                    vScrollPos = HIWORD(wParam); // Перемещение ползунка
                    break;
                }
            }
            // Ограничение позиции прокрутки
            vScrollPos = max(0, min(vScrollPos, maxScrollVertRange));
            if (vScrollPos != GetScrollPos(hWnd, SB_VERT)) {
                SetScrollPos(hWnd, SB_VERT, vScrollPos, TRUE);
                InvalidateRect(hWnd, NULL, TRUE); // Перерисовка окна
            }
            break;
        }

        case WM_HSCROLL: {
            // Обработка горизонтальной прокрутки
            switch (LOWORD(wParam)) {
                case SB_LINEUP: {
                    hScrollPos--; // Прокрутка на один символ влево
                    break;
                }
                case SB_LINEDOWN: {
                    hScrollPos++; // Прокрутка на один символ вправо
                    break;
                }
                case SB_PAGEUP: {
                    hScrollPos -= 8; // Прокрутка на восемь символов влево
                    break;
                }
                case SB_PAGEDOWN: {
                    hScrollPos += 8; // Прокрутка на восемь символов вправо
                    break;
                }
                case SB_THUMBTRACK: {
                    hScrollPos = HIWORD(wParam); // Перемещение ползунка
                    break;
                }
            }
            // Ограничение позиции прокрутки
            hScrollPos = max(0, min(hScrollPos, maxScrollHorzRange));
            if (hScrollPos != GetScrollPos(hWnd, SB_HORZ)) {
                SetScrollPos(hWnd, SB_HORZ, hScrollPos, TRUE);
                InvalidateRect(hWnd, NULL, TRUE); // Перерисовка окна
            }
            break;
        }

        case WM_COMMAND: {
            // Обработка команд меню
            switch (LOWORD(wParam)) {
                case ID_FILE_NEW: {
                    // Новый файл - очистка содержимого
                    if (!linesStorage.empty()) {
                        std::vector<std::string>().swap(linesStorage); // Очищаем вектор
                    }
                    SendMessage(hWnd, WM_SIZE, 0, sy << 16 | sx); // Обновление размера
                    InvalidateRect(hWnd, NULL, TRUE); // Перерисовка окна
                    break;
                }

                case ID_FILE_OPEN: {
                    // Открытие файла для чтения
                    std::ifstream in;
                    file.lpstrTitle = _T("Открыть файл для чтения");
                    file.Flags = OFN_HIDEREADONLY;
                    if (!GetOpenFileName(&file)) {
                        return 1; // Если файл не выбран
                    }
                    in.open(fileName); // Открываем файл для чтения
                    std::string str;
                    while (getline(in, str)) {
                        // Находим максимальную длину строки
                        if (maxStrLength < str.length()) {
                            maxStrLength = str.length();
                        }
                        linesStorage.push_back(str); // Добавляем строку в вектор
                    }
                    in.close();
                    SendMessage(hWnd, WM_SIZE, 0, sy << 16 | sx); // Обновление размера
                    InvalidateRect(hWnd, NULL, TRUE); // Перерисовка окна
                    break;
                }
                case ID_FILE_SAVE: {
                    // Сохранение файла
                    std::ofstream out;
                    file.lpstrTitle = _T("Открыть файл для записи");
                    file.Flags = OFN_NOTESTFILECREATE;
                    if (!GetSaveFileName(&file)) {
                        return 1; // Если файл не выбран
                    }
                    out.open(fileName); // Открываем файл для записи
                    for (auto it = linesStorage.begin(); it != linesStorage.end(); ++it) {
                        out << *it << '\n'; // Записываем каждую строку
                    }
                    out.close();
                    break;
                }
                case IDM_EXIT: {
                    DestroyWindow(hWnd); // Закрытие окна
                    break;
                }
                default: {
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
            break;
        }

        case WM_ERASEBKGND: {
            return 1; // Убираем мерцание при перерисовке, возвращая 1
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcOrig = BeginPaint(hWnd, &ps); // Начало рисования

            RECT rect;
            GetClientRect(hWnd, &rect); // Получение размеров клиентской области

            HDC hdc = CreateCompatibleDC(hdcOrig); // Создаем совместимый контекст устройства
            HBITMAP hbm = CreateCompatibleBitmap(hdcOrig, rect.right, rect.bottom); // Создаем битмап
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hbm); // Устанавливаем битмап

            // --------------------

            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1)); // Заполнение фона

            auto y = 0;
            for (auto it = linesStorage.begin() + vScrollPos;
                 it != linesStorage.end() && y < sy;
                 ++it, y += fontSize.cy) {
                if (hScrollPos < it->length()) {
                    // Выводим текст построчно с учётом горизонтального смещения
                    TabbedTextOutA(
                        hdc,
                        0,
                        y,
                        it->data() + hScrollPos,
                        it->length() - hScrollPos,
                        0,
                        NULL,
                        0);
                }
            }

            // --------------------

            // Копируем битмап на экран
            BitBlt(hdcOrig, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY); // NOLINT(readability-suspicious-call-argument)

            SelectObject(hdc, hOldBitmap); // Возвращаем оригинальный битмап
            DeleteObject(hbm); // Удаляем битмап
            DeleteDC(hdc); // Удаляем контекст устройства

            EndPaint(hWnd, &ps); // Завершаем рисование
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0); // Выход из программы
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam); // Обработка остальных сообщений
    }
    return 0;
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);

    switch (message) {
        case WM_INITDIALOG: {
            return (INT_PTR)TRUE;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
        }
    }
    return (INT_PTR)FALSE;
}
