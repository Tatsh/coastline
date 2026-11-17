# Command line

`coastline --help` lists the options and the subcommands, and `coastline SUBCOMMAND --help` lists
the options of a subcommand. Without a subcommand the game starts. The options of the game come
before the subcommand. `--debug` logs the debug messages of every category, and `--debug-gpu`
checks the use of the GPU with the validation layers of Vulkan when they are installed.

```shell
coastline --debug --width 640 --height 480
```

These are the subcommands:

- `import` converts a disc image and cartridge images ([Importing game data](import.md)).
- `export-pak` and `import-pak` exchange saves with a Controller Pak image
  ([Data and saves](data.md)).
- `migrate-portable` and `make-portable` move data between portable and platform storage
  ([Data and saves](data.md)).
- `run-script` drives the game with scripted input.

## Scripts

`run-script` starts the game at the splash screens, driven by scripted input, for unattended checks.
With no filename, or with `-`, the command reads standard input and waits for EOF before starting.
Use `coastline run-script < check.txt` to redirect a file, or finish interactive input with Ctrl-D
on Unix or Ctrl-Z followed by Enter on Windows.
Each line is a command, `#` starts a comment, and a blank line is ignored. A
line that starts with a frame number, from 1, acts at the frame. These are the commands:

- `autopilot` lets the computer drive the car of the first player in a race. It takes no frame.
- `screenshot-every N` saves every N frames. It takes no frame.
- `FRAME press BUTTON [FRAMES [PORT]]` presses a button of a controller for 6 frames or `FRAMES`.
  `BUTTON` is one of `a`, `b`, `z`, `start`, `l`, `r`, `up`, `down`, `left`, `right`, `cu`, `cd`,
  `cl`, or `cr`, and `PORT` is the controller from 1, the first unless given. A press of another
  controller connects it. In a race, on the first controller, `a` and `up` accelerate, `b` and
  `down` brake, `left` and `right` steer, `start` pauses, `z` is the start binding, `cu`, `cl`, and
  `cd` choose the cockpit, the bumper, and the chase camera, and `cr` chooses an item of the pause
  menu.
- `FRAME key KEY [FRAMES]` presses a key of the keyboard for 6 frames or `FRAMES`. `KEY` is the SDL
  name of the key, such as `Return`, `Escape`, or `F9`. `Ctrl`, `Alt`, `Shift`, and `Gui` are aliases
  for modifier keys. Overlapping key durations produce shortcuts such as Ctrl+A.
- `FRAME type TEXT` types the rest of the line while a screen takes text, or presses Backspace or
  Enter for `<backspace>` or `<enter>`.
- `FRAME mouse X Y [BUTTON [FRAMES]]` puts the mouse at a place of the 320 by 240 screen for 1
  frame or `FRAMES`, with `BUTTON` (`left`, `right`, or `none`) pressed. The game reads the real
  mouse when the script does not move it.
- `FRAME screenshot` saves the frame.
- `FRAME quit` exits after the frame.

The frames save to `screenshots` in the data directory as `frame-NNNNNN.png`, by the number of the
frame. A script that saves frames draws them at `Width` by `Height` and steps the splash screens
by a 60th of a second a frame, for the same captures on every machine. A line that cannot be read
stops the game before it opens a window, with the file and the line logged.

```text
autopilot
screenshot-every 60
120 press a
200 press start 12        # 12 frames on the first controller
300 key F9 6
400 type HELLO
450 type <backspace>
500 mouse 100 80 left 1   # X and Y of the 320 by 240 screen
600 screenshot
900 quit
```

```shell
coastline --minimized run-script check.txt
```

`--minimized` exists only in debug builds.

Without game data, a script that sends keys, text, or mouse input drives the
[setup wizard](first-run.md). A script without such input stops with an error.
