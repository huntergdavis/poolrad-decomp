# OVL_03BD +057A: raw-CPU model of unit 0x3BD public entry 4, the cell lookup (no new credit)

`OVL_03BD_057A` (unit 0x3BD, offset 057Ah, public entry 4, run-time stub `03D5:0034`) is **already strict**. Its credit
rests on the semantic translation `pr_u3bd_entry4` ([U3BD_CELL_ENTRY4.md](U3BD_CELL_ENTRY4.md)), which takes a cell
state struct and x/y bytes, not a CPU.

This change adds the **raw-CPU, full-state model** the hybrid executor needs to run the body in place in the live game.
It composes the existing raw model of the `+031F` range leaf (`pr_u3bd_in_grid_031f`). It proposes **no credit and no
ledger change**. Assigned by par-a (i21416) as batch 6, after `OVL_03BD_0155`.

`deja` recall could not be run (its MCP server fails with ENOENT). The prior work is cited:
- U3BD_CELL_ENTRY4.md, for the semantics and the map-id gate;
- `overlay_u3bd_lookup_fullstate.c` (commit `16bbc747`), for the house pattern of composing `+031F` on raw RAM.

**Its shift flag is deliberately not copied from that file.** Its `shr16` clears AF, which DOSBox-X does not do. That
defect was reported as i21438, and this model sets AF after `SHL` as DOSBox-X's 386 core does.

## Media and executable extent

- **Media:** START.EXE SHA-256 `d825df2b…a07bd`, GAME.OVR SHA-256 `872964a3…8614`. Both are checked by the audit.
- **Body:** unit 0x3BD `[057A,05F3)`, **121 bytes / 45 instructions**, at GAME.OVR file offset 192,113. SHA-256
  `2932e53408f1bba96864ff51e310132dda4c72411bd11cd4127c3d09ba010037`. It ends in its only return, `RETF 4` at `05F0`.
- **Leaf:** `+031F` `[031F,0350)`, 49 bytes / 18 instructions, SHA-256 `ab4d7f5e…4f281b`. It also ends in `RETF 4`.
- **Checked by the audit's source step:**
  - unit public entry 4 is `+057A`;
  - the one call is `CALL +031F` at `0589` (near, after `PUSH CS`);
  - there is no far call, interrupt or port instruction in either span;
  - there is **no relocation in either span** (the fixup at `05F9` lies past the body).

## What the body does

Frame after the far call: `[SP]` IP, `[SP+2]` CS, `[SP+4]` the y word slot (`[BP+6]`), `[SP+6]` the x word slot
(`[BP+8]`). Each slot is read, and possibly rewritten, as its low byte.

1. **Enter.** `PUSH BP; MOV BP,SP; SUB SP,2`. The byte local is `[BP-1]`.
2. **Range check.** `PUSH x; PUSH y; PUSH CS; CALL +031F`. The leaf returns AL=1 when both bytes are signed 0..15, and
   AL=0 otherwise.
3. **Gate.** `OR AL,AL`. If AL is 0 and the map byte `DS:[84DC]` is 0 or 0Ah, the result is 0. The byte is read twice,
   once for each compare.
4. **Wrap, in place.** Otherwise each of x then y is wrapped into 0..15 with signed compares, rereading the byte after
   each step: above 15 becomes 0, below 0 becomes 15.
5. **Lookup.** `DX := sign(x)`, `AX := sign(y) SHL 4`, `LES DI,[6A5C]`, `DI += AX`, `DI += DX`. The result is the byte at
   `ES:[DI+200h]`, the third 256-byte quarter of the buffer, stored to `[BP-1]`.
6. **Leave.** `MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 4`.

**Flags at return** are those of the last flag-setting instruction:
- **the lookup path:** `ADD DI,DX`;
- **the zero path:** the gate's `CMP`.

`SHL` leaves AF set for a non-zero count in DOSBox-X, but the next `ADD` overwrites it, so no undefined flag reaches the
return. **AH** keeps CBW's sign byte on the lookup path.

## Role

