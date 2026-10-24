# RES_00BA +1509: raw-CPU model of the record removal (no new credit)

`RES_00BA_1509` is **already credited**. Its credit rests on the semantic translation `pr_rec_res_00ba_1509`
([RESIDENT_SIDECAR_REMOVE.md](RESIDENT_SIDECAR_REMOVE.md)), which wR:p1's #1482 checks against the original bytes
(`res00ba_rec_models_audit.py`). That model is an io-callback interface (release, copy and presentation callbacks), not a CPU.

This change adds the **raw-CPU, full-state model** the hybrid executor needs. Route 3 reaches this body twice (codex's glue-gap census,
i22516). It is the 23B7/2E03 pattern, but the closure is the largest of the three: StrLoad, the resident prompt 0709:0BF5 with its whole
closure (ReadKey, EGA), and FreeMem. **No credit or ledger change is proposed.** Assigned by par-a (i22518).

**Pre-claim grep** (VP i22025, widened by PM i22086), on main be4827a3: `grep -rniE '(\+|0x|:)1509\b|RES_00BA_1509'` over
`src/recovered`, `tools/overlay`, `include` and the docs. Every hit was opened:
- the semantic `pr_rec_res_00ba_1509` (`resident_sidecar_remove.c/.h`), wR:p1's io adapter and the rec-models audit's span table;
- the ledger and inventory rows, the strict queue's callback note, `partial_callee_models.csv`;
- U77_STACK_189D, which calls `BA:1509` and stubs it (a caller, for the glue);
- `+1509` in `overlay_u25_string_cores.c` and `overlay_u6f_party_insert_1446_cpu.c`: other segments' offsets, unrelated.

No raw translation existed.

## Executable extent

START.EXE resident segment 00BA **[1509,15F7), 238 bytes / 83 instructions**, from the relocated image of
`res0899_quit_0000_cpu_audit.load_image`. Five far calls: StrLoad 0AF8:0B0B at 1524 and 15B3, the prompt 0709:0BF5 at 1529 and 15B8,
FreeMem 0AF8:0254 at 15EC. Their five segment words (1527, 152C, 15B6, 15BB, 15EF) are the only relocations. No interrupt or port
instruction. The one return is `RETF 8` at 15F4. The two Pascal literals at CS:14CE and CS:14E2 have no relocation; their lengths and
sha256 are in the evidence JSON, not their bytes.

## What the body does

Arguments: the record far pointer at `[BP+6]/[BP+8]`, the character far pointer at `[BP+0Ah]/[BP+0Ch]`.

1. `PUSH BP; MOV BP,SP; SUB SP,2Ch`.
2. **A null record** (OR of its two words): StrLoad the message at CS:14CE into `[BP-18h]`, call the prompt with colour 0Eh, return.
3. If the character's head `+0C8h/+0CAh` is the record, the head := the record's link `+2Ah/+2Ch`.
4. Otherwise walk from the head through `+2Ah` until the node whose link is the record:
   - **not found** (the walk reaches null): StrLoad the message at CS:14E2 into `[BP-2Bh]`, the prompt, return;
   - **found**: the predecessor's link := the record's link.
5. The record's link := 0:0; `FreeMem(record, 3Fh)`.
6. `MOV SP,BP; POP BP; RETF 8`.

A cyclic list never ends, as in DOS.

## Raw contract and stops

`pr_res_00ba_sidecar_remove_1509_cpu(bus, size, cpu, budget, env, diagnostic)`:
- exactly 1 MiB of A20-off RAM; IP 1509; entry SP in `[0200h,FFF2h]`;
- **each instruction of this body costs one unit of `*budget`**; StrLoad, the prompt and FreeMem do not (the prompt and FreeMem have
  their own caps in `*env`);
- **the bus:** every access through a far pointer the body did not create (the record, the character, the list nodes) goes through
  `pr_bus`; the stack, the code and DS globals are flat. StrLoad and FreeMem keep their flat contracts, the prompt its own (bus, EGA,
  ReadKey environment). **Declared exclusion:** FreeMem reaches the freed record flat, so a record in the A0000..AFFFF aperture is freed
  through RAM, not the planes (as for 0CC3).

The flags follow the pinned DOSBox-X stored rules: OR and XOR clear CF, OF and AF. The arithmetic helpers are 0CC3's.

| Stop | Pinned by |
| --- | --- |
| RETURNED | Unicorn and native |
| LIMIT | Unicorn: the budget sweep and the cyclic lists, with equal state and equal budget at the same unexecuted instruction; C test |
| RUNERROR | Unicorn: FreeMem's RunError 204 at SYSTEM:00E2 is a named stop in both runs (a record at offset 4; a record outside the heap); C test |
| KEY | Unicorn: the prompt's ReadKey EXIT (ctrl-C) and BREAK, through 0BF5's diagnostic |
| CALLEE | C test: the prompt's own stop (no prompt code) after StrLoad copied the message |
| CODE | C test: a far-call site that is not the original, at the call before its frame |
| TRANSFER | Unicorn: `char-on-frame+0` and `+2`, where the character's head word is the caller's return address on the frame; the unlink rewrites it and RETF 8 goes elsewhere, in both runs |
| ARGUMENT | C test: null bus, CPU, budget or env; short RAM; IP; SP. CPU and RAM untouched |

## Checks

