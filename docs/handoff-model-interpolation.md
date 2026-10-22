# Handoff: player model frame interpolation

Smooth camera, jittery player. Camera interpolation works; per-model (skeleton) interpolation is
staged in the worktree but never runs, and would be wrong even if it did.

Reference implementation: [ZeldaWWHDRecomp `runtime/src/interp.cpp`](https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp/blob/main/runtime/src/interp.cpp)
(Wii U, J3D engine). NSMBU is g3d, so offsets and hook points differ; the pass model, record/blend
rules and the save/restore sandwich carry over directly.

## 1. State of the work

- `runtime/src/hooks/interp.cpp` holds a staged (index, uncommitted) rewrite of pass pacing plus a
  first attempt at model pose blending. Do not drop it; the pacing half is in use (camera).
- The model half never executes, so there is nothing to preserve in its design, only its helper
  math (`model_to_quat`, `model_blend_mtx`, `model_pose_sane`) and its trace hooks.
- New decompiled facts belong in `vendor/decomp` (submodule `nsmbu-recomp/decomp`, branch `recomp`).

## 2. Symptom and evidence

Latest run (`nsmbu/logs/nsmbu.log`) sets `NSMBU_INTERP=1`, `NSMBU_INTERP_FPS=120`,
`NSMBU_INTERP_PACED=0`, `NSMBU_DISPLAY_HZ=240`, and interpolation is active:

```
[interp] frame interpolation 120 fps
[interp] 59.9 logic steps/s (120 fps, 2.00 frames per step)
```

`grep '\[interp\] model' nsmbu/logs/nsmbu.log` returns **nothing**: not one capture, not one blend,
not the periodic `models blended N fresh N cut N cur N` report. The model path is dead code at
runtime.

## 3. Where the code is

Everything interpolation-related lives in two files.

Pass model (`runtime/src/hooks/interp.cpp`):

| Line | Symbol |
| --- | --- |
| 218 | `pass_t()` fraction for this pass |
| 224 | `record_pass()` |
| 676-730 | `hook_029C52CC` (`ApplicationFramework::procCalc_`): logic pass advances `g_logic_steps`, holds draw via `f_029C5104` |
| 732-736 | `hook_029C4FE4` (`procDraw_`) |
| 604-638 | `after_pass()`: plans holds, `kDrop` skips the rest of a step |

Camera (works, use as the template):

| Line | Symbol |
| --- | --- |
| 238-261 | `apply_hold_camera()`: records `g_cam` on the record pass, blends, writes, restores |
| 738-765 | `hook_024FBF6C` (`RenderObjRenderMgr::calcView`): writes the blended box, re-derives the view/projection, restores |

Models (broken): `interp.cpp:263-486`

| Line | Symbol |
| --- | --- |
| 289-302 | `model_skeleton()` |
| 306-359 | `model_to_quat()`, `model_blend_mtx()` (reusable) |
| 361-375 | `model_pose_sane()` (reusable) |
| 377-398 | `model_capture()` |
| 400-448 | `model_apply()` |
| 469-478 | `hook_0291BD40` (`PlayerModel::calcBody`) |
| 480-486 | `hook_024F30DC` (`ModelG3d::calcGPU`) |

Registered in `tools/recomp/hooks.txt:21-26`.

Offsets in use: skeleton `[mdl+0x40]`, world matrix array `[skel+0x10]` (12 words per joint),
bone count `u16 [[skel]+8]`.

## 4. Defects in the staged model path

1. **Dead gate.** `interp.cpp:471` and `:482` require `NSMBU_INTERP_MODELS`, which nothing sets:
   no CLI flag, no settings key, no default. `runtime/src/core/env.h:137` declares it, `docs/env.md`
   does not document it. The hooks are compiled in and never armed.

2. **Uninitialised read.** `model_skeleton()` line 298 tests `wwatch::readable(arr, ...)` before
   `arr` is ever assigned (it is an out-parameter). `arr` is passed uninitialised from
   `model_capture()` (`:378`) and `model_apply()` (`:401`). Stack garbage decides whether capture
   and apply run at all, which also makes the failure non-deterministic.

3. **Capture at the wrong time.** `hook_0291BD40` captures *before* `PlayerModel::calcBody` runs
   `calcMdl()` -> `ModelEx.CalcWorld()`, so it snapshots the previous pass's drawn output rather
   than the step's true pose. `model_apply()` then keeps overwriting that same array with blended
   values (`:445`) and never restores it, so `prev` becomes a blend of a blend. With a dropped
   in-between pass (`interp_pacing.h:107`, `after_pass` `kDrop`) the last write is a partial blend
   and the next step's `prev` is a lie. This is the standout difference from the camera, which
   records only on the record pass and computes from clean endpoints.

4. **Body only.** `PlayerModel::calcHead` (`vendor/decomp/src/player/PlayerModel.cpp:783`) is not
   hooked. The head is a separate `ModelG3d` with its own skeleton, so the cap and face stay at the
   logic-step pose while the body blends.

