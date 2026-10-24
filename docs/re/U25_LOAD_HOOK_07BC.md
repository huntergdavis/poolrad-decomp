# U25 +07BC: the class-1 load hook as its own entry, over the strict +0E0C model's code (evidence; credit is par-e's)

Unit `0x25` `+07BC` is the ECL load word's class-1 hook. It returns a field of the active character, or a game global, for an
ECL address in `6B00h..6EFFh`, and reports through a found flag. It was **already translated once**, as `load_hook()` inside
the strict model [`overlay_u25_load_word_0e0c.c`](U25_LOAD_WORD_0E0C.md). That model's checker pins this span by hash, and
its document calls it "its complete +07BC dependency". This change adds **no second copy**:
- `pr_u25_load_hook_07bc` is a thin public entry, an argument guard and then the same `load_hook()`;
- this row gets its own checker, which enters at `+07BC` with the hook's own frame.

## Claim table

| Item | Value |
| --- | --- |
| Row | `OVL_0025_07BC`: game_evidenced, grade B, state, composable, 0 unready callees (queue regenerated on main `328ed22c`) |
| Extent | **[07BC,0A74), 696 bytes / 233 instructions**, ending in the only return `RETF 6` at `+0A71`; no relocations; SHA-256 `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451` |
| Model | the existing `load_hook()` in `src/recovered/overlay_u25_load_word_0e0c.c`; new public entry `pr_u25_load_hook_07bc` (declared in `overlay_u25_load_word_0e0c.h`) |
| C test | `tests/recovered/test_overlay_u25_load_word_0e0c.c`, new `test_hook_entry()` (asserts on) |
| Checker | `tools/overlay/u25_load_hook_07bc_audit.py`, `CHECKER_ROWS = ('OVL_0025_07BC',)`, `--compile-only`, `--mutate`, `--native` |
| Harness reuse | `u25_load_word_0e0c_audit.py`: `compare()` and `library()` take the entry symbol; the +0E0C default is unchanged |
| Callee | `OVL_0025_0756` (strict), called twice (`+0981`, `+099A`) with the active-character pointer |
| Census | no new flag-writing function; `flag_census.py --check`: 0 gaps |

**Role line:** for an ECL address in the class-1 range, returns the matching field of the active character's record, the
ECL file number `DS:5376`, or the window-1 record word `+67Ch`. A record field is a byte, zero- or sign-extended, or a word;
some keys use small tables of their own. It sets the caller's found flag to 1, and clears it when no key matches.

**Question for par-e:** this body's only translation sits inside a strict closure (`+0E0C`), whose checker already executed
it. Is it creditable as its own row, now that it has its own entry, checker, frame fixtures, mutation set and native? Or does
`+0E0C`'s credit already cover it? This document claims evidence only.

## Identity

**Factor 1:** the one caller is the near `PUSH CS; CALL 07BC` at `+0E4A`, inside the **strict-credited** `OVL_0025_0E0C`
(`strict_full_c_addresses.csv`). There is no entry slot and no fixup-backed site.

**Factor 2 inputs** (what `+07BC` itself reads or writes):
- the active-character pointer `DS:[5D92]` ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md)), dereferenced for every record key:
  - `+14h`;
  - `+6Eh` ([ECL_LOAD_HOOK_6B9B_RECORD.md](ECL_LOAD_HOOK_6B9B_RECORD.md));
  - `+73h`, signed ([ECL_LOAD_HOOK_6BA0_SIGNED.md](ECL_LOAD_HOOK_6BA0_SIGNED.md));
  - `+76h+i`;
  - `+84h`;
  - the money words `+88h..+90h`;
  - `+A0h`, `+B8h`, `+BAh`;
  - `+10Dh..+10Fh`, mapped to 1/80h/81h/0;
  - `+11Ch`;
  - the class byte `+15h`, mapped through the table at `+09AD..+0A3B`;
- `DS:5376`, the ECL file number used by the block load (GAMEPLAY_PATH.md);
- the window-1 record `DS:[49D6]`, word `+67Ch`;
- a **write** of the found flag through the caller's pointer: `+0E0C` passes `SS:&[BP-4]` and reads window 1 only when the flag is zero (`+0E41..+0E69`, ECL_LOAD_HOOK_6B9B_RECORD.md).

Forwarding `[5D92]` to `+0756` is not counted. par-a writes the argument.

## Frame and contract

After the call:
- `[SP]` IP, `[SP+2]` CS;
- `[SP+4]`/`[SP+6]` the far pointer to the found flag;
- `[SP+8]` the address word;
- `RETF 6`.

Locals:
- the result word `[BP-2]`, returned in AX. **Some paths never write it**: key `10Dh`, the unmatched class values, the not-found path. The model reads it from RAM, as DOS does;
- the byte `[BP-3]`.

The flag is set to 1 **before** the address word is read. A flag pointer aimed at the frame therefore changes the key the hook
sees, and the fixtures below do exactly that.

The RAM contract is +0E0C's: at least 1 MiB, A20-off wrapping, immutable code. Stops:
- `OK`;
- `ARGUMENT` (null RAM or CPU, short RAM, IP not 07BC; nothing changed);
- `TRANSFER` (a `+0756` call returned elsewhere).

A cyclic party list does not return, as in DOS.

## Evidence

**Original-byte Unicorn differential, entered at `+07BC`: 3,593 cases, 0 mismatches.**
- Compared: CPU14, all 1,048,576 RAM bytes, and the stop.
- **268/268 instructions reached**: the hook's 233 plus `+0756`'s 35. The checker refuses execution outside them.
- **0 declared-split arrivals; no masks.**

