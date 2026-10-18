# NSMBU port notes

## Upstream base

Use this commit as the last Wind Waker HD recomp commit merged into this tree.

Repository: https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp

Branch: `main`

Commit: `defb89f21607345e2e97b2c145a071a3f362640c`

Subject: `Recompiler: mulli, mullw and neg without signed overflow in the generated C (#82)`

Author date: Fri Oct 9 11:22:57 2026 +0200

The remote name in this clone is `upstream`.

The next update is one merge, not a rebase.

Follow `.agents/skills/upstream-merge/SKILL.md`.

## What you can reuse

The Wind Waker HD port and NSMBU are both Wii U titles.

`tools/recomp/recomp.py` turns one RPX into C.

`runtime/` implements Cafe OS calls and GX2 on Vulkan.

On Linux the build uses Vulkan.

The executable name in the runtime is `red-pro2.rpx`.

That file is `code/red-pro2.rpx` in the NSMBU filesystem.

The public headers in `references/headers` match NSMBU v1.3.0.

Each function comment in those headers has a guest address.
[names.md](names.md) maps those names and addresses to the names the port uses.

## Fork leftovers

These parts still need NSMBU-specific work.

- Language packs and many mods still follow upstream Cemu pack folder names.
- The build writes weak stubs for symbols the runtime still names from upstream.
  CMake runs `tools/recomp/guest_stubs.py` and compiles the output.

The upstream built-in mods and cheats are removed.

## Game files

User files live under `game/`:

- `game/game.wux` — disc image
- `game/game.key` — title key next to the image
- `game/common.key` — common key (or `WIIU_COMMON_KEY`)
- `game/update/` — USA 1.3.0 update (title version 64), with `code/`, `content/`, `meta/`
- `game/code/`, `game/content/`, `game/meta/` — output of `agent-nsmbu extract` (base disc plus update)

`.gitignore` ignores all of `game/` except `game/README.md` (layout and steps for this folder).

`agent-nsmbu extract` starts by checking `game/update/` is USA 1.3.0.
It then extracts `game/game.wux` into `game/code`, `game/content`, and `game/meta`.
It applies `game/update/` over those folders and checks the merged RPX matches the known USA 1.3.0 hash.

`agent-nsmbu build` configures `build/` in **Debug** and builds `nsmbu`.

If `game/code/red-pro2.rpx` exists, `agent-nsmbu build` runs `recomp.py` into `build/gen` when the RPX is newer than the generated code.

Otherwise it runs `stubgen.py` so the tree links without your RPX.

`agent-nsmbu build --release` uses **Release**.

`agent-nsmbu build --sanitizer` adds AddressSanitizer and UBSan.

Clang needs a GNU `libstdc++` (for example `libstdc++-14-dev` on Ubuntu).
`tools/build.py` adds the matching `-L` path when it finds `libstdc++.so` under `/usr/lib/gcc/`.

The `agent-nsmbu` CLI exports `NSMBU_USER_DIR` to the gitignored `nsmbu/` folder on dev commands.
`agent-nsmbu launch` starts `build/debug/nsmbu` with `--game` set to `game/` and `--save nsmbu/save`.
Default debug env vars (`NSMBU_PROFILE`, `NSMBU_VK_STATS`, `NSMBU_SYNC_STATS`, `NSMBU_CRASH_RECOVERY`) are set when they are not already in the environment.

`agent-nsmbu launch --no-debug` skips those debug defaults.
It still uses `nsmbu/`.

Portable installer releases keep their own `data/nsmbu` (settings and `save/`) via `portable.txt`.
The setup importer can also read `<repo>/nsmbu` from a source checkout.

`agent-nsmbu launch --trace` passes `--trace` for HLE logging.

`agent-nsmbu launch --gdb` and `agent-nsmbu launch --lldb` run the game under a debugger.

`agent-nsmbu run` runs `agent-nsmbu build` then `agent-nsmbu launch`.

`agent-nsmbu run --release` runs `agent-nsmbu build --release` then `agent-nsmbu launch --no-debug`.
This is the configuration the released installs use.

`agent-nsmbu launch` / `agent-nsmbu run` symlink each `modding/examples/` package into `nsmbu/mods/<id>/` before start.
Real folders already in mods are not replaced.
Enable packages in Mods.
Set `NSMBU_NO_EXAMPLE_MODS=1` to skip those links.

`agent-nsmbu run 3` loads save slot 3 after boot.
The number is the save slot.
Slots are 1 through 5.
The load starts at TV frame 200 and waits until the boot threads exist.

`agent-nsmbu run 1-1` does not load a save state.
It enables the `example.intro-skip` mod and sets its course pack option to `1-1`.
Any `N-M` token with that shape works the same way.
You cannot pass a slot and a course pack on one launch.

`agent-nsmbu profile` runs `build/debug/nsmbu` with the fixed-scene profile env (`NSMBU_NO_AUDIO`, profile and Vulkan stats, slow-swap log, log file under `.tmp/repro/`).
Default load is slot 2 at TV frame 700; the run stops at TV frame 1100.
Pass a slot number, or `--exit-at`, `--load-at`, or `--log`.
Use a Release binary (`agent-nsmbu build --release`) for hitch checks.

`agent-nsmbu recomp` runs `recomp.py` only.

Install the CLI with `cd cli && npm link`; see [cli.md](cli.md).

Put an extracted game you own in `game/`.

The layout is `game/code/`, `game/content/`, and `game/meta/`.

The headers match the USA v1.3.0 executable (title `0005000010101d00`, title version 64).

A disc image without that update has a different `red-pro2.rpx`.

Addresses in `references/headers` will not match that older file.

`game/` and `*.rpx` are gitignored.

## Recompile

Use Clang.

The generated C uses `musttail`.

```bash
python3 tools/recomp/recomp.py game/code/red-pro2.rpx build/gen
cmake -S . -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

The NSMBU executable does not call the guest stub symbols.

A generated game function with the same name replaces the stub.

The first run will miss Cafe OS imports the upstream title never called.

Add those imports in `runtime/src/hle/` after the recompiler report lists them.
