#include "../src/spi/interface.h"
#include "Common.h"


// Declare SPI dependency - required for the proxy to run the attach and detach points.
// Flags mean:
//   - LE1 is supported,
//   - the min SPI version is 2 (<=> any).
SPI_PLUGINSIDE_SUPPORT(L"ExamplePlugin", L"0.1.0", L"d00telemental", SPI_GAME_LE1, SPI_VERSION_ANY);

// Declare that this plugin loads after DRM.
SPI_PLUGINSIDE_POSTLOAD;

// Declare that this plugin's attach point should run in the main proxy thread.
SPI_PLUGINSIDE_SEQATTACH;


// Custom plugin logic.
#pragma region Custom plugin logic.

#pragma pack(4)
struct FString
{
    wchar_t* Data;
    int Count;
    int Max;
};

// Pattern for the function to hook.
#define P_STRINGBYREF "48 89 54 24 10 56 57 41 56 48 83 EC 30 48 C7 44 24 28 FE FF FF FF 48 89 5C 24 50 48 89 6C 24 60 45 8B F1 41 8B E8 48 8B DA 48 8B F1 33 C0 89 44 24 20 48 89 02"

// Prototype, pointer to store the original function, and the hook.
typedef FString* (*StringByRefT)(void* something, FString* outString, DWORD strRef, DWORD bParse);
StringByRefT StringByRef_orig = nullptr;
void* StringByRef_hook(void* something, FString* outString, DWORD strRef, DWORD bParse)
{
    auto result = StringByRef_orig(something, outString, strRef, bParse);
    writeln(L"StringByRef - %d => %s", strRef, result->Data);
    return result;
}

#pragma endregion


// A convenience macro for hook names.
#define MY_HOOK(NAME) "ExamplePlugin_" NAME


// Things to do once the plugin is loaded.
// If attach mode is sequential, keep things QUICK here.
SPI_IMPLEMENT_ATTACH
{
    Common::OpenConsole();
    writeln(L"OnAttach - hello!");
    Sleep(5000);

    // Find and hook the function which turns a StrRef into a widestring.

    SPIReturn rc;
    void* targetOffset = nullptr;

    rc = InterfacePtr->FindPattern(&targetOffset, P_STRINGBYREF);
    if (rc != SPIReturn::Success)
    {
        writeln(L"OnAttach - FindPattern failed with %d / %s", rc, SPIReturnToString(rc));
        return false;
    }

    rc = InterfacePtr->InstallHook(MY_HOOK("StringByRef"), targetOffset, StringByRef_hook, (void**)(&StringByRef_orig));
    if (rc != SPIReturn::Success)
    {
        writeln(L"OnAttach - InstallHook on 0x%p failed with %d / %s", targetOffset, rc, SPIReturnToString(rc));
        return false;
    }


    // Return false to report an error.
    // Async attach will discard this value.

    return true;
}


// Things to do just before the plugin is unloaded.
// Do not rely on this getting called, just like with the ON_DETACH notification.
// Keep the code here short & sweet, as it is always executed sequentially.
SPI_IMPLEMENT_DETACH
{
    SPIReturn rc = InterfacePtr->UninstallHook(MY_HOOK("StringByRef"));
    writeln(L"OnDetach - UninstallHook returned %d / %s", rc, SPIReturnToString(rc));

    Common::CloseConsole();

    // You can report an error here.
    // Doesn't do anything - yet - only writes to the proxy log.
    return true;
}
