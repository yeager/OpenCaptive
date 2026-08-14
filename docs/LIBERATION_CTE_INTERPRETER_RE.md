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

## Interpreter implemented (read-only expander)
`cte_expand()` in `src/data/liberation_cte.c` now expands a section to authentic
text for a supplied `CteState`. It handles the self-contained, text-producing
opcodes and safely skips side effects:
- plain text; `^^` -> newline; `^;` -> comment to end of line
- `^O<n>[a|b|...]` random variant (seeded PRNG, deterministic per state)
- `^XI<cond>[then|else]` conditional; conditions support `~` negation, ops
  `= < >`, and compound `(a!b)` (OR) / `(a&b)` (AND)
- `^XC<var>[c0|c1|...]` switch on a variable's integer value
- all other `^X…`, `^F…`, `^X=` etc. consume their operand and emit nothing
Validated against the real CITY_TEXT (48/48 sections expand with no leaked
opcode markers or bracket groups); unit-tested in `tests/test_liberation_cte.c`.

## Cross-table call targets (found)
`^XS`/`^XG` targets are **5-digit label ids** (14001, 17001, 30014, 40006, …),
disjoint from the CTE section-id space (bytes 31–180). They index a separate
string/label table, not other CTE sections. Resolving them (so a call-only
section yields its real spoken line) is the remaining work before the
interpreter can be wired to on-screen dialogue — this is what makes most
sections currently expand to empty text under an initial state.

## Section framing corrected + calls resolved (v1.1.133)
Two structural corrections, both verified against the real CITY_TEXT:

1. **Section id is 16-bit big-endian, not one byte.** Each section is framed
   `0xD7 <id-hi> <id-lo> 0x00 <len> [ content ]`. The id is the decimal label
   the script uses (`0x36b0`=14000, `0x4268`=17000, …). The earlier
   bracket-scan parser truncated the id to its low byte and merged sections,
   finding only 48; the marker-based parser finds the true **198 sections**.
   `<len>` is a one-byte hint that overflows past 255 bytes, so content is
   delimited by bracket matching / the next `0xD7` frame, not by `<len>`.

2. **`^XS`/`^XG` targets are these same 16-bit section ids** — calls resolve
   *inside* the CTE table, not into a separate string table as first suspected.
   Of 101 distinct call targets, 193/198 sections and all but 8 targets
   resolve; the 8 (4, 203, 40001, 40003, 40012, 40016, 40028, 40030) are
   dangling/terminal refs that end the interaction and correctly inline nothing.

`cte_expand` now inlines `^XS` (subroutine: insert then continue) and `^XG`
(goto: insert then end the section), with a per-expansion call stack that
refuses to re-enter an active id (cycle-safe, recursion-bounded). Skipped
opcode families were widened to consume their operands + bracket groups: the
`^Xf*…[…]` flag family, `^XM[…]` menus, and the lowercase location/status
templater (`^L<lo>-<hi>[…]` range, `^A/^Z/^s/^h/^w` substitutions).

Result: 66/198 sections emit authentic text under an empty state — including
the real clue quotes ("The tigers of wrath are wiser…", "Prisons are built
with stones of Law…", the full 32-entry set). The remaining sections gate on
game flags/menus, so their real lines need the state-field model (next step)
before they render. No section leaks a raw opcode marker or bracket group.

## Condition-variable inventory (complete, from all 198 real sections)
The `^XI`/`^XC` conditionals test these state variables (frequency across the
real CITY_TEXT). This is the authoritative list of what the state-field model
(next step) must supply for correct branch selection:

| var | uses | likely meaning (to confirm by binary trace) | derivable from |
|-----|------|---------------------------------------------|----------------|
| mc  | 27   | building/NPC category (record +2 bitfield)  | building record (context) |
| u   | 13+23| player response / menu-choice index         | interaction runtime |
| E   | 17   | signed record/plot status (E<0, E>2)        | plot state |
| g   | 13   | boolean flag (guilty/goods?)                | plot/police state |
| H   | 11   | signed relationship/hostility meter         | NPC/plot state |
| v   | 8    | boolean (visited/violence?)                 | plot state |
| K   | 6    | flag                                        | plot state |
| l   | 5    | flag (with r: l=1&r=2)                       | plot state |
| I   | 3    | counter (I>0)                               | plot state |
| a,r,e,s,f,x,M,w,h,me,zc,J,t,b,L,B,m,j,c,G | 1-3 | assorted flags/counters/gender | mixed |

Context-derivable vars (`mc` category, `zc` gender) can be supplied
immediately from the building/champion record and would make those branches
correct on their own; the plot-state vars (E, g, H, K, l, I, …) require the
mission/reputation state machine and are the larger part of the next step.

## `mc` (building/service category) — decoded from authentic text (v1.1.135)
The master conversation gate `mc` was decoded WITHOUT the binary, by expanding
the mc-branched sections and reading the game's own words. Section 200 (the
player's opening "what I want" line) cleanly separates the commercial subtypes,
and section 10000 (the service menu) confirms them:

| mc | authentic line (section 200 / 10000) | category |
|----|--------------------------------------|----------|
| 13 | "Yes, I wanted to do some banking business." + AC/No balance/withdrawal/deposit menu | **bank** |
| 16, 18 | "I was rather hoping you could do some repairs for me." | **repair / workshop** |
| 11 | "What have you got?" | **browse shop** |
| 0, 3, 14, 15, 17, 19, 20, 21, 22 | "Actually I hoped you might be able to let me have …" | **general shop / vendor** |
| 4, 5, 6, 7, 8, 9, 10, 12 | (no purchase line — commercial sections empty) | **non-commercial** (police/records/residence/bar/business; conversation is topic-driven, not purchase-driven) |

This is derived purely from the original dialogue, so it needs no synthetic
assumption. It is enough to route the commercial buildings (bank, shop, repair)
to their authentic CTE entry once the runtime supplies `mc`. The finer split of
the non-commercial band (4-12) still wants the binary trace or a citygen
cross-reference, and the reconstruction's `BuildingType` enum (0-8,
liberation_citygen.h) has no bank/repair member, so wiring those two also needs
a taxonomy addition — tracked in TODO.

