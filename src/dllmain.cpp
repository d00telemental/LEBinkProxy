#include "dllexports.h"

#include <Windows.h>

#include "conf/version.h"
#include "gamever.h"
#include "utils/io.h"
#include "utils/hook.h"
#include "dllstruct.h"
#include "modules/asi_loader.h"
#include "modules/console_enabler.h"
#include "modules/launcher_args.h"


//#define FIND_PATTERN(TYPE,VAR,NAME,PAT,MASK) \
//temp = Memory::ScanProcess(PAT, MASK); \
//if (!temp) { \
//    GLogger.writeFormatLine(L"FindOffsets: ERROR: failed to find " NAME L"."); \
//    return false; \
//} \
//GLogger.writeFormatLine(L"FindOffsets: found " NAME L" at %p.", temp); \
//VAR = (TYPE)temp;
//
//// Run the logic to find all the patterns we need in the process memory.
//bool FindOffsets()
//{
//    BYTE* temp = nullptr;
//
//    switch (GLEBinkProxy.Game)
//    {
//    case LEGameVersion::LE1:
//        FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE1_UFunctionBind_Pattern, LE1_UFunctionBind_Mask);
//        FIND_PATTERN(UE::tGetName, UE::GetName, L"GetName", LE1_GetName_Pattern, LE1_GetName_Mask);
//        break;
//    case LEGameVersion::LE2:
//        FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE2_UFunctionBind_Pattern, LE2_UFunctionBind_Mask);
//        FIND_PATTERN(UE::tGetName, UE::NewGetName, L"NewGetName", LE2_NewGetName_Pattern, LE2_NewGetName_Mask);
//        break;
//    case LEGameVersion::LE3:
//        FIND_PATTERN(UE::tUFunctionBind, UE::UFunctionBind, L"UFunction::Bind", LE3_UFunctionBind_Pattern, LE3_UFunctionBind_Mask);
//        FIND_PATTERN(UE::tGetName, UE::NewGetName, L"NewGetName", LE3_NewGetName_Pattern, LE3_NewGetName_Mask);
//        break;
//    default:
//        GLogger.writeFormatLine(L"FindOffsets: ERROR: unsupported game version.");
//        break;
//    }
//
//    return true;
//}
//
//// Run the logic to hook all the offsets we found in FindOffsets().
//bool DetourOffsets()
//{
//    MH_STATUS status;
//
//    // Set up UFunction::Bind hook.
//    status = MH_CreateHook(reinterpret_cast<LPVOID*>(&UE::UFunctionBind_orig), UE::HookedUFunctionBind, UE::UFunctionBind);
//    if (status != MH_OK)
//    {
//        GLogger.writeFormatLine(L"DetourOffsets: ERROR: creating hk (UFunctionBind) failed, status = %d.", status);
//        if (status == MH_ERROR_NOT_EXECUTABLE)
//        {
//            GLogger.writeFormatLine(L"    (target: %d, hook: %d)", Memory::IsExecutableAddress(UE::UFunctionBind), Memory::IsExecutableAddress(UE::HookedUFunctionBind));
//        }
//        return false;
//    }
//    GLogger.writeFormatLine(L"DetourOffsets: UFunctionBind hk created.");
//
//
//    // Enable the hook we set up previously.
//    status = MH_EnableHook(UE::UFunctionBind);
//    if (status != MH_OK)
//    {
//        GLogger.writeFormatLine(L"DetourOffsets: ERROR: enabling hk (UFunctionBind) failed, status = %d", status);
//        return false;
//    }
//    GLogger.writeFormatLine(L"DetourOffsets: UFunctionBind hk enabled.");
//
//    return true;
//}
//
//// Hook CreateWindowExW.
//bool DetourWindowCreation()
//{
//    MH_STATUS status;
//
//    // Set up UFunction::Bind hook.
//    status = MH_CreateHook(reinterpret_cast<LPVOID*>(&DRM::CreateWindowExW_orig), DRM::CreateWindowExW_hooked, CreateWindowExW);
//    if (status != MH_OK)
//    {
//        GLogger.writeFormatLine(L"DetourOffsets: ERROR: creating hk (CreateWindowExW) failed, status = %d.", status);
//        if (status == MH_ERROR_NOT_EXECUTABLE)
//        {
//            GLogger.writeFormatLine(L"    (target: %d, hook: %d)", Memory::IsExecutableAddress(CreateWindowExW), Memory::IsExecutableAddress(DRM::CreateWindowExW_hooked));
//        }
//        return false;
//    }
//    GLogger.writeFormatLine(L"DetourOffsets: CreateWindowExW hk created.");
//
//
//    // Enable the hook we set up previously.
//    status = MH_EnableHook(CreateWindowExW);
//    if (status != MH_OK)
//    {
//        GLogger.writeFormatLine(L"DetourOffsets: ERROR: enabling hk (CreateWindowExW) failed, status = %d", status);
//        return false;
//    }
//    GLogger.writeFormatLine(L"DetourOffsets: CreateWindowExW hk enabled.");
//
//    return true;
//}

