#include "dllexports.h"
#include "dllincludes.h"

#include "utils/io.h"
#include "utils/memory.h"

#include "drm.h"
#include "launcher.h"
#include "modules.h"
#include "proxy_info.h"
#include "ue_types.h"

#include "globals.h"


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
        FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE1_UFunctionBind_Pattern, LE1_UFunctionBind_Mask);
        FIND_PATTERN(UE::tGetName, UE::GetName, L"GetName", LE1_GetName_Pattern, LE1_GetName_Mask);
        break;
    case LEGameVersion::LE2:
        FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE2_UFunctionBind_Pattern, LE2_UFunctionBind_Mask);
        FIND_PATTERN(UE::tGetName, UE::NewGetName, L"NewGetName", LE2_NewGetName_Pattern, LE2_NewGetName_Mask);
        break;
    case LEGameVersion::LE3:
        FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE3_UFunctionBind_Pattern, LE3_UFunctionBind_Mask);
        FIND_PATTERN(UE::tGetName, UE::NewGetName, L"NewGetName", LE3_NewGetName_Pattern, LE3_NewGetName_Mask);
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
    status = MH_CreateHook(reinterpret_cast<LPVOID*>(&UE::UFunctionBind_orig), UE::HookedUFunctionBind, UE::UFunctionBind);
    if (status != MH_OK)
    {
        GLogger.writeFormatLine(L"DetourOffsets: ERROR: creating hk (UFunctionBind) failed, status = %d.", status);
        if (status == MH_ERROR_NOT_EXECUTABLE)
        {
            GLogger.writeFormatLine(L"    (target: %d, hook: %d)", Memory::IsExecutableAddress(UE::UFunctionBind), Memory::IsExecutableAddress(UE::HookedUFunctionBind));
        }
        return false;
    }
    GLogger.writeFormatLine(L"DetourOffsets: UFunctionBind hk created.");


    // Enable the hook we set up previously.
    status = MH_EnableHook(UE::UFunctionBind);
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


    // Register modules.
    GModules.Register(new AsiLoaderModule);     // this must be activated after console is patched
    GModules.Register(new LauncherArgsModule);  // this must be activated instead of console patching


    // Handle Launcher logic if it's a launcher (special case)
    // or the common trilogy stuff if it's not.
    if (GAppProxyInfo.Game == LEGameVersion::Launcher)
    {
        Sleep(3 * 1000);  // wait two seconds instead of waiting for DRM because nothing's urgent
        GLogger.writeFormatLine(L"OnAttach: welcome to Launcher!");

        Launcher::ParseCmdLine(GetCommandLineW());

        if (Launcher::GLaunchTarget != LEGameVersion::Unsupported)
        {
            if (nullptr != CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Launcher::LaunchGame, nullptr, 0, nullptr))
            {
                // ... Launched successfully, will either load ASI next or auto-terminate
            }
            else
            {
                GLogger.writeFormatLine(L"OnAttach: failed to create a LaunchGame thread, error code = %d", GetLastError());
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
    }


    // Errors past this line are not critical.
    // --------------------------------------------------------------------------

    GModules.Activate("AsiLoader");
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
