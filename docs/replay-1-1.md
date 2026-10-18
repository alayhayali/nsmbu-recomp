# 1-1 replay profile

This note records one Release run of `nsmbu replay 1-1` on 2026-10-17.

The course holds the 60 fps vsync limit.

The largest CPU cost inside that limit is a 4.00 MiB texture upload on every frame.

## Machine and build

The CPU is an Intel Core i5-12600KF.

The GPU is an Intel Arc B580.

The Vulkan driver is Mesa 26.2.4 (ANV).

The binary is `build/rel/nsmbu` at commit `3240a61`.

`nsmbu replay` launches `build/nsmbu`.

That tree is a Debug build, so this run used the Release binary with the same launch settings.

## How the run was started

The command matches `nsmbu replay 1-1`.

`NSMBU_REPLAY_INPUT` is `replays/1-1.replay`.

The course pack is `1-1`.

`nsmbu launch` also sets `NSMBU_PROFILE`, `NSMBU_VK_STATS`, `NSMBU_SYNC_STATS`, and `NSMBU_CRASH_RECOVERY`.

The log is `.tmp/replay-11/nsmbu.log`.

The saved settings are 2x internal resolution, FXAA, anisotropy, the GamePad screen off, a windowed TV window, and FIFO present.

Interpolation mode is 0, so the log has 0 hold frames.

The loader listed Steam, MangoHud, Lossless Scaling, and Mesa implicit layers.

## Result

The process ran for 49.6 s and exited 0.

The replay ends at guest step 2463.

`[gx2]` stays at 59.9 swaps/s from TV frame 901 through frame 2701.

No `[gx2] slow swap` line appears.

From TV frame 840 through frame 2760, each 120-frame window averages 16.68 ms, 59.9 swaps/s, and 59.9 logic steps/s.

The render thread uses 3.53 ms of CPU in that span.

It waits 13.15 ms for the next commands.

The game thread waits 2.38 ms on DrawDone (2.00 waits a frame).

GPU, acquire, and present waits stay under 0.05 ms.

## Where the render-thread time goes

Draw work is 2.87 ms of the 3.66 ms of tracked ops.

That is 215 draws a frame, 13.3 us each.

| Phase | ms/frame |
| --- | ---: |
| textures | 1.98 |
| submit | 0.22 |
| shader | 0.15 |
| record | 0.10 |
| descriptors | 0.10 |
| uniforms | 0.09 |
| vertex | 0.06 |
| pass | 0.06 |
| indices | 0.04 |
| targets | 0.04 |
| pipeline | 0.03 |

Register writes are 0.39 ms for about 13,800 writes a frame.

Invalidate is 0.12 ms for about 121 calls a frame.

The swap op is 0.24 ms.

Texture uploads are 4.00 MiB on every logic frame in this span.

The renderer does about 1.04 full texture checks a frame and about 7.4 page-write faults a frame.

The 4.00 MiB size matches the CPU texture called out in the 2026-10-12 note in `docs/performance.md`.

This run leaves that surface unnamed.

87.4% of draws write registers outside the pointer and ALU set.

2.6% repeat the same registers.

10.0% change only buffer pointers or ALU constants.

The busiest extra register writes are `CB_COLOR`, `SQ_PGM`, and `SQ_TEX_RESOURCE`.

## Threads

During the course, CPU time as a share of one core is:

| Thread | Share |
| --- | ---: |
| GX2 render | 21% |
| guest | 10% |
| nsmbu | 2.5% |

Prepare Thread, sched tick, AX frame, RenderObjParallel, sound, and particles each stay under 2%.

Over the whole 49.6 s, GX2 render uses 9.08 s and the guest thread uses 4.53 s.

The guest thread uses about 10% of one core, so most of the game thread is asleep.

## Boot and the course load

TV frames 1–360 are the title.

Render-thread CPU there is 0.7 ms, with about 100 draws.

Texture uploads in that span stay near 0 MiB.

The course cost shows up in the window that ends at TV frame 480.

It is steady by TV frame 600.

Shader translation reaches 133 variants and 95 new programs by the end.

The boot loaded a warm Vulkan pipeline cache (6.2 MB).

## Replay match

The log reports 7 position mismatches.

The first is at step 1036.

The recorded Y is -768.000 and the replay Y is -767.016.

Frame time stays on the vsync limit on both sides of that step.

## Symbol samples

A second run recorded `perf` for the whole replay.

`perf_event_paranoid` was 1.

The data file is `.tmp/perf/nsmbu.data` (19,228 samples over 49.0 s, `cycles:u`).

The grouped report is `.tmp/perf/report_group_all.txt`.

This CPU records `cpu_atom/cycles/u` and `cpu_core/cycles/u`.

The shares below are the P-core column.

| Thread | P-core share |
| --- | ---: |
| GX2 render | 59.7% |
| guest | 34.8% |
| Prepare Thread | 3.7% |

| P-core share | Symbol |
| --- | --- |
| 8.30% | libc `rep movsb` (memcpy), on GX2 render |
| 8.16% | vDSO clock, mostly the guest thread |
| 6.69% | `gfxvk::decode_level_fast` |
| 5.11% | `XXH3_hashLong_64b_default` |
| 4.03% | libc `rep stos` (memset), on GX2 render |
| 2.02% | `gx2::apply_regs` |
| 1.73% | `f_02A15500` |
| 1.72% | `LatteAddrLib::ComputeSurfaceAddrFromCoordMacroTiledCached` |
| 1.61% | `gx2::execute_one` |
| 1.25% | `f_02A157FC` |
| 1.25% | `gfxvk::invalidate` |
| 1.08% | `gfxvk::draw` |

The memcpy, decode, xxHash, memset, and address-library rows are the texture upload.

Together those five rows are 25.9% of the P-core samples.

`f_02A15500` and `f_02A157FC` are the unnamed guest stream loop.

On this perf run the player position matched.

## After the page upload (same day)

A later Release run of the same replay kept the vsync limit once the course was warm.

From TV frame 1800 through frame 2760, each 120-frame window stays at 16.67–16.70 ms and 59.6–60.0 swaps/s.

At TV frame 2760 the textures phase is 0.33 ms and texture uploads are 0.00 MiB/frame.

Render-thread CPU in that window is 1.68 ms.

The game thread waits 0.72 ms on DrawDone.

The replay again reports 7 position mismatches, and the first is still step 1036 (recorded Y -768.000, replay Y -767.016).
