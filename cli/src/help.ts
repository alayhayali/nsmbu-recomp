export const HELP = `agent-nsmbu — NSMBU developer and agent CLI

Usage:
  agent-nsmbu build [--release] [--sanitizer] [cmake args…]
  agent-nsmbu launch [--no-debug] [--trace] [--gdb] [--lldb] [slot|course] […]
  agent-nsmbu run [--release] [slot|course] […]
  agent-nsmbu extract […]
  agent-nsmbu record <course> […]
  agent-nsmbu replay <course> […]
  agent-nsmbu profile […]
  agent-nsmbu perf […]
  agent-nsmbu installer [cmake args…]
  agent-nsmbu lint
  agent-nsmbu recomp
  agent-nsmbu scripted <name> […]

Agent control (single game instance):
  agent-nsmbu open [slot|course] […]
  agent-nsmbu status
  agent-nsmbu press <button|hex> [--frames N]
  agent-nsmbu stick <x> <y> [--frames N]
  agent-nsmbu rstick <x> <y> [--frames N]
  agent-nsmbu screenshot [path]
  agent-nsmbu wait --frame N | --step N [--timeout SEC]
  agent-nsmbu quit | close
`;
