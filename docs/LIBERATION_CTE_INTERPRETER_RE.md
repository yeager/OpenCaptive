# Liberation CTE interaction interpreter — reverse-engineering notes

These notes record where the authentic CTE interpreter lives in the Liberation
game binary (`LIBERATION_RESOURCE_GAME_BINARY`, CD32 hash
`db61f7e39fd31ac19b82216ea963711728d25518454fae42fd89c5bab52f2215`) and how its
state model is shaped. They are the foundation for building a faithful CTE
bytecode interpreter (decoder step 2). Nothing here is wired to on-screen text;
until the interpreter is complete and correct, the building overlay keeps its
current text rather than showing half-evaluated (wrong-context) output.

## Binary layout
- Amiga HUNK executable (`00 00 03 f3`). 4 hunks; hunk 0 is CODE, 225396 bytes.
- Disassembled with `m68k-elf-objdump -b binary -m m68k -D` (71,822 lines).
  capstone's m68k backend bails after ~44 instructions and is unusable here.

## Interpreter location (offsets into CODE hunk 0)
- The `^X` opcode reader/dispatcher is at **0xa29a**: it reads the next script
  byte (`moveb %a1@+,%d0`) and compares it against the sub-opcode letters
  (`cmpb #'T'`, `#'S'`, `#'X'`, `#'R'`, …), branching to per-opcode handlers.
- The `^` (0x5e) and `X` (0x58) top-level detection compares are at 0x6404,
  0xa2a8, 0xa482, 0xa4e4, 0xaca0, 0xaf82, 0xbac2 …
- The `^XI` conditional handler dispatch is near **0x7b54** (`cmpb #'I'`).
- Text-emit sink writes bytes via `%a2@+` (the output cursor).

## State model
- **`%a5` is the interaction state base register.** Every conditional and
  flag opcode reads/writes fields at fixed `%a5@(offset)`.
- The `^Xf` flag word is at **`%a5@(0x68de)`** (26846):
  - `^XfT` bit-test: `movew %a5@(0x68de),%d5; btst %d0,%d5`
  - `^XfS` set/clear: `bclr`/`bset %d1,%d5` then store back
  - `^XfX` toggle: `aslw %d1,%d0; eorw %d0,%d5`
  - `^XfR` clear-all: `clrw %a5@(0x68de)`
- **`%a5@(0x68e0)`** (26848) is a pointer to the current building/NPC record;
  byte at record `+2` holds the category bitfield (`btst #3`, shift, mask 3).
- The interpreter reads **dozens** of other state fields via `%a5@(N)` — the
  most-referenced offsets are 26768, 23142, 17333, 13460, 11540, 1958, 1928,
  104, 1590, 12110, 17314, 2016, 1214, 2048, 2004 (each 50–97 references). These
  are the single-letter script variables (`mc`, `v`, `g`, `s`, `H`, `I`, `K`,
  `E`, …) the CTE conditionals test.

## Why step 2 is a large, separate project
Faithfully expanding a CTE section requires evaluating its `^XI` conditionals,
which means modelling those dozens of `%a5` state fields AND every place the
rest of the 225 KB binary writes them (mission progression, reputation,
inventory, clue/plot flags from PGE). That is a reverse-engineering of
Liberation's whole interaction/plot state machine, not a mechanical
opcode-dispatch task. A partial model would evaluate conditionals wrong and
emit real text in the wrong context — a worse violation than leaving the
current placeholder. So the interpreter is built and verified in full before it
is wired.

## Next steps
1. Map each `^X`/`^Xf`/`^XC`/`^XI`/`^XG`/`^XS` handler from the disassembly.
2. Identify each `%a5@(offset)` state field's meaning by tracing its writers.
3. Model the needed subset in the runtime, kept in sync with real game state.
4. Route the building overlay through the interpreter; retire the invented nodes.
