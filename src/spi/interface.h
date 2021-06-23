#pragma once
#include <Windows.h>
#include <cstdio>


#define SPIDECL_STATIC   __declspec(noinline) static SPIReturn
#define SPIDECL          [[nodiscard]] __declspec(noinline) virtual SPIReturn
#define SPIDEFN          virtual SPIReturn


/// Game versions.
/// Not convertible with LEGameVersion!!!

#define SPI_GAME_L (1 << 0)
#define SPI_GAME_1 (1 << 1)
#define SPI_GAME_2 (1 << 2)
#define SPI_GAME_3 (1 << 3)


/// Plugin-side definition which marks the dll as supporting SPI.
#define SPI_PLUGINSIDE_SUPPORT(NAME,AUTHOR,GAME_FLAGS,SPIMINVER) \
extern "C" __declspec(dllexport) void SpiSupportDecl(wchar_t** name, wchar_t** author, int* gameIndexFlags, int* spiMinVersion) \
{ *name = NAME;  *author = AUTHOR;  *gameIndexFlags = GAME_FLAGS;  *spiMinVersion = SPIMINVER; }

/// Plugin-side definition which marks the dll for being loaded
/// at the earliest possible moment.
#define SPI_PLUGINSIDE_PRELOAD extern "C" __declspec(dllexport) bool SpiShouldPreload(void) { return true; }

/// Plugin-side definition which marks the dll for being loaded
/// only after DRM has finished decrypting the game.
#define SPI_PLUGINSIDE_POSTLOAD extern "C" __declspec(dllexport) bool SpiShouldPreload(void) { return false; }

/// Plugin-side definition which marks the dll for running
/// its attach point sequentially during game startup.
#define SPI_PLUGINSIDE_SEQATTACH extern "C" __declspec(dllexport) bool SpiShouldSpawnThread(void) { return false; }

/// Plugin-side definition which marks the dll for running
/// its attach point asynchronously during game startup.
#define SPI_PLUGINSIDE_ASYNCATTACH extern "C" __declspec(dllexport) bool SpiShouldSpawnThread(void) { return true; }


#define SPI_IMPLEMENT_ATTACH  extern "C" __declspec(dllexport) bool SpiOnAttach(ISharedProxyInterface* InterfacePtr)
#define SPI_IMPLEMENT_DETACH  extern "C" __declspec(dllexport) bool SpiOnDetach(void)


/// <summary>
/// Return value for all public SPI functions.
///   - 0 is illegal to return,
///   - 1 means success,
///   - [10; 100) mean failures,
///   - [100; ...) mean that normal operation is no longer possible.
/// </summary>
enum class SPIReturn : short
{
    // Adding stuff here requires adding stuff to MakeReturnCodeText() !!!
    Undefined = 0,
    Success = 1,
    FailureGeneric = 10,
    FailureDuplicacy = 11,
    FailureHooking = 12,
    FailureInvalidParam = 13,
    FailureUnsupportedYet = 14,
    ErrorFatal = 100,
    ErrorSharedMemoryUnassigned = 101,
    ErrorConcreteImpUnassigned = 102,
    ErrorAcquireBadMagic = 110,
    ErrorAcquireBadVersion = 111,
    ErrorAcquireLowVersion = 112,
    ErrorAcquireBadSize = 113,
    ErrorAcquireBadPointer = 114,
    ErrorWinApi = 115,
};

/// <summary>
/// Get a string describing an SPIReturn code.
/// </summary>
/// <returns>A const wide string</returns>
const wchar_t* SPIReturnToString(SPIReturn code)
{
    switch (code)
    {
    case SPIReturn::Undefined:                     return L"Undefined - illegal return code";
    case SPIReturn::Success:                       return L"Success - yay!";
    case SPIReturn::FailureGeneric:                return L"FailureGeneric - unspecified error";
    case SPIReturn::FailureDuplicacy:              return L"FailureDuplicacy - something unique was not unique";
    case SPIReturn::FailureHooking:                return L"FailureHooking - injection code returned an error";
    case SPIReturn::FailureInvalidParam:           return L"FailureInvalidParam - illegal parameter passed to an SPI method";
    case SPIReturn::FailureUnsupportedYet:         return L"FailureUnsupportedYet - feature is defined in SPI but not yet provided";
    case SPIReturn::ErrorFatal:                    return L"ErrorFatal - unspecified error AFTER WHICH EXECUTION CANNOT CONTINUE";
    case SPIReturn::ErrorSharedMemoryUnassigned:   return L"ErrorSharedMemoryUnassigned - internal pointer to the shared memory block was NULL";
    case SPIReturn::ErrorConcreteImpUnassigned:    return L"ErrorConcreteImpUnassigned - internal pointer to the implementation was NULL";
    case SPIReturn::ErrorAcquireBadMagic:          return L"ErrorAcquireBadMagic - incorrect magic in shared memory";
    case SPIReturn::ErrorAcquireBadVersion:        return L"ErrorAcquireBadVersion - version beyond acceptable range";
    case SPIReturn::ErrorAcquireLowVersion:        return L"ErrorAcquireLowVersion - version lower than requested";
    case SPIReturn::ErrorAcquireBadSize:           return L"ErrorAcquireBadSize - nonsensical size in shared memory";
    case SPIReturn::ErrorAcquireBadPointer:        return L"ErrorAcquireBadPointer - impl pointer is zero or points to bad memory";
    case SPIReturn::ErrorWinApi:                   return L"ErrorWinApi - a call to Win API function failed, check GetLastError()";
    default:                                       return L"UNRECOGNIZED RETURN CODE - CONTACT DEVELOPERS";
    }
}

/// <summary>
/// SPI declaration for use in ASI mods.
/// </summary>
class ISharedProxyInterface
{
public:

    //////////////////////
    // Typedefs
    //////////////////////

    typedef char* cstring;
    typedef wchar_t* wcstring;
    typedef const char* ccstring;
    typedef const wchar_t* wccstring;


    //////////////////////
    // Interface methods
    //////////////////////

    /// <summary>
    /// Get version of the SPI implementation provided by the host proxy.
    /// </summary>
    /// <param name="outVersionPtr">Output value for the version.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL GetVersion(DWORD* outVersionPtr) = 0;
    /// <summary>
    /// Get if the host proxy was built in debug or release mode.
    /// </summary>
    /// <param name="outIsRelease">Output value for the build mode.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL GetBuildMode(bool* outIsRelease) = 0;

    /// <summary>
    /// Open a new console window for output streams.
    /// </summary>
    /// <param name="outStream">stdout or null</param>
    /// <param name="errStream">stderr or null</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL OpenConsole(FILE* outStream, FILE* errStream) = 0;
    /// <summary>
    /// Free the allocated console.
    /// </summary>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL CloseConsole() = 0;

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
