# Log files for bug reports

Attach the newest useful log when you open a GitHub issue.
Prefer the crash log after a crash, the run log for graphics or gameplay problems, and the setup log when install or start fails.
Never attach full save states (`slotN.bin`).
They contain game code and data.
Portable states and save files are covered in [portable-save-states.md](portable-save-states.md).

## Which file to attach

| Situation | File | Notes |
| --- | --- | --- |
| Game crash | `crash-YYYYMMDD-HHMMSS.log` | Pick the newest stamp in the logs folder |
| Problem while playing (no crash) | `nsmbu.log` | Current run; the previous run is `nsmbu-previous.log` |
| Installer / first start failed | `setup.log` | Use **Copy log** in the setup window when you can |
| Setup window itself failed | `setup-window.log` | Or the fallback path below when `data/` is not writable |

Keys never appear in these logs.

## Game logs folder

Run and crash logs live under `…/logs/` next to settings (`host::log_dir()`).

| How you run the game | Logs folder |
| --- | --- |
| Portable release (zip with `portable.txt`; usual macOS / Windows / Linux zip) | `<release>/data/nsmbu/logs/` |
| Linux AppImage (no `portable.txt`) | `~/.config/nsmbu/logs/` |
| Non-portable install on Windows | `%APPDATA%\NSMBU\logs\` |
| Non-portable install on macOS | `~/Library/Application Support/NSMBU/logs/` |
| Built from source (`nsmbu launch` / `nsmbu run`) | `<repo>/nsmbu/logs/` |
| Android | `Android/data/org.nsmbrecomp.nsmbu/files/config/nsmbu/logs/` |

On each start the previous `nsmbu.log` is renamed to `nsmbu-previous.log`.
Then a new `nsmbu.log` is opened.
Crash handlers write `crash-<date>-<time>.log` into the same folder.
Set `NSMBU_LOG_FILE` to write somewhere else, or to `0` to skip the file.
On Android, **Share log** in the setup app shares `nsmbu.log` and `nsmbu-previous.log`.

## Setup logs

The installer writes `setup.log` in the data folder.

| How you install | Setup log |
| --- | --- |
| Portable release | `<release>/data/setup.log` |
| Linux AppImage | `~/.local/share/nsmbu/setup.log` (or `$XDG_DATA_HOME/nsmbu/setup.log`) |
| Non-portable Windows | `%LOCALAPPDATA%\NSMBU\setup.log` |
| Non-portable macOS | `~/Library/Application Support/nsmbu/setup.log` |

When the setup window cannot continue, it also appends to `setup-window.log` in that same data folder.
If the data folder is not writable, it falls back to:

| System | Fallback |
| --- | --- |
| macOS | `~/Library/Logs/NSMBU setup.log` |
| Windows | `%TEMP%\NSMBU setup.log` |
| Linux | `$TMPDIR/nsmbu-setup-log.log` (or `/tmp/nsmbu-setup-log.log`) |

`--data-dir` / `NSMBU_DATA_DIR` moves the data folder.
`setup.log` follows it.

## Source and developer overrides

`NSMBU_USER_DIR` (set by the `nsmbu` CLI to `<repo>/nsmbu`) moves settings and the logs folder with it.
`portable.txt` next to the game binary wins over `NSMBU_USER_DIR` and keeps logs under that release's `nsmbu/logs`.
Profile and replay tools may set `NSMBU_LOG_FILE` to a path under `.tmp/` (see [performance.md](performance.md) and [automation.md](automation.md)).
Those paths are for local measurement, not for player bug reports.
