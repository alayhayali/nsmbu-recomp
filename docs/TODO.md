# TODO

## Shaders

Shaders are translated when the game draws, not built from the game files ahead of time. GX2
registers and Latte microcode go through the vendored Cemu decompiler (`runtime/third_party/cemu`,
built as `cemu_latte`). Vulkan takes the GLSL backend and compiles it with glslang to SPIR-V; Metal
takes the MSL backend, and MSL sources are left out of builds without Metal (`CMakeLists.txt`). Both
keep a disk cache keyed by a recipe string: Metal in `~/Library/Caches/nsmbu/shaders.bin`, Vulkan in
`shadercache/vulkan-shaders/spirv.bin` plus the driver's own pipeline cache. `--warm-shaders` fills
the Metal cache and exits; Vulkan compiles on first use.

The only optional ahead-of-time input is the Metal shader head start,
`game/shadercache/headstart.bin`, read by `runtime/src/gfx/metal/shader_headstart.mm`. Only
`tools/shaderprep.py build` writes it, and `tools/yaz0.c` (loaded with ctypes) is what lets that
script open the Yaz0 → SARC → `BAHS` shader blobs.

- `tools/shaderprep.py` is not run by CMake, CI or the `nsmbu` CLI, and no `headstart.bin` is in the
  tree. The head start is therefore always absent and `shader_headstart.mm` only logs its hint. The
  planned `nsmbu shaderprep` subcommand (`docs/ts-tools-port.md`) is not written.
- `tools/yaz0.c` is reachable only through that script. It is compiled with a hardcoded `clang` into
  `build/libyaz0.dylib`, so a Linux run writes a `.so` with a macOS name, and no test covers it. The
  runtime has two more Yaz0 readers (`overlay/game_font.cpp`, `hooks/course_hot_reload.cpp`) that
  could share it.
- Vulkan has no head start and no offline SPIR-V prewarm, so a cold cache still compiles on the
  first draw.
- The Metal start-up replay costs about 6.2 s plus 1.6 s of head start, and `shaders.bin` reaches
  about 290 MB (`docs/performance.md`).
