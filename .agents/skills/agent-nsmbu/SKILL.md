---
name: agent-nsmbu
description: "NSMBU developer and agent CLI: build, launch, record/replay, and live game control (open, status, press, screenshot, wait, close). Use when driving the game from a shell, taking screenshots, short pad pulses, or running dev workflows. Prefer agent-nsmbu record/replay for long input timelines."
---

# agent-nsmbu CLI

TypeScript CLI under `cli/`.
It shells out to the existing Python tools for build, launch, profile, and headless scripted runs.
Canonical reference: `docs/cli.md`.

## Install once

Node 24 or newer (erasable TypeScript; the CLI runs as `node …/*.ts`).

```bash
cd cli && npm link
```

After that, every command is the global `agent-nsmbu` on `PATH`.
Do not invoke `node cli/bin/agent-nsmbu.ts` or a repo-root wrapper.

## Dev recipes

```bash
agent-nsmbu build                    # Debug → build/debug
agent-nsmbu build --release          # Release → build/release
agent-nsmbu build --no-debug         # RelWithDebInfo → build/release
agent-nsmbu build --sanitizer
agent-nsmbu launch                   # debug env defaults when unset
agent-nsmbu launch --no-debug        # release-style launch
agent-nsmbu launch --trace | --gdb | --lldb
agent-nsmbu run                      # build then launch
agent-nsmbu run --release
agent-nsmbu extract
agent-nsmbu record <course>
agent-nsmbu replay <course>
agent-nsmbu profile | agent-nsmbu perf
agent-nsmbu scripted <name>          # wraps tools/scripted-run.sh
```

`agent-nsmbu` sets `NSMBU_USER_DIR` to the gitignored `nsmbu/` folder on dev commands.

## Single instance only

Only one game process is supported.
There is no `--session` flag and no multi-instance control.
If `agent-nsmbu open` refuses because a game is already running, use `agent-nsmbu status` or `agent-nsmbu close` before starting again.

## The agent control loop

Use this for short interactive work: boot, observe, pulse buttons or sticks, wait for a frame, screenshot, quit.
Complex pad timelines stay on `agent-nsmbu record` / `agent-nsmbu replay` (or `NSMBU_RECORD_INPUT` / `NSMBU_REPLAY_INPUT`); the live CLI does not write replay files.

```bash
agent-nsmbu open 1-1
agent-nsmbu status
agent-nsmbu screenshot .tmp/agent/frame.png
agent-nsmbu press A
agent-nsmbu stick 0 -32768 --frames 30
agent-nsmbu wait --frame 700
agent-nsmbu screenshot
agent-nsmbu close
```

`open` builds if needed, spawns the game with agent control enabled, and waits until the control socket accepts.
`status` returns TV frame, logic step, optional player snapshot, and pid.
`press` accepts a button name or hex hold mask; default pulse is about 20 frames.
`stick` / `rstick` set axes until released or for `--frames N`.
`screenshot [path]` blocks until the PNG exists and prints the path.
`wait --frame N` or `wait --step N` polls `status` until the bound is reached.
`quit` and `close` are the same: graceful shutdown and cleanup under `.tmp/agent/`.

Headless exit-at-frame dumps without the interactive loop: `agent-nsmbu scripted <name>` (see `docs/automation.md`).

## When to use what

| Goal | Use |
| --- | --- |
| Compile or run from a checkout | `agent-nsmbu build`, `agent-nsmbu launch`, `agent-nsmbu run` |
| Full course input log | `agent-nsmbu record` / `agent-nsmbu replay` |
| Scripted env boots + frame PNGs | `agent-nsmbu scripted` |
| Agent looks at the game and taps buttons | `open` → `status` / `screenshot` → `press` / `stick` → `wait` → `close` |
