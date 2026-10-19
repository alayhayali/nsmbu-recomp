# Changelog

Add new notes under Unreleased.
At release, move those notes to a version heading and leave Unreleased empty.

## Unreleased

-   **Fix**: Leaving full screen no longer keeps a monitor-sized window.
    If the restored size fills the display, the TV window is centred at a normal windowed size.
-   **Change**: The installer progress screen uses a shorter overall bar.
    Details expands the log below the header.
-   **Change**: The installer's 1.3.0 update folder is optional and labeled as such.
    You can install from the base disc alone.
    Mods still need the update.
-   **Add**: Fatal and crash dialogs stay compact and offer Report crash.
    That opens a GitHub issue form with version, platform, hardware, and Logs filled in
    (error text and a log excerpt).
    It also opens the logs folder so you can attach the full file.
    What happens and Steps stay blank for you to fill in.
    Setup error screens also offer Report issue.
-   **Fix**: Setup and the game refuse an update-only dump (`0005000e…` / missing base courses)
    with a clear message instead of crashing on startup.
-   **Add**: The Europe game (title `00050000-10101e00`, title version 80 / **1.3.0**) works.
-   **Add**: The USA NSMBU + NSLU pack (title `00050000-1014b700`, title version 0) works.
-   **Add**: The Europe NSMBU + NSLU pack (title `00050000-1014b800`, title version 0) works.
-   **Add**: The USA base disc (title version 0) works without the 1.3.0 update.
    Mods are not supported on that build; the Mods tab says so.
    Prefer the 1.3.0 update (title version 64) for mods and the usual install path.
-   **Add**: The installer Keys screen lets you paste the disc key as 32 hex digits, like the common key.
-   **Add**: Mods can use a radio option (one button per choice) with its own label.
-   **Change**: Save state files carry neutral names: portable states are `slotN.nsstate` (was `slotN.wwstate`) and full states are written with a new file magic, so full states saved by older versions no longer load.
-   **Change**: The installer's save import looks for NSMBU save files (`rp_personal_savedata.dat` and `rp_savedata.dat`).
-   **Change**: The installer Keys screen fits the default window: denser disc summary and side-by-side key panels.
-   **Change**: Mod enable flags and settings live in one `mods.json` file.
    Older profile-based files still load; the next save writes the flat file.
-   **Remove**: The experimental language source: the installer option and the "From your European or Japanese game" settings entries are gone.
-   **Fix**: After a hitch, the game no longer sticks at 30 fps.
    Frame pacing follows the display refresh so courses can hold 60 fps.
-   **Fix**: World-map paths stay above the hill instead of sinking into the ground.
-   **Fix**: Closing the in-game error viewer no longer pauses the world map or leaves it slow.
-   **Fix**: Large course textures no longer re-upload every frame when only a small part changes.
-   **Fix**: The installer Keys and update screen shows the disc name, title id, size, and update checksum when those files are present.
-   **Fix**: The bug report copy names the real NSMBU save files.
-   **Fix**: Dropping a Cemu content or graphics pack no longer writes a `manifest.json` into that pack folder.
-   **Fix**: Number options in the Mods tab follow their step size.

## 0.4.2

-   **Fix**: The setup window follows the system dark or light theme.
-   **Fix**: Setup buttons and long labels no longer clip.
-   **Fix**: The install progress screen gives most of the space to the steps and keeps the Details log readable.
-   **Change**: Translating the game code shows live progress in the installer.

## 0.4.1

-   **Fix**: The first-run game build no longer fails while translating the game code.

## 0.4.0

-   **Add**: Drop-in Cemu content packs work without a player `manifest.json`.
-   **Add**: Display setting to keep running in the background when the window is not focused.
-   **Change**: Mod packages live under **Mods → Open mods folder** (`data/nsmbu/mods/` in a portable install).
    The old `ModManager/` tree is removed with no migration.
-   **Change**: The Mods tab is simpler: open the mods folder and copy packages in.
    It rescans about once per second while open when every package is disabled.
-   **Change**: The TV window size is remembered across launches.
-   **Change**: Bloom and ambient-occlusion mode controls are removed from the Graphics tab.
-   **Change**: Faster guest texture decode.

## 0.3.0

-   **Add**: Luau mods with optional game content and hot reload.
-   **Add**: Mods can list and spawn live level actors, and use high-level level and player helpers.
-   **Add**: Settings overlay Audio tab: mute, volume, and TV / GamePad / Auto output.
-   **Add**: Performance overlay lag breakdown for GPU, renderer CPU, vsync, waits, and idle.
-   **Change**: The settings overlay opens with `` ` `` or F12.
    F1–F5 load save-state slots.
-   **Change**: The Controls stick dead zone applies to gamepad input.
-   **Change**: Faster texture decode and faster save-state load and restore.
-   **Fix**: The port is USA 1.3.0 only.
-   **Fix**: Boot and draw-queue crashes are fixed, and the overworld path seam is fixed.
-   **Fix**: Save states load reliably after a rebuild.

## 0.2.0

-   **Fix**: The crash in World 1-1 is fixed.
-   **Fix**: Background flicker is fixed.
-   **Fix**: Starting at 2× resolution no longer corrupts textures.
-   **Fix**: Save states load correctly.
-   **Fix**: Audio playback is more reliable.
-   **Change**: Default keyboard maps run to Y and spin jump to ZR.
-   **Change**: Controls UI sizing is more consistent.
-   **Play**: World 1-1 through 1-3 are playable.
    Performance is low and frame rate often stays below 60 fps.

## 0.1

-   **Add**: The download is the NSMBU runtime and installer.
    It has no game files and no game code.
    The first start builds the game from your dump.
-   **Add**: The frame menu offers 60, 120, 165, and 240 fps.
    Game logic stays at 60 steps a second.
-   **Change**: The TV and the GamePad present together on one vsync.
