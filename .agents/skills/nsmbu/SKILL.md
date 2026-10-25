---
name: nsmbu
description: "nsmbu CLI, status, scripted headless boots, playtests, and NSMBU_* env vars. Use when driving the game, reading nsmbu status, waiting for a scene, measuring a Release replay, running a dev recipe, nsmbu scripted, nsmbu test, scripted inputs or exit frames, or adding, renaming, or reading an NSMBU_* variable."
---

# nsmbu

TypeScript CLI under `tools/cli/`.
Run `nsmbu --help` for the command list.

## Docs

Read `docs/cli.md` for dev recipes and the agent control loop.
Read `docs/automation.md` for `nsmbu scripted`, `nsmbu test`, and replay inputs.
Read `docs/env.md` when you add, rename, or read an `NSMBU_*` variable.

## One game

Only one game process is supported.
If `nsmbu open` refuses because a game is already running, run `nsmbu status` or `nsmbu close`.

## Prefer playtests

Do not drive regressions by chatting with a live game.
Add or extend a case in `tools/cli/src/playtest/cases.ts` and run `nsmbu test [name]`.
`nsmbu test --list` prints the case names.
Each case gets its own save under `.tmp/playtest/<name>/save`.

Use live `open` / `status` / `press` / `stick` / `screenshot` / `wait` only for a short look at the screen or a one-off pad pulse while writing that case.

## Observe

Read `nsmbu status` before acting.
The line names the scene, whether the world map or a course is up, the course pack, and the player position.

```text
frame=120 step=80 pid=1 scene=CourseSelect map=1 course=0 pack=1-1 entering=1-5 file=0 cp=-1 node=4 world=0 player=none
```

`scene` is `Boot`, `Flow`, `FileSelect`, `SaveSelect`, `Title`, `Menu`, `Map`, `CourseTask`, or `CourseSelect`.
`map=1` means the world-map select task is alive.
`course=1` means a course task is alive.
`pack` is the current course as `world-number`.
`entering` is the course being loaded when it differs from `pack`.
`cp` is the map checkpoint index.
`world` and `node` are the map position when `map=1`.
`player=none` means there is no course player snapshot.
Fields that are not ready are left off the line.

Take a screenshot when the question is what is on screen.
`nsmbu screenshot [path]` blocks until the PNG exists and prints the path.

## Move

`nsmbu wait --scene NAME` polls status until `scene` matches.
`--timeout` is seconds.
A timeout prints the last status line.

`open`, `press`, `stick`, `screenshot`, `status`, and `wait` block until their work is done.
Use `wait --step N` or `wait --scene NAME` when the next action needs guest progress.

## Inputs

A long pad timeline stays on `record` and `replay`.
Use `replay --release` for a measured course.
