# Profiling

Canonical addresses are USA v1.3.0 `red-pro2.rpx` (see [names.md](names.md)).
Historical measurements stay in [performance.md](performance.md).

## The two profilers

Game sampler (built in):

- Turn on with `NSMBU_GAME_SAMPLE=120`.
  `SIGUSR2` prints a report.
- You get a per-PC histogram and a per-thread table in the game log.
- The log is `nsmbu/logs/*.log` or `.tmp/repro/nsmbu.log`.

`perf` (external):

- Turn on with `nsmbu perf`.
- You get full stacks over every thread.
- Artifacts land in `.tmp/perf/`.

World-map profiles use the debug boot in [shortcuts.md](shortcuts.md).
`nsmbu scripted` changes present mode and resolution, so it is the wrong boot for a settings-matched map profile.

`nsmbu replay --release 1-1` runs the Release course from `replays/1-1.replay`.
It sets the profile variables when unset and prints `[replay] log`.
Add `--uncapped`, `--exit-at N`, or `--log PATH` for a bounded run.
The 2026-10-17 baseline is [replay-1-1.md](replay-1-1.md) (59.9 swaps/s, the 4.00 MiB texture upload is now a page update, textures phase ~0.33 ms).

`nsmbu profile` runs a fixed 1-1 scene (slot 2 at TV frame 700, exit at 1100).
The log is `.tmp/repro/nsmbu.log`.
The methods are in [performance.md](performance.md).

`nsmbu perf` runs that scene under `perf record`.
It writes `.tmp/perf/nsmbu.data`, `game.log`, `report.txt`, and `report_flat.txt`.

Attach to a live session with `nsmbu perf --attach PID --seconds 30`.
It asks for sudo and sets `perf_event_paranoid` to 1 when the kernel refuses sampling (per boot).

## Read the artifacts

`[gx2] frame N, X swaps/s`, `[gx2] slow swap`, and `[prof]` per-phase ms lines are the first check.
Steady 60 fps holds ~59.9 swaps/s.

A `[game sample]` report counts `pc - binary_base` of the built `nsmbu` binary (usually `build/debug/nsmbu`), CPU-time sampled.
The thread table at the bottom gives the busy share per game thread (`GX2 render` about 60-70%, `game` about 20% is normal).

`.tmp/perf/nsmbu.data` resolves against the binary by build ID.
Keep that binary unchanged until the data is read.

## In-binary sample rows

```sh
python3 tools/game_sample_report.py LOG
```

Names the enclosing symbol.
`f_XXXXXXXX` rows print the header name from `tools/recomp/headers_names.txt`.

## Off-binary sample rows

Rows with huge offsets are mapped libraries, and the log alone cannot name them.
Resolve by re-running with `/proc/<pid>/maps` captured, or use perf, which symbolicates everything itself.
Sample offsets between runs share low-12 bits with the same library, so clusters are comparable.
Bases are not.

## Known hotspot map

Linux, Intel Arc + ANV, from 2026-10-13 profiles.

- libc `memcpy` / `memmove` / `memset` loops (~40%) are upload and decode copies.
  The `memcpy@plt` row is the funnel for the same calls.
- `gfxvk::decode_level_fast`, `XXH3`, and `LatteAddrLib::ComputeSurfaceAddr…` are the CPU texture decode, hash, and upload path.
- The vDSO cluster is busy clock reads in deadline waits (`[gx2] slow swap`, acquire wait).
- libc `ioctl` is GPU submissions (i915).
- `futex` / `pthread_cond` are job-queue wakeups.
- `f_02A15500` and `f_02A157FC` are an unnamed guest byte-at-a-time stream loop on the game thread (~5%).
  Name it via [names.md](names.md) before optimizing.

## Name an address

`grep "XXXXXXXX" tools/recomp/headers_names.txt` first.
Full rules are in [names.md](names.md).
Reads of guest code live in `build/gen/code_*.c` (`f_XXXXXXXX`).
