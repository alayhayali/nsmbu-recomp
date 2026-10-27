# Build, launch, test, userdata

Install the CLI once: `cd tools/cli && npm link` (Node 24+).
Playtests and dump switches are in the `nsmbu` skill.
Command list and install notes are in [cli.md](cli.md).

## Compile

Generated C uses `musttail`, so the build uses Clang.

On Linux, `tools/build/build.py` adds the `libstdc++` path when it finds `libstdc++.so` under `/usr/lib/gcc/`.

`nsmbu build` runs the recompiler when `game/code/red-pro2.rpx` is newer than `build/gen`.

When that RPX is absent, the build fills `build/gen` with `tools/recomp/stubgen.py` so the tree still links.

An RPX from a disc that lacks the 1.3.0 update has different addresses.

Those addresses miss `references/headers` and `tools/recomp/headers_names.txt`.

A warm `nsmbu build` still runs `cmake -S` and regenerates guest stubs.
Configure only when the cache or compiler flags are missing.
Regenerate guest stubs only when `hooks.txt`, `hooks_*.txt`, or the recomp inputs changed.
The `0 hooks` line from stub generation is normal.
Hooks live in `build/gen`.

## Launch

`nsmbu launch` sets `NSMBU_PROFILE`, `NSMBU_VK_STATS`, `NSMBU_SYNC_STATS`, and `NSMBU_CRASH_RECOVERY` when those variables are unset.

`nsmbu launch --no-debug` and `nsmbu run --release` leave those variables unset.

`nsmbu launch` and `nsmbu run` symlink each package under `modding/examples/` into `nsmbu/mods/<id>/`.

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

`.git-hooks/pre-push` runs a full local `ctest` before a push.

Profiling follows [perf.md](perf.md).

## Local userdata

The `nsmbu` CLI points `NSMBU_USER_DIR` at the gitignored `nsmbu/` folder on dev commands.

A portable release uses the same layout under `data/nsmbu/` when `portable.txt` sits next to the binary.

The run log is `nsmbu/logs/nsmbu.log`.

`nsmbu/mods.json` stores the enabled mods and their options.
