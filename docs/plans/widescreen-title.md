# Title screen at 21:9

## Goal

Use the title screen as the check for 21:9 edges.
Boot with `NSMBU_ASPECT=21:9`.
The picture is 1707×720 inside a 16:9 window, so the black bars above and below the window are not the bug.

## What the title shows

- A bush pops in on the strip outside the original 16:9 frame.
- The logo, “Press 2!”, the copyright line, and the version are wider than the same elements at 16:9.
- Fades do not paint the new side strips black.
  The file-select iris stops at the same 16:9 circle.
- A one-pixel sky-blue line sits on the far right of the picture.

## Blue column

The wide color target is `ceil(1280 * 4/3 - 0.01)` pixels, which is 1707.
The draw scale is `1707/1280`.
In float32, `1280` times that scale is `1706.99997`.
The Vulkan scissor cast that value to an integer by truncating, so column 1706 was never drawn.
On the title that column stays sky blue.
On the world map the same column stayed black, because that scene clears black.

`cover_max` rounds the far scissor edge outward, and `cover_min` rounds the near edge inward.
The column is then covered by the same draws as the pixel beside it.

## UI stretch

Framebuffer orthos (`±640` by `±360`, and the other exact TV and GamePad sizes) stay at 16:9.
Only a draw whose vertex shader reads that matrix is placed in the centered 1280-wide viewport.
Any other draw keeps the full 1707-wide viewport.
A 1280-unit ortho in that viewport is 4/3 too wide, which is the stretched logo.

World cameras still expand and must keep the full viewport.
The title logo is a draw the current uniform scan does not classify.

Next step: capture the title at 16:9 and at 21:9 and measure the logo box.
Find the uniform the title UI shader actually reads, and center that draw.
Do not center world draws or full-screen fades.

## Transitions

The fade and the iris are full-screen quads in the 1280×720 ortho.
Centering that ortho leaves the side strips uncovered, so the edges stay on the scene instead of black.
A solid cover can use the full wide viewport, because stretching a black quad does not show.
The logo cannot.

`LayerMgr` draws fades on `cLayer_Fader` and `cLayer_Fader_DRC`.
The file-select iris is the same hole: it opens only to the 16:9 circle.

Next step: find the fader and iris draws and size them to the whole target.
Keep the title logo and the other HUD centered at 16:9 scale.

## Bush pop-in

The wider camera shows scenery the 16:9 spawn window never kept on screen.
`CullViewFrustum::update` (`0x024D7910`) builds its side planes from the projection aspect.
`DistantViewMgr::calcView_` (`0x022A7760`) writes the widened projection before that cull runs, so the distant-view frustum itself should already be wide.
`BgScrollMgr` still stores screen left and right for a 1280-wide view.
Actors and background parts spawn from those edges, so a bush appears when it crosses the old border even though the camera can already see past it.
The title is the still shot of that border.

Next step: log screen left and right, and the cull aspect, on the title.
Expand that scroll window by the same horizontal factor as the camera.
The bush should already be in frame, not cut on at `x = (1707 - 1280) / 2`.

## Check

- Boot hidden with `NSMBU_ASPECT=21:9` and dump a title frame and a fade frame.
- The far-right column matches the column beside it.
- The logo width-to-height matches the 16:9 capture.
- A fade frame is black across all 1707 columns.
- The side bush stays put across a few title frames.

## Out of scope

- Aspect presets other than the ones already in the port.
- GamePad target size.
  Those targets stay 854×480.
- The world-map overlay bars.
  Those stay on the 1280×720 frame.
