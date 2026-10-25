# NSMBU native port

## Rules

Assume that the Typescript to Lua watcher is already running.
Never comment on github issues.

## Verify

The `nsmbu` cli is on `PATH`.
Game regressions go in `tools/cli/src/playtest/cases.ts` and run with `nsmbu test`.

## Changelog

When editing `CHANGELOG.md`, use the `changelog` skill.
Notes for the example mods go in `modding/examples/CHANGELOG.md` and use that same form.

## Code style

Match the file you are editing.
Put the opening brace on the same line as the declaration.
Indent with 4 spaces.
The `nsmbu` target compiles with `-Wall`.

## Decomp

Any discoveries you find about the source code document and decompile it into the `decomp/` repo.
