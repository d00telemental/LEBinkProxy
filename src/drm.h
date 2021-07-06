#pragma once

#include "gamever.h"
#include "utils/io.h"
#include "utils/event.h"
#include "dllstruct.h"


namespace DRM
{
    static Utils::Event* DrmEvent = nullptr;
    bool GameWindowCreated = false;

    typedef HWND(WINAPI* CREATEWINDOWEXW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
    CREATEWINDOWEXW CreateWindowExW_orig = nullptr;
    HWND WINAPI CreateWindowExW_hooked(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        //GLogger.writeln(L"CreateWindowExW: lpWindowName = %s", lpWindowName);
        if (nullptr != lpWindowName
            && (0 == wcscmp(lpWindowName, GLEBinkProxy.WinTitle) || 0 == wcscmp(lpWindowName, L"SplashScreen")))
        {
            GLogger.writeln(L"CreateWindowExW: matched a title, signaling the event [%p]", DrmEvent);
            if (DrmEvent && !DrmEvent->Set())
            {
                auto error = GetLastError();
                GLogger.writeln(L"CreateWindowExW: event was not null but Set failed (%d)", error);
            }
        }
        return CreateWindowExW_orig(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    }



    void InitializeDRMv2()
    {
        DrmEvent = new Utils::Event(L"drm_wait");
    }

    void WaitForDRMv2()
    {
        GLogger.writeln(L"WaitForDRMv2: waiting for DRM...");

        if (!DrmEvent->InError())
        {
            auto rc = DrmEvent->WaitForIt(10000);  // 10 seconds timeout should be more than enough
            switch (rc)
            {
            case Utils::EventWaitValue::Signaled:
                GLogger.writeln(L"WaitForDRMv2: event signaled!");
                delete DrmEvent;
                DrmEvent = nullptr;
                break;
            default:
                GLogger.writeln(L"WaitForDRMv2: event wait failed (EventWaitValue = %d)", (int)rc);
            }
        }
    }
}
