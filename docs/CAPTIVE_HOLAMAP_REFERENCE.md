# Captive holomap reference

OpenCaptive's Captive startup surface uses a real DOSBox-X capture of the
original CAPPO holomap while the original vector/object decoder is being
recovered. It is reference media from the supplied original runtime, not
generated terrain, labels, stars, markers, or replacement text.

Source capture: the supplied `CAPPO.EXE` session in DOSBox-X, after the
original `INTRO.EXE` and `FILEPLAY.EXE` complete, showing `CAPTIVE MISSION
0001` in the navigation view. The checked-in runtime asset
`assets/captive/holamap-initial.png` is that real frame reduced 2× to the
native 320×200 canvas. Its SHA-256 is
`7ba17570145bbdb186330f5a3aeb7152e7966a7c41a279fc49d8f1ec4bf1272b`.
Nearest-neighbour sampling preserves the original pixel values at native
resolution.

The original arrow controls remain interactive. Mouse coordinates are mapped
through the centered native canvas and use the original 18×18 control regions;
keyboard arrows and the original numeric keypad controls use the same state
path. OpenCaptive maps `NUMPAD 2/4/6/8` to cursor movement, `NUMPAD 7` to
`ORBIT`, and `NUMPAD 9` to `LAND`. The ladder commands select complete,
authenticated CAPPO frames captured from DOSBox-X; OpenCaptive does not
enlarge the opening bitmap or synthesize map detail.

The zoom captures are:

- `assets/captive/holamap-zoom-out.png` — `NUMPAD 1`
  (`4f2dbe1f997fcbd22c5bb029cf3afd8685f5f8ef1177c9868ec5c4e6b08102bc`);
- `assets/captive/holamap-zoom-in.png` — `NUMPAD 3`
  (`40330335de7d8daa76e2d634680f1c630be834cfb27f53951812815660c39303`).

The complete verified checkpoint is now available as six real CAPPO frames:

- `assets/captive/holamap-target.png` — the source-authenticated Butre-route
  navigation frame, where the green point identifies the planet destination
  (`b068a7b898db1b9db78bab52b700a57d4248d7b2038758a5eb73d96dd0697362`);
- `assets/captive/orbit-reference.png` — the original orbit view after
  `ORBIT` reaches that target (`4f483a01a6cca9f9aafa2e740294ec160c0b42a2d68f27086c4eb4e9a6495caa`);
- `assets/captive/landing-transition-reference.png` — the original landing
  transition after selecting the white point and pressing `LAND`
  (`2b837877f712ad70b195c0b3c45a0925eb28354e074e4d7a4ffb840e92b78205`);
- `assets/captive/landed-dungeon-reference.png` — the original land-level
  dungeon frame, with terrain rather than water
  (`df8f1e669072e84edfbc85af26a698c8bb34eb690900ef6083cbf07eca087939`).

`ORBIT` and `LAND` use these authenticated frames and never fall through to
procedural space, planet, or landing art. The native fallback presents the
authenticated Butre-route frame during transit. The explicit Orbit control (or
the second keypad-7 command after `FLIGHT PATH SET`) advances to the
authenticated orbit frame and reveals its real white landing point. The landed
surface remains a real checkpoint until the CAPPO mission/runtime decoder can
supply live dungeon state; no generated dungeon is used.

The native fallback keeps the landing transition open until a live
CAPPO/DOSBox-X handoff proves the landed state. It does not use a wall-clock
delay to turn captured frames into false runtime events. A reloaded dump is
accepted only when its complete VGA surface exactly matches the tracked orbit
or landed checkpoint; partial frames and guessed memory fields are rejected.

The DOSBox-X verification sequence used for the current reference is:

1. Run `CAPTIVE.BAT 1` from the directory containing the supplied `CAPPO.EXE`;
   the batch file passes the mission argument through to CAPPO.
2. Select VGA, then the original sound and music devices.
3. Allow `INTRO.EXE` and `FILEPLAY.EXE` to finish; CAPPO then opens on
   `CAPTIVE MISSION 0001` in the real navigation view.
4. Fly to the real Butre destination recorded by the original mission state,
   identified by the green planet point, and press `ORBIT`. In the orbit view
   the white circle is the valid landing point. Press `LAND` and wait for the
   land-level frame. If a landing shows only water, it is the wrong point and
   must not be accepted as Captive mission data. OpenCaptive must not invent
   either marker; both must come from original media/data.

All four checked-in frames were captured from the supplied CAPPO runtime in
DOSBox-X and reduced with nearest-neighbour sampling. Do not replace them
with generated art, generated terrain, a mockup, or a guessed landing point.
