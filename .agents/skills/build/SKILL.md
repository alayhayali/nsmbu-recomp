---
name: build
description: "Build, compile, just build, just launch, launch-release, ctest, host tests, stubgen, missing RPX, non-1.3.0 RPX wrong addresses, example mods symlinks, NSMBU_NO_EXAMPLE_MODS, nsmbu userdata, portable layout, pre-push, CI test_setup and test_builds. Use when building, launching, testing, or debugging link/recomp/stub or local mods/userdata paths."
---

# Build, launch, test, userdata

## Compile

Generated C uses `musttail`, so the build uses Clang.

On Linux, `tools/build.py` adds the `libstdc++` path when it finds `libstdc++.so` under `/usr/lib/gcc/`.

`just build` runs the recompiler when `game/code/red-pro2.rpx` is newer than `build/gen`.

When that RPX is absent, the build fills `build/gen` with `tools/recomp/stubgen.py` so the tree still links.

An RPX from a disc that lacks the 1.3.0 update has different addresses.

Those addresses miss `references/headers` and `tools/recomp/headers_names.txt`.

## Launch

`just launch` sets `NSMBU_PROFILE`, `NSMBU_VK_STATS`, `NSMBU_SYNC_STATS`, and `NSMBU_CRASH_RECOVERY` when those variables are unset.

`just launch-release` leaves those variables unset.

`just launch` and `just run` symlink each package under `modding/examples/` into `nsmbu/mods/<id>/`.

Existing real folders in that mods directory stay in place.

Set `NSMBU_NO_EXAMPLE_MODS=1` to skip those links.

## Tests

Host tests are CTest targets in `CMakeLists.txt`.

```bash
ctest --test-dir build --output-on-failure
```

Linux CI also runs these scripts after that `ctest`.

```bash
python3 tools/installer/test_setup.py
python3 tools/recomp/test_builds.py
```

`tools/git/pre-push` runs a full local `ctest` before a push.

Profiling follows `.agents/skills/perf/SKILL.md`.

## Local userdata

`just` points `NSMBU_USER_DIR` at the gitignored `nsmbu/` folder.

A portable release uses the same layout under `data/nsmbu/` when `portable.txt` sits next to the binary.

The run log is `nsmbu/logs/nsmbu.log`.

`nsmbu/mods.json` stores the enabled mods and their options.