The cases cover:
- every key `0..3FFh`, plus 256 random address words and the wraps `0`, `6AFF`, `6F00`, `FFFF`;
- all 256 values of each byte field:
  - `+14h`, `+6Eh`, `+84h`, `+BAh`, `+11Ch`;
  - signed `+73h` and `+A0h`;
- the indexed table at `+76h+i` (keys A5h..ACh);
- word fields at five values;
- `+10Dh..+10Fh` combinations;
- all 256 class bytes;
- `DS:5376`, and `+67Ch` behind `DS:[49D6]`;
- the `+0756` party walks: lengths 0/1/3/257, target found, last, or absent;
- **the found flag aimed at every byte of the hook's own frame (SP−6..SP+9)**;
- entry SPs that wrap, A20 wraps of SS and DS, and DS = SS aliases.

**One shape is excluded: exactly 4 fixtures, by a frame rule, not by DS = SS.** With DS = SS, the hook's frame can cover
`DS:5D96..5D99`, the party-walk link that `+0756` follows. On a `+0756` key (`2B1h`, `2B4h`) the frame then rewrites the
link before the walk, and the walk becomes cyclic. A cyclic walk never returns, in DOS or in C, by the shared contract.

The frame spans entry SP − 18h to entry SP + 9, measured from the bytes:
- the hook's `PUSH BP; SUB SP,4` reaches SP − 6;
- its two pushed pointer words and the far return into `+0756` reach SP − 0Eh;
- `+0756`'s `PUSH BP; SUB SP,8` reaches SP − 18h;
- the highest byte is the address argument at SP + 9.

`party_link_overlap()` in the checker excludes a fixture exactly when that range meets `5D96..5D99` on those keys. That is
keys 2B1h/2B4h at SP 5D98h and 5DA8h. A probe confirmed that each of the four exceeds the original's execution bound, so
none of them returns. **The other 31 DS = SS fixtures are compared.** They include both `+0756` keys at SP 49D8h, 5378h
and 3FF6h, with the frame over `DS:49D6` and `DS:5376`.

The real stack cannot sit there. The game's stack is `SS:SP = 14F2:4000` (16 KiB, growing down). No in-game `MOV SS` or
`LSS` exists. Both facts are documented in [U25_ALLOC_1951_CPU.md](U25_ALLOC_1951_CPU.md) and
[REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md). A frame therefore lies at offsets 0000h..4009h of SS. Even with DS = SS it
never reaches offset 5D96h unless the stack has already wrapped, which corrupts memory first.

**Mutation: 17 mutants inside `load_hook()`, 17 caught by the differential** (the quick case set). The mutants:
- the flag value;
- the key bias;
- three field offsets;
- sign versus zero extension;
- the A5h..ACh bound;
- a word read as a byte;
- the 100h results swapped;
- the 10Ch compare;
- 10Dh writing the local;
- a class table value;
- the class range bound (caught on FLAGS);
- the 312h global;
- the 33Eh offset;
- the not-found path leaving the flag;
- `RETF 4`;
- a wrong `+0756` return address (TRANSFER).

**Native (synthetic DOSBox-X, packaged build, not the pinned one):** 39 cases call `+07BC` directly with its frame:
- each key class;
- the not-found and wrap addresses;
- 12 class bytes.

CPU14, the 64-byte frame, the flag byte and the preserved address all match the C model on the rebuilt state. That is
3,744 bytes, **0 masks**.

The +0E0C audit's own run is unchanged by this change: its default entry, cases and expectations are the same.

## Not established

- Natural reachability of a saved game into `+07BC`; the native frames are synthetic.
- The pinned DOSBox-X build.
- Whether this row is creditable on its own (par-e's question above).

## Reproduction

    python3 tools/overlay/u25_load_hook_07bc_audit.py
    python3 tools/overlay/u25_load_hook_07bc_audit.py --mutate
    python3 tools/overlay/u25_load_hook_07bc_audit.py --native
    python3 tools/overlay/u25_load_hook_07bc_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main OVL_0025_07BC OVL_0025_0E0C

Media, assembly and captures stay under ignored `resources/`.


## Reviewer acceptance record (added by the review lane, not the author)

Reproduced by par-e at main `58a0a276` (2026-10-23/24), through `poolrad-heavy.sh`. The checked C closure, headers and
audit are byte-identical on `9ff74887`. Unicorn: 3,593 cases, 268/268 instructions (hook 233 + 0756 35), 0 unvisited, 0 declared flag-split arrivals; all 1,048,576 RAM bytes and 14 CPU words, flag masks 0.
Mutation: 17/17 caught. Native: 39 cases (masked_flags 0). Code body 696 B, SHA-256 `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451`, re-derived from GAME.OVR.

Identity: par-a's two-factor argument in [U25_IDENTITY_ARGUMENTS.md](U25_IDENTITY_ARGUMENTS.md) (at `2c7d77af`). I re-derived every factor-1 site from the
media: each near site decodes as `0E E8` at an instruction boundary inside the caller extent par-a names.

**Reviewer-added limit (native oracle):** the native run used /usr/bin/dosbox-x 2026.01.02 (SHA-256 99d74601...), not the pinned 5a9c7c53 build. Native agreement is claimed for that binary only.
The primary evidence is the full-1 MiB Unicorn differential.

**07BC-specific (the credit question in this document).** Creditable as its own row. Strict credit counts original
routines, not C files. `[07BC,0A74)` is a distinct routine with its own extent that `OVL_0025_0E0C`'s row does not count;
0E0C starts above 0A74, so the spans do not overlap. That its only translation lives in 0E0C's C file is the same case
as `OVL_007F_0000`, which 0054/0A88 compose and which is credited separately. Factor 1 is the strongest kind: the sole
caller, near `+0E4A`, is inside strict-credited 0E0C.
