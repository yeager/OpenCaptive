# Captive holomap reference

OpenCaptive's Captive startup surface uses a real DOSBox-X capture of the
original CAPPO holomap while the original vector/object decoder is being
recovered. It is reference media from the supplied original runtime, not
generated terrain, labels, stars, markers, or replacement text.

Source capture:

- `capture/cappo_000.png`
- 640×400 DOSBox-X frame, SHA-256
  `62490ac991bcc6928f486b9ad0b06083bfa80d38b7540500c06d0c81cc02a199`

The checked-in runtime asset `assets/captive/holamap-initial.png` is the same
frame reduced 2× to the native 320×200 canvas. Its SHA-256 is
`8766657baa32ffda4ba5d106d148a63d194b27bc81e2e782cf4006ab45eb88ec`.
Nearest-neighbour sampling preserves the original pixel values at native
resolution.

The original arrow controls remain interactive. Mouse coordinates are mapped
through the centered native canvas and use the original 18×18 control regions;
keyboard arrows use the same state path. The two original ladder hitboxes now
change nearest-neighbour zoom on the verified map panel while leaving the HUD
and control bank untouched.

The complete verified checkpoint is now available as four real CAPPO frames:

- `assets/captive/holamap-target.png` — the frame in which the green target
  point is visible (`b068a7b898db1b9db78bab52b700a57d4248d7b2038758a5eb73d96dd0697362`);
- `assets/captive/orbit-reference.png` — the original orbit view after
  `ORBIT` reaches that target (`4f483a01a6cca9f9aafa2e740294ec160c0b42a2d68f27086c4eb4e9a6495caa`);
- `assets/captive/landing-transition-reference.png` — the original landing
  transition after selecting the white point and pressing `LAND`
  (`2b837877f712ad70b195c0b3c45a0925eb28354e074e4d7a4ffb840e92b78205`);
- `assets/captive/landed-dungeon-reference.png` — the original land-level
  dungeon frame, with terrain rather than water
  (`df8f1e669072e84edfbc85af26a698c8bb34eb690900ef6083cbf07eca087939`).

`ORBIT` and `LAND` use these authenticated frames and never fall through to
procedural space, planet, or landing art. The native implementation remains
at the landed reference until the CAPPO mission/runtime decoder can supply
the live dungeon state; it does not enter a generated dungeon.

The DOSBox-X verification sequence used for the current reference is:

1. Run `CAPTIVE.BAT` from the directory containing the supplied `CAPPO.EXE`.
2. Select VGA, then the original sound and music devices.
3. Allow `INTRO.EXE` and `FILEPLAY.EXE` to finish; CAPPO then opens on
   `CAPTIVE MISSION 0001` in the real navigation view.
4. The green blinking point is the flight target. Press `ORBIT` to fly there;
   in the orbit view the white point is the valid landing point. Press `LAND`
   and wait for the land-level frame. If a landing shows only water, it is the
   wrong point and must not be accepted as Captive mission data.

All four checked-in frames were captured from the supplied CAPPO runtime in
DOSBox-X and reduced with nearest-neighbour sampling. Do not replace them
with generated art, generated terrain, a mockup, or a guessed landing point.
