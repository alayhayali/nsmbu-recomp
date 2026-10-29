# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Append each change under Unreleased as it lands; at release the `changelog` skill cleans that block into player-facing version notes.

## [Unreleased]

### Added

- Press **R3** (right stick click) on a controller to open or close the settings overlay, like **Home** and hold **Select** (−).
- A 30 fps choice in Graphics.
  It draws half as often and runs two logic steps per picture, so the game keeps its speed.
- Graphics can set any frame rate from 60 to 1000.
  30 fps is only the preset.
  Fast forward runs the game as fast as the computer can, with one game step per picture and no frame limit.
- `nsmbu installer --no-open` packages the installer and returns without opening setup.
- Scene checks for agents live in the `scenes` skill.
  The world map, a course pack, and the file menu each have a short boot that stops on `nsmbu status`.

### Fixed

- Linux release packaging keeps the SDL3 license next to the cached SDL3 install so a cache hit no longer fails the Package step.

### Changed

- `nsmbu build` and `nsmbu run` skip CMake configure when the build cache is already up to date, and use a quiet log level when configure does run.
- Graphics frame rate: Custom is a preset radio on its own row with a number field beside it.
  Preset rates stay selectable while Custom is active.
- The performance overlay averages fps and logic steps/s over the same 120 frames as the frame-time graph, not since you opened the overlay or reset.
- Performance overlay controls moved from Graphics to the top of Debug.
- Graphics card choice moved from Graphics to the bottom of Debug.
- The performance overlay shows GPU name, driver, and Vulkan version on separate lines.
- The performance overlay background is slightly less transparent.
- High frame rates use a present cap with a wall-clock accumulator: logic stays at 60 steps/s, and blend time comes from the open step instead of a per-rate frame ladder.
- Guest-code notes live in the docs.
  The guest skill is gone.
- The GamePad screen starts off.
  Off is the first choice in Settings.
- Internal resolution adds 360p and 540p for slower machines.
  The choices read 360p, 540p, 720p, 1080p, 1440p, and 2160p/4K.
  720p is the Wii U picture.
  Lower choices draw fewer pixels and are scaled up to the window.
  R cycles the list.
- Translating the game to C uses every CPU core, so install and a full recompile finish sooner.
- The TV window title shows the NSMBU release version and the installed game build (for example `v0.7.2 - USA v64`, or `0.7.2 git - EU v0` on a local build).
- A later release tag reuses the Linux compiler cache, so that build compiles less.
- Each ctest runs with its own `NSMBU_USER_DIR` under `test-users/`, so tests no longer write the `nsmbu/` folder.
  A new `user_folder_isolation` test checks that a saved `controls.json` lands there and not in the home folder.
- Each `nsmbu test` case runs with its own user folder (`.tmp/playtest/<name>/user`) for settings, controls, and the shader cache.
  Cases that use the repo save also get a copy of `settings.ini` and `controls.json`.
- `tools/run/scripted-run.sh` now uses its own user folder under the run folder (`.tmp/<name>/user`) unless `NSMBU_USER_DIR` is set.
- Recompiling an unchanged game no longer rewrites every generated C file, so a normal rebuild skips guest code that did not change.
- Hooked game functions and instruction-level hook sites keep plain register form in the recompiled C (the runtime hook can read and write the Cpu mid-function).
- `tools/gametest/run_scenario.sh` and `compare.py` compare two scripted runs.
  The runtime writes a status line per logic step (scene, course, pack, player position) with `NSMBU_TEST_STATUS_TRACE`, and the compare tool aligns the runs on a named scene and prints the first differing step.
  `NSMBU_TEST_ALIGN_SCENE` names the scene, and `NSMBU_TEST_FIXED_SEED=1` counts scripted presses, sticks, keys, and frame dumps from that scene.
  A boot that misses the scene is retried and its log is kept.
