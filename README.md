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

## Changelog

<details>
  <summary>Click to expand.</summary><br/>
  
**Version 0.17**
  - Added a cheat to automatically set the player's faction to neutral.
  - Added a hotkey to pause the current time of day.
  - Added two new hardcoded teleport locations (The Glarebreak, Far Zenith Base).
  - Added the ability to set multipliers for individual player damage types through a separate configuration file.
  - Fixed a potential bug when UnlockUltraHardRestrictions was enabled.
  - Fixed DisableFocusMagnetism not working while riding Clawstriders.
  
**Version 0.16**
  - Added slider for graphical LOD bias under Miscellaneous for parity with the HZD:R version.
  
**Version 0.15**
  - Added support for game version 1.5.80.0.
  
**Version 0.14**
  - Added an option to the player inventory viewer to disable localized item names.
  - Fixed various focus camera types ignoring DisableFocusMagnetism.
  - Renamed the "Enable Infinite Ammo Reserves" UI option to "Enable Infinite Ammo (Clip)".
  - Adjusted various UI text and spacing.
  
**Version 0.13**
  - Added quality of life option to disable all camera magnetism, centering Aloy in front of the camera.
  - Added quality of life option to disable focus auto aim magnetism.
  - Added hotkey to toggle pausing AI processing.
  - Added asset override to disable atmospheric fog.
  
**Version 0.12**
  - Fixed UnlockUltraHardRestrictions so it now shows UH health bars when enabled.
  - Tweaked some asset override loading code. Very minor load screen optimizations.
  
**Version 0.11**
  - Added asset override to increase map tile level of detail (LOD).
  - Added asset override to adjust incoming/outgoing player damage multipliers.
  - Added two more teleport locations.
  - The InventoryStackMultiplier cheat now applies even if IncreaseInventoryStacks is false.
  
**Version 0.10.1**
  - Added a debug option to disable the "compiling shaders" loading screen hang when out of bounds.
  
**Version 0.10**
  - Added numerous teleport locations to the ingame menu. Special thanks to ashkenov.
  - Added more entity names to the entity spawner. Special thanks to PirateTV1.
  - Added more items to the item spawner.
  - Added a cheat option to force customization settings in ultra hard difficulty mode.
  - Eliminated death barriers near the edge of the map when UnlockMapBoundaries is enabled.
  
**Version 0.9.2**
  - Changed patching strategy for dialogue overrides. Should fix dialogue-quest soft locks.
  
**Version 0.9.1**
  - Added most spawnable game items to the inventory editor window.
  
**Version 0.9**
  - Added simple inventory editor under Cheats.
  - Added support for future character dialogue overrides.
  - Fixed random noclip position bug while in menus.
  
**Version 0.8.1**
  - Removed accidental inclusion of debug mod_log.txt file.
  
**Version 0.8**
  - Added a cheat to unrestrict AI versus AI damage. Machines can now damage each other beyond half health.
  - Added a cheat to unrestrict Ultra Hard menu options, allowing for aim assist and other customizations.
  - Added a cheat to unrestrict trap and tripwire limits.
  - Added a cheat to enable infinite clip ammo by default.
  - Added a cheat to enable infinite machine override timers.
  - Added a hotkey to toggle timescale overrides.
  
**Version 0.7**
  - Added time of day settings under the Gameplay menu.
  - Added Weather Setup overrides under the Cheats menu.
  - Added more entity spawner entries and substantially more names. Most useful spawns are labeled.
  - Added an asset override to disconnect mount aiming from camera aiming.
  - Removed the focus open behavior asset override as it's not needed. There's a comment explaining why.
  
**Version 0.6**
  - Added an asset override to disable all post-process colorization (yellow tint).
  - Added support for player character variant override files.
  - Added more names to the entity list again. Special thanks to PirateTV1.
  - Disabled Guerrilla's crash and telemetry logging.
  
**Version 0.5**
  - Added NPC faction dropdown to the entity spawner.
  - Added more names to the entity list. Special thanks to PirateTV1.
  - Added quick save/quick load key binds and menu options.
  - Added a cheat option to enable god mode by default.
  - Fixed god mode and demigod mode not applying consistently. They are now mutually exclusive.
  - Fixed UnlockMapBoundaries not correctly applying to flying mounts. Invisible walls are still present.
  
**Version 0.4**
  - Added full list to entity spawner. 2000 entries are missing due to technical limitations.
  - Added an asset override to prevent flying mounts from gradually slowing down.
  - Added an asset override to increase/set a custom flying mount speed.
  - Fixed infinite ammo cheat toggles so they're now mutually exclusive.
  - Fixed menu toggling for some non-US keyboard layouts.
  - Revised some wording in the configuration file.
  
**Version 0.3**
  - Added a cheat to reveal the world map menu (fog of war).
  - Added a cheat to unlock photomode restrictions in settlements.
  - Added a cheat option to adjust inventory stacks by a multiplier for balancing.
  - Added an asset override to use Horizon Zero Dawn's focus open behavior (press instead of hold).
  - Added an asset override to mute the focus pulse "ding" sound.
  - Added an asset override to change the time between weather transitions.
  - Fixed free crafting cheat so it now applies to merchants and workshops.
  - Fixed inventory stack cheat so it now applies to food and potions.
  - Fixed various bugs in the entity spawner menu.
  - Fixed hang when using the free camera during cutscenes.
  - Changed the mounted heavy weapon asset override as it doesn't seem to work properly. Only specific weapons apply.
  
**Version 0.2**
  - Added support for previous game versions (1.0.37.0, 1.0.38.0, 1.0.40.0).
  - Added preliminary entity spawner menu and hotkey. WARNING: buggy.
  - Added an asset override to allow mounting horses with all weapons, including heavy pickups.
  
**Version 0.1**
  - Initial release.
</details>

## License

- No license provided. TBD.
- Dependencies are under their respective licenses.
