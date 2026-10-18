# Automation runs

Headless boots use `agent-nsmbu scripted <name>` from the repo root (wraps `tools/scripted-run.sh`).
The script copies `nsmbu/save` into `.tmp/<name>/save`, then starts `build/debug/nsmbu` (or another build dir if present) with that save.
Frame dumps land in `.tmp/<name>/` as `frame_<n>.png`.
The repo `nsmbu/save` stays unread by the game.

## Steps

1. Set `NSMBU_EXIT_AT_FRAME`.
   Set `NSMBU_DUMP_FRAMES` to the frames you will look at.
   Set `NSMBU_PRESS` and `NSMBU_STICK` when the boot must press buttons.
   `NSMBU_RES_SCALE` defaults to 1.
2. Run `agent-nsmbu scripted <name>` from the repo root.
   The run is done when the process exits and each dumped `frame_<n>.png` is in `.tmp/<name>`.
3. Read one dumped frame.
   Continue only when that frame is the scene you mean.
4. For a shake check, dump the same consecutive frames at scale 1 and at scale 2.
   Run `python3 tools/flicker.py FIRST LAST .tmp/<name>`.
   The check is done when both scales have a result and you have looked at one frame from each.

## Environment the script sets

`tools/scripted-run.sh` always exports these values:

| Variable | Value | Role |
| --- | --- | --- |
| `NSMBU_USER_DIR` | `nsmbu/` (or `user/` if that is the only userdata folder) | Settings and logs stay in the repo userdata tree |
| `NSMBU_HIDDEN_WINDOWS` | `1` | Keeps game windows off screen |
| `NSMBU_NO_HOST_INPUT` | `1` | Ignores keyboard, mouse, and gamepad; scripted input only |
| `NSMBU_VK_PRESENT_MODE` | `immediate` | Uncapped present for faster boots |
| `NSMBU_NO_AUDIO` | `1` | Skips opening an audio output device |
| `NSMBU_RES_SCALE` | `1` unless already set | Internal resolution scale (1–4) |

The binary also receives `--game <repo>/game` and `--save .tmp/<name>/save`.

## Environment you set

Set these on the command line before the script:

| Variable | Required | Format / notes |
| --- | --- | --- |
| `NSMBU_EXIT_AT_FRAME` | yes | TV frame number; the process quits in order at that frame |
| `NSMBU_DUMP_FRAMES` | when you need PNGs | Comma-separated TV frames, e.g. `7000` or `800,1200,2000` |
| `NSMBU_PRESS` | optional | `from-to:bits` list; see below |
| `NSMBU_STICK` | optional | `from-to:x:y` list; see below |
| `NSMBU_RSTICK` | optional | Same form as `NSMBU_STICK` for the right stick |
| `NSMBU_KEYS` | optional | `from-to:key[+key…]` list of host key ids |
| `NSMBU_RES_SCALE` | optional | Overrides the script default of `1` |
| `NSMBU_MOD_MANAGER_DIR` | when mods must load | Absolute path to userdata that holds `mods/` and `mods.json` |

With `NSMBU_NO_HOST_INPUT` set, the mod manager stays off unless `NSMBU_MOD_MANAGER_DIR` points at a storage tree.
For a normal repo userdata tree, use `NSMBU_MOD_MANAGER_DIR=<repo>/nsmbu`.

### Scripted buttons

`NSMBU_PRESS` holds VPAD bits for inclusive TV-frame ranges.

```
from-to:hexbits,from-to:hexbits,…
```

`8000` is A.
Use a short pulse, about 20 frames.
A hold that is already down when a dialog opens does not press the button again.

### Scripted sticks

`NSMBU_STICK` and `NSMBU_RSTICK` set stick axes for inclusive TV-frame ranges.

```
from-to:x:y,from-to:x:y,…
```

Positive x is right.
Positive y is up.
Values are typically `-1` to `1`.

## Example

`nsmbu/save` is already past the opening cutscene.
This recipe starts from that save.

```
NSMBU_PRESS='1800-1820:8000,2600-2620:8000,3400-3420:8000,4200-4220:8000,5000-5020:8000,6300-6340:8000' \
NSMBU_STICK='5400-6000:1:0' \
NSMBU_DUMP_FRAMES=7000 \
NSMBU_EXIT_AT_FRAME=7100 \
tools/scripted-run.sh w11
```

Frame 7000 is the course.
The picture is the big tree, the bricks, and Mario on the ground.

## Mute

The script sets `NSMBU_NO_AUDIO=1` so no output device is opened.
The mix still runs.
Interactive mute is in the settings overlay (` or F12) > Audio.

## Related test aids

These are separate from `scripted-run.sh` and appear in benches and device checks:

- `NSMBU_RECORD_INPUT` — path of a text log of pad changes, keyed by guest logic step
- `NSMBU_REPLAY_INPUT` — play that log back and ignore the live pad
- `NSMBU_TEST_ORIGIN`, `NSMBU_TEST_PRESS`, `NSMBU_TEST_STICK`, `NSMBU_TEST_RSTICK`, `NSMBU_TEST_MODE`, `NSMBU_TEST_END` — timed scenarios after a logic-step origin
- `NSMBU_DUMP_PRESENT`, `NSMBU_DUMP_LOAD_FRAMES`, `NSMBU_CAPTURE` — present / load-path captures (see `docs/vulkan.md`)
- `NSMBU_STATE_LOAD_AT`, `NSMBU_PORTABLE_SAVE_AT` — save-state load points (see `docs/portable-save-states.md`)

## Input replays

`NSMBU_RECORD_INPUT` writes one line each time the pad changes.
The step count starts when in-level play begins (`AreaTask` is live), then follows guest logic steps (one per simulation tick, not per rendered frame).
New files use the `# nsmbu-replay` header and `replays/<course>.replay` paths from `agent-nsmbu record` / `agent-nsmbu replay`.
Each pad line ends with the player position (`x y z`).
A `# start` line records player index, lives, coins, course coins, power-up mode (0–8), and spawn position; replay checks that snapshot at step 0.
Replay logs the first step where position differs by more than 5 cm, and a match or mismatch count when it finishes.
A slow boot does not shift later lines.
Legacy `# nsmbu-input` files (including version 1 boot logic steps and v3 `@` witness lines) still load when pointed at by `NSMBU_REPLAY_INPUT`.
`NSMBU_REPLAY_INPUT` applies the latest line whose step is at or before the current step.
Live keyboard, mouse, and gamepad input is replaced for that run.
Intro-skip button pulses stay outside the file, so play the file back with the same course pack.

```
agent-nsmbu record 1-1
agent-nsmbu replay 1-1
agent-nsmbu replay --release 1-1
```

`agent-nsmbu record 1-1` writes `replays/1-1.replay` from the start of that course.
`agent-nsmbu replay 1-1` plays `replays/1-1.replay` on `build/debug/nsmbu`.
`agent-nsmbu replay --release 1-1` plays it on `build/release/nsmbu`.
Both set `NSMBU_PROFILE`, `NSMBU_VK_STATS`, `NSMBU_SYNC_STATS`, and `NSMBU_CRASH_RECOVERY` when those variables are unset.
The command prints `[replay] log <path>`.
The default path is `nsmbu/logs/nsmbu.log`.
`--uncapped` sets `NSMBU_UNCAPPED=1`.
`--exit-at N` quits at that TV frame.
`--log PATH` writes that file instead.
The game quits once the guest passes the last recorded step.