- Linux and Windows CI run the benchmark tooling tests, new `tools/test_shaderprep.py` shader preparation tests, and the compare tool tests.
  CI runs on every branch push only (no separate `pull_request` run); tag pushes do not start it.
- Windows on ARM64 release: `NSMBU-<version>-windows-arm64.zip`, built and tested on a `windows-11-arm` runner with llvm-mingw `aarch64-w64-mingw32` and the embeddable `python.windows-arm64`.
  `guard.py` checks its Python files against `tools/release/python-windows-arm64-files.json`.
- Rolling development build: a nightly check of `main` publishes the `devel-build` pre-release (`<last release>-devel.<sha>`, never marked latest) when `main` moved since the last one.
  Its notes are the `## [Unreleased]` block (`notes.py --devel`).
- Windows setup stops before any download when the install folder is too deep for the game (more than 179 characters), and unpacks the compiler with long-path (`\\?\`) paths.
  `windows.yml` runs `test_setup.py LongPaths` on MSYS2 Python and python.org Python 3.14, and `release.yml` repeats the check in a deep release folder and in one character past the limit.
- The Windows executables declare `longPathAware`.
- The Linux AppImage pins the type2 runtime by SHA-256 (`appimage.py` `RUNTIME`, passed to appimagetool with `--runtime-file`) and ships its license texts, which `guard.py` checks.
  `appimage.py --extract-runtime` recovers the runtime from a built AppImage, and the release workflow checks that the AppImage reports the pinned runtime commit.
- `nsmbu installer --no-open` packages the installer and returns without opening setup.
- Decompiled `SndAudioMgr::startSound` / `startSystemSe`, `CourseSelectLayerMgr::createLayers`, and `ReflectionRenderObjLayer::render_` in `vendor/decomp/src`.
  The frame-loop, map-shadow, and soft-restart notes answer their open claims: the draw-context lock and unlock, the task-manager `afterCalc`, the eleven-node `DualScreenMethodTreeMgr` layout, the TV `"DepthShadow"` creation sites, and the `0x1017C814` `SndAudioMgr` method table.

### Fixed

- Windows on ARM64 crash logs list the faulting instruction first in the host backtrace (same CONTEXT unwind as x64), so release CI `crash_log_module` passes on `windows-arm64`.
- Settings text fields (including the custom frame rate) accept keyboard input while the overlay is open.
- 30 fps runs a full game step for each picture update, including the wait that finishes the previous picture.
  Clearing the render list twice without that wait crashed the render thread.
- 60 fps stays at 60 when a higher rate was used earlier.
  The performance overlay no longer reports that higher rate.
- Vulkan prefers a dedicated GPU when one can draw the game windows.
  Graphics (` or F12) can prefer dedicated, prefer integrated, or one listed card.
  The choice applies on the next start.
  `NSMBU_VK_GPU` overrides it for that start.
- The TV window title uses ASCII separators so punctuation no longer shows as garbled text on some systems.
- Metal: a new render target starts with the texels already in guest memory.
  A course's static tiles drawn from its working tileset, such as the used block after hitting a ? block, were invisible.

### Fixed

- A 165 Hz present cap stays on 165.
  The present wait was keeping the time it actually woke, so each short overrun started the next frame late and the overlay sat near 162.
- A late world-map frame no longer starts a second present slot while it waits for the previous flip.
  That extra wait held the overworld near 108 fps.
- Frame time stays on the present cap when a light frame would otherwise show early and the next full draw shows late.
  The present wait keeps running queued GPU loads, so the boot screen can finish.
- Install and recomp tools find generated code and hook lists when the install path contains square brackets.
- High present caps keep their cadence when a frame runs late, so the world map no longer drifts well below the selected rate.
- Logic presents at high frame rates draw the finished step (blend t = 1) instead of the open-step leftover, so Mario no longer jitters while running at 120/165.
- World-map presents no longer defer or idle-repeat against the present-cap wake schedule, so the overworld can hold the same host frame rate as courses.
- A hitch can run up to four guest logic steps on one present, and the leftover time stays in the accumulator so the game slows under load.
- Hold frames blend the course camera across the open logic step, and logic frames still draw the finished step.

