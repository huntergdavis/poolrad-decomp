# OVL_0038 +0CC3: the U9E dispatcher's slot-40 handler (remove a map object), composed over StrLoad, resident 1693 and FreeMem (strict body; credit is par-e's)

`OVL_0038_0CC3` (unit 0x38, entry 34, stub `0041:00CA`) removes one object from the map:
- **Find.** It looks in the far-pointer list at `DS:[6DBB]` for the node whose first dword is the 32-bit key and whose type
  byte `+12h` equals the object record's byte `+3` shifted right by 4.
- **Message.** If one is found, it shows a 26-byte message through resident `00BA:1693`.
- **Restore the cells.** For each of the node's four cells it puts the map back: 1Fh where an entry of the 7-byte table at
  `DS:66D6` sits on the cell, otherwise the byte the node saved.
- **Unlink and free.** It unlinks the node and frees it with SYSTEM FreeMem.
- **Re-stamp.** It marks 1Eh on every cell of every node still in the list.

This change adds the model, its C test, its checker, the native harness for the not-found walk, the mutation set, three census
rows and this document. It also adds an optional named-stop map to the shared unit-0x7F world's runner. **par-e decides credit.**

## Claim table

| Item | Value |
| --- | --- |
| Row | `OVL_0038_0CC3`: game_evidenced, grade A, state, composable, 0 unready callees; claim i22096, approved i22097 |
| Extent | **[0CC3,0FA2), 735 bytes / 266 instructions**, ending at the only return `RETF 0Ah` at `+0F9F`. The queue's 745-byte bound runs into the next routine's 9-byte string at `0FA2` |
| Model | `src/recovered/overlay_u38_0cc3_cpu.c`, `include/poolrad/recovered/overlay_u38_0cc3_cpu.h` |
| C test | `tests/recovered/test_overlay_u38_0cc3_cpu.c` (asserts on) |
| Checker | `tools/overlay/ovl_0038_0cc3_cpu_audit.py`, `CHECKER_ROWS = ('OVL_0038_0CC3',)`, `--compile-only`, `--mutate`, `--native` |
| Callees | SYSTEM StrLoad `0AF8:0B0B`, resident `00BA:1693` (resident_covered, its own audit), SYSTEM FreeMem `0AF8:0254` (runtime_covered, `system_freemem_0254_cpu.c`) |
| Census | three new helper rows, `overlay_u38_0cc3_cpu.c:arith`, `:mulw` and `:shr16`, classified; `flag_census.py --check`: 0 gaps |

**Role line:** removes a map object: finds its node by key and type in the object list at `DS:6DBB`, shows a message,
restores its four map cells (1Fh under a listed figure, else the saved byte), unlinks and frees the node, then re-marks
(1Eh) the cells of every remaining object.

## Identity

**Factor 1: a new class.** The handler has **no call site**. It is reached only through the U9E dispatcher's table:

| Link | Bytes |
| --- | --- |
| L1, the installer store | credited `OVL_0038_31A2`, `+33A0 B8 CA 00` mov ax,00CAh; `+33A3 BA 41 00` mov dx,0041h; `+33A6 A3 C8 68` mov [68C8h],ax; `+33A9 89 16 CA 68` mov [68CAh],dx. **The segment word `0041h` at `+33A4` is fixup-relocated** |
| L2, the slot | 68C8h = 6828h + 4·28h: slot 40 |
| L3, the dispatch | `OVL_009E_0000` `+0013` `mov al,[bp+10h]; xor ah,ah; mov di,ax; shl di,1; shl di,1` then `+001E lcall [di+6828h]` |
| L4, the stub | `0041:00CA` is `CD 3F C3 0C 00`: INT 3Fh to offset 0CC3h of unit 0x38, whose stub header is at file offset 0EC5Bh |

**Limit, stated:** of the dispatcher's 11 far call sites, the 8 that pass a constant use 0Bh, 17h, 2Bh, 2Ch, 3Ah and 4Eh,
never 28h. Slot 40 is reached only through the record-byte callers or the undecoded near callers
([OVL_009E_0000_HANDLERS.md](OVL_009E_0000_HANDLERS.md)). The PM sent L1–L4 to the VP with a PASS recommendation (i22112).

**Factor 2: the body uses documented game state:**
- it **writes** map bytes through the map pointer `DS:6716/6718` ([U31_CANDIDATE_CHECK_0255_CPU.md](U31_CANDIDATE_CHECK_0255_CPU.md));
- it **compares** each cell with the coordinates of the 7-byte records at `DS:66D6`, count `DS:6715`
  ([U5E_NEIGHBOR_1BF8.md](U5E_NEIGHBOR_1BF8.md));
