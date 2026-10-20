---
name: nsmbu
description: "nsmbu CLI, scripted headless boots, and NSMBU_* env vars. Use when driving the game, measuring a Release replay, running a dev recipe, nsmbu scripted, scripted inputs or exit frames, or adding, renaming, or reading an NSMBU_* variable."
---

# nsmbu

TypeScript CLI under `tools/cli/`.
Run `nsmbu --help` for the command list.

## Docs

Read `docs/cli.md` for the global `nsmbu` command, dev recipes, and agent control loop.
Read `docs/automation.md` for `nsmbu scripted`, headless boots, and scripted inputs or exit frames.
Read `docs/env.md` when you add, rename, or read an `NSMBU_*` variable (`runtime/src/core/env.h`: `env::NAME()`, `.name()`, typed reads, `env::unset`).

## Install once

Node 24 or newer.

```bash
cd tools/cli && npm link
```

After that, every command is the global `nsmbu` on `PATH`.

Fish completions: `nsmbu completion fish --install` (then `source ~/.config/fish/completions/nsmbu.fish` in open shells).

## One game

Only one game process is supported.
If `nsmbu open` refuses because a game is already running, run `nsmbu status` or `nsmbu close`.

## Which command

| Goal | Command |
| --- | --- |
| Compile or start a checkout | `build`, `launch`, `run`, `wine` |
| Test the Report crash dialog | `crash` |
| Full course input log | `record <course> [name]` / `replay <name>` |
| Measured Release course | `replay --release <course>` |
| Uncapped course, stop at a frame | `replay --release --uncapped --exit-at N <course>` |
| Scripted boot and frame PNGs | `scripted <name>` |
| Convert save data to JSON or back | `save <save.dat\|save.json>` |
| Look and tap | `open` -> `status` / `screenshot` -> `press` / `stick` -> `wait` -> `close` |

`open`, `press`, `stick`, `screenshot`, and `wait` block until their work is done.
Do not sleep between control commands; use `wait --step N` when the next action needs guest progress.
| Open a course file | `open game/content/Common/course_res_pack/1-27.szs` |

The loading card for `open 1-5` is in `docs/nsmbu/course-open.md`.

`screenshot [path]` blocks until the PNG exists and prints the path.
A long pad timeline stays on `record` and `replay`.

## Replay measure

`replay` starts `build/debug/nsmbu`.
`replay --release` starts `build/release/nsmbu`.
Both set `NSMBU_PROFILE`, `NSMBU_VK_STATS`, `NSMBU_SYNC_STATS`, and `NSMBU_CRASH_RECOVERY` when those variables are unset.
The command prints `[replay] log <path>` before the game starts.
The default path is `nsmbu/logs/nsmbu.log`.
`--log PATH` writes that file instead.
`--uncapped` sets `NSMBU_UNCAPPED=1`.
`--no-exit-on-input` sets `NSMBU_REPLAY_EXIT=none` so the game keeps running on live input after the recording ends.
`--exit-at N` sets `NSMBU_EXIT_AT_FRAME`.
`launch --no-debug` leaves the profile variables unset.
Use `replay --release` for a measured course.