## [0.7.2] - 2026-10-27

### Fixed

- On 21:9, enemies, blocks, bushes, and other course scenery no longer pop in at the old 16:9 edge.
  The title parade stays centered.

## [0.7.1] - 2026-10-27

### Changed

- Aspect ratio is under Settings → Display and is marked experimental.
- Ultrawide layouts keep edge HUD on the screen with the same side margin as 16:9.
  Centered menus and title UI stay in a 16:9 frame while the world view expands.

### Fixed

- The aspect ratio chosen in Settings is restored on the next launch.
  A set `NSMBU_ASPECT` value still overrides the saved choice.
- Setup lets you choose **Copy saves and settings from another NSMBU folder** when no earlier install is detected.
- Install and setup work when the install path contains square brackets.
- On 21:9, world-map Menu, Map View, and Items labels stay on their buttons instead of clipping at the screen edge.
- On 21:9, title characters stay centered under the logo.
- On 21:9, fades fill the wide picture.
- On 21:9, file-select full-width art fills the frame; buttons and cards stay 16:9.
- On 21:9, title-screen scenery at the edges no longer pops in late.
- On 21:9, the title screen no longer shows a one-pixel bright column on the far right.
- On 21:9, the world map no longer shows dark bars just outside the old 16:9 frame.
- Ultrawide no longer stretches the GamePad picture; only the TV view grows.

### Added

- Settings → Saves shows a short preview of the real game save (files, world, lives, play time) and opens that saves folder from the top of the tab.
- The performance overlay shows average fps and logic steps/s, plus GPU and CPU timing.
  Reset clears those averages; Copy performance report includes them.

### Changed

- Wider and taller windows show more of the scene instead of stretching a 16:9 picture.
  The HUD stays in its original frame.
- Settings language and content-mod Restart relaunches the app, the same way as switching Metal and Vulkan.
- Setup precompiles shaders as part of install, repair, and update after the game is built.
  Menu and Ready to play keep a Recompile shaders button to run that pass again.
- Settings → Language lists only the text languages on the installed disc.
  A USA dump shows English, French, and Spanish; a European dump adds the other European languages.
- Settings tabs: Controls is first; Saves sits between Audio and Mods; Debug is capitalized.
- The window title no longer shows a live frame rate.
  Use the performance overlay for fps.
- The performance overlay no longer shows the renderer, scale, and frame-mode footer under the graph.

### Fixed

- World-map shadows no longer flicker at 120/144 fps frame interpolation.
- 144 and 165 fps keep the rate you select when a frame runs early or late.
  Late frames no longer restart the schedule and settle well below the selection.
  Early world-map frames show on the next time slot instead of being dropped.
- High frame rates keep normal game speed (60 steps a second).
  Dropping every in-between frame no longer leaves the game running about twice as fast.
- The Settings restart confirmation stays centered on screen.
- The performance overlay shows the frame rate once, above the frame-time graph.

## [0.6.3] - 2026-10-25

### Fixed

- Entering another course from the world map at a high frame rate no longer crashes.
- A Quick Save is still there after you quit and start the game again.
  The file shows that course progress, and opening the file keeps it.
- Save states can be taken on the world map and inside a course.

### Changed

- A portable save state that does not match your game save is refused instead of loading.
  Use a state file from the same save slot on this machine.

### Removed

- The published game no longer accepts the live control link that `nsmbu` uses to press buttons and read status while you play.
  Builds you compile yourself still support it.

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

[Unreleased]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.7.2...HEAD
[0.7.2]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.7.1...v0.7.2
[0.7.1]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.7.0...v0.7.1
[0.7.0]: https://github.com/nsmbu-recomp/nsmbu-recomp/compare/v0.6.3...v0.7.0
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
