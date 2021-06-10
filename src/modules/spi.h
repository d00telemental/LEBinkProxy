#pragma once
#include <Windows.h>


// Forward decls for while I'm doing the SDK R&D.

class UObject;
class UFunction;
struct FFrame;


#define SPIDECL [[nodiscard]] __declspec(noinline)
#define SPIDEF

#define PROCESSEVENT_ARGS UObject* pObject, UFunction* pFunction, void* pParms, void* const Result
#define CALLFUNCTION_ARGS UObject* pContext, UFunction* pNode, FFrame* Frame, void* const Result


/// <summary>
/// Return value for all public SPI functions.
/// </summary>
enum class SPIReturn : short
{
    Undefined = 0,
    Success = 1,
    FailureGeneric = 2,
    FailureDuplicacy = 3,
    FailureHooking = 4,
    ErrorFatal = 100
};

/// <summary>
/// SPI declaration for use in ASI mods.
/// Only defines the Acquire() method which is used to get a pointer to a concrete struct.
/// </summary>
class ISharedProxyInterface
{
public:
    typedef const char* cstring;
    typedef void(__stdcall* hlOnetimeCb)(void* pData);
    typedef bool(__stdcall* hlProcessEventCb)(PROCESSEVENT_ARGS);
    typedef bool(__stdcall* hlCallFunctionCb)(CALLFUNCTION_ARGS);

    /// <summary>
    /// Get a pointer to a concrete SPI struct initialized by the bink proxy - if it exists.
    /// </summary>
    /// <param name="ppInterfaceOut">Output for the concrete pointer.</param>
    /// <param name="szModName">Short name of the plugin ([3; 63] chars).</param>
    /// <param name="szModAuthor">Short name of the plugin's author ([3; 63] chars).</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    static SPIReturn Acquire(ISharedProxyInterface** ppInterfaceOut, cstring szModName, cstring szModAuthor);

    /// <summary>
    /// Use bink proxy's built-in injection library to detour a procedure.
    /// </summary>
    /// <param name="szName">Name of the hook used for logging purposes.</param>
    /// <param name="pTarget">Pointer to detour.</param>
    /// <param name="pDetour">Pointer to what to detour the target with.</param>
    /// <param name="ppOriginal">Pointer to where to write out the original procedure.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL SPIReturn InstallHook(cstring szName, LPVOID* pTarget, LPVOID* pDetour, LPVOID** ppOriginal);
    /// <summary>
    /// Remove a hook installed by <see cref="ISharedProxyInterface::InstallHook"/>.
    /// </summary>
    /// <param name="szName">Name of the hook to remove.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL SPIReturn UninstallHook(cstring szName);

    /// <summary>
    /// Schedule a callback to run once, and only once, as soon as possible once UnrealScript VM starts working.
    /// </summary>
    /// <param name="cbFunc">Callback of <see cref="hlOnetimeCb"/> to schedule.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL virtual SPIReturn RunOncePostInit(hlOnetimeCb cbFunc);

    /// <summary>
    /// Add a callback to the chain of functions to run in ProcessEvent BEFORE the original ProcessEvent is called.
    /// </summary>
    /// <param name="cbFunc">
    /// The callback to add, should return true unless the original ProcessEvent must not be called.
    /// </param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL virtual SPIReturn OnProcessEventPre(hlProcessEventCb cbFunc);
    /// <summary>
    /// Add a callback to the chain of functions to run in ProcessEvent AFTER the original ProcessEvent is called.
    /// </summary>
    /// <param name="cbFunc">
    /// The callback to add, should return true only if execution needs to continue down the line.
    /// </param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL virtual SPIReturn OnProcessEventPost(hlProcessEventCb cbFunc);

    /// <summary>
    /// Add a callback to the chain of functions to run in CallFunction BEFORE the original CallFunction is called.
    /// </summary>
    /// <param name="cbFunc">
    /// The callback to add, should return true unless the original CallFunction must not be called.
    /// </param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL virtual SPIReturn OnCallFunctionPre(hlCallFunctionCb cbFunc);

    /// <summary>
    /// Add a callback to the chain of functions to run in CallFunction AFTER the original CallFunction is called.
    /// </summary>
    /// <param name="cbFunc">
    /// The callback to add, should return true only if execution needs to continue down the line.
    /// </param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL virtual SPIReturn OnCallFunctionPost(hlCallFunctionCb cbFunc);
};
