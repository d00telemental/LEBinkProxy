#include "proxy.h"

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <cstdio>
#include <cstring>
#include "../minhook/include/MinHook.h"

#include "drm.h"
#include "io.h"
#include "memory.h"
#include "patterns.h"


#pragma region Structs and typedefs

// A prototype of a function which LE uses to decode
// a char name from a FNameEntry structure.
typedef void* (__stdcall* tGetName)(void* name, wchar_t* outBuffer);
tGetName GetName = nullptr;

// A prototype of UFunction::Bind method used to
// bind UScript functions to native implementations.
typedef void(__thiscall* tUFunctionBind)(void* pFunction);
tUFunctionBind UFunctionBind = nullptr;
tUFunctionBind UFunctionBind_orig = nullptr;


// A partial representation of a FFrame structure used
// across the UScript VM to encapsulate an execution stack frame.
#pragma pack(4)
struct FFramePartial
{
    BYTE a[0x1C];
    void* Object; // 0x1C
    BYTE* Code;   // 0x24
};

// A partial representation of a UFunction class.
struct UFunctionPartial
{
    BYTE a[0xF8];
    void* Func;
};

#pragma endregion

#pragma region Game-originating functions

// A wrapper around GetName which can be used to retrieve object names.
wchar_t* GetObjectName(void* pObj)
{
    wchar_t buffer[2048];
    wchar_t** name = (wchar_t**)::GetName((BYTE*)pObj + 0x48, buffer);
    //wprintf_s(L"Utilities::GetName buffer = %s, returned = %s\n", buffer, *(wchar_t**)name);
    return *name;
}

// A GNative function which takes no arguments and returns TRUE.
void AlwaysPositiveNative(void* pObject, FFramePartial& pFrame, void* pResult)
{
    wprintf_s(L"AlwaysPositiveNative: called for %s.\n", ::GetObjectName(pObject));

    pFrame.Code++;
    *(long long*)pResult = TRUE;
}

// A hooked wrapper around UFunction::Bind which calls the original and then
// binds IsShippingPCBuild and IsFinalReleaseDebugConsoleBuild to AlwaysPositiveNative.
void HookedUFunctionBind(void* pFunction)
{
    auto name = ::GetObjectName(pFunction);

    UFunctionBind_orig(pFunction);

    if (0 == wcscmp(name, L"IsShippingPCBuild") || 0 == wcscmp(name, L"IsFinalReleaseDebugConsoleBuild"))
    {
        wprintf_s(L"UFunction::Bind(%s) pFunction = 0x%p, Func = 0x%p\n", name, pFunction, *(void**)((BYTE*)pFunction + 0xF8));
        ((UFunctionPartial*)pFunction)->Func = AlwaysPositiveNative;
    }
}

#pragma endregion


#define FIND_PATTERN(TYPE,VAR,NAME,PAT,MASK) \
temp = Memory::ScanProcess(PAT, MASK); \
if (!temp) { \
    printf("FindOffsets: ERROR: failed to find " NAME " .\n"); \
    return false; \
} \
printf("FindOffsets: found " NAME " at %p.\n", temp); \
VAR = (TYPE)temp;

// Run the logic to find all the patterns we need in the process memory.
bool FindOffsets()
{
    BYTE* temp = nullptr;

    FIND_PATTERN(tUFunctionBind, UFunctionBind, "UFunction::Bind", LE1_UFunctionBind_Pattern, LE1_UFunctionBind_Mask);
    FIND_PATTERN(tGetName, GetName, "GetName", LE1_GetName_Pattern, LE1_GetName_Mask);

    return true;
}


void __stdcall OnAttach()
{
    MH_STATUS status;

    IO::SetupConsole();
    printf_s("OnAttach: hello there!\n");


    // Initialize MinHook.
    status = MH_Initialize();
    if (status != MH_OK)
    {
        printf_s("OnAttach: ERROR: failed to initialize MinHook.\n");
        return;
    }


    // Wait until the game is decrypted.
    DRM::WaitForFuckingDenuvo();


    // Find offsets for UFunction::Bind and GetName.
    // Other threads are suspended for the duration of the search.
    Memory::SuspendAllOtherThreads();
    if (!FindOffsets())
    {
        printf_s("OnAttach: aborting...\n");
        return;
    }
    Memory::ResumeAllOtherThreads();


    // Set up UFunction::Bind hook.
    status = MH_CreateHook(UFunctionBind, HookedUFunctionBind, reinterpret_cast<LPVOID*>(&UFunctionBind_orig));
    if (status != MH_OK)
    {
        printf_s("OnAttach: ERROR: MH_CreateHook failed, status = %s\n", MH_StatusToString(status));
        if (status == MH_ERROR_NOT_EXECUTABLE) printf_s("    (target: %d, hook: %d)\n", Memory::IsExecutableAddress(UFunctionBind), Memory::IsExecutableAddress(HookedUFunctionBind));
        return;
    }
    printf_s("OnAttach: hook created.\n");

    // Enable the hook we set up previously.
    status = MH_EnableHook(UFunctionBind);
    if (status != MH_OK)
    {
        printf_s("OnAttach: ERROR: MH_EnableHook failed, status = %s\n", MH_StatusToString(status));
        return;
    }
    printf_s("OnAttach: hook enabled.\n");
}

void __stdcall OnDetach()
{
    printf_s("OnDetach: goodbye :(\n");
    IO::TeardownConsole();
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

    default:
        return TRUE;
    }
}
