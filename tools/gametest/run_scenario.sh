#!/usr/bin/env bash
# run_scenario.sh OUT [VAR=value ...]
# One scripted, hidden, muted run for compare.py. The game boots on a copy of the save, and the run
# stops STEPS logic steps (default 300) after the first logic step of scene ALIGN (default Title).
# OUT holds status.txt (one line per logic step), log, console.log and perf.txt.
# User folder, shader cache and states live under OUT. Extra VAR=value arguments go to the game's
# environment (NSMBU_PRESS, NSMBU_STICK, ... count from the align scene when FIXED_SEED is on).
# Environment: NSMBU_BIN, GAME, SAVE, ALIGN, STEPS, FIXED_SEED (default 1), SHADER_CACHE,
# BOOT_TRIES (default 3), BOOT_LIMIT seconds to reach the scene (default 240), RUN_LIMIT seconds
# for the scene part (default 300). A boot that never reaches the scene is retried and its log is
# kept next to OUT as OUT.bootfailN.log.
set -u
root=$(cd "$(dirname "$0")/../.." && pwd)
if [ $# -lt 1 ]; then
    echo "usage: run_scenario.sh OUT [VAR=value ...]" >&2
    exit 2
fi
out=$(realpath -m "$1")
shift
game=${GAME:-$root/game}
save=${SAVE:-}
if [ -z "$save" ]; then
    save="$root/nsmbu/save"
    if [ ! -d "$save" ] && [ -d "$root/user/save" ]; then
        save="$root/user/save"
    fi
fi
align=${ALIGN:-Title}
steps=${STEPS:-300}
boot_tries=${BOOT_TRIES:-3}
boot_limit=${BOOT_LIMIT:-240}
run_limit=${RUN_LIMIT:-300}
bin=${NSMBU_BIN:-}
if [ -z "$bin" ]; then
    for base in "$root/build/debug" "$root/build/release" "$root/build/rel" "$root/build"; do
        if [ -f "$base/nsmbu" ]; then
            bin="$base/nsmbu"
            break
        fi
        if [ -f "$base/nsmbu.exe" ]; then
            bin="$base/nsmbu.exe"
            break
        fi
    done
fi
if [ -z "$bin" ] || [ ! -f "$bin" ]; then
    echo "missing nsmbu binary; run: nsmbu build (or set NSMBU_BIN)" >&2
    exit 2
fi
if [ ! -d "$save" ]; then
    echo "missing save folder $save" >&2
    exit 2
fi

pid=""
stop_game() {
    if [ -n "$pid" ]; then
        kill "$pid" 2>/dev/null
        sleep 1
        kill -9 "$pid" 2>/dev/null
        wait "$pid" 2>/dev/null
        pid=""
    fi
}
trap stop_game EXIT INT TERM HUP

align_step() {
    local line
    line=$(grep -m1 '\[test\] align at logic step' "$out/log" 2>/dev/null) || line=""
    if [ -n "$line" ]; then
        echo "$line" | sed -n 's/.*logic step \([0-9][0-9]*\).*/\1/p'
        return 0
    fi
    awk -v s="$align" 'tolower($2) == tolower(s) { print $1; exit }' "$out/status.txt" 2>/dev/null
}

last_step() {
    tail -n 1 "$out/status.txt" 2>/dev/null | awk '{ print $1 }'
}

done_run=n
aligned=n
for attempt in $(seq 1 "$boot_tries"); do
    rm -rf "$out"
    mkdir -p "$out/user" "$out/states"
    cp -a "$save" "$out/save"
    : > "$out/status.txt"
    cd "$out" || exit 2
    env NSMBU_HIDDEN_WINDOWS=1 NSMBU_NO_HOST_INPUT=1 NSMBU_NO_AUDIO=1 NSMBU_NO_GAMEPAD=1 \
        NSMBU_VK_PRESENT_MODE=immediate NSMBU_RES_SCALE="${NSMBU_RES_SCALE:-1}" \
        NSMBU_USER_DIR="$out/user" NSMBU_SHADER_CACHE="${SHADER_CACHE:-$out/shadercache.bin}" \
        NSMBU_STATE_DIR="$out/states" NSMBU_LOG_FILE="$out/log" \
        NSMBU_TEST_STATUS_TRACE="$out/status.txt" NSMBU_TEST_ALIGN_SCENE="$align" \
        NSMBU_TEST_FIXED_SEED="${FIXED_SEED:-1}" "$@" \
        "$bin" --game "$game" --save "$out/save" > "$out/console.log" 2>&1 &
    pid=$!
    t0=$(date +%s)
    aligned=n
    start=""
    for _ in $(seq 1 $((boot_limit * 10))); do
        kill -0 "$pid" 2>/dev/null || break
        start=$(align_step)
        if [ -n "$start" ]; then
            aligned=y
            break
        fi
        sleep 0.1
    done
    t1=$(date +%s)
    done_run=n
    if [ "$aligned" = y ]; then
        for _ in $(seq 1 $((run_limit * 10))); do
            kill -0 "$pid" 2>/dev/null || break
            last=$(last_step)
            if [ -n "$last" ] && [ "$last" -ge $((start + steps)) ] 2>/dev/null; then
                done_run=y
                break
            fi
            sleep 0.1
        done
    fi
    t2=$(date +%s)
    stop_game
    printf "wall_boot_s %s wall_scenario_s %s aligned %s done %s attempt %s\n" \
        "$((t1 - t0))" "$((t2 - t1))" "$aligned" "$done_run" "$attempt" > "$out/perf.txt"
    if [ "$aligned" = y ]; then
        break
    fi
    cp "$out/log" "$out.bootfail$attempt.log" 2>/dev/null || cp "$out/console.log" "$out.bootfail$attempt.log"
done
rm -rf "$out/save" "$out/states"
echo "$(basename "$out"): aligned=$aligned done=$done_run $(wc -l < "$out/status.txt" | tr -d ' ') status lines"
grep -iE "halt|crash|assert|fatal" "$out/log" 2>/dev/null | head -3
[ "$aligned" = y ] && [ "$done_run" = y ]
