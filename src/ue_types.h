#pragma once
#include "dllincludes.h"

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

//#pragma pack(4)
//struct FFramePartial
//{
//    BYTE a[0x24];
//    BYTE* Code;    // LE1 - 0x24, LE2 - 0x24, LE3 - 0x28
//};

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

//struct UFunctionPartial
//{
//    BYTE a[0xF8];
//    void* Func;    // LE1 - 0xF8, LE2 - 0xF0, LE3 - 0xD8
//};

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