### `mc` is an engine-set input, confirmed
The CTE script never writes `mc` (0 occurrences of `^X=mc`/`^X+mc`; the script
only writes plot/relationship flags H, K, I, E, D, M, J, L). So `mc` is supplied
by the engine before the interaction runs — it is the building's service
context, read from the building record. This pins the remaining work squarely in
the binary: recover how the engine computes `mc` from the record. It cannot be
derived from the CTE data alone.

### Why `mc` is not the generated building type
The reconstruction's citygen sets `CityBuilding.type = PRNG % 9` and is
parity-locked to the original, storing no finer category. Yet the original has
banks (`mc=13`) and repair shops (`mc=16/18`), which are not among the 9 types.
So `mc` is the **service** the building offers, derived at interaction time from
a building attribute — a `BUILDING_BUSINESS` can host banking, repair, or
vending. Routing bank/repair transaction dialogue therefore requires the
original's `mc`-derivation logic recovered from the binary (a recursive-descent
m68k trace of how it computes `mc` from the building record); a guessed
`BuildingType`→`mc` map would surface wrong-context dialogue. The
**informational** dialogue does not need this — it was wired via
`liberation_city_text` in v1.1.136-138.

## Tooling unblocked: radare2 with m68k (2026-08-14)
radare2 6.2.0 is now available for the binary trace (installed from the official
macOS pkg without admin: verify SHA256, `pkgutil --expand-full`, run from the
extracted payload; persisted at `~/.local/bin/r2portable`). It disassembles the
m68k CODE hunk far more reliably than linear objdump. First findings from it:
`a5` is the global-data base (matches the prior RE `%a5` state base); a `'['`
scanner loop sits at CODE-hunk offset 0x6068 (`cmpi.b #'[',(a1)+ ; bne` then
`lea 0x5b6c(a5),a2`); and the CTE opcode dispatch uses a jump table on the
opcode byte (no char comparisons), which is why char-anchored searches missed
it. The remaining `mc` trace — find the `^XI` variable read's `%a5` offset, then
its writers — is now tractable and is the next step (jump-table dispatch means
r2's `aa` under-covers; drive analysis manually from anchors).

## mc-derivation CRACKED with radare2 (2026-08-14)
Traced on the CD32 game binary db61f7e3… (CODE hunk 225396 bytes, the one the
earlier anchors match; extract via hash_extract, CODE at file offset 44).
radare2 (`~/.local/bin/r2portable -a m68k -b 32 -e cfg.bigendian=true`) gives
clean disassembly; addresses below are CODE-hunk-relative.

**`mc` is the current NPC/person's profession field, not the building type.**
- The `^XI`/`^XC` condition reader (dispatch under the `^X` handler at 0xa29a;
  `^XI` at 0x7b54) resolves `mc` at 0xa1a2: `a0 = *(0x6890(a5)); d2 = word[a0+8]`.
  `0x6890(a5)` is the current person/NPC record pointer (default static buffer
  0x6894(a5) set at 0x6190; a real NPC bound at 0xa55c). So **mc = word at
  person_record+8 = the NPC's profession/service code.** (`me` at 0xa18a reads
  the *building* record+2 category byte instead; building record = 0x68e0(a5).)

**The profession (person+8) is computed from the building record's category
byte via function 0xa738** (set at 0xa6b4-0xa6ba: `d0 = building_record[+2];
bsr 0xa738; move.w d1,person+8`). 0xa738 decodes the category byte → mc:
- plot-flag overrides (globals 0x75b8/0x75b9/0x75bb(a5)) → mc 20–23 (0x14–0x17)
- category **bit 3 set** → mc = −1/−2 (0xff/0xfe: no normal service)
- category **& 7 == 0** → mc = 0  (general shop/vendor)
- category **& 7 == 7** → mc = −3 (0xfd)
- category **& 7 in 1..6** → pseudo-random pick via 0xa988→0xd444: 0xa988 hashes
  building_record bytes [0],[1],[2] plus a global (0x1d7b(a5)) into a seed
  (stored 0x5a66(a5)) and calls the PRNG 0xd444, selecting the specific
  commercial service (bank=13, repair=16/18, …) deterministically per building.

**Implication for wiring bank/repair:** the reconstruction must (1) form the
building record's category byte (it already keeps a category in flags bits 2-5),
(2) apply the 0xa738 decode + 0xa988/0xd444 selection to get `mc`, (3) run the
CTE with that `mc`. Remaining to fully replicate: decode 0xd444 (the PRNG) and
0xa988's exact seed→service mapping so the 1..6 commercial split (which building
becomes a bank vs repair vs vendor) is byte-faithful. This is now tractable in
r2 — the mechanism and every entry point are located.
