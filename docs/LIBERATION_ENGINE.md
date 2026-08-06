# Liberation: implementation status

> Updated for v1.1.79. This status page deliberately separates verified presentation from prototype gameplay systems.

## What runs today

Liberation currently opens only after the CD32 presentation track has been
found by its SHA-256 digest and its presentation resources have been verified.
It can display the first fully decoded frame of the original intro and city
ANIM resources at their native 320×256 PAL resolution. Selecting a game, the
intro-to-city transition and the F10 display/audio menu are intentionally
limited to that presentation boundary.

This is not yet a playable reimplementation of Liberation: Captive 2. The
runtime does not invent city movement, buildings, objectives, NPCs, interiors,
saves or plot progression. Those earlier procedural substitutes were removed
because they were neither decoded from the CD32 media nor visually or
logically comparable with the original game.

## Verified media boundary

The CD32 track is selected by SHA-256:

```text
f807b1385c0996d54ed10afab271a7dd31d2c6dc6a18f13196ad2a79a0af8a80
```

The presentation bundle is selected by SHA-256:

```text
1d3a335d254c0eae919a712dd73bd41b24ed897bf145ed118ccf2277baa7a35f
```

The loader does not use original filenames as fallbacks. ANIM resources are
IFF `FORM` containers with `PALL`, `PACK` and `SCPT` chunks. The RNC method 2
decoder has been verified for the first complete planar frame of the two
known presentation forms. `SCPT` record boundaries are retained for analysis;
their opcode meanings, timing model and the remaining packed delta/layer data
are not yet decoded.

For analysis, `liberation_presentation_capture --all <data-dir> <output-dir>`
exports every first decodable frame from the verified presentation bundle in
one pass. Each output is named with the decoded FORM's SHA-256 digest, not an
original filename. Some outputs are deliberately sparse or black because the
original presentation uses them as layers; the export is evidence for SCPT
recovery, not a claim that the first frames form complete screens by themselves.

See [the technical notes](wiki/Liberation-Technical.md) for resource hashes,
container observations and the exact current limitations.

## Work required for playable parity

1. Recover the ANIM script interpreter, including every `SCPT` opcode and
   packed delta/layer operation.
2. Decode the original city and building state formats and the inputs/outputs
   of the verified CityGen and PlotGen executables.
3. Decode plot, dialogue and save-state records without flattening their
   control streams into plain text.
4. Build small parsers with original-output fixtures and pixel comparisons.
5. Wire only verified state and rendering operations into the live runtime.

Until then, screenshots of the first decoded city frame establish only that
the source pixels and their placement are authentic; they do not establish
gameplay or animation parity.
