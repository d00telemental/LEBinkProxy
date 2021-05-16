# LEBinkProxy

A small proxy DLL which enables dev. console in Mass Effect 1 (Legendary Edition).

## Usage

   1. In your game binary directory (`Game\ME1\Binaries\Win64`), rename `bink2w64.dll` into `bink2w64_original.dll`.
   2. Copy the built proxy DLL (`Release\bink2w64.dll`) into the same folder.
   3. Launch the game.
   4. Use `~` and `Tab` to open small and big console viewports. If your console keybindings are different, use them.

If you build the proxy from source, the proxy DLL can be found in the default Visual Studio build path.
If you download a built bundle, the proxy DLL can be found at `Release\bink2w64.dll`.

Should the file proxy not work, check the log file at `Game\ME1\Binaries\Win64\bink2w64_proxy.log`.

![Example of the proxy at work](https://cdn.discordapp.com/attachments/842859242032988190/843204183917854750/unknown.png)
