#include "../src/spi/interface.h"
#include "Macros.h"


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


// Mod-specific SPI pointer.
// Internally used to uniquely identify the mod.
ISharedProxyInterface* GISPIPtr;


// Things to do once the plugin is loaded.
SPI_IMPLEMENT_ATTACH
{
    GISPIPtr = InterfacePtr;  // passed to the library by the loader
    SPIReturn rc;

    // Open a new console for logging, or attach to an existing one.
    rc = GISPIPtr->OpenSharedConsole(stdout, stderr);

    // Report the progress.
    fwprintf_s(stdout, L"Initialized SPI, opened a console.\n");

    return true;
}

// Things to do just before the plugin is unloaded.
// WARNING: do not rely on this being called, just like with the ON_DETACH notification.
SPI_IMPLEMENT_DETACH
{
    // This may also return an error code, but in reality
    // we have neither need nor time to gracefully handle that.
    ISharedProxyInterface::Release(&GISPIPtr);

    return true;
}