void __stdcall OnAttach()
{
    // Open console or log.
    Utils::SetupOutput();

    GLogger.writeFormatLine(L"LEBinkProxy by d00telemental");
    GLogger.writeFormatLine(L"Version=\"" LEBINKPROXY_VERSION L"\", built=\"" LEBINKPROXY_BUILDTM L"\", config=\"" LEBINKPROXY_BUILDMD L"\"");
    GLogger.writeFormatLine(L"Only trust distributions from the official NexusMods page:");
    GLogger.writeFormatLine(L"https://www.nexusmods.com/masseffectlegendaryedition/mods/9");

    // Initialize MinHook.
    MH_STATUS mhStatus;
    if (!GHookManager.IsOK(mhStatus))
    {
        GLogger.writeFormatLine(L"OnAttach: ERROR: failed to initialize MinHook (mh code = %d).", mhStatus);
        return;
    }

    // Initialize global settings.
    GLEBinkProxy.Initialize();

    // Register modules (console enabler, launcher arg handler, asi loader).
    GLEBinkProxy.AsiLoader = new AsiLoaderModule;
    GLEBinkProxy.ConsoleEnabler = new ConsoleEnablerModule;
    GLEBinkProxy.LauncherArgs = new LauncherArgsModule;

    // Handle logic depending on the attached-to exe.
    switch (GLEBinkProxy.Game)
    {
        case LEGameVersion::LE1:
        case LEGameVersion::LE2:
        case LEGameVersion::LE3:
        {
            break;
        }
        case LEGameVersion::Launcher:
        {
            Sleep(3000);  // wait three seconds instead of waiting for DRM because nothing's urgent
            GLogger.writeFormatLine(L"OnAttach: welcome to Launcher!");
            if (!GLEBinkProxy.LauncherArgs->Activate())
            {
                GLogger.writeFormatLine(L"OnAttach: ERROR: handling of Launcher args failed, aborting!");
            }
            break;
        }
        default:
        {
            GLogger.writeFormatLine(L"OnAttach: unsupported game, bye!");
            return;
        }
    }

    // Inject ASIs.

    if (!GLEBinkProxy.AsiLoader->Activate())
    {
        GLogger.writeFormatLine(L"OnAttach: ERROR: loading of one or more ASI plugins failed!");
    }

    return;











    //{
    //    // Wait until the game is decrypted.
    //    DetourWindowCreation();
    //    DRM::WaitForDRMv2();
    //    // Suspend game threads for the duration of bypass initialization
    //    // because IsShippingPCBuild gets called *very* quickly.
    //    // Should be resumed on error or just before loading ASIs.
    //    Memory::SuspendAllOtherThreads();
    //    // Find offsets for UFunction::Bind and GetName.
    //    bool foundOffsets = FindOffsets();
    //    if (!foundOffsets)
    //    {
    //        Memory::ResumeAllOtherThreads();
    //        GLogger.writeFormatLine(L"OnAttach: aborting...");
    //        return;
    //    }
    //    // Hook everything we found previously.
    //    bool detouredOffsets = DetourOffsets();
    //    if (!detouredOffsets)
    //    {
    //        Memory::ResumeAllOtherThreads();
    //        GLogger.writeFormatLine(L"OnAttach: aborting...");
    //        return;
    //    }
    //    Memory::ResumeAllOtherThreads();
    //}
}

void __stdcall OnDetach()
{
    GLogger.writeFormatLine(L"OnDetach: goodbye, I thought we were friends :(");
    Utils::TeardownOutput();
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