5. **Guest state poisoned.** Blended matrices are left in the world array after the draw. The same
   array feeds `PlayerModel::calcHeadPos` (`PlayerModel.cpp:510`), `ModelG3d::calcBounding_`,
   `getBoneWorldMatrix`, and the next step's capture. WWHD hit exactly this class of bug (their
   issue #68: attached models placed from blended matrices and then blended twice) and fixed it by
   restoring.

6. **Unverified side effect.** `interp.cpp:446` sets bit `0x10` at `skel+4`. Nothing in
   `vendor/decomp` does this; the decomp recomputes through `calcMdl`/`CalcWorld`. If the bit marks
   the skeleton dirty, the blend is either discarded or applied twice.

## 5. Reference: how Zelda WWHD does it

Hook: `J3DModel::viewCalc` (`027F55FC`), which computes view-space draw matrices right before the
model is queued for drawing.

```c
// record pass: remember the exact matrices, then draw normally
if (record) { p.mtx = mtx; p.pass = g_record_passes; p.w.assign(cur, cur + words); orig(c); return; }

// logic pass / blended hold pass
if (model_not_recorded_last_step) { fresh; orig(c); return; }   // p.pass + 1 != g_record_passes
if (memcmp(prev, cur, 4*words) == 0) { still; orig(c); return; } // nothing moving
if (joints::blend(...).cut) { cut; orig(c); return; }             // teleport

memcpy(mtx, mid, 4*words);
orig(c);
memcpy(mtx, saved, 4*words);   // restore the exact pose
```

Rules that carry over:

1. **Record on the record pass, stamp a generation.** `p.pass = g_record_passes`, and blend only
   when `p.pass + 1 == g_record_passes`. A step whose in-between passes were dropped therefore
   degrades to "draw exact", not to "blend across a stale step". Same counter NSMBU already has
   (`interp.cpp:705`) and the same rule the camera obeys (`g_cam.step + 1 == g_logic_steps`,
   `interp.cpp:750`).
2. **Save, write blended, call orig, restore.** Guest memory holds the exact pose before and after
   the hook; only the draw sees the blend.
3. **Early-outs keep it cheap and safe**: not recorded last step -> `fresh`; identical snapshots ->
   `still`; root moved more than `INTERP_MODEL_CUT` -> `cut`; exact steps -> bypass entirely.
4. **Rotation slerp, translation/scale lerp** via `pacing::slerp_weights`; element-wise fallback
   when the matrix is not rotation x scale. NSMBU's `model_blend_mtx()` (`interp.cpp:336`) already
   implements this against `interp_pacing.h`.
5. **Hide/show of joints between steps** (WWHD tracks per-joint `stale` matrices for parts that were
   hidden on the previous step). NSMBU has the same class of problem with power-up and propeller
   parts; treat as follow-up, not first cut.

Mapping:

| WWHD | NSMBU |
| --- | --- |
| `J3DModel::viewCalc` `027F55FC` | `ModelG3d::calcGPU` `024F30DC` |
| `J3DModel` UBO update `027F5018` (job thread) | none: `CalcView` writes view matrices synchronously inside `calcGPU` |
| world mtx `[jnt+0x10]`, count `u16 [jnt+0x2C]`, model `+0x2C` | world mtx `[skel+0x10]`, count `u16 [[skel]+8]`, model `+0x40` |
| `camera_draw` `024FFC40` | `RenderObjRenderMgr::calcView` `024FBF6C` |
| main loop body `025F172C` | `procCalc_` `029C52CC` / `procDraw_` `029C4FE4` |
| 30 Hz logic, 1/3/7 in-betweens | 60 Hz logic, 1/3/7 in-betweens |

Do **not** copy J3D offsets. NSMBU offsets above were derived for the staged code and still need a
dump to confirm (see section 8).

## 6. Recommended fix

### 6.1 Hook point

Use `ModelG3d::calcGPU` only (`024F30DC`). `RenderObjRenderMgr::calcGPU`
(`vendor/decomp/src/graphics/RenderObjRenderMgr.cpp:85-98`) walks every render object each draw
pass, so this one hook covers the player body, the player head (drawn separately by
`PlayerModel::draw`, `PlayerModel.cpp:829`), enemies, props and platforms. Delete
`hook_0291BD40` and its `hooks.txt` line, and its entries in `tools/recomp/test_builds.py:78` and
`tools/recomp/test_coverage.py:84`.

### 6.2 Record

On `record_pass()` (`interp.cpp:224`), for each skeleton seen:

```
p.mtx = arr; p.joints = count; p.pass = g_record_passes;
p.pose = read(arr, 12 * count);
```

Read happens before any write in that pass, so it is the step's exact pose.

### 6.3 Blend and restore

