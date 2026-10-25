---
name: nsmbu
description: "Playtests (`nsmbu test`), cases.ts helpers, status, scripted boots, and NSMBU_* env vars. Use when adding or running a playtest, reproducing a game bug as a case, reading nsmbu status, measuring a Release replay, running nsmbu scripted, or adding, renaming, or reading an NSMBU_* variable."
---

# nsmbu

TypeScript CLI under `tools/cli/`.
Run `nsmbu --help` for the command list.

## Docs

Read `docs/automation.md` for playtests, `nsmbu scripted`, and replay inputs.
Read `docs/cli.md` for install, build/launch, and the short live control commands.
Read `docs/env.md` when you add, rename, or read an `NSMBU_*` variable.

## Playtest

Repro and regression work is a case, not a live control chat.

1. Reuse or extend helpers in `tools/cli/src/playtest/cases.ts` (`reachMap`, `loadPack`, `leaveCourse`, `quickSaveHere`, and the other locals in that file).
2. Add or extend a `PlayCase` in the same file.
3. Run `nsmbu test [name]`.
   `nsmbu test --list` prints the names.
4. Done when that case exits 0 (or fails with an assertion you can fix).

Each case uses its own save under `.tmp/playtest/<name>/save`.
Scene moves and map reads that need guest Lua stay inside the case through `Session` helpers.

## Status

`nsmbu status` is the observation line while writing a case or checking a stuck run.

```text
frame=120 step=80 pid=1 scene=CourseSelect map=1 course=0 pack=1-1 entering=1-5 file=0 cp=-1 node=4 world=0 player=none
```

`scene` is `Boot`, `Flow`, `FileSelect`, `SaveSelect`, `Title`, `Menu`, `Map`, `CourseTask`, or `CourseSelect`.
`map=1` / `course=1` mean those tasks are alive.
`pack` and `entering` are course ids.
`world` / `node` / `cp` are map fields when `map=1`.
Fields that are not ready are left off the line.
`nsmbu screenshot [path]` when the question is what is on screen.

## One game

Only one game process is supported.
If `nsmbu open` refuses because a game is already running, run `nsmbu status` or `nsmbu close`.

## Live look

`open` / `press` / `stick` / `screenshot` / `wait` are for a short look or one pad pulse while writing the case.
They block until done, use `wait --scene NAME` or `wait --step N` instead of shell sleeps.
A lasting check belongs in the playtest step above.

## Inputs

A long pad timeline stays on `record` and `replay`.
Use `replay --release` for a measured course.
