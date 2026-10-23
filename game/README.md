# `game/` — your copy of NSMBU

This folder holds **your** Wii U dump, keys, optional update, and extracted game files.

Nothing here except this readme is in git.

## Before `nsmbu extract`

Put these in **this** folder (`game/`):

| Path | What it is |
|------|------------|
| `game.wux` | Your New Super Mario Bros. U disc image (`.wux`), USA or Europe |
| `game.key` | Title key for that image (16 raw bytes or one line of 32 hex digits) |
| `common.key` | Wii U common key from your console (same formats), **or** set `WIIU_COMMON_KEY` |
| `update/` | Optional matching **1.3.0** update (already extracted). Recommended for mods. |

In the setup window you can also type or paste each key as 32 hex digits instead of a key file.

### `update/` layout

| Path | Contents |
|------|----------|
| `update/code/red-pro2.rpx` | Update executable (must match a known USA or Europe 1.3.0 hash) |
| `update/content/` | Update content |
| `update/meta/` | Update metadata |

USA 1.3.0 is title version **64**.
Europe 1.3.0 is title version **80**.
The USA base disc alone is title version **0** (build `USA_V0`).
The Europe base disc alone is title version **0** (build `EU_V0`).
Both are playable.
**Mods are not supported** on either base disc without the update.
`game.key` must be the key for **this** `game.wux`.

## Extract

From the repository root:

```bash
nsmbu extract
```

That:

1. Checks `game/update/` when present (known 1.3.0 build).
2. Extracts `game/game.wux` into `game/code`, `game/content`, `game/meta`.
3. Applies `game/update/` when present, then checks that `code/red-pro2.rpx` is a known build.

The port runs with `--game` pointing at this folder.

## More detail

See [CONTRIBUTING.md](../CONTRIBUTING.md) and [docs/nsmbu/port.md](../docs/nsmbu/port.md).
See [docs/builds.md](../docs/builds.md) for USA, base-disc, and Europe address maps.