```
auto it = g_models.find(skel);
if (it == end || it->mtx != arr || it->pose.size() != words) { fresh; orig; return; }
if (it->pass + 1 != g_record_passes) { fresh; orig; return; }
if (memcmp(it->pose, cur, 4 * words) == 0) { still; orig; return; }
if (root_joint_moved > INTERP_MODEL_CUT) { cut; orig; return; }
mid = blend(it->pose, cur, pass_t());
memcpy(arr, mid, 4 * words);
orig(c);                      // CalcView turns the blended world matrices into view matrices
memcpy(arr, cur, 4 * words);  // exact pose back in guest memory
```

Two things to confirm, because they decide whether the restore can stay here:

- If the game's `calcGPU` fills the model's uniform block from the **view** matrix array (what
  `CalcView` writes), restoring right after `orig` is correct and cheap.
- If it fills it from the **world** array and the block is consumed later (draw pass, or a job
  thread), the restore has to move after the draw: hook the draw entry/exit
  (`ModelG3d::drawOpa_`/`drawXlu_`) or restore at the end of each pass. Compare frames with and
  without the restore to decide.

Do not carry over the `st16(skel + 4, ...)` flag write. If the blend is invisible without it, find
out what the bit means before adding it back.

An alternative that avoids the record pass entirely is a two-slot scheme (capture the exact pose at
the first draw of each step, `prev = last cur`), but it needs a capture point that is guaranteed to
run before any blend write, and it silently blends across dropped steps. Prefer the WWHD rule.

### 6.4 Gates

- On whenever interpolation is on; keep `NSMBU_INTERP_MODELS=0` as the opt-out, matching
  `WWHD_INTERP_MODELS`.
- Keep `enabled()`, `course_play_active()`, and `!g_exact_step`.
- Skip skeletons over the joint cap (1024 today; 256 is enough for NSMBU characters and bounds the
  memcpy).
- `model_clear()` must run from `ss_reset()` (`interp.cpp:497`) so a save state load never blends
  across the jump; it already does.

### 6.5 Fix the existing helpers

- `model_skeleton()`: test `wwatch::readable(a, 12 * 4 * count)`, not `arr`.
- Keep `model_pose_sane()` but report rejections through `model_stats` so a bad offset shows up as
  `fresh`/`cut` instead of silence.
- Keep `INTERP_MODEL_TRACE` and `model_stats_report()`; add the model address and the record
  generation to the trace line so record/blend pairing is visible.

## 7. Verify

Build and drive the game (`nsmbu build`, then a scripted run; see `docs/automation.md`):

```bash
NSMBU_EXIT_AT_FRAME=12000 NSMBU_INTERP=1 NSMBU_INTERP_FPS=120 NSMBU_INTERP_PACED=0 \
NSMBU_INTERP_MODEL_TRACE=40 NSMBU_DUMP_FRAMES=1000,1001,1002 \
NSMBU_PRESS=600-620:8000 NSMBU_STICK=900-20000:1:0 \
nsmbu scripted model-interp
```

Acceptance:

- `grep '\[interp\] model' nsmbu/logs/nsmbu.log` shows `blended` lines and the periodic stats with
  `blended` dominant over `fresh`/`cut`.
- `python3 tools/frames/flicker.py FIRST LAST .tmp/model-interp` on consecutive frames: the player
  advances smoothly, the head stays attached and in phase with the body, the camera is unchanged.
- At least one enemy and one prop still look right, and no model shears or pops.
- No crash, no flood of `fresh` (that means the record pass never drew the model).

## 8. Open questions

1. **Offsets.** Confirm `[mdl+0x40]` -> skeleton, `[skel+0x10]` -> world matrix array,
   `u16 [[skel]+8]` -> bone count against a live dump (dump the first words of the array and compare
   with a bone world matrix read through `ModelG3d::getBoneWorldMatrix`, `ModelG3d.cpp:1219`).
2. **Restore point** (6.3): after `calcGPU`, or after the draw.
3. **Skeleton sharing.** `ModelG3d` owns `mModelEx` by value in the decomp, so each actor instance
   has its own skeleton and per-skeleton keying is correct. Verify with two of the same enemy on
   screen and a trace line per skeleton.
4. **Drawn twice per pass** (stereo, DRC, shadows, reflections): the record/read is idempotent per
   generation and per pass, but confirm no model flips between `blended` and `still` inside one
   pass.
5. **Hidden joints**: parts toggled between steps (super mushroom body, propeller) can blend from a
   matrix that was never updated. Add per-joint staleness only if frames show it.
6. **Cost**: `ModelG3d::calcGPU` runs for every render object. At 120 fps with a full course that is
   one `memcmp` plus a `12 * joints` memcpy per model per step, plus the blend on moving ones. Check
   `[interp] ... frames per step` and the profile (`docs/performance.md`) before and after.

## 9. Files the fix touches

- `runtime/src/hooks/interp.cpp` (model block only)
- `tools/recomp/hooks.txt` (drop `0291BD40` if the calcBody hook goes)
- `tools/recomp/test_builds.py`, `tools/recomp/test_coverage.py` (same)
- `docs/env.md` if `NSMBU_INTERP_MODELS` semantics change to opt-out
- `CHANGELOG.md` only if this becomes a player-facing note (use the `changelog` skill)
