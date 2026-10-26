# NSMBU native port

## Rules

Assume that the Typescript to Lua watcher is already running.
Never comment on github issues.
Add each commit message/change to the changelog it will be reformated later for release.

## Verify

The `nsmbu` cli is on `PATH`.
Game regressions go in `tools/cli/src/playtest/cases/` (shared helpers in `helpers.ts`) and run with `nsmbu test`.
Write tests for each feature implemented so we can catch regressions. You might need to improve the test harness in some cases.

## Code style

Match the file you are editing.
Put the opening brace on the same line as the declaration.
Indent with 4 spaces.
The `nsmbu` target compiles with `-Wall`.

## Decomp

Any discoveries you find about the source code document it in the `vendor/decomp/docs` repo.
