# `game/` — your copies of NSMBU

This folder holds **your** Wii U dumps, keys, optional updates, and extracted game files.

Nothing here except this readme is in git.

## Walkthrough

```bash
nsmbu game
```

1. Drop any named `.wux` (or `.wud`) into `game/`.
2. Drop keys into `game/` too (`common.key`, Cemu `keys.txt`, and a sidecar `.key` next to the image).
3. Run `nsmbu game` — it moves the image into a version folder as `game.wux`, keeps the disc key as `game/<id>.key`, then can extract and recomp.

`nsmbu game usa-1.3.0` activates that folder without the menu.
Add `--recomp` to recomp after activating.

## Layout

```text
game/
  active                 # written by nsmbu game (e.g. usa-1.3.0)
  common.key             # Wii U common key (or use keys.txt / WIIU_COMMON_KEY)
  keys.txt               # optional Cemu-style keys (common= / disc=)
  usa-base.key           # disc key for usa-base (from the sidecar on import)
  eu-base.key
  My Disc.wux            # drop zone — any name; imported by nsmbu game
  My Disc.key            # optional sidecar disc key for that image
  usa-base/
    game.wux             # moved + renamed on import (optional after extract)
    code/ content/ meta/ # extract of the base disc (title version 0)
  usa-1.3.0/
    code/ content/ meta/ # base extract with the 1.3.0 update merged in
  eu-base/
    game.wux
    code/ content/ meta/
  eu-1.3.0/
    code/ content/ meta/
```

Recommended ids: `usa-base`, `usa-1.3.0`, `eu-base`, `eu-1.3.0`, `nsmbu-nslu-usa`, `nsmbu-nslu-eu`.

Version folders only need `code/`, `content/`, and `meta/` to play.
Keys stay under `game/` — not inside the version folder.
`game.wux` in a version folder is only needed to (re)extract.

`*-base` is title version 0.
`*-1.3.0` is the merged update (mods).
A 1.3.0 folder can be a merge of `*-base` plus an update’s `code/`, `content/`, and `meta/` with no disc image.

`nsmbu build`, `nsmbu launch`, and playtests resolve the active folder via `game/active`.

If you still have a flat `game/code/` tree from before, run `nsmbu game` to migrate it into a version folder.

## Keys

| Path | What it is |
|------|------------|
| `common.key` | Wii U common key (16 raw bytes or one line of 32 hex digits) |
| `keys.txt` | Cemu-style file with `common=` and optional `disc=` lines |
| `<id>.key` | Disc key for `game/<id>/game.wux` (created when you import a sidecar `.key`) |
| `WIIU_COMMON_KEY` | Environment variable alternative for the common key |

A sidecar `Something.key` next to a dropped `Something.wux` is moved to `game/<id>.key` on import.
Legacy `game.key` / `common.key` inside a version folder still work if present.

## Updates

Optional matching **1.3.0** update as `game/<id>/update/` (`code/`, `content/`, `meta/`), applied on extract.

USA 1.3.0 is title version **64**.
Europe 1.3.0 is title version **80**.
The USA base disc alone is title version **0** (build `USA_V0`).
The Europe base disc alone is title version **0** (build `EU_V0`).
Both are playable.
**Mods are not supported** on either base disc without the update.

## More detail

See [CONTRIBUTING.md](../CONTRIBUTING.md) and [docs/nsmbu/port.md](../docs/nsmbu/port.md).
See [docs/builds.md](../docs/builds.md) for USA, base-disc, and Europe address maps.
