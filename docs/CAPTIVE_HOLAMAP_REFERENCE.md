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
keyboard arrows use the same state path. The reference frame is intentionally
not modified with synthetic cursor art or invented mission data. Enter also
does not enter the unfinished dungeon compositor.

The DOSBox-X verification sequence used for the current reference is:

1. Run `CAPTIVE.BAT` from the directory containing the supplied `CAPPO.EXE`.
2. Select VGA, then the original sound and music devices.
3. Allow `INTRO.EXE` and `FILEPLAY.EXE` to finish; CAPPO then opens on
   `CAPTIVE MISSION 0001` in the real navigation view.
4. Use the original arrow controls to move the map cursor. The ladder-down
   control zooms in, ladder-up zooms out, `ORBIT` flies to the selected point,
   and `LAND` starts the landing transition.

The current native implementation intentionally stops at the verified
navigation frame until the same mission/runtime records and landing state are
decoded. Do not replace this asset with generated art or a mockup.
