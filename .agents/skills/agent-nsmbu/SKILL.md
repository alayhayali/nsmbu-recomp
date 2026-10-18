---
name: agent-nsmbu
description: "NSMBU developer CLI for build, launch, course replay, screenshots, and short pad pulses. Use when driving the game, measuring a Release replay, or running a dev recipe."
---

# agent-nsmbu

TypeScript CLI under `cli/`.
Run `agent-nsmbu --help` for the command list.
Canonical reference: `docs/cli.md`.

## Install once

Node 24 or newer.

```bash
cd cli && npm link
```

After that, every command is the global `agent-nsmbu` on `PATH`.

## One game

Only one game process is supported.
If `agent-nsmbu open` refuses because a game is already running, run `agent-nsmbu status` or `agent-nsmbu close`.

## Which command

| Goal | Command |
| --- | --- |
| Compile or start a checkout | `build`, `launch`, `run` |
| Full course input log | `record <course>` / `replay <course>` |
| Measured Release course | `replay --release <course>` |
| Uncapped course, stop at a frame | `replay --release --uncapped --exit-at N <course>` |
| Scripted boot and frame PNGs | `scripted <name>` |
| Look and tap | `open` -> `status` / `screenshot` -> `press` / `stick` -> `wait` -> `close` |

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
`--exit-at N` sets `NSMBU_EXIT_AT_FRAME`.
`launch --no-debug` leaves the profile variables unset.
Use `replay --release` for a measured course.
