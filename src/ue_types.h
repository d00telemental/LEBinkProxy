#pragma once

#include "gamever.h"
#include "utils/io.h"
#include "dllstruct.h"


#define SYMCONCAT_INNER(X, Y) X##Y
#define SYMCONCAT(X, Y) SYMCONCAT_INNER(X, Y)


namespace UE
{
    // Structure of a generic array used across the engine.
    #pragma pack(4)
    template<typename T>
    struct TArray
    {
        T* Data;
        DWORD Count;
        DWORD Max;
    };

    // Structure of a TArray specialization used as a string.
    struct FString : TArray<wchar_t>
    {
        [[nodiscard]] __forceinline wchar_t* GetStr() const noexcept { return Data; }
    };


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
            memset(bufferLE1, 0, 2048);
            wchar_t bufferLE23[16];
            memset(bufferLE23, 0, 16);

            //GLogger.writeln(L"DEBUG DEBUG DEBUG: ATTACH AT %p", &Name);
            //Sleep(30 * 1000);

            auto nameEntryPtr = &Name;

            switch (GLEBinkProxy.Game)
            {
            case LEGameVersion::LE1:
                return *(wchar_t**)UE::GetName(nameEntryPtr, bufferLE1);
            case LEGameVersion::LE2:
            case LEGameVersion::LE3:
                UE::NewGetName(nameEntryPtr, bufferLE23);
                return *(wchar_t**)bufferLE23;
            default:
                GLogger.writeln(L"GetObjectName: ERROR: unsupported game version.");
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
        GLogger.writeln(L"UE::AlwaysPositiveNative: called for %s.", pObject->GetName());

        switch (GLEBinkProxy.Game)
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
            GLogger.writeln(L"UE::AlwaysPositiveNative: ERROR: unsupported game version.");
            break;
        }
    }

    // A GNative function which takes no arguments and returns FALSE.
    void AlwaysNegativeNative(UObjectPartial* pObject, void* pFrame, void* pResult)
    {
        GLogger.writeln(L"UE::AlwaysNegativeNative: called for %s.", pObject->GetName());

        switch (GLEBinkProxy.Game)
        {
        case LEGameVersion::LE1:
            ((FFramePartialLE1*)(pFrame))->Code++;
            *(long long*)pResult = FALSE;
            break;
        case LEGameVersion::LE2:
            ((FFramePartialLE2*)(pFrame))->Code++;
            *(long long*)pResult = FALSE;
            break;
        case LEGameVersion::LE3:
            ((FFramePartialLE3*)(pFrame))->Code++;
            *(long long*)pResult = FALSE;
            break;
        default:
            GLogger.writeln(L"UE::AlwaysNegativeNative: ERROR: unsupported game version.");
            break;
        }
    }


    // A hooked wrapper around UFunction::Bind which calls the original and then
    // binds IsShippingPCBuild and IsFinalReleaseDebugConsoleBuild to AlwaysPositiveNative.
    void HookedUFunctionBind(UObjectPartial* pFunction)
    {
        UFunctionBind_orig(pFunction);

        auto name = pFunction->GetName();

        if (0 == wcscmp(name, L"IsShippingPCBuild") || 0 == wcscmp(name, L"IsShippingBuild")
            || 0 == wcscmp(name, L"IsFinalReleaseDebugConsoleBuild"))
        {
            switch (GLEBinkProxy.Game)
            {
            case LEGameVersion::LE1:
                GLogger.writeln(L"UFunctionBind (LE1): %s (pFunction = 0x%p).", name, pFunction);
                ((UFunctionPartialLE1*)pFunction)->Func = AlwaysPositiveNative;
                break;
            case LEGameVersion::LE2:
                GLogger.writeln(L"UFunctionBind (LE2): %s (pFunction = 0x%p).", name, pFunction);
                ((UFunctionPartialLE2*)pFunction)->Func = AlwaysPositiveNative;
                break;
            case LEGameVersion::LE3:
                GLogger.writeln(L"UFunctionBind (LE3): %s (pFunction = 0x%p).", name, pFunction);
                ((UFunctionPartialLE3*)pFunction)->Func = AlwaysPositiveNative;
                break;
            default:
                GLogger.writeln(L"HookedUFunctionBind: ERROR: unsupported game version.");
                break;
            }
        }

        // Thanks to Mgamerz's research into why LE3 profiles disappeared:
        if (0 == wcscmp(name, L"IsShip") && GLEBinkProxy.Game == LEGameVersion::LE3)
        {
            GLogger.writeln(L"UFunctionBind (LE3): %s (pFunction = 0x%p).", name, pFunction);
            ((UFunctionPartialLE3*)pFunction)->Func = AlwaysNegativeNative;
        }
    }
}
