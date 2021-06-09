#pragma once

#include "gamever.h"
#include "utils/io.h"
#include "utils/event.h"
#include "dllstruct.h"


// DRM handling utilities.
namespace DRM
{
    static Sync::Event* DrmEvent = nullptr;
    bool GameWindowCreated = false;

    typedef HWND(WINAPI* CREATEWINDOWEXW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
    CREATEWINDOWEXW CreateWindowExW_orig = nullptr;
    HWND WINAPI CreateWindowExW_hooked(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        GLogger.writeFormatLine(L"CreateWindowExW: lpWindowName = %s", lpWindowName);
        if (nullptr != lpWindowName && 0 == wcscmp(lpWindowName, GLEBinkProxy.WinTitle))
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
}