**Original-byte Unicorn differential: 126 cases, 0 mismatches.** It runs on the 0BF5 audit's world: the loader-expanded image at a test
base with its relocations, the independent Python EGA on the A000 aperture, the declared environment (INT 16h from a per-case key script,
INT 21h 25h/35h/2Ch, ports 40h/43h/61h/C0h, QUIT's INT 10h stub, the EGA ports), and a Turbo Pascal heap under the program's DS. Compared
after every case, **unmasked**: CPU14, **all 1,048,576 RAM bytes**, the full EGA state, the ordered EGA OUT events, the ordered VRAM write
operands, the ordered environment events, the stop, and **the remaining budget**.
- 78 RETURN, 42 LIMIT, 2 RUNERROR, 2 TRANSFER, 1 EXIT, 1 BREAK.
- **83/83 instruction starts, 14/14 conditional outcomes.**
- **77 independent statements** on plain returned cases, without the C model: which message the body loaded (its length byte in
  the frame temporary), the head and every remaining link after the unlink, and the removed record's cleared link.

Cases:
- a null record (modes 1 and 3); absent from an empty list and from a list;
- removing the head, the middle, the tail, the only record, and from five records in shuffled order;
- records sharing a segment at offsets 0 and 8 (both valid FreeMem blocks): the head compare and the walk compare each match the segment word
  and miss the offset (1552→156E, 1589→158B; the quick round's two unreached outcomes);
- FreeMem: the block just under HeapPtr; RunError for a record at offset 4 and for a record outside the heap;
- two cyclic lists (absent: LIMIT; found late: RETURNED);
- the prompt's keys: an extra key, two keys, a buffered key; ctrl-C with and without a command tail (EXIT); Ctrl-Break (BREAK);
- every flag image of the 0BF5 audit, on a removal and on a null record; five entry SPs, each on a removal and a null record;
- the character's head on the frame (three offsets);
- a budget sweep; random lists, targets, modes, flags and registers.

**Mutation: 12 deliberate mutants, 12 caught by the differential, after an unmutated baseline on the same 18-case subset passed
(13 RETURN, 3 LIMIT, 1 RUNERROR, 1 EXIT; PM rule i22687).** Every case's world is built from its own name; the random cases are seeded by
name, never by run order, so a subset builds the same worlds as the full run.
- the null test reads one word; the first message at 14CF; colour 0Fh; the head at +0C6h; the walk's link at +28h; the record's link not
  cleared; FreeMem size 40h; RETF 6; the walk compare skips the segment word; the prompt's KEY stop read as CALLEE; FreeMem's RunError read
  as CALLEE; the walk not charged to the budget.
- An earlier round found two things, both fixed before this round:
  - **"the record's link not cleared" survived as a case gap.** Every removed record linked to a block at offset 0, so writing 0 to the
    offset word changed nothing. `remove-linked-to-offset-8` catches it (1 RAM byte).
  - **"the walk compare skips the segment word" did not compile** (an unused label). It was rewritten to keep the label, and now it is caught.
    A mutant that does not build is not counted either way.
- The quick round also left two conditional outcomes unreached (1552→156E, 1589→158B). The same-segment cases close both.

**Native (DOSBox-X machine=ega; packaged DOSBox-X 2026.01.02, sha256 99d74601, the same release as pinned 5a9c7c53):** **23 cases, 7,907,584 units compared, 0 mismatches, no masks** (CPU14, the six regions `6000:EC00+400`, `3C7C:0000+10000`, `4D00:0000+3000`, `5800:0000+200`, `5000:1000+800`, `2F00:0000+100`, the key counts and 4 × 65,536 plane bytes per case; the 6 prompt cases drew 15,360 plane bytes). The cases: a null record and an absent record in modes 1 and 3; removing the head, middle, tail, only record and from five; an empty list; the block under HeapPtr; the two same-segment compares; two keys; seven flag images on a removal and two on a null record.
- The world is the 0BF5 native's: the relocated image re-read before every case, video mode 0Dh with the planes pre-filled through the real
  ports, INT 16h hooked to replay the key script. Each case's DS, heap, character, font and PSP regions are built on the C side and read by
  the oracle program, so both runs start from the same bytes.
- Compared: CPU14, the stack window (minus the 10 DOS-side bytes under each INT 16h call), every loaded region, the key count, and all
  four planes. Not compared: OUT events and the EGA indexed registers. RunError cases are not run natively (they end the program).

**C test** (CHECK macro, independent of NDEBUG; 27 checks): removing the head (head, links, the free-list block), the middle of five,
the top block (HeapPtr lowered), RunError, a null record (StrLoad's copy, then the prompt's stop), an absent record, a cyclic list to LIMIT,
CODE at a changed call site, budget 0, the guards.

**Census:** no new flag rows. The helpers are 0CC3's; the arithmetic row `1b69180a…` now lists 0CC3 and 1509, with a widened caller
description.

**Aperture census:** a new `bus,yes-with-ega-oracle,yes` row; the two (caller, flat callee) pairs are classified: StrLoad of the code
literals into frame temporaries, and FreeMem of the caller's record (the declared exclusion).

**credit_evidence_kind.csv:** regenerated. `RES_00BA_1509` now lists this checker beside `res00ba_rec_models_audit.py`. That touches a
credit table, but **the credit itself is unchanged**.

## What this does not show

- Credit or identity: unchanged.
- The hybrid glue: par-b's next step, after merge.
- Cyclic lists beyond the budget. They never end, by contract.
- A record inside the A000 aperture being freed: declared exclusion (FreeMem is flat).

## Reproduction

    python3 tools/overlay/res00ba_sidecar_remove_1509_cpu_audit.py --unicorn --mutate --native --output resources/res00ba-1509-evidence.json
    python3 tools/overlay/res00ba_sidecar_remove_1509_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main RES_00BA_1509
