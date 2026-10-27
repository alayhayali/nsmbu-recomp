# Widescreen layout (0.7.1)

## Goal

Keep the wider/taller world view from 0.7.0, but stop stretching NSMBU UI and stop actors popping in outside the 16:9 frame.
Changing the aspect ratio mid-session must leave layout and projection in a clean state.

## Symptoms

- UI (menus, HUD, overlays) stretches with the window instead of staying in a 16:9 letterbox or anchored HUD frame.
- Items, enemies, and other actors appear (pop in) in the expanded side/top strips outside the original border.
- Switching aspect modes (16:9, match window, 16:10, 21:9, 32:9) leaves panes or projection in a bad state until restart.

## Current state

0.7.0 expands the camera for non-16:9 windows via [runtime/src/gfx/aspect_view.h](../../runtime/src/gfx/aspect_view.h) and hooks in [runtime/src/hooks/aspect.cpp](../../runtime/src/hooks/aspect.cpp).
The changelog says the HUD stays in its original frame; that path is incomplete for NSMBU.

Pane roles and fill/HUD names in [runtime/src/gfx/aspect_panes.h](../../runtime/src/gfx/aspect_panes.h) and `is_hud_edge_pane` still look like Zelda WW HD layouts (`L_Rupy_00`, `N_TV_00`, `PF_PauseTV_00`, …).
Those names do not match NSMBU layouts, so fill/HUD/content classification is wrong and menus stretch.

Actor visibility / culling still follows the original 16:9 frustum or spawn window while the camera draws a wider view, so objects load or appear in the new borders.

Aspect changes update `g_game` on swap and force a short dirty window on roots, but offsets, stored FOV, and pane transforms are not fully reset when the mode changes.

## Work

- Map NSMBU layout pane names for fill backgrounds, HUD edges, and projected world UI; replace WW HD tables.
- Keep content (menus) centred at 16:9 scale; only expand world projection.
- Cull or clip actor spawn / draw so nothing pops in outside the intended border (or expand culling with the view deliberately and document which).
- On aspect mode change: clear pane offsets, FOV cache, and root anchor state; force a full layout recalc.
- Add playtests that boot, change aspect via overlay/env, and assert UI bounds and no off-frame pop-in.

## Out of scope

- New aspect presets beyond the existing modes.
- Cemu aspect-pack shader paths.
