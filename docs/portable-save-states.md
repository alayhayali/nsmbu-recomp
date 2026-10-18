# Portable save states

The small portable state is unused.
The overlay help in `runtime/src/overlay/overlay.cpp` says the game writes a full save state.
That help says the small portable state is unused.

Save states come in two kinds that share the five slots:

| | Portable (default) | Full (debugging) |
|---|---|---|
| File | `slotN.nsstate` | `slotN.bin` |
| Size | a few KB (refused above 64 KB) | about 270–300 MB |
| Holds | the save data of the current save file, the player's place, a header | all guest memory (game code, decompressed assets), threads, HLE state |
| Shareable | yes, attach it to bug reports | **never**: it contains game code and data |
| Loads into | any session of the same game, any build | only the same build, exactly where it was saved |
| Exact | no (see below) | yes |

Both live in the states folder (its path is shown in the Saves tab; releases keep it in `data/nsmbu`, builds from source
use `nsmbu/states/` with `nsmbu launch` / `nsmbu run`, or `~/Library/Application Support/nsmbu/states/` on macOS and the configuration folder elsewhere; `NSMBU_STATE_DIR` overrides it). Loading a slot loads whichever kind it holds (the newer file if it
holds both). Crash Recovery's automatic states are always full states.

## Choosing the kind

- **Save** (Saves tab of the settings overlay, Shift+F1–F5, the macOS Save States menu) makes a
  portable state, **only while you control the player character** (see below).
- *Full save states (large, contain game data, don't share) – for debugging* in the Saves tab (or
  the Save States menu on macOS) switches Save to full states; the choice is kept in
  `states/full_save_states.cfg`.
- `NSMBU_FULL_SAVE_STATES=1` / `=0` decides for one start. The scripted-test variables
  `NSMBU_STATE_SAVE_AT`, `NSMBU_STATE_LOAD_AT`, `NSMBU_TEST_SAVE` and `NSMBU_TEST_LOAD` keep using full
  states (unless `NSMBU_FULL_SAVE_STATES=0`).

A slot can hold both kinds: saving one kind never deletes the other. The slot loads (and shows) the
newer one; when a portable state is the newer one and an older full state (`slotN.bin`) is still
there, the Saves tab says so under the slot ("also holds an older full state (slotN.bin, 273 MB)")
and the log notes it when saving, so the large file is not forgotten (delete it in the states
folder if you no longer need it).

## When a portable state can be saved

Only while the player controls the player character, under the same conditions under which the game
itself opens its pause menu: no event or cutscene running (and not for 5 frames after one), no
message or dialogue box, no game menu open, no stage change or scene transition in progress, the
player character is the controlled actor, and the player is not on a rope-like actor (the play
state's player status bit 0x00800000, which only those procedures set): they would restart in
mid-air. Otherwise nothing is written; the screen shows
"can't save during a cutscene or dialogue - try again when you have control of the player" and the
log names the reason (`[savestate] slot N: portable state refused: ...`). This covers the Save button
and Shift+F1–F5. Full save states and Crash Recovery's automatic states can be made at any time.
Loading is not affected.

## Bug reports

The Saves tab's **Copy save for bug report** copies the paths of the newest portable state and of the
save files (`rp_personal_savedata.dat` and `rp_savedata.dat`). The issue template asks for them. A
developer loads a received state by copying it into the states folder as `slotN.nsstate` and loading
slot N, or with `NSMBU_PORTABLE_LOAD=<file>` (applied as soon as a save file is being played).

## What a portable state holds

A UTF-8 text file of `key = value` lines (`runtime/src/save/portable_state.h`):

- header: `format` (1), `title_id` and `title_version` (from `meta/meta.xml`), `game_hash` (a hash
  of `red-pro2.rpx`, to tell executables apart; not its contents), `runtime` (version and commit),
  `created`, `file_slot` (save file 0–2), `player_name`;
- place: `stage`, `start_point`, `start_room`, `layer` (how the stage was entered), `room` (the
  player's room), `player_pos`, `player_angle_y` (shape angle), `player_proc`, `on_ship` (the player
  rides the ship), `has_ship`, `ship_pos`, `ship_angle_y` (the ship, when it is in the stage),
  `time_of_day` and `date` (day counter; day of week = date % 7);
- `savedata`: the save data block, made by the game's own save functions: the current stage's save
  is put into the save memory and the game's own start-stage save step runs as it does before
  writing (both undone afterwards, so making a state changes nothing in the game), then the block is
  written out. Inventory, flags, progress, time of day;
- `hd_player`, `hd_status`, `hd_event`, `hd_map`: the per-file save sections (16, 4, 20 and 220
  bytes), the stored ones with the live data copied over by the game's own copy functions;
- `checksum`: CRC-32 of everything before it.

Weather is not recorded: it follows from the progress, the place and the time of day.

**Guard**: the writer and the reader accept only these fields, the binary ones at exactly these
sizes, text values up to 128 characters and files up to 64 KB; a file that breaks any of this is
not written, and not read. A state can therefore never carry a memory dump.
`runtime/tools/portable_state_test.cpp` tests the format (round trip, version mismatch, both
checksums, the size guard, unknown or repeated fields).

## Loading

At the frame boundary on the game's main thread, once a save file is being played (the title
screen and file select wait):

1. the save data goes into the game, the current stage's save makes the stage memory the loaded one
   (the stage change puts it back), the dungeon bits and temporary flags start fresh, the per-file
   sections are copied in, and the game's refresh sets the item buttons and equipment from the
   loaded data;
2. the player's room, position and angle go into the game's restart info (as when the game restarts
   a room after a fall), and the next stage is the recorded one with spawn point -1, the recorded
   room and layer.
   With the ship in the stage (on it, or beside it), the turn restart is used instead: it gets the
   player's and the ship's position and heading with spawn point -3, the ship is put back, and the
   player starts on it or standing beside it.

The log then says `[savestate] portable load: arrived in <stage> room <n> at x y z (distance d from
the saved position)` (with the ship also where the ship is and whether the player is on it).

The state goes into the save file being played. When it was made in another save file, the screen
shows "This state is from save file N; it is loaded into save file M (saving in game will write it
there)" for a few seconds (and the log notes it); loading goes ahead.

Save-state notices (saved, refused, loaded, this one) show at the bottom of the picture for a few
seconds and in the macOS window title.

**Not restored** (it is not a snapshot): enemies, items lying around, moving platforms and other
actor state start as the stage starts them; a running cutscene, dialogue or minigame is not resumed;
the player starts standing, or aboard the ship when it is in the stage; the camera starts behind the
player; the file is loaded into the current save file of the session (saving in game afterwards
writes it there; `file_slot` only drives the notice above).

Controller metadata: format 1 accepts optional `controller = 1` (GamePad) or `controller = 2` (Pro Controller). Loading restores that host input mode. Missing metadata or `0` leaves the current mode unchanged. Unknown values are refused; the existing field and total-size guards still apply.
