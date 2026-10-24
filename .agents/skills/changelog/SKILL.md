---
name: changelog
description: "Edit CHANGELOG.md or modding/examples/CHANGELOG.md in Keep a Changelog form: player-facing notes, type sections, importance order within a section, and release cleanup. Use when adding, rewriting, or sorting changelog entries, preparing Unreleased for a version heading, or when AGENTS.md requires the changelog skill."
---

# Changelog

`CHANGELOG.md` and `modding/examples/CHANGELOG.md` follow [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
GitHub Release bodies copy the matching version block from `CHANGELOG.md` via `tools/release/notes.py`.

## Which file

`CHANGELOG.md` is the game, installer, and Mods tab.
`modding/examples/CHANGELOG.md` is the example mods in that folder.
A note about one example mod's behaviour goes only in the examples changelog.
A note about loading packages, the Mods tab, or install stays in `CHANGELOG.md`.

## When to edit

Append under `## [Unreleased]` as work lands.
At release, the `release` skill moves the `CHANGELOG.md` block under a dated `## [X.Y.Z] - YYYY-MM-DD` heading and leaves Unreleased empty.
Refresh the compare links at the bottom of `CHANGELOG.md` when versions change.
The examples changelog keeps its own headings and is not copied into the GitHub Release.
The same rules apply to every version heading, not only Unreleased.

## Audience

Write for someone who plays or installs the game.
Keep installer, crash reporting, mods UI, and in-game behaviour the player can notice or act on.
Drop CLI, tools, CI, docs paths, env vars, address maps, and other developer-only detail.
When a change is purely technical (upload paths, decode, caches, and similar), write only the performance the player feels, not the mechanism.
Skip the note when there is no player-visible performance or behaviour change.

## Sections

Group notes under these `###` headings only, and omit empty ones:

1. `### Added`
2. `### Changed`
3. `### Deprecated`
4. `### Removed`
5. `### Fixed`
6. `### Security`

Within each section, list notes from most to least important to a player:

1. New playable builds or content (regions, packs, disc layouts).
2. Gameplay, performance, and display fixes that change how a session feels.
3. Crash or setup failures that block play, and clearer recovery (report dialogs).
4. Install and first-run changes that affect getting into the game.
5. Breaking changes the player must act on (save states, paths, formats).
6. Mods and packaging behaviour the player or mod author sees.
7. Smaller UI polish.

## Wording

Lead with the outcome the player notices.
Prefer plain product names over title ids, keep a title id only when it disambiguates an install.
One bullet is one change.
Do not combine separate features or fixes in one bullet.
Use a continuation line only for a caveat or how-to of that same change.
One sentence per line, wrap continuation lines with two spaces under the bullet.

## Entry shape

```markdown
## [Unreleased]

### Added

- Short outcome in plain language.
  Optional second sentence for the caveat or how-to.

### Fixed

- …

## [0.4.2] - 2026-10-14

### Fixed

- …
```

Version headings use brackets and an ISO date (`## [X.Y.Z] - YYYY-MM-DD`).
Unreleased has no date.
Keep each bullet to one outcome plus at most one short supporting sentence unless a breaking caveat needs a third line.

## Steps

1. Read the changelog for that change (`CHANGELOG.md` or `modding/examples/CHANGELOG.md`) and the change being documented.
2. Decide whether the change is player-facing, skip the edit when it is not.
3. Place the note under the right `###` section using Audience, Sections, and Wording.
4. Re-sort that section so importance order still holds.
5. Done when types are grouped Keep-a-Changelog style, each section reads top-down by player importance, and no developer-only notes remain.
