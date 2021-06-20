#include <Windows.h>
#include "../src/spi/interface.h"


#define PLUGIN_NAME  L"ExamplePlugin"
#define PLUGIN_AUTH  L"d00telemental"

#define GAME_INDEX   1  // this is a native plugin for LE1
#define MIN_SPI_VER  2  // any SPI version

// Utility macros for reporting interface errors.
// Obviously, not required for the SPI to work.
#define SYMCONCAT_INNER(X, Y) X##Y
#define SYMCONCAT(X, Y) SYMCONCAT_INNER(X, Y)
#define RETURN_ON_SPI_ERROR_IMPL(MSG,ERR,FILE,LINE) do { \
    wchar_t SYMCONCAT(temp, LINE)[1024]; \
    swprintf_s(SYMCONCAT(temp, LINE), 1024, L"%s\r\nCode: %d\r\nLocation: %S:%d", MSG, ERR, FILE, LINE); \
    MessageBoxW(nullptr, SYMCONCAT(temp, LINE), PLUGIN_NAME " SPI error", MB_OK | MB_ICONERROR | MB_APPLMODAL | MB_TOPMOST); \
    return; \
} while (0)
#define RETURN_ON_SPI_ERROR(MSG,ERR) RETURN_ON_SPI_ERROR_IMPL(MSG, ERR, __FILE__, __LINE__);


// Mod-specific SPI pointer.
// Internally used to uniquely identify the mod.
ISharedProxyInterface* GISPIPtr;


void OnAttach()
{
    SPIReturn rc;

    // Get a pointer to the proxy interface.
    rc = ISharedProxyInterface::Acquire(&GISPIPtr, MIN_SPI_VER, GAME_INDEX, PLUGIN_NAME, PLUGIN_AUTH);
    if (rc != SPIReturn::Success)
    {
        RETURN_ON_SPI_ERROR(L"Failed to acquire an interface pointer, aborting...", rc);
    }

    wchar_t buffer[256];
    swprintf_s(buffer, 256, L"Acquired pointer: 0x%p", GISPIPtr);
    MessageBoxW(nullptr, buffer, L"Success?!", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);

    // Open a new console for logging, or attach to an existing one.
    rc = GISPIPtr->OpenSharedConsole(stdout, stderr);

    //// Wait until DRM decrypts the game, 10 seconds at most.
    //rc = GISPIPtr->WaitForDRM(10000);

    // Report the progress.
    fwprintf_s(stdout, L"Initialized SPI, opened a console, waited for DRM.\n");

    //// Hook UObject::ProcessEvent.
    //void* origFunc;
    //rc = GISPIPtr->InstallHook("ProcessEvent", nullptr, nullptr, &origFunc);

    return;
}

void OnDetach()
{
    // No need to uninstall hooks, the interface will do it on its own.

    // This may also return an error code, but in reality
    // we have neither need nor time to gracefully handle that.
    ISharedProxyInterface::Release(&GISPIPtr);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(OnAttach), nullptr, 0, nullptr);
        return true;
    case DLL_PROCESS_DETACH:
        OnDetach();
        return true;
    default:
        return true;
    }
}
