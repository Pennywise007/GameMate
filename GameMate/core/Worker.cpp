#include "pch.h"
#include "psapi.h"
#include "resource.h"

#include "Worker.h"

namespace {

HWINEVENTHOOK g_hook = nullptr;
HHOOK g_mouseHook = nullptr;
HHOOK g_keyboardHook = nullptr;

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        // Получение информации о текущем активном окне
        /*HWND activeWindow = GetForegroundWindow();

        DWORD pid;
        GetWindowThreadProcessId(activeWindow, &pid);

        // Получаем дескриптор процесса по его PID
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProcess != nullptr) {
            TCHAR processName[MAX_PATH];
            GetModuleFileNameEx(hProcess, nullptr, processName, MAX_PATH);

            // Отображаем информацию о процессе и окне в консоль
            std::wcout << L"Process Name: " << processName << std::endl;

            CloseHandle(hProcess);
        }*/
    }

    // Продолжаем цепочку обработки сообщений
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        // Получение информации о текущем активном окне
       /* HWND activeWindow = GetForegroundWindow();

        DWORD pid;
        GetWindowThreadProcessId(activeWindow, &pid);

        // Получаем дескриптор процесса по его PID
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProcess != nullptr) {
            TCHAR processName[MAX_PATH];
            GetModuleFileNameEx(hProcess, nullptr, processName, MAX_PATH);

            // Отображаем информацию о процессе и окне в консоль
            std::wcout << L"Process Name: " << processName << std::endl;

            CloseHandle(hProcess);
        }*/
    }

    // Продолжаем цепочку обработки сообщений
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void CALLBACK windowFocusChanged(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    int ads = 0;
    ++ads;
    // retreive and use the title of hwnd as needed...
}

} // namespace

class CMyWnd : public CWnd
{
public:
    CBitmap m_bitmap;

    CMyWnd()
    {
        // Загрузите ваше изображение в m_bitmap
        m_bitmap.LoadBitmapW(IDB_PNG_CROSSHAIR_0_16);

        HINSTANCE instance = AfxGetInstanceHandle();
        const CString editLabelClassName(typeid(*this).name());

        // регистрируем наш клас
        WNDCLASSEX wndClass;
        if (!::GetClassInfoEx(instance, editLabelClassName, &wndClass))
        {
            // Регистрация класса окна которое используется для редактирования ячеек
            memset(&wndClass, 0, sizeof(WNDCLASSEX));
            wndClass.cbSize = sizeof(WNDCLASSEX);
            wndClass.style = CS_DBLCLKS;
            wndClass.lpfnWndProc = ::DefMDIChildProc;
            wndClass.hInstance = instance;
            wndClass.lpszClassName = editLabelClassName;

            if (!RegisterClassEx(&wndClass))
                ::MessageBox(NULL, L"Can`t register class", L"Error", MB_OK);
        }

        // Установите стиль WS_POPUP чтобы окно не имело рамок и заголовка
        CreateEx(WS_EX_TOPMOST, CString(typeid(*this).name()), NULL, WS_POPUP | WS_VISIBLE, CRect(0, 0, 1000, 1000), NULL, NULL);

        // Установите прозрачность окна
       // SetWindowLongPtr(GetSafeHwnd(), GWL_EXSTYLE, GetWindowLongPtr(GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_LAYERED);

        // Установите прозрачность окна
      //  SetLayeredWindowAttributes(RGB(255, 0, 255), 0, LWA_COLORKEY);
    }

    afx_msg void OnPaint()
    {
        CPaintDC dc(this); // контекст устройства для рисования

        // Создайте мемориальный контекст устройства для рисования на маске битового блока
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);

        // Выберите ваш битмап в мемориальный контекст устройства
        CBitmap* pOldBitmap = memDC.SelectObject(&m_bitmap);

        // Нарисуйте битмап в окне
        dc.BitBlt(0, 0, 100, 100, &memDC, 0, 0, SRCCOPY);

        // Освободите объекты, чтобы избежать утечек памяти
        memDC.SelectObject(pOldBitmap);
    }

    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CMyWnd, CWnd)
    ON_WM_PAINT()
END_MESSAGE_MAP()

std::shared_ptr<CMyWnd> wnd;


Worker::Worker()
{
    /*g_hook = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, &windowFocusChanged, 0, 0, WINEVENT_OUTOFCONTEXT);
    if (g_hook == nullptr) {
        std::cerr << "Failed to set active window hook" << std::endl;
    }

    g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    if (g_mouseHook == nullptr) {
        std::cerr << "Failed to set mouse hook" << std::endl;
    }

    // Установка хука для отслеживания событий клавиатуры
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (g_keyboardHook == nullptr) {
        std::cerr << "Failed to set keyboard hook" << std::endl;
    }*/
    wnd = std::make_shared<CMyWnd>();


    // Установите окно наверх всех других окон
  /*  wnd.SetWindowPos(&wnd, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
  */
    OnSettingsChangedByUser();
}

Worker::~Worker()
{
    /*UnhookWinEvent(g_hook);
    UnhookWindowsHookEx(g_mouseHook);
    UnhookWindowsHookEx(g_keyboardHook);*/
}

void Worker::OnSettingsChangedByUser()
{
    if (m_workingThread.joinable())
        m_workingThread.interrupt_and_join();

    std::list<TabConfiguration> activeTabSettings;
    for (const auto& tabSettings : ext::get_service<Settings>().tabs)
    {
        if (!tabSettings->enabled)
            continue;
        activeTabSettings.emplace_back(*tabSettings);
    }

    if (!activeTabSettings.empty())
        m_workingThread.run(&Worker::WorkingThread, this, std::move(activeTabSettings));
}

void Worker::WorkingThread(std::list<TabConfiguration> activeTabSettings)
{
    
}
