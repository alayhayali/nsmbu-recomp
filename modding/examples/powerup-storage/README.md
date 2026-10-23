# Power-up storage

Keeps the power-up you walk into while you already have one.
Press the use button to spend the stored power-up at any time.

The reserve box sits at the top center of the picture and shows the stored power-up.
Each entered player has their own box.

## Behaviour

- Small players collect power-ups normally.
- A player with a power-up already stores the next power-up they touch.
- The player keeps the power-up they had; the new one goes to the reserve.
- The use button summons the stored power-up above the player so they can pick it up.
- The reserve survives between courses until the player leaves the course.

## Options

| Option | Default | Meaning |
| --- | --- | --- |
| `use_button` | `Minus` | Button that uses the stored power-up. |
| `show_hud` | on | Draw the reserve box. |

## Build

```bash
npm --prefix modding run build
```
