# Automation runs

Issue and release game regressions belong in playtests.
Use `nsmbu scripted` when you need exit-at-frame dumps or a fixed `NSMBU_PRESS` timeline without a playtest case.
Long pad timelines stay on `record` / `replay`.

## Playtests

`nsmbu test` boots a debug game per case and checks issue and release regressions.
Selected cases run in parallel (default 2 at a time; set `NSMBU_TEST_JOBS` to change).
Add or extend a case under `tools/cli/src/playtest/cases/` and register it in `cases/index.ts`.
Reuse helpers in `tools/cli/src/playtest/helpers.ts` (`reachMap`, `loadPack`, `leaveCourse`, `enterCourse`, `quickSaveHere`, and the other exports) before inventing a new flow.
Map round-trips use `leaveCourse` / `enterCourse`, not `Scene.leave_course` alone.
Scene moves that need guest Lua stay inside the case through `Session` helpers.

`nsmbu test --list` prints the case names and summaries (the live list).
`nsmbu test opened-course` runs one case.
Only `--list` is a valid flag; other options are rejected.
Each case uses `.tmp/playtest/<name>/` for its save, control socket, and log file, so `nsmbu/save` and the live agent socket stay unread.
Shader cache and settings stay in the shared `nsmbu/` userdata.

Issue links that the cases cover:

