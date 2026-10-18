nsmbu_dir := justfile_directory() / "nsmbu"
export NSMBU_USER_DIR := nsmbu_dir

default:
    @just --list

build *args:
    python3 tools/build.py {{args}}

run *args:
    python3 tools/build.py {{args}} && python3 tools/launch.py {{args}}

# Ninja Release build under build/rel, package a real SDK, then open setup.
installer *args:
    #!/usr/bin/env bash
    set -euo pipefail
    rel=build/rel
    cmake_extra=(-G Ninja -DNSMBU_SETUP_GUI=ON)
    python3 tools/build.py --release --dir "$rel" --target nsmbu -- "${cmake_extra[@]}" {{args}}
    python3 tools/build.py --release --dir "$rel" --target nsmbu-extract -- "${cmake_extra[@]}" {{args}}
    python3 tools/build.py --release --dir "$rel" --target nsmbu-setup -- "${cmake_extra[@]}" {{args}}
    export NSMBU_INSTALLER_BUILD="$PWD/$rel"
    pkg=$(python3 tools/installer/dev_pkg.py)
    export NSMBU_SETUP_PKG="$pkg/"
    if [[ -x "$pkg/nsmbu-launcher" ]]; then
      exec "$pkg/nsmbu-launcher" --setup
    fi
    exec "$rel/nsmbu-setup" --setup

launch *args:
    python3 tools/launch.py {{args}}

record course *args:
    mkdir -p "{{justfile_directory()}}/replays"
    NSMBU_RECORD_INPUT="{{justfile_directory()}}/replays/{{course}}.replay" python3 tools/launch.py {{course}} {{args}}

replay course *args:
    NSMBU_REPLAY_INPUT="{{justfile_directory()}}/replays/{{course}}.replay" python3 tools/launch.py {{course}} {{args}}
