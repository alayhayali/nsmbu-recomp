---
name: perf
description: "Profiling NSMBU: swaps/s, profile lines, perf reports, and the course replay baseline. Use when measuring performance, reading a profile, or running a replay benchmark."
---

# Profiling the game (nsmbu-recomp)

Canonical addresses are USA v1.3.0 `red-pro2.rpx` (see `docs/names.md`).

## The two profilers

| | Game sampler (built in) | perf (external) |
| --- | --- | --- |
| Turn on | `NSMBU_GAME_SAMPLE=120`; `SIGUSR2` prints a report | `nsmbu perf` |
| Get | per-PC histogram + per-thread table, in the game log | full stacks over every thread |
| Ends up in | `nsmbu/logs/*.log` or `.tmp/repro/nsmbu.log` | `.tmp/perf/` |

- `nsmbu replay --release 1-1` runs the Release course from `replays/1-1.replay`.
  It sets the profile variables when unset and prints `[replay] log`.
  Add `--uncapped`, `--exit-at N`, or `--log PATH` for a bounded run.
  The 2026-10-17 baseline is `docs/replay-1-1.md` (59.9 swaps/s, the 4.00 MiB texture upload is now a page update, textures phase ~0.33 ms).
- `nsmbu profile` runs a fixed 1-1 scene (slot 2 at TV frame 700, exit at 1100).
  The log is `.tmp/repro/nsmbu.log`.
  The methods are in `docs/performance.md`.
- `nsmbu perf` runs that scene under `perf record`.
  It writes `.tmp/perf/nsmbu.data`, `game.log`, `report.txt`, and `report_flat.txt`.
- Attach to a live session: `nsmbu perf --attach PID --seconds 30`.
- Asks for sudo and sets `perf_event_paranoid` to 1 when the kernel refuses sampling (per boot).

## Read the artifacts

- `[gx2] frame N, X swaps/s`, `[gx2] slow swap` and `[prof]` per-phase ms lines are the first check.
  Steady 60 fps holds ~59.9 swaps/s.
- `[game sample]` report: counts are `pc - binary_base` of the built `nsmbu` binary (usually `build/debug/nsmbu`), CPU-time sampled.
  The thread table at the bottom gives the busy share per game thread (`GX2 render` about 60-70%, `game` about 20% is normal).
- `.tmp/perf/nsmbu.data` resolves against the binary by build ID: keep that binary unchanged until the data is read.

## In-binary sample rows

```sh
python3 tools/game_sample_report.py LOG
```

Names the enclosing symbol; `f_XXXXXXXX` rows print the header name from `tools/recomp/headers_names.txt`.

## Off-binary sample rows

Rows with huge offsets are mapped libraries, and the log alone cannot name them.
Resolve by re-running with `/proc/<pid>/maps` captured, or just use perf, which symbolicates everything itself.
Sample offsets between runs share low-12 bits with the same library, so clusters are comparable.
Bases are not.

## Known hotspot map (Linux, Intel Arc + ANV, from 2026-10-13 profiles)

| Cluster | Means |
| --- | --- |
| libc `memcpy`/`memmove`/`memset` loops (~40%) | upload/decode copies: the `memcpy@plt` row is the funnel for the same calls |
| `gfxvk::decode_level_fast` + `XXH3` + `LatteAddrLib::ComputeSurfaceAddr…` | CPU texture decode, hash, upload path |
| vDSO cluster | busy clock reads in deadline waits (`[gx2] slow swap`, acquire wait) |
| libc `ioctl` | GPU submissions (i915) |
| `futex`/`pthread_cond` | job-queue wakeups |
| `f_02A15500` + `f_02A157FC` | unnamed guest byte-at-a-time stream loop on the game thread (~5%); name it via `docs/names.md` before optimizing |

## Name an address

`grep "XXXXXXXX" tools/recomp/headers_names.txt` first.
Full rules are in `docs/names.md`.
Reads of guest code live in `build/gen/code_*.c` (`f_XXXXXXXX`).
