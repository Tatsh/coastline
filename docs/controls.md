# Controls

## Keyboard

The keyboard drives the menus as the controller of the console. The arrow keys move, X is A, C is
B, Z is Z, Enter is Start, A and S are L and R, and I, K, J, and L are the C buttons. The keypad
arrows are the directional pad.

A race reads the keyboard bindings of Boss Rally. The arrow keys steer, accelerate, and brake,
Right Ctrl brakes, A and Z change gear, and Escape pauses.

## Keyboard shortcuts

Ctrl+Z undoes the last paint-shop edit outside a dialog. In the main menu, press Up, Up, Down,
Down, Left, Right, Left, Right, L, and R on the first controller to unlock Season Champion in
track selection for the current game session. Directions are read from the directional pad, the
stick, or the arrow keys. On the keyboard, A and S enter L and R. Season Champion is the unfinished
course of the cutscene after a season (`gamewin`), with incomplete collision.

Ctrl+Shift+R restarts the game from its intro without confirmation.

[Display](display.md) lists the keys of the window, the frame rate, and screenshots.

## Free camera

F8 toggles the free camera throughout the application, including setup, movies, menus, logos,
and paused or running races. Flat screens become panels viewed in perspective while moving.
Move with W, A, S, and D, rise with E, descend with Q, and look with the mouse. Two-finger vertical
scrolling or the mouse wheel moves forward and backward. Shift increases movement speed.
Free camera draws the whole track without distance fog or changes to saved settings.
Escape exits free camera without pausing or navigating back; F8 also toggles the mode off.
On a gamepad, Back toggles the camera, the left stick moves, the right stick looks,
the shoulder buttons descend and rise,
and pressing the left stick increases speed. Menu, driving, and text input remain active.
The mouse controls the camera while free camera is enabled. The HUD disappears for screenshots.

## Gamepad bindings

The `Controls` section of `BossRally.ini` maps the gamepad. `Preset` chooses one of the presets below,
and a key of an action present in the section replaces the binding of the preset for the action.
A value lists SDL gamepad names separated by commas (`a`, `b`, `x`, `y`, `back`, `start`,
`leftshoulder`, `rightshoulder`, `leftstick`, `rightstick`, `dpup`, `dpdown`, `dpleft`, `dpright`,
`lefttrigger`, `righttrigger`, `leftx`, `lefty`, `rightx`, and `righty`). `a`, `b`, `x`, and `y`
are the south, east, west, and north buttons. A `+` or `-` after an axis selects one half of it,
and an axis without a sign reads its positive half. An axis steers and accelerates in proportion,
and a button gives the full amount. An empty value unbinds the action. The log records each name
that is not a button or an axis, and the game ignores it.

`DeadZone` is the share of the travel of each axis that reads as 0, from `0` to `0.9`, and the
rest of the travel reads as it is. The default of `0.07` matches the dead zone of the steering of
the Nintendo 64 release. A worn stick that steers or cancels the directional pad at rest needs a
larger value.

The car of a player steers by the handling chosen on the car select screen, as the Nintendo 64
release does. The wheels turn towards the stick at a rate of the handling and return to the middle
at once, and a stick pushed past 72% of its travel takes the whole lock. The lock narrows with the
speed. The computer's cars and the `autopilot` of a script steer as in the PC release.

```ini
[Controls]
DeadZone = 0.07
Preset = Default
Accelerate = a,righttrigger
SteerLeft = leftx-,dpleft
```

These are the keys and the bindings of each preset.

