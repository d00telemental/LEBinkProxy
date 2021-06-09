#pragma once
#include "dllincludes.h"

namespace UE
{
    // A prototype of a function which LE1 uses to decode
    // a char name from a FNameEntry structure.
    typedef void* (__stdcall* tGetName)(void* name, wchar_t* outBuffer);
    tGetName GetName = nullptr;

    // A prototype of a function which LE2/3 use to decode
    // a char name from a FNameEntry structure.
    typedef void* (__stdcall* tNewGetName)(void* name, wchar_t* outBuffer);
    tNewGetName NewGetName = nullptr;

    // Note:
    // I've got a pretty solid way of getting names without hooking ToString (^),
    // but this works and seems to be stable across patches,
    // so I'm not gonna change it anytime soon.


    // A prototype of UFunction::Bind method used to
    // bind UScript functions to native implementations.
    typedef void(__thiscall* tUFunctionBind)(void* pFunction);
    tUFunctionBind UFunctionBind = nullptr;
    tUFunctionBind UFunctionBind_orig = nullptr;


    // A partial representation of a UObject class.

    struct UObjectPartial
    {
        BYTE a[0x48];  // 0x48 seems to be the offset to Name across all three games
        void* Name;

        // A game-agnostic wrapper around GetName to retrieve object names.
        wchar_t* GetName()
        {
            wchar_t bufferLE1[2048];
            wchar_t bufferLE23[16];
            memset(bufferLE23, 0, 16);

            auto nameEntryPtr = Name;

            switch (GAppProxyInfo.Game)
            {
            case LEGameVersion::LE1:
                return *(wchar_t**)UE::GetName(nameEntryPtr, bufferLE1);
            case LEGameVersion::LE2:
            case LEGameVersion::LE3:
                UE::NewGetName(nameEntryPtr, bufferLE23);
                return *(wchar_t**)bufferLE23;
            default:
                GLogger.writeFormatLine(L"GetObjectName: ERROR: unsupported game version.");
                return nullptr;
            }
        }
    };


    // A partial representation of a FFrame structure used
    // across the UScript VM to encapsulate an execution stack frame.

    #pragma pack(4)
    struct FFramePartialLE1
    {
        BYTE a[0x24];
        BYTE* Code;
    };
    #pragma pack(4)
    struct FFramePartialLE2
    {
        BYTE a[0x24];
        BYTE* Code;
    };
    #pragma pack(4)
    struct FFramePartialLE3
    {
        BYTE a[0x28];
        BYTE* Code;
    };


    // A partial representation of a UFunction class.
    // TODO: once I have SDKs built, use complete definitions for UObject -> UField -> UStruct -> UFunction.

    struct UFunctionPartialLE1
    {
        BYTE a[0xF8];
        void* Func;
    };
    struct UFunctionPartialLE2
    {
        BYTE a[0xF0];
        void* Func;
    };
    struct UFunctionPartialLE3
    {
        BYTE a[0xD8];
        void* Func;
    };


    // A GNative function which takes no arguments and returns TRUE.
    void AlwaysPositiveNative(UObjectPartial* pObject, void* pFrame, void* pResult)
    {
        GLogger.writeFormatLine(L"UE::AlwaysPositiveNative: called for %s.", pObject->GetName());

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
            GLogger.writeFormatLine(L"UE::AlwaysPositiveNative: ERROR: unsupported game version.");
            break;
        }
    }


    // A hooked wrapper around UFunction::Bind which calls the original and then
    // binds IsShippingPCBuild and IsFinalReleaseDebugConsoleBuild to AlwaysPositiveNative.
    void HookedUFunctionBind(UObjectPartial* pFunction)
    {
        UFunctionBind_orig(pFunction);

        auto name = pFunction->GetName();
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
}
