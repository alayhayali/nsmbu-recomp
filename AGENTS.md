# NSMBU native port

Native port of New Super Mario Bros. U, USA v1.3.0 (title `00050000-10101d00`, title version 64).
`tools/recomp/` translates `red-pro2.rpx` to C ahead of time.
`runtime/` implements Cafe OS and GX2 as native code.
Linux and Windows render with Vulkan; macOS with Metal.
The game dump, keys, and extracted files stay in `game/` on the machine that owns them; layout in `game/README.md`.

Upstream base from ZeldaWWHDRecomp is in `README.md` and `docs/nsmbu.md`.
For a newer upstream commit, follow `.agents/skills/upstream-merge/SKILL.md`.

## Skills

Reach `.agents/skills/<name>/SKILL.md` when the branch matches:

- **build** — compile, `just build` / `just launch` / `launch-release`, ctest, stubgen or missing RPX, wrong update RPX addresses, example mod symlinks, `nsmbu/` userdata
- **guest** — `f_XXXXXXXX`, hooks, recomp `build/gen`, stubs, guest memory `ea`, `headers_names` / `names.py`, regional builds
- **upstream-merge** — merge a newer Wind Waker HD recomp commit (user-invoked)
- **perf** — profiling artifacts and profile lines (user-invoked)
- **release** — cut a GitHub release, tag `v*`, watch release workflow
- **changelog** — player-facing CHANGELOG summary
- **logs** — `nsmbu/logs/nsmbu.log` and log files under `nsmbu/logs`

## Docs

- **env** — `docs/env.md` when you add, rename, or read an `NSMBU_*` variable (`runtime/src/env.h`: `env::NAME()`, `.name()`, typed reads, `env::unset`)
- **automation** — `docs/automation.md` for `tools/scripted-run.sh`, headless boots, and scripted inputs or exit frames
- **names** — `docs/names.md` to name guest symbols (USA v1.3.0 address is the key)

## Code style

Match the file you are editing.
Put the opening brace on the same line as the declaration.
Indent with 4 spaces.
The `nsmbu` target compiles with `-Wall`.
Warnings in `gamecode` and `cemu_latte` are expected; that code is generated or vendored.