- it adds the step tables `DS:2736/273F` ([OVL_0068_068D_CPU.md](OVL_0068_068D_CPU.md)) to each node's coordinates;
- it **edits** the object list at `DS:6DBB`. That list is not yet named in any document, and this note does not name it.

## Frame and contract

After the far call:
- `[SP]` IP, `[SP+2]` CS;
- `[SP+4]` the dispatcher's forwarded byte, unused;
- `[SP+6]/[SP+8]` the far pointer to the object record;
- `[SP+10]/[SP+12]` the 32-bit key;
- `RETF 0Ah`.

Locals:
- `[BP-28h..]`: the message temporary;
- `[BP-0Eh]`: the table count;
- `[BP-0Dh]/[BP-0Ch]`: y and x;
- `[BP-0Bh]`: the cell index i;
- `[BP-0Ah]`: the type nibble, later the table index;
- `[BP-9]`: the found flag;
- `[BP-8..-5]`: the node;
- `[BP-4..-1]`: the walk pointer.

The node layout, as the body reads it:
- `+0` the key dword;
- `+4` the next link;
- `+8..+0Bh` the saved bytes;
- `+0Ch..+0Fh` the live flags;
- `+10h/+11h` the signed x and y;
- `+12h` the type.

Contract: `pr_u38_0cc3_cpu(bus, size, cpu, budget, limits, diagnostic)`:
- exactly 1 MiB through the bus, with an EGA attached for the message;
- IP 0CC3;
- entry SP in `[0200h,FFF2h]`.

Every access through a far pointer the body did not create goes **through the bus**: the record, the list nodes and the map.
The stack, the code and the DS globals are flat. StrLoad, 1693 and FreeMem keep their own contracts.

Budget: each instruction of this body costs one unit. The callees cost none; 1693 and FreeMem have their own caps in `limits`.

| Stop | Class | Pinned by |
| --- | --- | --- |
| RETURNED | RETURN | Unicorn: 136 of 453 cases; native (not-found walk): 16 of 16 |
| RUNERROR | architectural: FreeMem's RunError 204 on an invalid block, CPU at SYSTEM:00E2 | Unicorn: 2 cases, `runerror-offset-4` (a block at offset 4) and `runerror-outside-heap` (a block above HeapPtr); both runs reach SYSTEM:00E2 |
| LIMIT | host cap | Unicorn: 313 cases. The budget sweeps, the three cyclic lists, and the alias cases whose map or node writes make the walk cyclic (`grid-on-frame-08/-06`, `grid-on-node+04/+06`, `grid-on-list-head`, `node-on-frame`) stop with equal state at the same arrival |
| TRANSFER | control transfer | Unicorn: 2 cases, `grid-on-frame+02/+04` (the map cell over the return address); the oracle stops at the same garbage address |
| ARGUMENT / CODE / CALLEE | non-architectural | C test; 0 in the differential |

## Evidence

**Original-byte Unicorn differential on the composed closure: 453 cases, 0 mismatches.**
- Every body runs as its original bytes at its own segment: this span, StrLoad, FreeMem's four spans, and the 1693 closure with
  the independent Python EGA.
- The span's three relocated segment words are installed from the unit's fixup table.
- Compared after every case, **unmasked**: CPU14, **all 1,048,576 RAM bytes**, the EGA planes, latches and registers, the ordered
  OUT events and VRAM operands, the stop and the remaining budget.

Outcomes:
- 136 RETURNED, 313 LIMIT, 2 RUNERROR, 2 TRANSFER; 0 ARGUMENT/CODE/CALLEE;
- **266/266 instruction starts** in this body (1,239 across the closure);
- **104 independent statements**.

What the independent statement computes, on plain returning cases (no aliasing, a valid heap, an acyclic list), without the C model:
- the matched node;
- each restored cell, 1Fh or the saved byte;
- the list head or predecessor link after the unlink;
- every 1Eh re-stamp;
- the message temporary, SP and CS:IP.

It encodes two facts the bytes taught. A first version got them wrong, and the differential caught it:
- the restore loop keeps the cell as two bytes, zero-extended, while the re-stamp loop keeps 16-bit sums `CBW(base) + CBW(step)`;
- the overlap test compares `CBW(table x)` with the zero-extended cell byte, so a cell at x ≥ 80h never matches.

**Declared splits, counted and never masked** (shared `pinned_shift_flags.py`):
- **24** states under the pinned-MULW split: `MUL DX` keeps SF and AF, and PF covers DX:AX;
- **16** states under the AF-after-SHR split: `SHR AX,CL` with CL = 4 sets AF in the pinned core.

