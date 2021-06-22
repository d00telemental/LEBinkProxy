#include "../src/spi/interface.h"

// This plugin works with any interface version (2 is the first one).
#define SPI_MINVER 2


// Declare that this plugin uses SPI.
// Required for the proxy to load it.
SPI_PLUGINSIDE_SUPPORT(L"ExamplePlugin", L"d00telemental", MELE_FLAG_1 | MELE_FLAG_2, SPI_MINVER);

// Declare that this plugin loads before DRM.
SPI_PLUGINSIDE_PRELOAD;


// Things to do once the plugin is loaded.
SPI_IMPLEMENT_ATTACH
{
    auto rc = InterfacePtr->OpenConsole(stdout, stderr);

    printf_s("Hello world!\n");
    fflush(stdout);

    return true;
}

// Things to do just before the plugin is unloaded.
// WARNING: do not rely on this getting called, just like with the ON_DETACH notification.
SPI_IMPLEMENT_DETACH
{
    return true;
}