- `startup` — [#6](https://github.com/nsmbu-recomp/nsmbu-recomp/issues/6)
- `opening-cinematic`, `save-roundtrip`, `course-enter`, `save-state` — [#14](https://github.com/nsmbu-recomp/nsmbu-recomp/issues/14) (Quick Save / story map / high fps)
- `course-enter-120` — [#15](https://github.com/nsmbu-recomp/nsmbu-recomp/issues/15) at 120 fps
- `map-shadow-120` — [#20](https://github.com/nsmbu-recomp/nsmbu-recomp/issues/20) at 120 fps (world map stays open)
- `opened-course` — 0.6.1 course-open / leave-to-map regressions

Closed-issue coverage outside playtests:

- [#9](https://github.com/nsmbu-recomp/nsmbu-recomp/issues/9) multiplayer bindings — `ctest -R input_mapping` (`test_issue_9_multiplayer_bindings`).
- [#11](https://github.com/nsmbu-recomp/nsmbu-recomp/issues/11) Windows common-key paste — `tools/installer/test_setup.py` (`test_issue_11_windows_common_key_paste`).
- [#7](https://github.com/nsmbu-recomp/nsmbu-recomp/issues/7) choppy gameplay has no deterministic playtest; shader precompile and the Vulkan smoke suite cover the shipped mitigations.
- 0.6.2 mismatched color-target freeze — Vulkan renderer smoke (`mismatched_color_targets`).

Map reads use the USA v1.3.0 addresses in `docs/nsmbu/course-open.md` and `vendor/decomp/include/course_select/NOTES_map_spawn.md`.
The run needs `game/` and a debug build.
It is not part of `ctest`.
Run it before a release when this machine has a dump.

## Scripted boots

Headless boots use `nsmbu scripted <name>` from the repo root (wraps `tools/run/scripted-run.sh`).
The script copies `nsmbu/save` into `.tmp/<name>/save`, then starts `build/debug/nsmbu` (or another build dir if present) with that save.
Frame dumps land in `.tmp/<name>/` as `frame_<n>.png`.
The repo `nsmbu/save` stays unread by the game.

### Steps

1. Set `NSMBU_EXIT_AT_FRAME`.
   Set `NSMBU_DUMP_FRAMES` to the frames you will look at.
   Set `NSMBU_PRESS` and `NSMBU_STICK` when the boot must press buttons.
   `NSMBU_RES_SCALE` defaults to 1.
2. Run `nsmbu scripted <name>` from the repo root.
   The run is done when the process exits and each dumped `frame_<n>.png` is in `.tmp/<name>`.
3. Read one dumped frame.
   Continue only when that frame is the scene you mean.
4. For a shake check, dump the same consecutive frames at scale 1 and at scale 2.
   Run `python3 tools/frames/flicker.py FIRST LAST .tmp/<name>`.
   The check is done when both scales have a result and you have looked at one frame from each.

### Environment the script sets

`tools/run/scripted-run.sh` always exports these values:

| Variable | Value | Role |
| --- | --- | --- |
| `NSMBU_USER_DIR` | `nsmbu/` (or `user/` if that is the only userdata folder) | Settings and logs stay in the repo userdata tree |
| `NSMBU_HIDDEN_WINDOWS` | `1` | Keeps game windows off screen |
| `NSMBU_NO_HOST_INPUT` | `1` | Ignores keyboard, mouse, and gamepad; scripted input only |
| `NSMBU_VK_PRESENT_MODE` | `immediate` | Uncapped present for faster boots |
| `NSMBU_NO_AUDIO` | `1` | Skips opening an audio output device |
| `NSMBU_RES_SCALE` | `1` unless already set | Internal resolution scale (1–4) |

The binary also receives `--game` set to the active dump under `game/<id>/` and `--save .tmp/<name>/save`.

### Environment you set

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

### Example

`nsmbu/save` is already past the opening cutscene.
This recipe starts from that save.

```
NSMBU_PRESS='1800-1820:8000,2600-2620:8000,3400-3420:8000,4200-4220:8000,5000-5020:8000,6300-6340:8000' \
NSMBU_STICK='5400-6000:1:0' \
NSMBU_DUMP_FRAMES=7000 \
NSMBU_EXIT_AT_FRAME=7100 \
tools/run/scripted-run.sh w11
```

Frame 7000 is the course.
The picture is the big tree, the bricks, and Mario on the ground.

### Mute

The script sets `NSMBU_NO_AUDIO=1` so no output device is opened.
The mix still runs.
Interactive mute is in the settings overlay (` or F12) > Audio.

## Related test aids

These are separate from `scripted-run.sh` and appear in benches and device checks:

- `NSMBU_RECORD_INPUT` — path of a text log of pad changes, keyed by guest logic step
- `NSMBU_RECORD_COURSE` — course named in the log's `course=` header
- `NSMBU_REPLAY_INPUT` — play that log back and ignore the live pad
- `NSMBU_REPLAY_EXIT` — `none` keeps the game running on live input when the replay input ends
- `NSMBU_REPLAY_TRACE` — log the replay clock arming, control gate, and early step positions
- `NSMBU_TEST_ORIGIN`, `NSMBU_TEST_PRESS`, `NSMBU_TEST_STICK`, `NSMBU_TEST_RSTICK`, `NSMBU_TEST_MODE`, `NSMBU_TEST_END` — timed scenarios after a logic-step origin
- `NSMBU_DUMP_PRESENT`, `NSMBU_DUMP_LOAD_FRAMES`, `NSMBU_CAPTURE` — present / load-path captures (see `docs/vulkan.md`)
- `NSMBU_STATE_LOAD_AT`, `NSMBU_PORTABLE_SAVE_AT` — save-state load points (see `docs/portable-save-states.md`)

## Input replays

The `.replay` format is a text input log; [replay-format.md](replay-format.md) lists each line, key, and field.
`NSMBU_RECORD_INPUT` writes one line each time the pad changes.
The step count starts when in-level play begins (`AreaTask` is live), then follows guest logic steps (one per simulation tick, not per rendered frame).
New files start with `version=1` and use `replays/<name>.replay` paths from `nsmbu record` / `nsmbu replay`.
Metadata lines are `key=value`; `#` starts a comment.
`course=` names the course; `nsmbu replay` launches it.
`origin=control` (what new files write) arms the step count when the game hands control to the player, so a slow boot or an uncapped run does not shift the recording.
Files without `origin=` fall back to arming when the player reaches a recorded `pos=`, which depends on how long the load takes.
`NSMBU_RECORD_COURSE` supplies that name when the runtime writes the file.
Each pad line ends with the player position (`x y z`).
Start config lines record `player=`, `lives=`, `player_coins=`, `coins=`, and `mode=`, one per line; replay starts the player with the recorded power-up mode.
`mode=` is `small`, `normal`, `fire`, `mini`, `propeller`, `penguin`, `ice`, `squirrel`, or `psquirrel`; numeric `0`–`8` still loads.
The buttons field is `-` when no button is down, otherwise `+`-joined names such as `A+Y`; legacy hex values still load.
`exit=input` (the default) quits when the recorded input ends; `exit=none` keeps the game running and hands control back to live input.
`--no-exit-on-input` sets `NSMBU_REPLAY_EXIT=none` for one run.
Replay logs the first step where position differs by more than 5 cm, and a match or mismatch count when it finishes.
A slow boot does not shift later lines.
Legacy `!` and `#` directive files, including `# nsmbu-input` version 1 boot logic steps and v3 `@` witness lines, still load when pointed at by `NSMBU_REPLAY_INPUT`.
`NSMBU_REPLAY_INPUT` applies the latest line whose step is at or before the current step.
Live keyboard, mouse, and gamepad input is replaced for that run.
Intro-skip button pulses stay outside the file, so play the file back with the same course pack.

```
nsmbu record 1-1
nsmbu record 2-1 bug-1
nsmbu replay 1-1
nsmbu replay bug-1
nsmbu replay --release 1-1
```

`nsmbu record 1-1` writes `replays/1-1.replay` from the start of that course.
`nsmbu record 2-1 bug-1` writes `replays/bug-1.replay` with `course=2-1`.
`nsmbu replay 1-1` plays `replays/1-1.replay` on `build/debug/nsmbu`.
`nsmbu replay bug-1` plays that file on the course named in its header.
`nsmbu replay --release 1-1` plays it on `build/release/nsmbu`.
Both set `NSMBU_PROFILE`, `NSMBU_VK_STATS`, `NSMBU_SYNC_STATS`, and `NSMBU_CRASH_RECOVERY` when those variables are unset.
The command prints `[replay] log <path>`.
The default path is `nsmbu/logs/nsmbu.log`.
`--uncapped` sets `NSMBU_UNCAPPED=1`.
`--exit-at N` quits at that TV frame.
`--log PATH` writes that file instead.
The game quits once the guest passes the last recorded step.
