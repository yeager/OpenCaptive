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

## Verification boundary

This procedure proves the original startup chain, VGA mode, intro handoff, and
Mission 0001 route-selection boundary. It does not turn a reference image, a
memory dump, or a locally generated route into evidence of arrival, orbit,
landing, or dungeon entry.

Use only the original runtime and the player's real files for those states.
No replacement planet, landing marker, terrain, dungeon, scrolling status text,
or procedural fallback may be substituted. If the original session has not
visibly reached destination orbit and produced the original landed view, stop
at that boundary and report it as unverified.

The graphical OpenCaptive start menu launches the same authentic DOSBox-X
runtime as a separate normal game window and then closes its launcher process.
This is intentional: DOSBox-X/CAPPO must own the original VGA timing, audio,
mouse, keyboard and game state. The launcher must not keep a second
debugger-backed or native gameplay surface alive behind it.

For debugger-backed input verification, use the repository's diagnostic
sequence harness. It injects make/break bytes through DOSBox-X's emulated AT
keyboard controller and the original CAPPO IRQ1 handler; it does not write a
private CAPPO matrix or create a route, planet, landing point, or dungeon.

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

DOSBox-X captures the system pointer when the game starts so CAPPO can receive
its original INT 33 motion. Press `Ctrl+F10` to release or recapture the
pointer. This is a DOSBox-X shortcut, not a Captive command. The repository
profile deliberately uses `mouse emulation=locked`; `integration` would stop
feeding CAPPO while the pointer is captured.

On a MacBook keyboard, the number row is not the Captive keypad. Use the
original on-screen arrow buttons, or a real/emulated numeric keypad, for
navigation. In particular, `7` and `9` mean ORBIT and LAND only when CAPPO
receives the keypad scan codes; ordinary number-row `7`/`9` input is not a
substitute. This avoids mistaking a key that was never delivered to CAPPO for
a broken flight or landing state.

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

The live DOSBox-X gate has been observed to produce CAPPO's original
`FLIGHT PATH SET` message after ORBIT. Pressing LAND before the ship arrives
then produces the original `SWAN NOT YET IN ORBIT` message. Both messages are
useful diagnostics: they prove that the real input reached CAPPO, but neither
message proves that the destination was reached. Continue only when the
runtime itself shows arrival/orbit.

On macOS, a repeatable manual sequence is:

1. Bring the DOSBox-X window to the front and click once inside the viewport.
2. Choose VGA with the number-row `1` at the startup prompt.
3. Let the original intro finish; press Space only when the intro asks for a
   key.
4. Select the blinking green planet with the original arrow controls.
5. Activate ORBIT and wait for the real transit state to finish.
6. Activate LAND only after the original runtime confirms orbit.

Do not use a number-row `7`/`9` as a substitute for the keypad commands on a
MacBook. Use the on-screen controls or a real/emulated numeric keypad so the
DOS keypad scan codes reach CAPPO.

The valid evidence sequence is:

```text
green planet marker -> ORBIT -> original transit state -> destination orbit
-> white landing circle -> LAND -> original dungeon view
```

Do not continue by selecting a hard-coded coordinate or fabricating a dungeon
when one of these transitions is missing.

## Normal launch versus diagnostics

For interactive play, use the helper command above or launch Captive from the
OpenCaptive start menu with authentic data. The helper opens a standalone
DOSBox-X window for manual testing; the start menu uses the same normal launch
path so the original runtime remains fully interactive in DOSBox-X. Do not add these
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

That gate currently proves the authentic initial target/cursor alignment and
the `FLIGHT PATH SET` boundary. The supplied Mission 0001 VGA state already
places the green destination marker beneath CAPPO's magenta cursor; moving
the holomap first moves away from the real target and is not a valid route
proof. It does not claim that the automated route has reached `ARRIVED AT
DESTINATION`, `NOW IN ORBIT`, or `LANDING SUCCESSFUL`.

Longer transit observation is supported by the diagnostic harness. Its
extended response timeout only gives DOSBox-X time to return a complete
original dump; it does not create an arrival or advance the game locally.
The harness's mouse-motion commands use DOSBox-X's authentic integration
register encoding and never write CAPPO's pointer coordinates directly.

The automated ORBIT-dispatch probe is not part of the verified release path:
direct debugger queue injection is not equivalent to a real keyboard event and
must not be used as parity evidence.

The full-sequence diagnostic may log CAPPO's live `CS:IP`, `DS`, `AX`, `SI`,
and `DI` after scan `47`. Those registers are observation evidence only. In
particular, the previously recorded `0824:CF87` address byte-matches source
offset `0xD387`, a rendering helper, and is not an Orbit/arrival proof. The
probe never writes a guessed orbit or landing state into memory; only a changed
original VGA frame and matching original runtime state can advance the parity
boundary.

## Troubleshooting

### DOSBox-X debugger stepping

`VRT` and `TIMERIRQ` are DOSBox-X debugger observation commands, not Captive
commands. `VRT` resumes execution to the next vertical retrace, while
`TIMERIRQ` runs one emulated system-timer interrupt. Neither command may create
a planet, route, orbit, landing point, dungeon, or status message. A debugger
session that reaches `FLIGHT PATH SET` is still only a route-selection result;
arrival must be observed in the original CAPPO state and VGA output before
`LAND` can be tested.

Do not use debugger stepping during normal play. It can pause the original
game, alter input timing, or leave DOSBox-X at a debugger prompt. The
repeatable interactive procedure remains the authentic `CAPTIVE.BAT 1` launch
described above.

| Symptom | Correct action |
|---|---|
| VGA choice does nothing | Click inside the game viewport, then press `1`. |
| Debugger window appears | Quit and restart with the normal helper; remove debugger flags. |
| System pointer moves but Captive pointer does not | Click the DOSBox-X game viewport once so it is captured; the supplied profile must say `mouse emulation=locked`. Press `Ctrl+F10` once if the pointer was released, then click the viewport again. |
| The ship is still in transit | Wait for the original orbit/arrival state before pressing `9` for LAND. |
| A landed view contains only water | Restart from orbit and fly to the white landing circle. |
| The viewport looks stretched or corrupted | Use the repository profile; it forces VGA, surface output, integer scaling, and the CAPPO-compatible VGA memory setting. |
| An old or unexpected game state appears | Close every old DOSBox-X window and follow the Daily reset checklist. |
