# LEBinkProxy

## Deprecation notice

Due to various reasons, the version of this project currently used across the Mass Effect Trilogy modding scene has been maintained by **ME3Tweaks** at their [fork](https://github.com/ME3Tweaks/LEBinkProxy) for some time.
As of 08 Apr 2024, that fork is unlinked, all further development goes there, and this repository is in archival mode, if only for preservation reasons.

-----

![Mass Effect Modding Discord community](https://i.imgur.com/eDRpjZS.jpg)

A small proxy DLL which enables dev. console in Mass Effect 1, 2 and 3 (Legendary Edition).

## Usage

   1. In your game binary directory (`Game\ME?\Binaries\Win64`), rename `bink2w64.dll` into `bink2w64_original.dll`.
   2. Copy the built proxy DLL (`Release\bink2w64.dll`) into the same folder.
   3. Launch the game.
   4. Use `~` and `Tab` to open small and big console viewports. If your console keybindings are different, use them.

If you built the proxy from source, the proxy DLL would be found in the default Visual Studio build path.
If you downloaded a built bundle, the proxy DLL would be found at `Release\bink2w64.dll`.

Should this proxy not work, make sure you are using the latest version and then check the log file at `Game\ME?\Binaries\Win64\bink2w64_proxy.log`. **You can get support for LEBinkProxy at [Mass Effect Modding Discord](https://discord.gg/su8XjdUQPw).**

## For developers

In the original Mass Effect trilogy, certain mods required a DLL bypass to work. This led many developers to distribute a Bink proxy with their mods. Even after almost the entire scene moved to use [ME3Tweaks Mod Manager](https://github.com/ME3Tweaks/ME3TweaksModManager) (M3), which has a built-in Bink proxy installer (currently only for Original Trilogy), some developers continue(d) to ship their own DLLs, which resulted in many different versions of the tool being spread all over the Internet.

At the time this page was last updated, this proxy was not required for any content or graphic mod to work. **Thus, shipping it with your mod is neither necessary nor of any value to the user**. Until M3 supports installing this proxy out of the box, the preferred way of getting it is from the NexusMods page Downloads section, or from the GitHub Releases page.

## Screenshot (LE1)

![Example of the proxy at work](https://i.imgur.com/MRgzZzg.png)

## Credits

To inject necessary functionality into the game, LEBinkProxy uses slightly modified [MinHook](https://github.com/TsudaKageyu/minhook) library by Tsuda Kageyu. The license and a statement of changes for it can be found in the proxy's source files, which are distributed to the end user.
