#pragma once
#include <Windows.h>
#include <cstdio>


#define SPIDECL_STATIC   __declspec(noinline) static SPIReturn
#define SPIDECL          [[nodiscard]] __declspec(noinline) virtual SPIReturn
#define SPIDEFN          virtual SPIReturn


/// Plugin-side definition which marks the dll as supporting SPI.
#define SPI_PLUGINSIDE_SUPPORT(NAME,AUTHOR,GAME,SPIMINVER) \
extern "C" __declspec(dllexport) void SpiSupportDecl(wchar_t** name, wchar_t** author, int* gameIndex, int* spiMinVersion) \
{ *name = NAME;  *author = AUTHOR;  *gameIndex = GAME;  *spiMinVersion = SPIMINVER; }

/// Plugin-side definition which marks the dll as one
/// that should be loaded ASAP.
#define SPI_PLUGINSIDE_PRELOAD extern "C" __declspec(dllexport) bool SpiShouldPreload(void) { return true; }

/// Plugin-side definition which marks the dll as one
/// that should be loaded only after DRM has finished decrypting the game.
#define SPI_PLUGINSIDE_POSTLOAD extern "C" __declspec(dllexport) bool SpiShouldPreload(void) { return false; }


#define SPI_IMPLEMENT_ATTACH  extern "C" __declspec(dllexport) bool SpiOnAttach(ISharedProxyInterface* InterfacePtr)
#define SPI_IMPLEMENT_DETACH  extern "C" __declspec(dllexport) bool SpiOnDetach(void)


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

const wchar_t* ReturnCodeToString(SPIReturn code)
{
    switch (code)
    {
    case SPIReturn::Undefined:                     return L"Undefined - illegal return code";
    case SPIReturn::Success:                       return L"Success";
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
    default:                                       return L"UNRECOGNIZED RETURN CODE";
    }
}

/// <summary>
/// SPI declaration for use in ASI mods.
/// </summary>
class ISharedProxyInterface
{
    //////////////////////
    // Base fields
    //////////////////////

    static ISharedProxyInterface* instance_;
    static HANDLE bufferMapping_;
    static BYTE* sharedBuffer_;

    //////////////////////
    // Private methods
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
    /// Get a pointer to a concrete SPI struct initialized by the bink proxy - if it exists.
    /// </summary>
    /// <param name="interfacePtr">Output for the concrete pointer.</param>
    /// <param name="spiMinVersion">Earliest supported SPI version.</param>
    /// <param name="gameIndex">Index of the game, 0 = Launcher, 1..3 = Mass Effect 1..3.</param>
    /// <param name="modName">Short name of the plugin.</param>
    /// <param name="modAuthor">Short name of the plugin's author.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL_STATIC Acquire(ISharedProxyInterface** interfacePtr, unsigned int spiMinVersion, int gameIndex, wccstring modName, wccstring modAuthor)
    {
        if (gameIndex < 0 || gameIndex > 3)  return SPIReturn::FailureInvalidParam;
        if (!modName || !modAuthor)          return SPIReturn::FailureInvalidParam;
        if (!interfacePtr)                   return SPIReturn::FailureInvalidParam;

        wchar_t nameBuffer[256];
        swprintf(nameBuffer, 256, L"Local\\LE%dPROXSPI_%d", gameIndex, GetCurrentProcessId());

        bufferMapping_ = OpenFileMappingW(FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, FALSE, nameBuffer);
        if (!bufferMapping_)
        {
            return SPIReturn::ErrorWinApi;
        }

        // WARNING: DO NOT CHANGE THE SIZE WITHOUT CHANGES TO THE IMPLEMENTATION!!!
        sharedBuffer_ = (BYTE*)MapViewOfFile(bufferMapping_, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, 0x100);
        if (!sharedBuffer_)
        {
            return SPIReturn::ErrorWinApi;
        }

        // Shared memory structure may change, but the following is harcoded:
        //  - the allocated size is 0x100;
        //  - the first 3 members are `char Magic[4]`, `DWORD Version`, `DWORD Size`;
        //  - the concrete pointer is at `Size - 8`.

        if (0 != memcmp(sharedBuffer_, "SPI", 4))
        {
            return SPIReturn::ErrorAcquireBadMagic;
        }

        DWORD candidateVersion = *(DWORD*)(sharedBuffer_ + 4);
        if (candidateVersion < 2 || candidateVersion > 100)
        {
            return SPIReturn::ErrorAcquireBadMagic;
        }
        if (candidateVersion < spiMinVersion)
        {
            return SPIReturn::ErrorAcquireLowVersion;
        }

        DWORD candidateSize = *(DWORD*)(sharedBuffer_ + 8);
        if (candidateSize < 0x3C || candidateSize > 0x100)
        {
            return SPIReturn::ErrorAcquireBadSize;
        }

        auto candidatePtr = *(ISharedProxyInterface**)(sharedBuffer_ + candidateSize - 8);
        if (!candidatePtr)  // TODO: add some access checks
        {
            return SPIReturn::ErrorAcquireBadPointer;
        }

        // Write out a pointer to the concrete implementation.
        // All actual interface calls will happen through that.
        *interfacePtr = candidatePtr;

        return SPIReturn::Success;
    }
    /// <summary>
    /// Clean up everything that needs to get cleaned up and unregister the plugin.
    /// </summary>
    /// <param name="interfacePtr"></param>
    /// <returns></returns>
    SPIDECL_STATIC Release(ISharedProxyInterface** interfacePtr)
    {
        if (!interfacePtr || !*interfacePtr)
        {
            return SPIReturn::FailureInvalidParam;
        }

        return SPIReturn::Success;
    }


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
    /// Open a new console window for output streams, or simply redirect them to an existing one.
    /// Works by incrementing an internal counter of all the calls to itself.
    /// </summary>
    /// <param name="outStream">stdout or null</param>
    /// <param name="errStream">stderr or null</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL OpenSharedConsole(FILE* outStream, FILE* errStream) = 0;
    /// <summary>
    /// Decrements the opened console counter, closes it when the counter reaches zero.
    /// </summary>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL CloseSharedConsole() = 0;

    /// <summary>
    /// Wait until the DRM decrypts the host game.
    /// </summary>
    /// <param name="timeoutMs">Max time to wait before continuing execution.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL WaitForDRM(int timeoutMs) = 0;

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

HANDLE ISharedProxyInterface::bufferMapping_;
BYTE* ISharedProxyInterface::sharedBuffer_;
