---
name: release
description: "Cut an NSMBU GitHub release: move CHANGELOG Unreleased to a version, local verify (ctest, installer test_setup, test_builds, nsmbu installer smoke), commit, push tag v*, and watch the release workflow. Use when the user asks to release, prepare a release, cut a version, push a release tag, watch release CI, or rerun a release until it is published and green."
---

# Release

Ship a versioned GitHub Release from `main` by pushing a `v*` tag.
The workflow is `.github/workflows/release.yml`.
Local pushes run `.git-hooks/pre-push`, which builds and runs the full `ctest` suite (path-alias tests such as `mod_content_startup` first), then `tools/installer/test_setup.py` and `tools/recomp/test_builds.py`.

**Default:** keep fixing and retagging until the release workflow is green and the GitHub Release exists.
Only stop on failure without retagging when the user explicitly says to watch only, report only, or not retag.

## Steps

1. Confirm the version string with the user if they did not name one (example `0.4.0`).
   The git tag is `v` plus that string (`v0.4.0`).
   The CHANGELOG heading is Keep a Changelog form without `v` (`## [0.4.0] - YYYY-MM-DD`).
2. On a clean intent to release from current work:
   - Follow the `changelog` skill to clean and order Unreleased, then move that block under a new `## [X.Y.Z] - YYYY-MM-DD` heading (ISO date).
   - Leave `## [Unreleased]` empty above it.
   - Update the compare links at the bottom of `CHANGELOG.md`.
   - Fold any further commits meant for this version into the same `## [X.Y.Z]` block before tagging.
   - Commit that change with any other release-ready files the user asked to include.
3. Local verify (required before push or tag).
   Prefer a configure with `-DNSMBU_SETUP_GUI=ON` (`build/linux` from pre-push shape, or `build/release-upload` after `nsmbu installer`).
   Run the same post-`ctest` scripts as pre-push and CI:

   ```bash
   ctest --test-dir <build> --output-on-failure
   python3 tools/installer/test_setup.py
   python3 tools/recomp/test_builds.py
   ```

   Then build a release-shaped installer package and open setup:

   ```bash
   nsmbu installer
   ```

   That builds the Linux release package at `build/installer` (the folder the zip would contain) and opens `nsmbu-launcher --setup`.
   Smoke the setup window: welcome or home actions, Keys paste fields when a disc image is in play, and that controls fit without clipping.
   Quit the window when done.
   Stop the release if any command fails or the setup smoke looks wrong.
4. Push `main` to `origin` when the release commit is not yet on the remote.
5. Create the tag on that commit: `git tag vX.Y.Z`.
   Push only that tag: `git push origin vX.Y.Z`.
   If the tag already exists on `origin` from a failed attempt, retag as in step 8.
6. Find the new `release` workflow run for the tag (`gh run list --workflow=release.yml --limit 3`).
   Watch it: `gh run watch <id> --exit-status`.
7. On success: report the run URL and the GitHub Release URL (`gh release view vX.Y.Z --json url -q .url`).
   Confirm the release is not a draft (`gh release view vX.Y.Z --json isDraft -q .isDraft` is `false`).
8. On failure:
   - Report the run URL, which jobs failed, and the failing test or step from `gh run view <id> --log-failed`.
   - Fix the failure, commit, push `main`, retag (`git tag -d vX.Y.Z`, `git push origin :refs/tags/vX.Y.Z`, `git tag vX.Y.Z`, `git push origin vX.Y.Z`), and return to step 6.
   - Repeat until the run succeeds and the release is published.
   - Only skip retagging when the user explicitly asked to watch or report without fixing.

Done when local verify passed, the tag is on `origin`, the release workflow conclusion is `success`, and the GitHub Release exists and is not a draft.

## Rules

- Tag pattern must be `v*` or the workflow does not publish.
- `tools/release/notes.py` copies the matching `## [X.Y.Z]` block from `CHANGELOG.md` into the GitHub Release body.
  That heading must exist before the tag build publishes.
- Do not create an empty version heading.
- Do not push a tag that already exists on `origin` without retagging after a fix (the default loop) or an explicit retag request.
- Do not push `main` or the tag until step 3 local verify has passed on this tree.
- Dry run without publishing: `gh workflow run release.yml -f dry_run=true` (no tag required).
  Use that only when the user asks for a dry run.

## Jobs to expect

| Job | Role |
| --- | --- |
| version | Sets `VERSION` from the tag |
| macos / windows / linux-x86_64 / linux-aarch64 | Build, package, unit tests |
| publish | Upload zips / AppImage and `gh release create` |

Any failed package job skips publish.
