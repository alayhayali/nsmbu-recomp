---
name: shortcuts
description: "Reusable NSMBU boot commands and NSMBU_* timelines. Use when entering the world map, scripting NSMBU_PRESS, launching the binary for a profile, or saving a command that should be reused."
---

# Shortcuts

Append a shortcut here when a boot, pad timeline, or profile command works a second time.
Each entry is the command, the log line that shows it worked, and what a later press would do.
Full variable lists stay in `docs/env.md` and `docs/automation.md`.
Course replay benchmarks stay in the `perf` skill.

## World map

Launch `build/debug/nsmbu` yourself when the run must use `nsmbu/settings.ini` (165 fps interpolation, resolution scale, present mode).
`nsmbu scripted` forces an immediate present, resolution scale 1, hidden windows, and a copy of the save.

`NSMBU_USER_DIR` must be the repo `nsmbu/` directory.
A `portable.txt` beside the binary wins over that variable.
Without either one, settings and the shader cache come from `~/.config/nsmbu`.

`8000` is A.
Pulses are 20 TV frames.
A button that is already down when a dialog opens does not press again.
On a 165 Hz debug boot from `nsmbu/save` (file 0 on the Acorn Plains map), 200 frames between pulses is enough.
The slower 800-frame list in `docs/automation.md` is for walking into a course.

```bash
NSMBU_USER_DIR="$PWD/nsmbu" \
NSMBU_LOG_FILE="$PWD/.tmp/repro/map.log" \
NSMBU_PRESS='600-620:8000,800-820:8000,1000-1020:8000,1200-1220:8000,1400-1420:8000,1600-1620:8000,1800-1820:8000,2000-2020:8000,2200-2220:8000,2400-2420:8000' \
NSMBU_EXIT_AT_FRAME=5600 \
NSMBU_PROFILE=1 NSMBU_VK_STATS=1 NSMBU_NO_AUDIO=1 NSMBU_INTERP_PASS_STATS=1 \
./build/debug/nsmbu --game "$PWD/game/usa-1.3.0" --save "$PWD/nsmbu/save"
```

Done when the log contains `[scene] CourseSelect from save` and later `[prof]` lines are the map.
File select appears around frame 600.
Course select is up before frame 3000.
A settled debug map is about 165 swaps/s, acquire 1.00 per frame, about 60 hold frames out of 120, and about 82 logic steps/s.
Another A after that scene enters the course.
The stick and final A in the `docs/automation.md` example do that on purpose.

`NSMBU_STATE_LOAD_AT` at TV frame 700 does not reach this save.
The guest thread in the state is not alive yet, and a portable load of `slot1.nsstate` fails its checksum against the current save.

## Pad bits

`NSMBU_PRESS` is `from-to:hexbits` ranges, comma-separated.
Combine bits with a sum.

- A `8000`, B `4000`, X `2000`, Y `1000`
- L `0020`, R `0010`, ZL `0080`, ZR `0040`
- Plus `0008`, Minus `0004`, Home `0002`
- Up `0200`, Down `0100`, Left `0800`, Right `0400`

`NSMBU_STICK` is `from-to:x:y`.
Positive x is right.
Positive y is up.

## Other boots

- `nsmbu scripted <name>` copies `nsmbu/save` into `.tmp/<name>/save` and writes `frame_<n>.png` there.
  Set `NSMBU_EXIT_AT_FRAME` and, when a picture is required, `NSMBU_DUMP_FRAMES`.
- `nsmbu replay --release 1-1` is the course benchmark.
  The `perf` skill owns that baseline.
- `nsmbu status` is the live scene line.
  `scene=CourseSelect` with `map=1` is the world map.
- `NSMBU_LOG_FILE=0` skips the log file.
  Any other path replaces `nsmbu/logs/nsmbu.log`.
  Profile logs belong under `.tmp/`.
