# RES_00BA +2E46: raw-CPU model of the character link flags test (no new credit)

`RES_00BA_2E46` is **already credited**. Its credit rests on the semantic translation `pr_rec_res_00ba_2e46`
([RESIDENT_CHARACTER_LINK_FLAGS.md](RESIDENT_CHARACTER_LINK_FLAGS.md)), which wR:p1's #1482 checks against the original bytes
(`res00ba_rec_models_audit.py`). That model takes an io struct, not a CPU.

This change adds the **raw-CPU, full-state model** the hybrid executor needs. Route 3 reaches this body 11 times (codex's glue-gap
census, i22516). The hybrid's shadow compares CPU14 and all 1 MiB, which an io adapter around the semantic model cannot reproduce.
It is the 23B7/2E03 pattern, and it **composes the raw 2E03 model** for its one call. **No credit or ledger change is proposed.**
Assigned by par-a (i22518), claimed in i22523.

**Pre-claim grep** (VP i22025, widened by PM i22086), on main 015a1741: `grep -rniE '(\+|0x|:)2E46\b|RES_00BA_2E46'` over
`src/recovered`, `tools/overlay`, `include` and the docs CSVs. Every hit was opened:
- the semantic `pr_rec_res_00ba_2e46` (`resident_character_link_flags.c/.h`);
- wR:p1's io adapter (`res00ba_io_adapter.c`);
- the rec-models audit's span table;
- the ledger and credit rows.

No raw translation existed. The only branch, `codex/resident-2e46-accounting`, is merged accounting.

## Executable extent

START.EXE resident segment 00BA **[2E46,2E8B), 69 bytes / 27 instructions**, from the relocated image of
`res0899_quit_0000_cpu_audit.load_image`, with no relocation in the span. It has one call, `PUSH CS; CALL 2E03` at 2E52/2E53, and one
return, `RETF 4` at 2E88. The checker decodes `[2E03,2E8B)`: the callee's 67 bytes and 23 instructions plus this body. It asserts both
returns and the single call, and that both spans are free of relocations, interrupts, ports and loops.

## What the body does

Frame after the far call: `[SP]` IP, `[SP+2]` CS, `[SP+4]/[SP+6]` the character far pointer. The local result byte is `[BP-1]`.

1. `PUSH BP; MOV BP,SP; SUB SP,2`.
2. Push the character pointer, then `PUSH CS; CALL 2E03`. This is the link check: AL = 1 when the character has a link and
   `DS:[558Eh + 16·index]` is above 1, where index is the link's byte +2Eh. See [RES_00BA_2E03_CPU.md](RES_00BA_2E03_CPU.md).
3. `OR AL,AL; JE`: a zero result gives `[BP-1] := 0`.
4. Otherwise: `LES DI,[BP+6]; LES DI,ES:[DI+0CCh]`. Then `AL := ES:[DI+2Eh]; XOR AH,AH; MOV DI,AX; MOV CL,4; SHL DI,CL`.
   Then `AL := DS:[DI+5590h]; AND AL,14h; CMP AL,14h; JNE`. When both bits are set, `[BP-1] := 1`; otherwise 0.
5. `MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 4`.

## Role

The body tests the character link flags: the character has a qualifying link (2E03), **and** the linked entry's flags byte in the
`DS:5590` table has both bits 14h set. The role is the credited one; this note changes nothing about identity.

## Raw contract and stops

`pr_res_00ba_link_flags_2e46_cpu(ram, size, cpu)`:
- exactly 1 MiB of A20-off RAM, flat; IP 2E46;
- **entry SP in [0010h,FFF8h]**, so that 2E03's entry SP, 12 bytes lower, stays inside its own contract `[0004h,FFF8h]`.

The flags follow the pinned DOSBox-X stored rules:
- OR/AND/XOR clear CF, OF and AF;
- SHL sets AF for a non-zero count, and OF is MSB(result) XOR CF.

