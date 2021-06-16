#pragma once
#include <Windows.h>


// Forward decls for while I'm doing the SDK R&D.

class UObject;
class UFunction;
struct FFrame;


#define SPIDECL [[nodiscard]] __declspec(noinline) virtual
#define SPIDEF


/// <summary>
/// Return value for all public SPI functions.
/// 
/// 0 is illegal to return,
/// 1 means success,
/// [10; 100) mean failures,
/// [100; ...) mean that normal operation is no longer possible.
/// </summary>
enum class SPIReturn : short
{
    Undefined = 0,
    Success = 1,
    FailureGeneric = 10,
    FailureDuplicacy = 11,
    FailureHooking = 12,
    ErrorUnsupportedYet = 99,
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

    /// <summary>
    /// Get a pointer to a concrete SPI struct initialized by the bink proxy - if it exists.
    /// </summary>
    /// <param name="ppInterfaceOut">Output for the concrete pointer.</param>
    /// <param name="szModName">Short name of the plugin ([3; 63] chars).</param>
    /// <param name="szModAuthor">Short name of the plugin's author ([3; 63] chars).</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    static SPIReturn Acquire(ISharedProxyInterface** ppInterfaceOut, cstring szModName, cstring szModAuthor)
    {

    }

    /// <summary>
    /// Use bink proxy's built-in injection library to detour a procedure.
    /// </summary>
    /// <param name="szName">Name of the hook used for logging purposes.</param>
    /// <param name="pTarget">Pointer to detour.</param>
    /// <param name="pDetour">Pointer to what to detour the target with.</param>
    /// <param name="ppOriginal">Pointer to where to write out the original procedure.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL SPIReturn InstallHook(cstring szName, LPVOID* pTarget, LPVOID* pDetour, LPVOID** ppOriginal) = 0;
    /// <summary>
    /// Remove a hook installed by <see cref="ISharedProxyInterface::InstallHook"/>.
    /// </summary>
    /// <param name="szName">Name of the hook to remove.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code. All codes above 1 must be treated as failure.</returns>
    SPIDECL SPIReturn UninstallHook(cstring szName) = 0;
};
