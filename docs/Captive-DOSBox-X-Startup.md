# Captive DOSBox-X startup

This is the repeatable startup procedure for testing Captive with the
original game data. It deliberately uses the real `CAPTIVE.BAT 1` chain and
the repository's VGA profile. Do not start `CAPPO.EXE` directly.

## Normal daily launch

From the repository root, run:

```sh
tools/run_captive_dosbox_x.sh /Users/bosse/.opencaptive/captivedebug/captive
```

The helper selects `/opt/homebrew/bin/dosbox-x` on Apple Silicon when it is
available. To use another verified DOSBox-X binary, set `DOSBOX_X_BIN`:

```sh
DOSBOX_X_BIN=/path/to/dosbox-x \
  tools/run_captive_dosbox_x.sh /path/to/authentic/captive-data
```

The data directory must contain the original Captive files, including
`CAPTIVE.BAT`. OpenCaptive must not create replacement maps, planets,
landings, droids, or dungeon data when these files are missing.

The graphical OpenCaptive start menu launches the same authentic DOSBox-X
runtime in its own window. OpenCaptive does not create a second gameplay
surface or a replacement state underneath it.

## Exact repeatable startup

Use this order every time:

```sh
cd /Users/bosse/Documents/OpenCaptive
tools/run_captive_dosbox_x.sh /Users/bosse/.opencaptive/captivedebug/captive
```

Then, in the DOSBox-X window:

1. Choose `1` (**VGA**) at the video-mode prompt.
2. Click inside the game viewport once so DOSBox-X owns keyboard and mouse focus.
3. Let the original intro run until Captive accepts the normal input.
4. Use `Ctrl+F10` only to release or recapture the DOSBox-X mouse lock.

Never launch `CAPPO.EXE` directly. The batch file performs the original
startup/unpack chain and selects the real game data. Starting the executable
directly is a different, unsupported state and is a common cause of a black,
stalled, or incorrect viewport.

## Daily reset checklist

Use this short checklist when a previous DOSBox-X session has been left open:

1. Close the old DOSBox-X window. If macOS asks whether to quit a running
   program, choose **No** for the window you still want to use; choose **Yes**
   only when deliberately closing the stale session.
2. If that warning stays on top, do not send game keys yet. Bring the warning
   forward, dismiss it, and click the new DOSBox-X viewport once. A warning
   window left above the viewport captures the keyboard and makes the game
   appear frozen.
3. Start OpenCaptive or run the helper command above.
4. Confirm that the new window title is **DOSBox-X** and that it shows the
   Captive video-mode prompt.
5. Click the game viewport, press `1` for VGA, and wait for the original
   intro.

Do not reuse a window that is already showing a dungeon, a debugger prompt, or
an old planet. A fresh launch is required for each startup verification.

## First screen and keyboard focus

1. Wait until DOSBox-X displays **Please Select your Video Mode**.
2. Click once inside the DOSBox-X game viewport. This gives the SDL window
   keyboard focus; clicking only the title bar is not sufficient.
3. Press `1` for **VGA**.
4. Wait for the original Captive title/intro to finish loading.

If the number is ignored, click inside the viewport again and press `1`.
There is no debugger prompt in a normal launch.

When testing with the helper, the only supported launch command is:

```sh
tools/run_captive_dosbox_x.sh /Users/bosse/.opencaptive/captivedebug/captive
```

The helper mounts that directory as `C:`, changes to `C:`, and runs
`CAPTIVE.BAT 1`. This is the original startup path. Do not add a second
`CAPPO.EXE` command, do not copy generated files into the data directory, and
do not use a debugger launch for an interactive test.

DOSBox-X may capture the system pointer when the game starts. Press
`Ctrl+F10` to release or recapture the pointer. This is a DOSBox-X shortcut,
not a Captive command.

## Space navigation sequence

Captive's original controls are still owned by the DOS runtime:

- keypad `7`: **ORBIT**
- keypad `9`: **LAND**
- keypad arrows: steer the ship
- the on-screen arrow buttons: the same navigation controls

The blinking green marker identifies the destination planet. After reaching
that planet, the white circle identifies the landing point. Press **ORBIT**
and wait for the ship to enter the destination planet's orbit before pressing
**LAND**. Landing early is rejected by the original game. If the selected
point is surrounded by water, it is the wrong landing point; return to orbit
and find the white landing circle.

The zoom keys change the original navigation state. They must not be treated
as a post-process pixel enlargement of the rendered frame.

The original keyboard mapping used by the game is:

| Key | Original action |
|---|---|
| keypad `7` | ORBIT; begin travel toward the selected planet |
| keypad `9` | LAND; valid only after arrival in orbit |
| keypad `8` | move forward |
| keypad `2` | move backward / descend |
| keypad `4` / `6` | turn left / right |
| keypad `1` / `3` | climb / descend ladder in the landed view |
| keypad `5` | no movement |

The on-screen navigation buttons must call these same original actions. A
button that only enlarges the last image is not navigation and is a failure.

The tested route has four separate states:

```text
planet marker selected -> FLIGHT PATH SET -> ARRIVED AT DESTINATION / NOW IN ORBIT -> LAND
```

`ORBIT` starts the flight. It does not mean that the ship has already arrived.
Wait until the original runtime reports or visibly enters the destination orbit;
only then use `LAND`. `LAND` is not a shortcut into a dungeon. A successful
landing must produce the original landed view and its real local terrain; a
water-only view is evidence that the destination point was wrong.

## Normal launch versus diagnostics

For interactive play, use the helper command above or launch Captive from the
OpenCaptive start menu with authentic data. The DOSBox-X window owns the
original game, its viewport, and its input. OpenCaptive's launcher only starts
that process; it is not a replacement gameplay surface. Do not add these
debugger options to either normal path:

- `-break-start`
- `-set debuggerrun=debugger`
- the FIFO/`expect` diagnostic harnesses

Those options intentionally pause CAPPO in the DOSBox-X debugger and are for
memory dumps, disassembly, and exact VGA comparisons. They are not evidence
of working mouse input, orbit, landing, or dungeon navigation.

For a reproducible startup-only diagnostic, use:

```sh
tools/captive_dosbox_intro.expect \
  /Users/bosse/.opencaptive/captivedebug/captive
```

For the current real-data Mission 0001 route gate, use:

```sh
tools/verify_captive_target_route.sh \
  /Users/bosse/.opencaptive/captivedebug/captive
```

That gate currently proves the authentic `FLIGHT PATH SET` boundary. It does
not claim that the automated route has reached `ARRIVED AT DESTINATION`,
`NOW IN ORBIT`, or `LANDING SUCCESSFUL`.

The automated ORBIT-dispatch probe is not part of the verified release path:
direct debugger queue injection is not equivalent to a real keyboard event and
must not be used as parity evidence.

## Troubleshooting

| Symptom | Correct action |
|---|---|
| VGA choice does nothing | Click inside the game viewport, then press `1`. |
| Debugger window appears | Quit and restart with the normal helper; remove debugger flags. |
| System pointer moves but Captive pointer does not | Click the DOSBox-X viewport and check `Ctrl+F10`; do not use a paused diagnostic session as a mouse test. |
| The ship is still in transit | Wait for the original orbit/arrival state before pressing `9` for LAND. |
| A landed view contains only water | Restart from orbit and fly to the white landing circle. |
| The viewport looks stretched or corrupted | Use the repository profile; it forces VGA, surface output, integer scaling, and the CAPPO-compatible VGA memory setting. |
| An old or unexpected game state appears | Close every old DOSBox-X window and follow the Daily reset checklist. |
