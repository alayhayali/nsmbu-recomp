nsmbu_dir := justfile_directory() / "nsmbu"
export NSMBU_USER_DIR := nsmbu_dir

default:
    @just --list

build *args:
    python3 tools/build.py {{args}}

run *args:
    python3 tools/build.py {{args}} && python3 tools/launch.py {{args}}

launch *args:
    python3 tools/launch.py {{args}}

record course *args:
    mkdir -p "{{justfile_directory()}}/replays"
    NSMBU_RECORD_INPUT="{{justfile_directory()}}/replays/{{course}}.replay" python3 tools/launch.py {{course}} {{args}}

replay course *args:
    NSMBU_REPLAY_INPUT="{{justfile_directory()}}/replays/{{course}}.replay" python3 tools/launch.py {{course}} {{args}}
