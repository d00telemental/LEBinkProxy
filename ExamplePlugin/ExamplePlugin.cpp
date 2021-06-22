#include "../src/spi/interface.h"


// Declare that this plugin uses SPI - required for the proxy to load it.
// Flags mean:
//   - LE1 and LE2 are supported,
//   - OnAttach is called sequentially,
//   - the min SPI version is 2 (<=> any).
SPI_PLUGINSIDE_SUPPORT(L"ExamplePlugin", L"d00telemental", SPI_GAME_1 | SPI_GAME_2, SPI_ATTACH_SEQ, 2);

// Declare that this plugin loads before DRM.
SPI_PLUGINSIDE_PRELOAD;


// Things to do once the plugin is loaded.
SPI_IMPLEMENT_ATTACH
{
    auto _ = InterfacePtr->OpenConsole(stdout, stderr);

    printf_s("Hello world!\n");
    fflush(stdout);

    return true;
}

// Things to do just before the plugin is unloaded.
// Do not rely on this getting called, just like with the ON_DETACH notification.
SPI_IMPLEMENT_DETACH
{
    return true;
}
