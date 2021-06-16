#pragma once
#include <Windows.h>


#define SPIDECL          [[nodiscard]] __declspec(noinline) virtual SPIReturn
#define SPIDECL_STATIC   __declspec(noinline) static SPIReturn
#define SPIDEFN


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
    FailureInvalidParam = 13,
    ErrorUnsupportedYet = 99,
    ErrorFatal = 100
};

/// <summary>
/// SPI declaration for use in ASI mods.
/// </summary>
class ISharedProxyInterface
{
    //////////////////////
    // Base fields
    //////////////////////



public:

    //////////////////////
    // Typedefs
    //////////////////////


    typedef char* cstring;
    typedef wchar_t* wcstring;
    typedef const char* ccstring;
    typedef const wchar_t* wccstring;


    //////////////////////
    // Duplicated methods
    //////////////////////


    /// <summary>
    /// Get the expected name of the file mapping to acquire.
    /// </summary>
    /// <param name="buffer">Output buffer</param>
    /// <param name="len">Length of the output buffer in characters</param>
    /// <param name="gameIndex">Index of the game, 0 = Launcher, 1..3 = Mass Effect 1..3</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL_STATIC MakeMemoryName(wcstring buffer, size_t len, int gameIndex)
    {
        if (gameIndex < 0 || gameIndex > 3)
        {
            return SPIReturn::FailureInvalidParam;
        }

        swprintf(buffer, len, L"Local\\LE%dPROXSPI_%d", gameIndex, GetCurrentProcessId());
        return SPIReturn::Success;
    }

    /// <summary>
    /// Get a pointer to a concrete SPI struct initialized by the bink proxy - if it exists.
    /// </summary>
    /// <param name="interfacePtr">Output for the concrete pointer.</param>
    /// <param name="modName">Short name of the plugin.</param>
    /// <param name="modAuthor">Short name of the plugin's author.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL_STATIC Acquire(ISharedProxyInterface** interfacePtr, ccstring modName, ccstring modAuthor)
    {

    }
    /// <summary>
    /// Clean up everything that needs to get cleaned up and unregister the plugin.
    /// </summary>
    /// <param name="interfacePtr"></param>
    /// <returns></returns>
    SPIDECL_STATIC Release(ISharedProxyInterface** interfacePtr)
    {

    }


    //////////////////////
    // Interface methods
    //////////////////////


    /// <summary>
    /// Wait until the DRM decrypts the host game.
    /// </summary>
    /// <param name="timeoutMs">Max time to wait before continuing execution.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL WaitForKickoff(int timeoutMs) = 0;

    /// <summary>
    /// Use bink proxy's built-in injection library to detour a procedure.
    /// </summary>
    /// <param name="name">Name of the hook used for logging purposes.</param>
    /// <param name="target">Pointer to detour.</param>
    /// <param name="detour">Pointer to what to detour the target with.</param>
    /// <param name="original">Pointer to where to write out the original procedure.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL InstallHook(ccstring name, LPVOID target, LPVOID detour, LPVOID* original) = 0;
    /// <summary>
    /// Remove a hook installed by <see cref="ISharedProxyInterface::InstallHook"/>.
    /// </summary>
    /// <param name="name">Name of the hook to remove.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL UninstallHook(ccstring name) = 0;
};

typedef ISharedProxyInterface* ISPIPtr;
