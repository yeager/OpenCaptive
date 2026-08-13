# Captive holomap reference

OpenCaptive's Captive startup surface uses a real DOSBox-X capture of the
original CAPPO Mission 0001 navigation/map surface while the original
vector/object decoder is being
recovered. It is reference media from the supplied original runtime, not
generated terrain, labels, stars, markers, or replacement text.

Source capture: the supplied `CAPPO.EXE` session in DOSBox-X, after the
original `INTRO.EXE` and `FILEPLAY.EXE` complete, showing `CAPTIVE MISSION
0001` in the navigation/map view. The checked-in runtime asset
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

The recorded Mission 0001 coordinate is only one part of CAPPO's navigation
state. A live DOSBox-X run can display the target coordinate while CAPPO is
still in its `FLIGHT PATH SET` phase; that is not evidence of arrival and must
not be used to enter Orbit. The native bridge therefore requires the complete
authenticated CAPPO Orbit VGA handoff, not merely a coordinate or marker
match. This is an exact source checkpoint, not a generated green marker or a
procedural map update.

The zoom captures are:

- `assets/captive/holamap-zoom-out.png` — `NUMPAD 1`
  (`4f2dbe1f997fcbd22c5bb029cf3afd8685f5f8ef1177c9868ec5c4e6b08102bc`);
- `assets/captive/holamap-zoom-in.png` — `NUMPAD 3`
  (`40330335de7d8daa76e2d634680f1c630be834cfb27f53951812815660c39303`).

The reference set contains real CAPPO frames captured during separate
DOSBox-X sessions. It is not, by itself, proof that one replay completed the
whole route; a replay gate must authenticate each transition from the current
CAPPO VGA surface before OpenCaptive changes phase.

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
procedural space, planet, or landing art. The native diagnostic path presents
the authenticated Butre-route frame only when the caller supplies a real CAPPO
checkpoint; it is not a normal game fallback. A second Orbit button press or
keypad-7 command is not treated as arrival: the DOSBox-X session remains on
the holomap with CAPPO's `FLIGHT PATH SET` status, even after the displayed
coordinate reaches the recorded target. The authenticated orbit frame is
accepted only after a live CAPPO/DOSBox-X handoff proves that CAPPO has
reached orbit. The landed surface remains a real checkpoint until the CAPPO
mission/runtime decoder can supply live dungeon state; no generated dungeon
is used. A second Orbit button press or keypad-7 command is not treated as
arrival; the original runtime must first report arrival.

The native diagnostic path keeps the landing transition open until a live
CAPPO/DOSBox-X handoff proves the landed state. It does not use a wall-clock
delay to turn captured frames into false runtime events. A reloaded dump is
accepted only when its complete VGA surface exactly matches the tracked orbit
or landed checkpoint; partial frames and guessed memory fields are rejected.

The DOSBox-X verification sequence used for the current reference is:

1. Run `CAPTIVE.BAT 1` from the directory containing the supplied `CAPPO.EXE`;
   the batch file passes the mission argument through to CAPPO.
2. Select VGA, then the original sound and music devices.
3. Allow `INTRO.EXE` and `FILEPLAY.EXE` to finish; FILEPLAY uses its own IRQ1
   scan-byte field at `CS:0027`, so the original space-bar make code is
   `0x39`, not a BIOS-buffer character. CAPPO then opens on its original
   continuation surface. Send the documented DEL/left-mouse action (`0x53` in
   CAPPO's relocated IRQ1 queue) once to enter `CAPTIVE MISSION 0001` in the
   real navigation view. Sending a keypad scan before this continuation is
   consumed tests the wrong CAPPO phase. The first live frame after this
   handoff is the original Mission 0001 navigation/map surface, not a
   generated dungeon or a locally fabricated planet view.
4. Fly using CAPPO's real flight controls until CAPPO itself reports arrival;
   the displayed coordinate and the green/red map markers are useful evidence
   but are not sufficient on their own. Only then press `ORBIT`. In the Orbit
   view the white circle is the valid landing point. Press `LAND` and wait for
   the land-level frame. If a landing shows only water, it is the wrong point
   and must not be accepted as Captive mission data. OpenCaptive must not
   invent either marker; both must come from original media/data.

   The live bridge sends each original XT make scan together with its matching
   break scan. CAPPO keeps a pressed-state bit for these controls, so sending
   repeated make bytes alone is not equivalent to a physical keypad and can
   silently ignore later route steps.

All four checked-in frames were captured from the supplied CAPPO runtime in
DOSBox-X and reduced with nearest-neighbour sampling. Do not replace them
with generated art, generated terrain, a mockup, or a guessed landing point.