The role is a **cell lookup in the map buffer that unit 0x3BD entry 7 fills** (U3BD_CELL_ENTRY4.md: "cell" names the
byte at (x, y) in the buffer's third quarter). Further game meaning is not asserted.

## Membership chain

Facts only; nothing here admits a member.

- `OVL_03BD_057A` is a row of `strict_full_c_addresses.csv`, with U3BD_CELL_ENTRY4.md as its evidence doc.
- This model adds a raw-CPU entry and changes no count.
- **On the hybrid save walk the body arrives 4 times** (stage (b) observer, walk c).

## Raw contract and stops

`pr_u3bd_cell_057a_cpu(bus, size, cpu)`:

- exactly 1 MiB of A20-off RAM behind a `pr_bus`;
- IP `057Ah`;
- **entry SP in [0010h, FFF8h]**, so the leaf's frame below and the return frame above never straddle the segment end;
- no host budget: the body has no loop, and the leaf has none either.

| Stop | Class | Origin | Pinned by |
| --- | --- | --- | --- |
| RETURNED | RETURN | `RETF 4` to the entry frame's return address | Unicorn: 874 of 874 cases; native: 34 of 34 |
| TRANSFER | control transfer | the leaf's `RETF`, or this body's, popped another return address | **not reached by any case**. No store in the body or the leaf can reach a return word (the wrap stores rewrite x/y in place, `[BP-1]` sits below BP, and the leaf writes only its own frame and local). It is kept as a guard |
| ARGUMENT | ARGUMENT | null bus/RAM/CPU, size, IP, entry SP outside [0010,FFF8], or the leaf rejecting its entry | C test (ASan/UBSan): CPU and RAM untouched. The leaf-rejection path restores the 16 bytes below the entry SP. It cannot occur with the checks above, and is kept for atomicity |

## Pointer census

| Pointer | Source | The body's own access | Bus? |
| --- | --- | --- | --- |
| the buffer far pointer `DS:[6A5C]`/`[6A5E]` | a DS global | the two words are read flat; **the pointee byte `ES:[DI+200h]` is read** | words: no; **the read: yes, through the bus** |
| the map byte `DS:[84DC]` | a DS global | read twice | no (flat DS) |
| the entry frame (x, y, the return address) and the local `[BP-1]` | the caller and the body | reads; x and y are rewritten in place; the local is written | no |
| the leaf's frame | created here | the leaf's pushes, its local and `RETF 4` | no |

**What pins the bus claim.** Unicorn has no planar EGA, so the "buffer read bypasses the bus" mutant survives the
differential, as expected. **It is caught by the C test**: with the buffer at `A000:0000`, the model reads plane 0
(`77h`) and not the RAM byte (`11h`).

## Checks

The audit is `tools/overlay/ovl_03bd_057a_cpu_audit.py` (`CHECKER_ROWS = ('OVL_03BD_057A',)`). The original bytes, with
the leaf, run in Unicorn; the C runs on the same state. The comparison covers **CPU14 and all 1,048,576 RAM bytes, with
no flag mask**.

**Unicorn: 874 cases, every one equal.**

- **Coverage:** 45 of 45 body instruction starts, 18 of 18 leaf starts, **14 of 14 conditional outcomes** of the body.
- **Values:** every x and every y byte, across both signed wrap boundaries, plus the corner pairs.
- **The gate:** maps 0, 0Ah, 1, 9, 0Bh and FFh, each with an in-grid and an out-of-grid pair.
- **Buffer pointer:** offsets that wrap at 16 bits (`FC00`, `FDFF`, `FE00`, `FEFF`, `FFFF`), and segments `FFFF`
  (A20 wrap), `FFF0`, `0000` and `A000`.
- **High bytes; entry SP edges; 11 incoming-flag values,** each on both the lookup and the zero path.
- **DS = SS** with the map byte, then the buffer pointer, on every byte from BP−12h to BP+9. Below BP this is the leaf's
  frame: the leaf's own pushes rewrite the map byte or pointer between the two reads.
- **The looked-up cell aliased to the frame;** three code segments; 240 random cases.

**Independent statements.** 817 cases are also checked without the C model: AL from the stated rule, and SP/BP/CS:IP on
return.

**Mutants: 11. Nine are caught,** 8 by the differential and 1 (the bus bypass) by the C test.

| Mutant | Result |
| --- | --- |
| cell at +1FFh | caught |
| rows 8 wide (SHL 3) | caught |
| x and y swapped | caught |
| unsigned wrap compares | caught |
| map 0Ah not gated | caught |
| `ADD DI,DX` flags not computed | caught |
| result not stored to `[BP-1]` | caught |
| RETF pops 2 | caught |
| buffer read bypasses the bus | caught by the C test |
| **"the map byte is read once"** | **survives, and is equivalent**: nothing is written between the two reads |
| **"SHL leaves AF clear"** | **survives, and is equivalent at every observable point**: `ADD DI,AX` overwrites AF on the next instruction |

**C test:** `tests/recovered/test_overlay_u3bd_cell_057a_cpu.c`, run under ASan/UBSan. It covers the lookup, the gate on
maps 0 and 10, in-place wrapping, the return flags, the bus read with and without an EGA, and atomic argument
rejection.

**Native:** `tools/overlay/ovl_03bd_057a_native.py`, DOSBox-X normal/386 (the host's packaged build, sha256
`99d74601…0df3`), with no masks.

**34 of 34 cases pass**, comparing 11,142,072 bytes:
- nine x/y pairs across the wrap boundaries;
- the gate on maps 0, 0Ah and 1;
- buffer offsets 0000, FDFF and FFFF;
- high bytes;
- six incoming-flag values on both paths, with IF cleared;
- SP edges;
- DS = SS with the map byte on eight frame and leaf-frame bytes.

## What this does not show

- **No credit and no identity claim.**
- **TRANSFER is unreached by construction**; it is kept as a guard.
- **The stub precondition does not apply.** The body calls no other overlay unit, only its own `+031F` near.
- **Not yet glued into the hybrid.**

## Evidence kind (2026-10-22)

Registering `CHECKER_ROWS = ('OVL_03BD_057A',)` moves this row in `docs/re/credit_evidence_kind.csv` from `unit-test` to `checker`, regenerated with `credit_evidence_kind.py --write`. **The credit count does not move**; only the recorded kind of evidence gets stronger.
