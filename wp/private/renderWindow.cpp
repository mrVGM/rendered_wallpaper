#include "renderWindow.h"
#include "tasks.h"
#include "dim.h"

#include "tasks.h"

#include <chrono>
#include <thread>
#include <windef.h>

namespace
{
    tasks::Channel<HWND>* channel = nullptr;
    LRESULT StaticWndProc(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
    {
        if (channel && uMsg == WM_CREATE)
        {
            channel->push(hWnd);
        }

        return static_cast<LRESULT>(DefWindowProc(hWnd, uMsg, wParam, lParam));
    }


    HWND getWorkerWindow()
    {
        HWND shellWindow = GetShellWindow();
        HWND def = FindWindowEx(
            shellWindow,
            nullptr,
            "SHELLDLL_DefView",
            nullptr);

        HWND worker = FindWindowEx(
            shellWindow,
            def,
            "WorkerW",
            nullptr);

        return worker;
    }

    tasks::ThreadPool windowLoop(1);

    HWND creteRenderWindowInternal()
    {
        HWND shellWindow = GetShellWindow();
        HWND def = FindWindowEx(
                shellWindow,
                nullptr,
                "SHELLDLL_DefView",
                nullptr);

        SendMessage(
                shellWindow,
                0x052C,
                0,
                0
                );

        HWND worker = FindWindowEx(
                shellWindow,
                def,
                "WorkerW",
                nullptr);

        const char* windowClassName = "WP Window";
        WNDCLASSEX wcex;

        ZeroMemory(&wcex, sizeof(wcex));
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_OWNDC;
        wcex.lpfnWndProc = &StaticWndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = GetModuleHandle(NULL);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = windowClassName;

        RegisterClassEx(&wcex);

        DWORD dwStyle = WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
        DWORD dxExStyle = 0;

        RECT windowRect;
        windowRect.left = 0;
        windowRect.top = 0;
        windowRect.right = wp::WIDTH;
        windowRect.bottom = wp::HEIGHT;

        AdjustWindowRect(&windowRect, dwStyle, FALSE);

        tasks::Channel<HWND> handleChannel;
        channel = &handleChannel;

        CreateWindow(
                windowClassName,
                "Wallpaper Window",
                dwStyle,
                windowRect.left, windowRect.top,
                windowRect.right - windowRect.left,
                windowRect.bottom - windowRect.top,
                NULL, NULL, GetModuleHandle(NULL), nullptr);

        HWND window = channel->pop();
        channel = nullptr;

        SetParent(window, worker);

        return window;
    }

}

HWND wp::creteRenderWindow()
{
    tasks::Channel<HWND> windowHandle;
    windowLoop.run([&](){
        HWND handle = creteRenderWindowInternal();
        windowHandle.push(handle);

        while (true)
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
                if (!GetMessage(&msg, NULL, 0, 0)) {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    HWND h = windowHandle.pop();
    return h;
}
