#pragma once


#pragma region Plugin-side utilities

/// Game versions for SpiSupportDecl.
/// Not convertible with LEGameVersion or SPIGameVersion!!!

#define SPI_GAME_LEL (1 << 0)
#define SPI_GAME_LE1 (1 << 1)
#define SPI_GAME_LE2 (1 << 2)
#define SPI_GAME_LE3 (1 << 3)


/// SPI version macros.
/// Duplicate the stuff in version.h!!!

#define SPI_VERSION_ANY     3
#define SPI_VERSION_LATEST  3

/// Plugin-side definition which marks the dll as supporting SPI.
#define SPI_PLUGINSIDE_SUPPORT(NAME,AUTHOR,VERSION,GAME_FLAGS,SPIMINVER) \
extern "C" __declspec(dllexport) void SpiSupportDecl(wchar_t** name, wchar_t** author, wchar_t** version, int* gameIndexFlags, int* spiMinVersion) \
{ *name = NAME;  *author = AUTHOR; *version = VERSION; *gameIndexFlags = GAME_FLAGS;  *spiMinVersion = SPIMINVER; }

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

/// Plugin-side boilerplate macro for defining the plugin attach point.
/// This is run when the plugin is loaded by SPI (!), not when the DLL itself is loaded.
#define SPI_IMPLEMENT_ATTACH  extern "C" __declspec(dllexport) bool SpiOnAttach(ISharedProxyInterface* InterfacePtr)
/// Plugin-side boilerplate macro for defining the plugin detach point.
/// This *should* run when the plugin is unloaded by SPI (!), not when the DLL itself is unloaded.
/// WARNING: DO NOT RELY ON THIS BEING RUN
#define SPI_IMPLEMENT_DETACH  extern "C" __declspec(dllexport) bool SpiOnDetach(ISharedProxyInterface* InterfacePtr)

#pragma endregion


#pragma region Error codes

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
    FailureDeprecated = 15,
    FailurePatternInvalid = 16,
    FailurePatternTooLong = 17,
    ErrorFatal = 100,
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
    case SPIReturn::FailureDuplicacy:              return L"FailureDuplicacy - something unique was not unique, or something that should have existed didn't";
    case SPIReturn::FailureHooking:                return L"FailureHooking - injection code returned an error";
    case SPIReturn::FailureInvalidParam:           return L"FailureInvalidParam - illegal parameter passed to an SPI method";
    case SPIReturn::FailureUnsupportedYet:         return L"FailureUnsupportedYet - feature is defined in SPI but not yet provided";
    case SPIReturn::FailureDeprecated:             return L"FailureDeprecated - feature is defined in SPI but must not be used";
    case SPIReturn::FailurePatternInvalid:         return L"FailurePatternInvalid - provided pattern was ill-formed";
    case SPIReturn::FailurePatternTooLong:         return L"FailurePatternTooLong - provided pattern is too long";
    case SPIReturn::ErrorFatal:                    return L"ErrorFatal - unspecified error AFTER WHICH EXECUTION CANNOT CONTINUE";
    case SPIReturn::ErrorWinApi:                   return L"ErrorWinApi - a call to Win API function failed, check GetLastError()";
    default:                                       return L"UNRECOGNIZED RETURN CODE - CONTACT DEVELOPERS";
    }
}

#pragma endregion


#pragma region The public-facing interface

#define SPIDECL [[nodiscard]] __declspec(noinline) virtual SPIReturn
#define SPIDEFN virtual SPIReturn

/// Game versions for GetHostGame.
/// Not interchangable with LEGameVersion!!!
enum class SPIGameVersion
{
    Launcher = 0,
    LE1 = 1,
    LE2 = 2,
    LE3 = 3
};

/// <summary>
/// SPI declaration for use in ASI mods.
/// </summary>
class ISharedProxyInterface
{
public:
    /// <summary>
    /// Get version of the SPI implementation provided by the host proxy.
    /// </summary>
    /// <param name="outVersionPtr">Output value for the version.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL GetVersion(unsigned long* outVersionPtr) = 0;
    /// <summary>
    /// Get if the host proxy was built in debug or release mode.
    /// </summary>
    /// <param name="outIsRelease">Output value for the build mode.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL GetBuildMode(bool* outIsRelease) = 0;
    /// <summary>
    /// Get the game this host proxy is attached to (1 - 3 or Launcher).
    /// </summary>
    /// <param name="outGameVersion">Output value for the game.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL GetHostGame(SPIGameVersion* outGameVersion) = 0;

    /// <summary>
    /// Search the main game module for a PEiD-style pattern.
    /// </summary>
    /// <param name="outOffsetPtr">Output value for the offset, set to NULL if not found.</param>
    /// <param name="combinedPattern">PEiD-style pattern specifying 100 bytes (299 chars + \0) at most.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL FindPattern(void** outOffsetPtr, char* combinedPattern) = 0;

    /// <summary>
    /// Use bink proxy's built-in injection library to detour a procedure.
    /// </summary>
    /// <param name="name">Name of the hook used for logging purposes.</param>
    /// <param name="target">Pointer to detour.</param>
    /// <param name="detour">Pointer to what to detour the target with.</param>
    /// <param name="original">Pointer to where to write out the original procedure.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL InstallHook(const char* name, void* target, void* detour, void** original) = 0;
    /// <summary>
    /// Remove a hook installed by <see cref="ISharedProxyInterface::InstallHook"/>.
    /// </summary>
    /// <param name="name">Name of the hook to remove.</param>
    /// <returns>An appropriate <see cref="SPIReturn"/> code.</returns>
    SPIDECL UninstallHook(const char* name) = 0;
};

#pragma endregion
