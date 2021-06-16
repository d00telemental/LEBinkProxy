#include <Windows.h>

#include "../src/spi/interface.h"

ISharedProxyInterface* GISPIPtr;

void __stdcall OnAttach()
{
    //Utils::SetupOutput();

    // Get a pointer to the proxy interface.
    SPIReturn rc = ISharedProxyInterface::Acquire(&GISPIPtr, "ExamplePlugin", "d00telemental");
    if (rc != SPIReturn::Success)
    {
        return;
    }

    // Wait until DRM decrypts the game.
    GISPIPtr->WaitForKickoff();

    // Hook UObject::ProcessEvent.
    void* origFunc;
    rc = GISPIPtr->InstallHook("ProcessEvent", nullptr, nullptr, &origFunc);

    return;
}

void __stdcall OnDetach()
{
    //Utils::TeardownOutput();

    // No need to uninstall hooks,
    // the interface will do it on its own.
    ISharedProxyInterface::Release(&GISPIPtr);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)OnAttach, nullptr, 0, nullptr);
        return TRUE;
    case DLL_PROCESS_DETACH:
        OnDetach();
        return TRUE;
    }
    return true;
}
