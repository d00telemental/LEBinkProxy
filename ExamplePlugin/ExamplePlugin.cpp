#include "../src/spi/interface.h"


// "Configuration" macros,
// in practice they should be set and forgotten.
#define PLUGIN_NAME  L"ExamplePlugin"
#define PLUGIN_AUTH  L"d00telemental"
#define GAME_INDEX   1  // this is a native plugin for LE1
#define SPI_MINVER   2  // any SPI version works (2 is the first one)


// Declare that this plugin uses SPI.
// Required for the entire logic to work,
// if the loader fails to find this it stops
// loading the entire plugin.
SPI_PLUGINSIDE_SUPPORT(PLUGIN_NAME, PLUGIN_AUTH, GAME_INDEX, SPI_MINVER);

// Declare that this plugin must
// be loaded only after the DRM wait.
SPI_PLUGINSIDE_POSTLOAD;


// Things to do once the plugin is loaded.
SPI_IMPLEMENT_ATTACH
{
    // Open a new console for logging.
    auto rc = InterfacePtr->OpenConsole(stdout, stderr);

    // Report the progress.
    fwprintf_s(stdout, L"Initialized SPI, opened a console.\n");
    fflush(stdout);

    return true;
}

// Things to do just before the plugin is unloaded.
// WARNING: do not rely on this being called, just like with the ON_DETACH notification.
SPI_IMPLEMENT_DETACH
{
    return true;
}