| Key               | `Default`                    | `Kart`                       |
| ----------------- | ---------------------------- | ---------------------------- |
| `SteerLeft`       | `leftx-,dpleft`              | `leftx-,dpleft`              |
| `SteerRight`      | `leftx+,dpright`             | `leftx+,dpright`             |
| `Accelerate`      | `a,righttrigger`             | `b`                          |
| `Brake`           | `b,lefttrigger`              | `a`                          |
| `Reverse`         | `x`                          | `x`                          |
| `GearUp`          | `rightshoulder`              | `rightshoulder,righttrigger` |
| `GearDown`        | `leftshoulder`               | `leftshoulder,lefttrigger`   |
| `CockpitCamera`   | `righty-`                    | `righty-`                    |
| `BumperCamera`    | `rightx-`                    | `rightx-`                    |
| `ChaseCamera`     | `righty+,y`                  | `righty+,y`                  |
| `PauseMenuUp`     | `lefty-,dpup`                | `lefty-,dpup`                |
| `PauseMenuDown`   | `lefty+,dpdown`              | `lefty+,dpdown`              |
| `PauseMenuSelect` | `a`                          | `b`                          |
| `Pause`           | `start`                      | `start`                      |
| `StartOrRespawn`  | `back`                       | `back`                       |
| `MenuA`           | `a`                          | `b`                          |
| `MenuB`           | `b,x`                        | `a,x`                        |
| `MenuZ`           | `lefttrigger`                | `lefttrigger`                |
| `MenuStart`       | `start`                      | `start`                      |
| `MenuL`           | `leftshoulder`               | `leftshoulder`               |
| `MenuR`           | `rightshoulder,righttrigger` | `rightshoulder,righttrigger` |
| `MenuCUp`         | `righty-`                    | `righty-`                    |
| `MenuCDown`       | `righty+`                    | `righty+`                    |
| `MenuCLeft`       | `rightx-`                    | `rightx-`                    |
| `MenuCRight`      | `rightx+`                    | `rightx+`                    |
| `MenuDUp`         | `dpup`                       | `dpup`                       |
| `MenuDDown`       | `dpdown`                     | `dpdown`                     |
| `MenuDLeft`       | `dpleft`                     | `dpleft`                     |
| `MenuDRight`      | `dpright`                    | `dpright`                    |
| `MenuStickLeft`   | `leftx-`                     | `leftx-`                     |
| `MenuStickRight`  | `leftx+`                     | `leftx+`                     |
| `MenuStickUp`     | `lefty-`                     | `lefty-`                     |
| `MenuStickDown`   | `lefty+`                     | `lefty+`                     |

`Kart` follows Mario Kart 8 on the Pro Controller, with the east button accelerating and the south
button braking. A press of `StartOrRespawn` starts a race, and a second press within half a second
resets the car. The menu keys drive the menus, and the race keys drive both players of a race.

In the pause menu, Escape continues the race whatever item is chosen, and `Pause` of the gamepad
chooses the item, as Start does on the console. A race pauses when the window loses the focus.
`PauseOnFocusLoss` in the `[Game]` section is `1`, the default, to pause, or `0` to let the race
run on. A run of `run-script` never pauses on focus loss.

## Nintendo 64 layouts

`TypeA`, `TypeB`, `TypeC`, `TypeD`, and `SteeringWheel` are the button layouts of the Nintendo 64
release, with A of the console on `a`, B on `b`, Z on `lefttrigger`, L and R on the shoulders, and
the C buttons on the right stick. They bind the race keys below and the other keys as `Default`
does. The controller configuration screen of the options menu writes `Preset` with any of the
seven presets. Down reverses without A, and the console's e-brake (A and B together, or C down in
`TypeB`) has no binding.

| Key             | `TypeA`         | `TypeB`         | `TypeC`         | `TypeD`       | `SteeringWheel` |
| --------------- | --------------- | --------------- | --------------- | ------------- | --------------- |
| `SteerLeft`     | `leftx-`        | `leftx-`        | `dpleft`        | `leftx-`      | `leftx-`        |
| `SteerRight`    | `leftx+`        | `leftx+`        | `dpright`       | `leftx+`      | `leftx+`        |
| `Accelerate`    | `a`             | `a`             | `a`             | `lefty-`      | `a`             |
| `Brake`         | `b`             | `b`             | `b`             | `lefttrigger` | `b`             |
| `Reverse`       | `lefty+`        | `lefty+`        | `dpdown`        | `lefty+`      | `lefty+`        |
| `GearUp`        | `rightshoulder` | `rightshoulder` | `rightshoulder` | `b,dpup`      | `rightshoulder` |
| `GearDown`      | `lefttrigger`   | `lefttrigger`   | `leftshoulder`  | `a,dpdown`    | `leftshoulder`  |
| `CockpitCamera` | `righty-`       | `righty-`       | `righty-`       | `righty-`     | `righty-`       |
| `BumperCamera`  | `rightx-`       | `rightx-`       | `rightx-`       | `rightx-`     | `rightx-`       |
| `ChaseCamera`   | `righty+`       | `rightx+`       | `righty+`       | `righty+`     | `righty+`       |

## Players and devices

`Player1` and `Player2` choose the device of each player. A value is `auto`, the default,
`keyboard`, `gamepad1`, `gamepad2`, and so on for the gamepads in the order they connect, a part of
the name of a gamepad (`PS5`, `DualSense`, or `Xbox`), or `none`. With one player, the first player
takes the devices of both. With two players, the gamepads go to the players that are `auto` in
order, and the keyboard goes to the player that is `auto` without a gamepad. The keyboard and one
gamepad are then enough for two players, with the gamepad for the first. The second player of a race
drives with the keyboard bindings of the PC release (the arrow keys, Right Ctrl to brake, A and Z
to change gear). The log records the device of each player at the start and whenever a gamepad
connects or disconnects.

```ini
[Controls]
Player1 = keyboard
Player2 = DualSense
```
