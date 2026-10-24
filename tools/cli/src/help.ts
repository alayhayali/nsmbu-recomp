export const HELP = `nsmbu — NSMBU developer and agent CLI

Usage:
  nsmbu build [--release] [--sanitizer] [cmake args…]
  nsmbu launch [--no-debug] [--trace] [--gdb] [--lldb] [slot|course] […]
  nsmbu crash
  nsmbu wine [--no-debug] [--trace] [slot|course] […]
  nsmbu run [--release] [slot|course] […]
  nsmbu extract […]
  nsmbu save <save.dat|save.json> [output] [--output PATH]
  nsmbu record <course> [name] […]
  nsmbu replay [--release] [--uncapped] [--no-exit-on-input] [--exit-at N] [--log PATH] <name> […]
  nsmbu profile […]
  nsmbu perf […]
  nsmbu installer [--version VERSION]
  nsmbu lint
  nsmbu recomp
  nsmbu scripted <name> […]
  nsmbu completion fish [--install]

Agent control (single game instance):
  nsmbu open [slot|course|course-file] […]
  nsmbu status
  nsmbu press <button|hex> [--frames N]
  nsmbu stick <x> <y> [--frames N]
  nsmbu rstick <x> <y> [--frames N]
  nsmbu screenshot [path]
  nsmbu eval [code] [--file PATH] [--timeout MS]
  nsmbu wait --frame N | --step N [--timeout SEC]
  nsmbu quit | close
`;
