# RES_00BA +2E03: raw-CPU model of the character link check (no new credit)

`RES_00BA_2E03` is **already strict**. Its credit rests on the semantic translation `pr_rec_res_00ba_2e03`
([RESIDENT_CHARACTER_LINK_CHECK.md](RESIDENT_CHARACTER_LINK_CHECK.md)), which wR:p1's #1482 checks against the original
bytes (`res00ba_rec_models_audit.py`). That model takes an io struct, not a CPU.

This change adds the **raw-CPU, full-state model** the hybrid executor needs. The hybrid's V1 shadow compares CPU14 and
all 1 MiB, so an io adapter around the semantic model could not reproduce the flags or the frame (PM i22007, item E).
**No credit or ledger change** is proposed. Assigned by par-a as batch-8 step 2 (i22065, i22082).

**Pre-claim grep (VP i22025)**, on pulled main:
- `grep -rniE '(\+|0x)2E03' src/recovered tools/overlay include | grep -iE '00ba|res_?00ba|rec_'` gives 2 hits, both in
  `res00ba_rec_models_audit.py`;
- the row ID matches only the two semantic headers and that audit;
- a broader grep with no prefix finds only the semantic `resident_character_link_check.c` and `link_flags.c`.

No raw translation existed. `deja` was not used; the memory service and the tree were searched instead.

## Executable extent

- **Source of the bytes:** the relocated START.EXE image from `res0899_quit_0000_cpu_audit.load_image`. The image
  SHA-256 is checked; there is no relocation inside the span.
- **Body:** resident 00BA `[2E03,2E46)`, **67 bytes / 23 instructions**, SHA-256
  `2529d93502c187823bf86207d07e508ad1e5d70680bcf72802e6ac2ed347ffe7`. It is a leaf with no call, interrupt, port or loop,
  and its only return is `RETF 4` at `2E43`.

## What the body does

The frame is `[SP]` IP, `[SP+2]` CS, `[SP+4]`/`[SP+6]` the character far pointer.

1. **Enter.** `PUSH BP; MOV BP,SP; SUB SP,2`. The result byte is `[BP-1]`.
2. **Test the link.** `LES DI,[BP+6]`, then `MOV AX,ES:[DI+CC]; OR AX,ES:[DI+CE]`. If the link far pointer is zero,
   `JE 2E39`, and the result is 0.
3. **Follow the link.** `LES DI,[BP+6]` again, then `LES DI,ES:[DI+CC]` loads the link and `MOV AL,ES:[DI+2E]` reads
   the index byte.
4. **Look up the table.** `XOR AH,AH; MOV DI,AX; MOV CL,4; SHL DI,CL`, then `CMP BYTE [DI+558E],1; JBE 2E39`. Above 1,
   unsigned, gives 1; otherwise 0.
5. **Leave.** `MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 4`.

**Flags at return**, under the pinned stored rules:
- **zero-link path:** OR's flags (CF, OF and AF clear);
- **other path:** CMP's flags.

SHL's AF=1 and its stored OF are computed and then overwritten. CL is left at 4 and AH at 0 on the link path.

**`LES` is modelled as pinned DOSBox-X does it (prefix_none.h C4h).** The offset word is read at the EA. For an offset
above FFFCh the segment word is read at EA+2−10000h, that is at `(offset+2) mod 64K`. **This was measured natively**:
the `char-wrap` case put the character's link at offset FFFF. DOSBox-X took the segment from offset 0001, giving DI 0050;
the first model followed Unicorn and read physically contiguous, giving DI 0A50, and the native run caught it.

## Role

**A test of whether the character's link names a table entry above 1** (RESIDENT_CHARACTER_LINK_CHECK.md). Further game
meaning is not asserted.

## Raw contract and stops

`pr_res_00ba_link_check_2e03_cpu(ram, size, cpu)`:
- exactly 1 MiB of A20-off RAM;
- IP `2E03h`;
- entry SP in `[0004h,FFF8h]`;
- every operand is reread in source order;
- all accesses are flat RAM.

| Stop | Pinned by |
| --- | --- |
| RETURNED | Unicorn 270/270; native 28/28 |
| TRANSFER (`RETF 4` popped another return address) | not reached: nothing in the body writes the return frame. Kept as a guard |
| ARGUMENT | C test (ASan/UBSan, asserts on): CPU and RAM untouched |

## Checks

The audit is `tools/overlay/res00ba_link_check_2e03_cpu_audit.py` (`CHECKER_ROWS = ('RES_00BA_2E03',)`). The original
bytes run in Unicorn, and the C runs on the same state. The comparison covers **CPU14 and all 1,048,576 RAM bytes, with
no flag mask**.

**Unicorn: 270 cases, every one equal.**
- **Coverage:** 23/23 instruction starts, **4/4 conditional outcomes**, 246 independent statements (AL from the stated
  rule, and the frame on return).
- **The link:** no link; half links, where only one word is non-zero.
- **The table:** values 0, 1, 2, 7Fh, 80h and FFh, around the unsigned JBE.
- **Index values:** up to FFh, so the shift reaches DI = 0FF0h.
- **Wraps:** character and link offsets that wrap 16-bit sums; segments at the A20 wrap.
- **Machine state:** six incoming-flag values on both paths, and the SP edges.
- **Aliasing:** DS = SS with the table entry on each frame byte; the link's index byte and the character's link words on
  the frame.
- **200 random cases.**

**Declared split (VP i19555 rule).** Unicorn reads `LES`'s segment word contiguously. Only for the one case whose `LES`
offset is above FFFCh (`char-wrap-ff33`), the Unicorn-side fixture also holds the same segment word at the contiguous
address. The native run checks the wrapped-only state against DOSBox-X.

**Mutants: 10. Nine are caught**, 8 by the differential and 1 by the C test.

| Mutant | Result |
| --- | --- |
| only the low link word is tested | caught |
| the index byte is at link+2Fh | caught |
| the table stride is 8 (SHL 3) | caught |
| signed compare (JLE) instead of JBE | caught |
| OR leaves AF set | caught |
| CMP PF from the wrong byte | caught |
| the local is not cleared on the zero path | caught |
| RETF pops 2 | caught |
| LES reads the segment contiguously (the Unicorn behaviour) | caught by the C test (its decoy segment at physical +2) |
| **the character pointer is not re-read** | **survives, and is equivalent**: no store happens between the two `LES DI,[BP+6]`, so ES:DI already holds the same value |

**C test:** `tests/recovered/test_resident_00ba_link_check_2e03_cpu.c`, built with `cc -O1 -g -fsanitize=address,undefined`.
NDEBUG is not defined, so asserts are on. It covers:
- no link (OR's flags);
- table values 2, 1, 0 and 80h;
- a link with only the segment word non-zero;
- `LES` at offset FFFF with a decoy segment at physical +2;
- atomic argument rejection.

**Native:** `tools/overlay/res00ba_link_check_2e03_native.py`, DOSBox-X normal/386 (the host's packaged build,
sha256 `99d74601…0df3`), with no masks and IF cleared. 28 cases pass, comparing 9,175,824 bytes. They include `char-wrap`, which is
wrapped-only. The character's link words are placed on the frame at +4/+6 only. At −2/0 the link would be the saved BP and the return
address, which name low DOS memory or the oracle program: real DOS bytes there, fixture background in the C. Unicorn covers those.

## What this does not show

- **No credit and no identity claim.**
- **Not yet glued into the hybrid.**
