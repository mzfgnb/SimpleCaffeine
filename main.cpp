#include <windows.h>
#include <shellapi.h>
#include "resource.h"

// Уникальные идентификаторы для нашего меню и трей-иконки
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_TOGGLE 1002 // Идентификатор для кнопки переключения

// Глобальные переменные
NOTIFYICONDATAW nid = {};
HWND hHiddenWnd = NULL;
HMENU hMenu = NULL;
bool isCaffeineActive = true; // Переменная состояния

// Прототип функции, чтобы компилятор знал о ней заранее
void SetCaffeineState(bool enable, HINSTANCE hInstance);

// Функция для обновления состояния и смены иконки из ресурсов
void SetCaffeineState(bool enable, HINSTANCE hInstance) {
    isCaffeineActive = enable;

    if (enable) {
        // Включаем защиту от сна
        DWORD state = ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED;
        SetThreadExecutionState(state);
    }
    else {
        // Возвращаем обычный режим
        SetThreadExecutionState(ES_CONTINUOUS);
    }

    // Загружаем иконку из ресурсов (убедись, что твои иконки в resource.h называются IDI_ICON1 и IDI_ICON2)
    WORD iconId = enable ? IDI_ICON1 : IDI_ICON2;
    HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(iconId));

    // Если вдруг иконка не нашлась, берем дефолтную системную
    if (!hIcon) {
        hIcon = LoadIconW(NULL, IDI_APPLICATION);
    }

    // Обновляем структуру трея
    nid.hIcon = hIcon;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

    // Меняем подсказку при наведении
    if (enable) {
        wcscpy_s(nid.szTip, L"Кофеин: Активен");
    }
    else {
        wcscpy_s(nid.szTip, L"Кофеин: Отключен");
    }

    // Отправляем в систему команду обновить иконку в трее
    Shell_NotifyIconW(NIM_MODIFY, &nid);

    // Меняем текст пункта меню
    if (hMenu) {
        ModifyMenuW(hMenu, ID_TRAY_TOGGLE, MF_BYCOMMAND | MF_STRING, ID_TRAY_TOGGLE,
            enable ? L"Выключить кофеин" : L"Включать кофеин");
    }
}

// Оконная процедура для обработки системных сообщений и кликов по иконке
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Получаем hInstance текущего приложения через окно
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);

    switch (uMsg) {
    case WM_TRAYICON:
        // Обработка событий мыши по иконке в трее
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
            POINT curPoint;
            GetCursorPos(&curPoint);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_RIGHTALIGN, curPoint.x, curPoint.y, 0, hwnd, NULL);
        }
        break;

    case WM_COMMAND:
        // Обработка нажатий на пункты меню
        if (LOWORD(wParam) == ID_TRAY_EXIT) {
            SetCaffeineState(false, hInstance);
            Shell_NotifyIconW(NIM_DELETE, &nid);
            PostQuitMessage(0);
        }
        else if (LOWORD(wParam) == ID_TRAY_TOGGLE) {
            // Переключаем состояние при клике на пункт меню
            SetCaffeineState(!isCaffeineActive, hInstance);
        }
        break;

    case WM_DESTROY:
        SetCaffeineState(false, hInstance);
        Shell_NotifyIconW(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Главная точка входа для Windows-приложений (без консоли)
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"CaffeineTrayWindow";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassExW(&wc);

    // Создаем скрытое окно
    hHiddenWnd = CreateWindowExW(
        0, CLASS_NAME, L"Caffeine Background",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );

    if (!hHiddenWnd) return 0;

    // Заполняем структуру для иконки в трее
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hHiddenWnd;
    nid.uID = 1;
    nid.uCallbackMessage = WM_TRAYICON;

    // Создаем контекстное меню
    hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, L"Выключить кофеин");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Выход");

    // Включаем кофеин и ставим правильную стартовую иконку
    SetCaffeineState(false, hInstance);

    // Добавляем иконку в системный трей
    Shell_NotifyIconW(NIM_ADD, &nid);

    // Стандартный цикл обработки сообщений Windows
    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hMenu) DestroyMenu(hMenu);

    return (int)msg.wParam;
}