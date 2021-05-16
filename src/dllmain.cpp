#include "proxy.h"
#include "dllincludes.h"

#include "drm.h"
#include "io.h"
#include "memory.h"
#include "patterns.h"
#include "proxy_info.h"
#include "ue_types.h"


#pragma region Game-originating functions

// A wrapper around GetName which can be used to retrieve object names.
wchar_t* GetObjectName(void* pObj)
{
    wchar_t buffer[2048];
    wchar_t** name = (wchar_t**)::GetName((BYTE*)pObj + 0x48, buffer);  // 0x48 seemes to be the offset to Name across all three games
    //GLogger.writeFormatLine(L"Utilities::GetName buffer = %s, returned = %s", buffer, *(wchar_t**)name);
    return *name;
}

// A GNative function which takes no arguments and returns TRUE.
void AlwaysPositiveNative(void* pObject, void* pFrame, void* pResult)
{
    GLogger.writeFormatLine(L"AlwaysPositiveNative: called for %s.", ::GetObjectName(pObject));

    switch (GAppProxyInfo.Game)
    {
    case LEGameVersion::LE1:
        ((FFramePartialLE1*)(pFrame))->Code++;
        *(long long*)pResult = TRUE;
        break;
    case LEGameVersion::LE2:
        ((FFramePartialLE2*)(pFrame))->Code++;
        *(long long*)pResult = TRUE;
        break;
    case LEGameVersion::LE3:
        ((FFramePartialLE3*)(pFrame))->Code++;
        *(long long*)pResult = TRUE;
        break;
    default:
        GLogger.writeFormatLine(L"AlwaysPositiveNative: ERROR: unsupported game version.");
        break;
    }
}

// A hooked wrapper around UFunction::Bind which calls the original and then
// binds IsShippingPCBuild and IsFinalReleaseDebugConsoleBuild to AlwaysPositiveNative.
void HookedUFunctionBind(void* pFunction)
{
    UFunctionBind_orig(pFunction);

    auto name = ::GetObjectName(pFunction);
    if (0 == wcscmp(name, L"IsShippingPCBuild") || 0 == wcscmp(name, L"IsFinalReleaseDebugConsoleBuild"))
    {
        switch (GAppProxyInfo.Game)
        {
        case LEGameVersion::LE1:
            GLogger.writeFormatLine(L"UFunctionBind (LE1): %s (pFunction = 0x%p).", name, pFunction);
            ((UFunctionPartialLE1*)pFunction)->Func = AlwaysPositiveNative;
            break;
        case LEGameVersion::LE2:
            GLogger.writeFormatLine(L"UFunctionBind (LE2): %s (pFunction = 0x%p).", name, pFunction);
            ((UFunctionPartialLE2*)pFunction)->Func = AlwaysPositiveNative;
            break;
        case LEGameVersion::LE3:
            GLogger.writeFormatLine(L"UFunctionBind (LE3): %s (pFunction = 0x%p).", name, pFunction);
            ((UFunctionPartialLE3*)pFunction)->Func = AlwaysPositiveNative;
            break;
        default:
            GLogger.writeFormatLine(L"HookedUFunctionBind: ERROR: unsupported game version.");
            break;
        }

    }
}

#pragma endregion


#define FIND_PATTERN(TYPE,VAR,NAME,PAT,MASK) \
temp = Memory::ScanProcess(PAT, MASK); \
if (!temp) { \
    GLogger.writeFormatLine(L"FindOffsets: ERROR: failed to find " NAME L"."); \
    return false; \
} \
GLogger.writeFormatLine(L"FindOffsets: found " NAME L" at %p.", temp); \
VAR = (TYPE)temp;

// Run the logic to find all the patterns we need in the process memory.
bool FindOffsets()
{
    BYTE* temp = nullptr;

    FIND_PATTERN(tUFunctionBind, UFunctionBind, L"UFunction::Bind", LE1_UFunctionBind_Pattern, LE1_UFunctionBind_Mask);
    FIND_PATTERN(tGetName, GetName, L"GetName", LE1_GetName_Pattern, LE1_GetName_Mask);

    return true;
}


void __stdcall OnAttach()
{
    IO::SetupOutput();
    GLogger.writeFormatLine(L"OnAttach: hello there!");


    // Initialize global settings.
    GAppProxyInfo.Initialize();


    // Wait until the game is decrypted.
    DRM::WaitForFuckingDenuvo();


    // Initialize MinHook.
    MH_STATUS status;
    status = MH_Initialize();
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"OnAttach: ERROR: failed to initialize MinHook.");
        return;
    }


    // Find offsets for UFunction::Bind and GetName.
    // Other threads are suspended for the duration of the search.
    Memory::SuspendAllOtherThreads();
    if (!FindOffsets())
    {
        GLogger.writeFormatLine(L"OnAttach: aborting...");
        Memory::ResumeAllOtherThreads();
        return;
    }
    Memory::ResumeAllOtherThreads();


    // Set up UFunction::Bind hook.
    status = MH_CreateHook(UFunctionBind, HookedUFunctionBind, reinterpret_cast<LPVOID*>(&UFunctionBind_orig));
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"OnAttach: ERROR: MH_CreateHook failed, status = %s.", MH_StatusToString(status));
        if (status == MH_ERROR_NOT_EXECUTABLE)
        {
            GLogger.writeFormatLine(L"    (target: %d, hook: %d)", Memory::IsExecutableAddress(UFunctionBind), Memory::IsExecutableAddress(HookedUFunctionBind));
        }
        return;
    }
    GLogger.writeFormatLine(L"OnAttach: hook created.");


    // Enable the hook we set up previously.
    status = MH_EnableHook(UFunctionBind);
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"OnAttach: ERROR: MH_EnableHook failed, status = %s", MH_StatusToString(status));
        return;
    }
    GLogger.writeFormatLine(L"OnAttach: hook enabled.");
}

void __stdcall OnDetach()
{
    GLogger.writeFormatLine(L"OnDetach: goodbye :(");
    IO::TeardownOutput();
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