The helpers are 2E03's, verbatim, so their census rows are shared. LES reads its segment word at (offset+2) mod 64K, as in 2E03.

| Stop | Pinned by |
| --- | --- |
| RETURNED | Unicorn: every case; native: all cases |
| ARGUMENT | C test: null RAM/CPU, short RAM, IP not 2E46, SP outside the range; CPU and RAM untouched |
| TRANSFER | **unreachable inside the contract**. Neither 2E46 nor 2E03 stores into its own return frame: the only writes are pushes and `[BP-1]`. The model keeps the check defensively, and the mutant that drops it is therefore equivalent and is not counted |
| CALLEE | 2E03 stopping otherwise; unreachable inside the contract |

## Checks

**Original-byte Unicorn differential: 317 cases, 0 mismatches.** Both spans run as original bytes: this body, and its callee 2E03 through
the real `PUSH CS; CALL`. The C composes the raw 2E03 model. Compared after every case, **unmasked**: CPU14, **all 1,048,576 RAM bytes**,
and the stop.
- 317 RETURNED.
- **50/50 instruction starts** across both spans, and **8/8 conditional outcomes**.
- **253 independent statements**: AL from the stated rule (2E03's link check, then the AND 14h test), AH, SP+8, BP and CS:IP, computed
  without the C model on cases that alias neither frame.

Cases:
- the link check returning 0 (no link; table bytes 0 and 1);
- the flags byte at 00/04/10/14/15/34/EB/FF;
- either link word alone non-zero;
- indices 00..FF (DI up to 0FF0h);
- the link words and the link wrapping at 16 bits;
- A20 wraps of the character, the link and DS;
- six flag images on the hit, miss and no-link paths, and five entry SPs;
- with DS = SS, **both tables' entries on every byte of both frames** (this body's and 2E03's, which sits 12 below);
- the link byte and the character's link words on the frame;
- 200 random cases.

**Mutation: 12 deliberate mutants, 12 caught by the differential:** the flags table at 5591h; the mask is 04h; the compare is with 04h; JNE as JE; the 2E03 result is not tested (OR AL,AL skipped); the index byte is at link+2Fh; the table stride is 8 (SHL 3); AND leaves AF set; the return address into 2E56 is 2E55; the PUSH order of the character words is swapped; RETF pops 2 argument bytes; the local is not cleared on the zero path. The thirteenth candidate, "the TRANSFER check
dropped", is equivalent by the contract argument above and is not counted.

**Native (synthetic DOSBox-X; packaged DOSBox-X 2026.01.02, sha256 99d74601, the same release as pinned 5a9c7c53):** 33 cases.
- Both spans run as original bytes under a real DOS.
- The cases: no link; the table bytes; the flags bytes; indices; the wraps; flag images; SP bounds; frame aliases.
- CPU14 and five 64 KiB regions: **10,814,364 bytes, 0 mismatches, no masks.** IF is cleared in every native case.

**C test** (asserts on): the no-link, hit, single-bit and extra-bit flags bytes; the 2E03-zero path; index FFh; the return frame; the guards.

**Census:** no new flag rows. The helpers are 2E03's verbatim, so the shared row `670398ea…` now lists 2E03, 23B7 and 2E46, and its
description covers the union of callers. **Aperture census:** a new `flat` row, as for 2E03.

## What this does not show

- Credit or identity: unchanged.
- The hybrid glue: par-b's next step, after merge.
- **The link-word straddle** (`char-wrap-*` with +0CCh at FFFFh). The independent statement skips it because a contiguous word read
  differs from the wrapped LES segment read. The differential and the C test cover it, as for 2E03.

## Reproduction

    python3 tools/overlay/res00ba_link_flags_2e46_cpu_audit.py --unicorn --mutate --native --output resources/res00ba-2e46-evidence.json
    python3 tools/overlay/res00ba_link_flags_2e46_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main RES_00BA_2E46
