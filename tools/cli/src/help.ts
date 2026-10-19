export const HELP = `nsmbu — NSMBU developer and agent CLI

Usage:
  nsmbu build [--release] [--sanitizer] [cmake args…]
  nsmbu launch [--no-debug] [--trace] [--gdb] [--lldb] [slot|course] […]
  nsmbu run [--release] [slot|course] […]
  nsmbu extract […]
  nsmbu record <course> […]
  nsmbu replay [--release] [--uncapped] [--exit-at N] [--log PATH] <course> […]
  nsmbu profile […]
  nsmbu perf […]
  nsmbu installer [cmake args…]
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
  nsmbu wait --frame N | --step N [--timeout SEC]
  nsmbu quit | close
`;
