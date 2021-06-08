#pragma once
#include "dllincludes.h"
#include "event.h"
#include "io.h"
#include "proxy_info.h"

// DRM handling utilities.
namespace DRM
{
#ifdef LEBINKPROXY_USE_NEWDRMWAIT
    static Sync::Event* DrmEvent = nullptr;

    bool GameWindowCreated = false;
    typedef HWND(WINAPI* CREATEWINDOWEXW)(
        DWORD dwExStyle,
        LPCWSTR lpClassName,
        LPCWSTR lpWindowName,
        DWORD dwStyle,
        int X,
        int Y,
        int nWidth,
        int nHeight,
        HWND hWndParent,
        HMENU hMenu,
        HINSTANCE hInstance,
        LPVOID lpParam);
    CREATEWINDOWEXW CreateWindowExW_orig = nullptr;
    HWND WINAPI CreateWindowExW_hooked(
        DWORD dwExStyle,
        LPCWSTR lpClassName,
        LPCWSTR lpWindowName,
        DWORD dwStyle,
        int X,
        int Y,
        int nWidth,
        int nHeight,
        HWND hWndParent,
        HMENU hMenu,
        HINSTANCE hInstance,
        LPVOID lpParam)
    {
        GLogger.writeFormatLine(L"CreateWindowExW: lpWindowName = %s", lpWindowName);
        if (nullptr != lpWindowName && 0 == wcscmp(lpWindowName, GAppProxyInfo.WinTitle))
        {
            GLogger.writeFormatLine(L"CreateWindowExW: matched a title, signaling the event [%p]", DrmEvent);
            if (DrmEvent && !DrmEvent->Set())
            {
                GLogger.writeFormatLine(L"CreateWindowExW: event was not null but Set failed (%d)", GetLastError());
            }
        }
        return CreateWindowExW_orig(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    void WaitForDRMv2()
    {
        GLogger.writeFormatLine(L"WaitForDRMv2: waiting for DRM...");

        DrmEvent = new Sync::Event(L"drm_wait");
        if (!DrmEvent->InError())
        {
            auto rc = DrmEvent->WaitForIt(30 * 1000);  // 30 seconds timeout should be more than enough
            switch (rc)
            {
            case Sync::EventWaitValue::Signaled:
                GLogger.writeFormatLine(L"WaitForDRMv2: event signaled!");
                break;
            default:
                GLogger.writeFormatLine(L"WaitForDRMv2: event wait failed (EventWaitValue = %d)", (int)rc);
            }
        }
        delete DrmEvent;
    }
#else
    int GoodTitleHit = 0;
    const int RequiredHits = 1;

    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam)
    {
        int length = GetWindowTextLength(hWnd);
        wchar_t buffer[512];
        GetWindowText(hWnd, buffer, length + 1);

        if (GoodTitleHit == RequiredHits)
        {
            GLogger.writeFormatLine(L"WaitForDRM: hit enough times!");
            return FALSE;
        }

        if (IsWindowVisible(hWnd) && length != 0 && wcsstr(buffer, L"Mass Effect") != NULL)
        {
            //wprintf_s(L"  Window: %s\n", buffer);
            if (0 == wcscmp(GAppProxyInfo.WinTitle, buffer))
            {
                ++GoodTitleHit;
            }
        }

        return TRUE;
    }

    void WaitForDRM()
    {
        GLogger.writeFormatLine(L"WaitForDRM: waiting for DRM...");
        do
        {
            EnumWindows(enumWindowCallback, NULL);
        } while (GoodTitleHit != RequiredHits);
        GLogger.writeFormatLine(L"WaitForDRM: finished waiting for DRM!");
    }
#endif
}
