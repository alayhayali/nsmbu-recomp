---
name: logs
description: Log file locations for bug reports, crashes, setup failures, and the nsmbu CLI run log. Use when reading player or local logs, attaching evidence to an issue, or finding nsmbu.log / crash-*.log / setup.log.
---

Player and developer paths are in [docs/logs.md](../../../docs/logs.md).

In a source checkout with the `nsmbu` CLI, the current run log is `nsmbu/logs/nsmbu.log`.
The previous run is `nsmbu/logs/nsmbu-previous.log`.
Crash dumps in that folder are `crash-YYYYMMDD-HHMMSS.log`.

Issue attachments: download into `.tmp/issues/`.
Guest `lr` / `ctr` / call-chain mapping is in the `guest` skill (Crash log addresses).
