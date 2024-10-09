#include <windows.h>
#include <commdlg.h>
#include <tchar.h>

TCHAR* fileContent = nullptr; // Буфер для содержимого файла
int lineHeight = 0; // Высота строки для вывода
int yOffset = 0; // Смещение по вертикали

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void OpenAndDisplayFile(HWND);
void LoadFileContent(const TCHAR* fileName);
void DrawTextWithWrap(HDC hdc, RECT rect);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("FileViewerClass");
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hWnd = CreateWindow(wc.lpszClassName, _T("File Viewer"), WS_OVERLAPPEDWINDOW | WS_VSCROLL,
                             CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete[] fileContent; // Освобождаем буфер перед выходом
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            CreateWindow(_T("BUTTON"), _T("Open File"), WS_VISIBLE | WS_CHILD,
                         10, 10, 100, 30, hWnd, (HMENU)1, GetModuleHandle(NULL), NULL);
            break;
        }
        case WM_SIZE:
            InvalidateRect(hWnd, NULL, TRUE); // Перерисовываем окно при изменении размера
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                OpenAndDisplayFile(hWnd);
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rect;
            GetClientRect(hWnd, &rect);
            rect.top += 50; // Оставляем место для кнопки

            FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));

            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SelectObject(hdc, hFont);

            if (fileContent) {
                DrawTextWithWrap(hdc, rect);
            }

            EndPaint(hWnd, &ps);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void OpenAndDisplayFile(HWND hWnd) {
    OPENFILENAME ofn;
    TCHAR szFile[260] = {0};

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = _T("Text Files\0*.TXT\0All Files\0*.*\0");
    ofn.lpstrTitle = _T("Open a Text File");
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        LoadFileContent(szFile);
        InvalidateRect(hWnd, NULL, TRUE); // Перерисовываем окно
    }
}

void LoadFileContent(const TCHAR* fileName) {
    HANDLE hFile = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, _T("Could not open file"), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    char* ansiBuffer = new char[fileSize + 1]; // Буфер для ANSI текста
    DWORD bytesRead;

    if (!ReadFile(hFile, ansiBuffer, fileSize, &bytesRead, NULL)) {
        MessageBox(NULL, _T("Could not read file"), _T("Error"), MB_OK | MB_ICONERROR);
        CloseHandle(hFile);
        delete[] ansiBuffer;
        return;
    }
    ansiBuffer[bytesRead] = '\0'; // Завершение строки

    // Преобразуем ANSI в Unicode
    int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, ansiBuffer, -1, NULL, 0);
    delete[] fileContent;
    fileContent = new TCHAR[sizeNeeded];
    MultiByteToWideChar(CP_ACP, 0, ansiBuffer, -1, fileContent, sizeNeeded);

    delete[] ansiBuffer; // Освобождаем ANSI буфер
    CloseHandle(hFile);
}

// Функция для вывода текста с переносами строк
void DrawTextWithWrap(HDC hdc, RECT rect) {
    int width = rect.right - rect.left;
    int y = rect.top;

    // Настройка шрифта и получение высоты строки
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    lineHeight = tm.tmHeight + tm.tmExternalLeading;

    TCHAR lineBuffer[1024];
    const TCHAR* currentPos = fileContent;
    SIZE textSize;
    int len;

    while (*currentPos) {
        len = 0;

        // Определяем, сколько символов умещается в текущую ширину
        while (*currentPos && *currentPos != '\n') {
            lineBuffer[len] = *currentPos;
            lineBuffer[len + 1] = '\0';

            GetTextExtentPoint32(hdc, lineBuffer, len + 1, &textSize);
            if (textSize.cx > width) break;

            currentPos++;
            len++;
        }

        // Если конец строки - добавляем перенос строки
        if (*currentPos == '\n') currentPos++;

        // Печатаем строку и обновляем позицию по вертикали
        TextOut(hdc, rect.left, y, lineBuffer, len);
        y += lineHeight;

        // Если текст выходит за пределы окна, останавливаем
        if (y > rect.bottom) break;
    }
}
