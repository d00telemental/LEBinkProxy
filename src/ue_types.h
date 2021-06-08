#pragma once
#include "dllincludes.h"

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
