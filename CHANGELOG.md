# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Player-facing notes only; within each section, order by importance to someone playing the game.
Use the `changelog` skill when editing.

## [Unreleased]

## [0.6.3] - 2026-10-25

### Fixed

- Entering another course from the world map at a high frame rate no longer crashes.
- A Quick Save is still there after you quit and start the game again.
  The file shows that course progress, and opening the file keeps it.
- Save states can be taken on the world map and inside a course.

## [0.6.2] - 2026-10-24

### Fixed

- The screen no longer freezes while the music keeps playing if a frame's pictures are different sizes.

## [0.6.1] - 2026-10-24

### Removed

- Opening a course directly no longer rewrites the loading card's world name or green level number.

### Fixed

- The world map accepts input after the opening cinematic.
- The world map shows the course number after the opening cinematic.
- Loading another course while you are already playing one no longer crashes.
- Quitting to the world map from a course you opened no longer crashes.

## [0.6.0] - 2026-10-24

### Added

- The Europe base disc runs without the 1.3.0 update.
  Mods still need that update.
- Up to four controllers work as separate players, with per-player bindings and an option to keep one layout for every player.
- Setup can precompile shaders before you play, with a progress bar.
  You can skip it, and shaders still compile during play which may cause stutters.

### Changed

- Higher frame rates keep normal game speed and draw in-between frames, so characters move between the game's 60 steps.
  144 fps is available, and these options are experimental.
- The renderer uses less CPU each frame, so busy scenes and high frame rate settings hold their speed better.
- Save states are small again.
  Turn on Full save states in the Saves tab for an exact copy of the running game.
  A full state already in a slot still loads when it is the newest file there.
- The settings overlay is wider, and its title bar matches the panel with no separator line so it no longer looks like a movable window.

### Removed

- The frame rate is removed from the window title.

### Fixed

- Quitting to the world map from a course you started in puts Mario on that course's node.
- Frames stay evenly spaced while the game is waiting for the display.
- The Keys screen takes the disc key and the Wii U common key typed or pasted as 32 hex digits.
  A key file saved by Notepad or a Cemu `keys.txt` works too.

## [0.5.0] - 2026-10-19

### Added

- Europe NSMBU 1.3.0 works.
- The USA and Europe NSMBU + NSLU packs work.
- The USA base disc runs without the 1.3.0 update.
  Mods still need that update.
- Crash and setup error dialogs stay compact and offer Report crash / Report issue.
- Cemu content mods are supported drop them into the mods folder and enable them in F12 menu then reboot.

### Changed

- Report crash and Report issue leave Logs empty for you to attach, fill Platform from this machine, and put the error dialog text in From the app.
- You can install from the base disc alone.
  The 1.3.0 update folder is optional.
- Setup refuses update-only dumps with a clear message.
- The Keys screen accepts a pasted disc key.
- Save states use `.nsstate` names and a new full-state format.
  Older full states no longer load.
- Mod settings live in one `mods.json`.
- Mods can use radio options.
- Number options in the Mods tab honour their step.
- Installer progress layout is clearer.
- The Keys screen layout is clearer.
- Save import looks for the real NSMBU save files.
- Bug-report copy names the real NSMBU save files.

### Fixed

- Large courses run smoother.
- World-map paths stay above the hill.
- Report crash closes the error dialog after opening GitHub and the logs folder.
- Closing the error viewer no longer pauses or slows the map.

## [0.4.2] - 2026-10-14

### Changed

- Install progress gives space to the steps.
  Details stays readable.
- Translating the game code shows live progress.

### Fixed

- The setup window follows the system theme.
- Setup buttons and long labels no longer clip.

## [0.4.1] - 2026-10-14

### Fixed

- The first-run game build no longer fails while translating the game code.

## [0.4.0] - 2026-10-14

### Added

- Drop-in Cemu content packs work without a player `manifest.json`.
- Keep running in the background when the window is not focused.

### Changed

- Mod packages live under **Mods → Open mods folder**.
  The Mods tab opens that folder and rescans while every package is disabled.
- The TV window size is remembered across launches.
- Guest texture decode is faster.

### Removed

- The old `ModManager/` tree is removed with no migration.
- Bloom and ambient-occlusion controls are removed from Graphics.

## [0.3.0] - 2026-10-13

### Added

- Luau mods with optional game content and hot reload.
- Mods can list and spawn live level actors, and use high-level level and player helpers.
- Settings Audio tab: mute, volume, and TV / GamePad / Auto output.
- Performance overlay lag breakdown for GPU, renderer CPU, vsync, waits, and idle.

### Changed

- The settings overlay opens with `` ` `` or F12.
- F1–F5 load save-state slots.
- The Controls stick dead zone applies to gamepad input.
- Faster texture decode.
- Faster save-state load and restore.
- The port is USA 1.3.0 only.

### Fixed

- Boot and draw-queue crashes are fixed.
- The overworld path seam is fixed.
- Save states load reliably after a rebuild.

## [0.2.0] - 2026-10-10

### Added

- World 1-1 through 1-3 are playable.
  Performance is low and frame rate often stays below 60 fps.

### Changed

- Default keyboard maps run to Y and spin jump to ZR.
- Controls UI sizing is more consistent.

### Fixed

- World 1-1 no longer crashes.
- Background flicker is fixed.
- Starting at 2× resolution no longer corrupts textures.
- Save states load correctly.
- Audio playback is more reliable.

## [0.1] - 2026-10-09

### Added

- The download is the runtime and installer only.
  The first start builds the game from your dump.
- The frame menu offers 60–240 fps.
  Game logic stays at 60 steps a second.

### Changed

- The TV and GamePad present together on one vsync.

[Unreleased]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.6.3...HEAD
[0.6.3]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.6.2...v0.6.3
[0.6.2]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.6.1...v0.6.2
[0.6.1]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.6.0...v0.6.1
[0.6.0]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.5.0...v0.6.0
[0.5.0]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.4.2...v0.5.0
[0.4.2]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.4.1...v0.4.2
[0.4.1]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.1...v0.2.0
[0.1]: https://github.com/nsmbu-recomp/nsmbu-recomp/releases/tag/v0.1
