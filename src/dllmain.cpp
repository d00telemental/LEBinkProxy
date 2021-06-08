#include "proxy.h"
#include "dllincludes.h"

#include "drm.h"
#include "io.h"
#include "launcher.h"
#include "loader.h"
#include "memory.h"
#include "patterns.h"
#include "proxy_info.h"
#include "ue_types.h"


#pragma region Game-originating functions

// A game-agnostic wrapper around GetName to retrieve object names.
wchar_t* GetObjectName(void* pObj)
{
    wchar_t bufferLE1[2048];
    wchar_t bufferLE23[16];
    memset(bufferLE23, 0, 16);

    auto nameEntryPtr = (BYTE*)pObj + 0x48;  // 0x48 seems to be the offset to Name across all three games

    switch (GAppProxyInfo.Game)
    {
    case LEGameVersion::LE1:
        return *(wchar_t**)::GetName(nameEntryPtr, bufferLE1);
    case LEGameVersion::LE2:
        ::NewGetName(nameEntryPtr, bufferLE23);
        return *(wchar_t**)bufferLE23;
    case LEGameVersion::LE3:
        ::NewGetName(nameEntryPtr, bufferLE23);
        return *(wchar_t**)bufferLE23;
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
    GLogger.writeFormatLine(L"DetourOffsets: UFunctionBind hk created.");


    // Enable the hook we set up previously.
    status = MH_EnableHook(UFunctionBind);
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"DetourOffsets: ERROR: enabling hk (UFunctionBind) failed, status = %d", status);
        return false;
    }
    GLogger.writeFormatLine(L"DetourOffsets: UFunctionBind hk enabled.");

    return true;
}

#ifdef LEBINKPROXY_USE_NEWDRMWAIT
// Hook CreateWindowExW.
bool DetourWindowCreation()
{
    MH_STATUS status;

    // Set up UFunction::Bind hook.
    status = MH_CreateHook(reinterpret_cast<LPVOID*>(&DRM::CreateWindowExW_orig), DRM::CreateWindowExW_hooked, CreateWindowExW);
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"DetourOffsets: ERROR: creating hk (CreateWindowExW) failed, status = %d.", status);
        if (status == MH_ERROR_NOT_EXECUTABLE)
        {
            GLogger.writeFormatLine(L"    (target: %d, hook: %d)", Memory::IsExecutableAddress(CreateWindowExW), Memory::IsExecutableAddress(DRM::CreateWindowExW_hooked));
        }
        return false;
    }
    GLogger.writeFormatLine(L"DetourOffsets: CreateWindowExW hk created.");


    // Enable the hook we set up previously.
    status = MH_EnableHook(CreateWindowExW);
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"DetourOffsets: ERROR: enabling hk (CreateWindowExW) failed, status = %d", status);
        return false;
    }
    GLogger.writeFormatLine(L"DetourOffsets: CreateWindowExW hk enabled.");

    return true;
}
#endif

void __stdcall OnAttach()
{
    IO::SetupOutput();

    GLogger.writeFormatLine(L"LEBinkProxy by d00telemental");
    GLogger.writeFormatLine(L"Version=\"" LEBINKPROXY_VERSION L"\", built=\"" LEBINKPROXY_BUILDTM L"\", config=\"" LEBINKPROXY_BUILDMD L"\"");
    GLogger.writeFormatLine(L"Only trust distributions from the official NexusMods page:");
    GLogger.writeFormatLine(L"https://www.nexusmods.com/masseffectlegendaryedition/mods/9");


    // Initialize MinHook.
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        Memory::ResumeAllOtherThreads();
        GLogger.writeFormatLine(L"OnAttach: ERROR: failed to initialize MinHook.");
        return;
    }


    // Initialize global settings.
    GAppProxyInfo.Initialize();


    // Handle Launcher logic if it's a launcher (special case)
    // or the common trilogy stuff if it's not.
    if (GAppProxyInfo.Game == LEGameVersion::Launcher)
    {
        Sleep(2 * 1000);  // wait two seconds instead of waiting for DRM because nothing's urgent
        GLogger.writeFormatLine(L"OnAttach: welcome to Launcher!");

        auto cmdLine = GetCommandLineW();
        auto versionToLaunch = Launcher::ParseVersionFromCmd(cmdLine);
        if (versionToLaunch != LEGameVersion::Unsupported)
        {
            auto gamePath = Launcher::GameVersionToPath(versionToLaunch);
            auto gameParms = " -NoHomeDir -SeekFreeLoadingPCConsole -locale {locale} -Subtitles 20 -OVERRIDELANGUAGE=INT";

            STARTUPINFOA si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            if (0 != CreateProcessA(gamePath, const_cast<char*>(gameParms), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
            {
                GLogger.writeFormatLine(L"OnAttach: created a process (pid = %d), waiting until it ends...", pi.dwProcessId);

                WaitForSingleObject(pi.hProcess, INFINITE);
                GLogger.writeFormatLine(L"OnAttach: process ended, closing handles...");
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else
            {
                GLogger.writeFormatLine(L"OnAttach: failed to create process, error code = %d", GetLastError());
            }
        }
    }
    else
    {
        // Wait until the game is decrypted.
#ifdef LEBINKPROXY_USE_NEWDRMWAIT
        DetourWindowCreation();
        DRM::WaitForDRMv2();
#else
        DRM::WaitForDRM();
#endif

        // Suspend game threads for the duration of bypass initialization
        // because IsShippingPCBuild gets called *very* quickly.
        // Should be resumed on error or just before loading ASIs.
        Memory::SuspendAllOtherThreads();


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


        // Errors past this line are not critical.
        // --------------------------------------------------------------------------


        // Load in ASIs libraries.
        bool loadedASIs = Loader::LoadAllASIs();
        if (!loadedASIs)
        {
            GLogger.writeFormatLine(L"OnAttach: injected OK but failed to load (some?) ASIs.");
        }
    }
}

void __stdcall OnDetach()
{
    GLogger.writeFormatLine(L"OnDetach: goodbye, I thought we were friends :(");
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
