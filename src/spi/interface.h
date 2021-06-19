#pragma once
#include <Windows.h>
#include <cstdio>


#define SPIDECL_STATIC   __declspec(noinline) static SPIReturn
#define SPIDECL          [[nodiscard]] __declspec(noinline) virtual SPIReturn
#define SPIDEFN          virtual SPIReturn


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
    FailureWinApi = 14,
    FailureUnsupportedYet = 99,

    ErrorFatal = 100,
    ErrorSharedMemoryUnassigned = 101,
    ErrorConcreteImpUnassigned = 102,
    ErrorAcquireBadMagic = 110,
    ErrorAcquireBadVersion = 111,
    ErrorAcquireLowVersion = 112,
    ErrorAcquireBadSize = 113,
    ErrorAcquireBadPointer = 114
};

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


    SPIDECL_STATIC MakeReturnCodeText(wcstring buffer, SPIReturn code)
    {
        switch (code)
        {
        case SPIReturn::Undefined:                     { wcscpy(buffer, L"Undefined - illegal return code"); break; }
        case SPIReturn::Success:                       { wcscpy(buffer, L"Success"); break; }

        case SPIReturn::FailureGeneric:                { wcscpy(buffer, L"FailureGeneric - unspecified error"); break; }
        case SPIReturn::FailureDuplicacy:              { wcscpy(buffer, L"FailureDuplicacy - something unique was not unique"); break; }
        case SPIReturn::FailureHooking:                { wcscpy(buffer, L"FailureHooking - injection code returned an error"); break; }
        case SPIReturn::FailureInvalidParam:           { wcscpy(buffer, L"FailureInvalidParam - illegal parameter passed to an SPI method"); break; }
        case SPIReturn::FailureUnsupportedYet:         { wcscpy(buffer, L"FailureUnsupportedYet - feature is defined in SPI but not yet provided"); break; }
        case SPIReturn::FailureWinApi:                 { wcscpy(buffer, L"FailureWinApi - a call to Win API function failed, check GetLastError()"); break; }

        case SPIReturn::ErrorFatal:                    { wcscpy(buffer, L"ErrorFatal - unspecified error AFTER WHICH EXECUTION CANNOT CONTINUE"); break; }
        case SPIReturn::ErrorSharedMemoryUnassigned:   { wcscpy(buffer, L"ErrorSharedMemoryUnassigned - internal pointer to the shared memory block was NULL"); break; }
        case SPIReturn::ErrorConcreteImpUnassigned:    { wcscpy(buffer, L"ErrorConcreteImpUnassigned - internal pointer to the implementation was NULL"); break; }
        case SPIReturn::ErrorAcquireBadMagic:          { wcscpy(buffer, L"ErrorAcquireBadMagic - incorrect magic in shared memory"); break; }
        case SPIReturn::ErrorAcquireBadVersion:        { wcscpy(buffer, L"ErrorAcquireBadVersion - version beyond acceptable range"); break; }
        case SPIReturn::ErrorAcquireLowVersion:        { wcscpy(buffer, L"ErrorAcquireLowVersion - version lower than requested"); break; }
        case SPIReturn::ErrorAcquireBadSize:           { wcscpy(buffer, L"ErrorAcquireBadSize - nonsensical size in shared memory"); break; }
        case SPIReturn::ErrorAcquireBadPointer:        { wcscpy(buffer, L"ErrorAcquireBadPointer - impl pointer is zero or points to bad memory"); break; }

        default:                                       { wcscpy(buffer, L"UNRECOGNIZED RETURN CODE"); return SPIReturn::FailureInvalidParam; }
        }

        return SPIReturn::Success;
    }

    /// <summary>
    /// Get the expected name of the file mapping to acquire.
    /// </summary>
    /// <param name="buffer">Output buffer.</param>
    /// <param name="len">Length of the output buffer in characters.</param>
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
        auto rc = MakeMemoryName(nameBuffer, 256, gameIndex);
        if (rc != SPIReturn::Success)
        {
            return rc;
        }

        bufferMapping_ = OpenFileMappingW(FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, FALSE, nameBuffer);
        if (nullptr == bufferMapping_)
        {
            return SPIReturn::FailureWinApi;
        }

        // WARNING: DO NOT CHANGE THE SIZE WITHOUT CHANGES TO THE IMPLEMENTATION!!!
        sharedBuffer_ = (BYTE*)MapViewOfFile(bufferMapping_, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, 0x100);
        if (!sharedBuffer_)
        {
            return SPIReturn::FailureWinApi;
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
        if (candidateVersion < 2
            || candidateVersion > 100)         return SPIReturn::ErrorAcquireBadMagic;
        if (candidateVersion < spiMinVersion)  return SPIReturn::ErrorAcquireLowVersion;

        DWORD candidateSize = *(DWORD*)(sharedBuffer_ + 8);
        if (candidateSize < 0x3C
            || candidateSize > 0x100) return SPIReturn::ErrorAcquireBadSize;

        auto candidatePtr = *(ISharedProxyInterface**)(sharedBuffer_ + candidateSize - 8);
        if (candidatePtr == nullptr)  // TODO: add some access checks
        {
            return SPIReturn::ErrorAcquireBadPointer;
        }

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

    SPIDECL OpenSharedConsole(FILE* outStream, FILE* errStream) = 0;
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
