#include "proxy.h"
#include "dllincludes.h"

#include "drm.h"
#include "io.h"
#include "loader.h"
#include "memory.h"
#include "patterns.h"
#include "proxy_info.h"
#include "ue_types.h"
#include "version.h"


#pragma region Game-originating functions

// A wrapper around GetName which can be used to retrieve object names.
wchar_t* GetObjectName(void* pObj)
{
    wchar_t bufferLE1[2048];
    wchar_t** nameLE1;

    wchar_t bufferLE2[16];
    wchar_t** nameLE2;

    wchar_t bufferLE3[16];
    wchar_t** nameLE3;

    switch (GAppProxyInfo.Game)
    {
    case LEGameVersion::LE1:
        nameLE1 = (wchar_t**)::GetName((BYTE*)pObj + 0x48, bufferLE1);  // 0x48 seems to be the offset to Name across all three games
        //GLogger.writeFormatLine(L"Utilities::GetName: returning = %s", *(wchar_t**)name);
        return *nameLE1;
    case LEGameVersion::LE2:
        memset(bufferLE2, 0, 16);
        ::NewGetName((BYTE*)pObj + 0x48, bufferLE2);
        //GLogger.writeFormatLine(L"Utilities::NewGetName: returning = %s", *(wchar_t**)bufferLE2);
        return *(wchar_t**)bufferLE2;
    case LEGameVersion::LE3:
        memset(bufferLE3, 0, 16);
        ::NewGetName((BYTE*)pObj + 0x48, bufferLE3);
        //GLogger.writeFormatLine(L"Utilities::NewGetName: returning = %s", *(wchar_t**)bufferLE3);
        return *(wchar_t**)bufferLE3;
    default:
        GLogger.writeFormatLine(L"GetObjectName: ERROR: unsupported game version.");
        return nullptr;
    }
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

    switch (GAppProxyInfo.Game)
    {
    case LEGameVersion::LE1:
        FIND_PATTERN(tUFunctionBind, UFunctionBind, L"UFunction::Bind", LE1_UFunctionBind_Pattern, LE1_UFunctionBind_Mask);
        FIND_PATTERN(tGetName, GetName, L"GetName", LE1_GetName_Pattern, LE1_GetName_Mask);
        break;
    case LEGameVersion::LE2:
        FIND_PATTERN(tUFunctionBind, UFunctionBind, L"UFunction::Bind", LE2_UFunctionBind_Pattern, LE2_UFunctionBind_Mask);
        FIND_PATTERN(tGetName, NewGetName, L"NewGetName", LE2_NewGetName_Pattern, LE2_NewGetName_Mask);
        break;
    case LEGameVersion::LE3:
        FIND_PATTERN(tUFunctionBind, UFunctionBind, L"UFunction::Bind", LE3_UFunctionBind_Pattern, LE3_UFunctionBind_Mask);
        FIND_PATTERN(tGetName, NewGetName, L"NewGetName", LE3_NewGetName_Pattern, LE3_NewGetName_Mask);
        break;
    default:
        GLogger.writeFormatLine(L"FindOffsets: ERROR: unsupported game version.");
        break;
    }

    return true;
}

// Run the logic to hook all the offsets we found in FindOffsets().
bool DetourOffsets()
{
    MH_STATUS status;

    // Set up UFunction::Bind hook.
    status = MH_CreateHook(reinterpret_cast<LPVOID*>(&UFunctionBind_orig), HookedUFunctionBind, UFunctionBind);
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"DetourOffsets: ERROR: creating hk (UFunctionBind) failed, status = %d.", status);
        if (status == MH_ERROR_NOT_EXECUTABLE)
        {
            GLogger.writeFormatLine(L"    (target: %d, hook: %d)", Memory::IsExecutableAddress(UFunctionBind), Memory::IsExecutableAddress(HookedUFunctionBind));
        }
        return false;
    }
    GLogger.writeFormatLine(L"DetourOffsets: UFunctionBind hook created.");


    // Enable the hook we set up previously.
    status = MH_EnableHook(UFunctionBind);
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"DetourOffsets: ERROR: enabling hk (UFunctionBind) failed, status = %d", status);
        return false;
    }
    GLogger.writeFormatLine(L"DetourOffsets: UFunctionBind hook enabled.");

    return true;
}


void __stdcall OnAttach()
{
    IO::SetupOutput();

    GLogger.writeFormatLine(L"LEBinkProxy by d00telemental");
    GLogger.writeFormatLine(L"Version=\"" LEBINKPROXY_VERSION L"\", built=\"" LEBINKPROXY_BUILDTM L"\", config=\"" LEBINKPROXY_BUILDMD L"\"");
    GLogger.writeFormatLine(L"Only trust distributions from the official NexusMods page:");
    GLogger.writeFormatLine(L"https://www.nexusmods.com/masseffectlegendaryedition/mods/9");


    // Initialize global settings.
    GAppProxyInfo.Initialize();


    // Wait until the game is decrypted.
    DRM::WaitForFuckingDenuvo();


    // Suspend game threads for the duration of bypass initialization
    // because IsShippingPCBuild gets called *very* quickly.
    // Should be resumed on error or just before loading ASIs.
    Memory::SuspendAllOtherThreads();


    // Initialize MinHook.
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        Memory::ResumeAllOtherThreads();
        GLogger.writeFormatLine(L"OnAttach: ERROR: failed to initialize MinHook.");
        return;
    }


    // Find offsets for UFunction::Bind and GetName.
    bool foundOffsets = FindOffsets();
    if (!foundOffsets)
    {
        Memory::ResumeAllOtherThreads();
        GLogger.writeFormatLine(L"OnAttach: aborting...");
        return;
    }


    // Hook everything we found previously.
    bool detouredOffsets = DetourOffsets();
    if (!detouredOffsets)
    {
        Memory::ResumeAllOtherThreads();
        GLogger.writeFormatLine(L"OnAttach: aborting...");
        return;
    }

    Memory::ResumeAllOtherThreads();


    // --------------------------------------------------------------------------


    // Load in ASIs libraries.
    bool loadedASIs = Loader::LoadAllASIs();
    if (!loadedASIs)
    {
        GLogger.writeFormatLine(L"OnAttach: injected OK by failed to load ASIs.");
    }
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