Cases:
- the walk: empty list, no key, wrong type, high-word mismatch, both record nibbles, three type values;
- the unlink at head, middle, tail and only node, and the first of two matches;
- live-flag patterns;
- table counts 0/1/2/5, including a zero-dword entry and a high-word-only entry;
- signed and extreme coordinates and steps;
- the display world: four modes, three pages, three delay settings;
- six entry SPs and ten flag images;
- FreeMem: a valid offset-8 block, and two RunError blocks;
- three cyclic lists;
- the map cell on each word of the frame and on each field of the node, and over the list head;
- the record on the frame, and a node on the frame;
- a budget sweep 0..259 plus five higher budgets, and a missed-walk sweep 0..39;
- 40 random cases.

1,821 s on the heavy wrapper.

### Mutation

**24 deliberate mutants, 24 caught by the differential.** The run used the `ovl_0038_0cc3_mutation.py` case subset. It took two
rounds, both stated here.

**Round 1:** 19 caught. The mutants:
- the type shift;
- the key's high word;
- the type, link and live-flag offsets;
- the message offset;
- the table stride, and the loop bound JA→JAE;
- the 1Fh and 1Eh marks;
- the row width 51;
- the saved-byte offset;
- three cells instead of four;
- the head store;
- the FreeMem size;
- RETF 8;
- the TRANSFER check;
- RunError read as CALLEE;
- the 1693 call not charged to the budget.

**5 survived.** Each was a **gap in the cases**, not an equivalence:
- *The overlap test ignores the high word.* The fixture called `table-high-word-only` built a **low**-word-only entry. It is now
  `table-low-word-only`, and a true high-word-only entry was added.
- *SHR leaves AF clear*, *the step table zero-extended* and *MUL clears SF*. Each differs only at a budget stop inside a
  window of a few instructions. Three targeted stops were added at budgets measured with the C model:
  - 9, after `SHR AX,CL`;
  - 65, after the `CBW` at 0D7E of a negative step;
  - 515, at 0F75 after the re-stamp `MUL`, with SF set by the `ADD` at 0F6E.
- *The map write bypasses the bus.* No case put the map in the aperture. `grid-in-aperture` (map at A000:1000, compared
  through the EGA planes) was added.

**Round 2:** with the extended subset, all 5 are caught:
- `stop-after-cbw` (AX 00FF vs FFFF);
- `table-high-word-only`;
- `stop-after-shr` (the declared AF split demands AF);
- `stop-after-mul` (the split expects SF kept);
- `grid-in-aperture` (the planes).

The six new or changed cases match the original exactly in the differential.

### Native corroboration (partial; packaged DOSBox-X, not the pinned build)

DOSBox-X normal core, 386. **16 synthetic cases, all on the NOT-FOUND walk.** `+0CC3` runs as original bytes under a real
DOS, and the data, stack, node and record segments are loaded by real DOS reads. Cases:
- an empty list;
- one other key, the right key with the wrong type, and a high-word mismatch;
- five non-matching nodes;
- type nibbles 0 and Fh, and a record low nibble of Fh;
- five flag images;
- SP at both bounds, and an odd SP.

Against the C model on identical state, all 14 CPU words and **4,194,752 bytes** match: **0 mismatches, no masks.**
IF is cleared in every native case.

The host's packaged `/usr/bin/dosbox-x` was used (SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`),
not the pinned `5a9c7c53` build. Every found path goes through 1693's EGA display, so it is not run natively; see Not established.

## Not established

- The found path natively. The 1693 native machine is the same declared gap as for `+0203` and `+0A88`.
- A record, node or map in the A0000..AFFFF aperture in the differential. The bus path rests on the C test, where a node in the
  aperture is matched through plane data.
- The 1693 closure's and FreeMem's own caps; both are generous here.
- Natural reachability of slot 40: see the factor-1 limit.
- The meaning of the object list at `DS:6DBB`.
- Credit: par-e's.

## Reproduction

    python3 tools/overlay/ovl_0038_0cc3_cpu_audit.py --unicorn --output resources/38-0cc3-evidence.json
    python3 tools/overlay/ovl_0038_0cc3_cpu_audit.py --mutate --output resources/38-0cc3-mutation.json
    python3 tools/overlay/ovl_0038_0cc3_cpu_audit.py --native --output resources/38-0cc3-native.json
    python3 tools/overlay/ovl_0038_0cc3_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --require-current-main OVL_0038_0CC3
