# Horizon Forbidden West Gameplay Tweaks & Cheat Menu

Source code for the Horizon Forbidden West mod.

## Installation

- For developers, edit `CMakeUserEnvVars.json` and set `GAME_ROOT_DIRECTORY` to Horizon's root directory. The build script will automatically copy library files to the game folder.

- For manual Steam installs, copy `winhttp.dll` and `mod_config.ini` folder to the game's root folder. An example path is: `C:\Program Files (x86)\Steam\steamapps\common\Horizon Forbidden West Complete Edition\`.

## Building

### Requirements

- This repository and all of its submodules cloned.
- **Visual Studio 2022** 17.9.6 or newer.
- **CMake** 3.26 or newer.
- **Vcpkg**.

### hfw-gameplay-tweaks (Option 1, Visual Studio UI)

1. Open `CMakeLists.txt` directly or open the root folder containing `CMakeLists.txt`.
2. Select one of the preset configurations from the dropdown, e.g. `Universal Release x64`.
3. Build and wait for compilation.
4. Build files are written to the bin folder. Done.

### hfw-gameplay-tweaks (Option 2, Powershell Script)

1. Open a Powershell command window.
2. Run `.\Make-Release.ps1` and wait for compilation.
3. Build files from each configuration are written to the bin folder and archived. Done.

## License

- No license provided. TBD.
- Dependencies are under their respective licenses.