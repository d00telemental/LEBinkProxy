#include "../src/spi/interface.h"
#include "Common.h"


// Declare SPI dependency - required for the proxy to run the attach and detach points.
// Flags mean:
//   - LE1 and LE2 are supported,
//   - the min SPI version is 2 (<=> any).
SPI_PLUGINSIDE_SUPPORT(L"ExamplePlugin v0.1.0", L"d00telemental", SPI_GAME_LE1 | SPI_GAME_LE2, SPI_VERSION_ANY);

// Declare that this plugin loads before DRM.
SPI_PLUGINSIDE_PRELOAD;

// Declare that this plugin's attach point should run in a new thread.
SPI_PLUGINSIDE_ASYNCATTACH;


// Things to do once the plugin is loaded.
SPI_IMPLEMENT_ATTACH
{
    Common::OpenConsole();
    printf_s("ExamplePlugin::OnAttach - Hello world!\n");

    // Return false to report an error - but only if the attach mode is sequential.
    // Async attach will discard this value.
    return true;
}

// Things to do just before the plugin is unloaded.
// Do not rely on this getting called, just like with the ON_DETACH notification.
// Keep the code here short & sweet, as it is always executed sequentially.
SPI_IMPLEMENT_DETACH
{
    printf_s("ExamplePlugin::OnDetach - bye!\n");
    Common::CloseConsole();

    return true;
}
