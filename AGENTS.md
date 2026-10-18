# NSMBU native port

Native port of New Super Mario Bros. U, USA or Europe v1.3.0.
`tools/recomp/` translates `red-pro2.rpx` to C ahead of time.
`runtime/` implements Cafe OS and GX2 as native code.
Linux and Windows render with Vulkan; macOS with Metal.
The game dump, keys, and extracted files stay in `game/` on the machine that owns them; layout in `game/README.md`.

## Verify

The `nsmbu` cli is installed and can be used to interact with the game.

## Changelog

- Should only contain user facing update.
- Exclude cli and tools changes for example.
- Always update unreleased with changes use it as a raw log for cleanup later when releasing.

## Code style

Match the file you are editing.
Put the opening brace on the same line as the declaration.
Indent with 4 spaces.
The `nsmbu` target compiles with `-Wall`.
Warnings in `gamecode` and `cemu_latte` are expected; that code is generated or vendored.
