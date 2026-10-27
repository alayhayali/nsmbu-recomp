# NSMBU native port

## Rules

Assume that the Typescript to Lua watcher is already running.
Never comment on github issues.
Add each commit message/change to the changelog it will be reformated later for release.

## Verify

The `nsmbu` cli is on `PATH`.
Game regressions go in `tools/cli/src/playtest/cases/` (shared helpers in `helpers.ts`) and run with `nsmbu test`.
Write tests for each feature implemented so we can catch regressions.
You might need to improve the test harness in some cases.
`nsmbu game usa-1.3.0 --recomp` is slow.
Reuse existing `build/gen` unless `tools/recomp/recomp.py` changed or you need a different game version.
Hook list edits (`hooks.txt`) also need `--recomp`, then `nsmbu build`.
A runtime edit is `nsmbu build` only.
That command reconfigures CMake and regenerates guest stubs on every run, so do not configure again beside it.
`tools/build/build.py` should configure only when the cache or compiler flags are missing, and regenerate guest stubs only when `hooks.txt`, `hooks_*.txt`, or the recomp inputs changed.
Hidden boots launch `build/debug/nsmbu --game game/usa-1.3.0` (the dump folder that contains the rpx).
`nsmbu scripted` and `tools/run/scripted-run.sh` pass `game/` and exit before the first frame.
Title and file-select frame checks use `docs/shortcuts.md`.

## Code style

Match the file you are editing.
Put the opening brace on the same line as the declaration.
Indent with 4 spaces.
The `nsmbu` target compiles with `-Wall`.

## Decomp

Any discoveries you find about the source code document it in the `vendor/decomp/docs` repo.
