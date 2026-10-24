# RES_00BA +23B7: the shared raw entry for the character field-zero test (carved out of 0255; no new credit)

`RES_00BA_23B7` is **already strict**. Its credit rests on the semantic `pr_rec_res_00ba_23b7`
([RESIDENT_CHARACTER_FIELD_ZERO.md](RESIDENT_CHARACTER_FIELD_ZERO.md)), which #1482 checks against the original bytes.

A full-state translation of this body already existed, **inlined** in the credited model of unit 0x31 `+0255`
(`overlay_u31_candidate_check_0255_cpu.c`, "translated here in full state", with its `SUB SP,2` flags folded). As the PM
ruled (i22084), this change is a **carve-out, not a second copy**. That code is lifted into one shared raw entry,
`pr_res_00ba_zero_test_23b7_cpu`, which computes the **full pinned flags at its RETF** so that the folding stops there.
`+0255` now calls it. **No credit or ledger change** is proposed.

**Pre-claim grep (VP i22025, widened per PM i22089).** The hit inside the credited `+0255` model is this carve-out's
source. The named `(\+|0x)` grep missed it, because that file writes `00BA:23B7`; the audit hit led to it (i22083).

## Executable extent

- **Source of the bytes:** the relocated START.EXE image from `res0899_quit_0000_cpu_audit.load_image`. There is no
  relocation in the span.
- **Body:** resident 00BA `[23B7,23DB)`, **36 bytes / 13 instructions**, SHA-256
  `1591e4ada52c8b030f52ef6631d5274e94a2fb1df636f9d58fce5a15cc743c50`. It is a leaf; its only return is `RETF 4` at
  `23D8`.

## What the body does

1. `PUSH BP; MOV BP,SP; SUB SP,2`.
2. `LES DI,[BP+6]` loads the character far pointer.
3. `CMP BYTE ES:[DI+10Eh],0; JNE 23CE`. The result byte `[BP-1]` is 1 when that byte is zero, else 0.
4. `MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 4`.

**Flags at return** are CMP's, computed with the pinned stored rules. `SUB SP,2`'s six flags are computed too, and then
overwritten.

**`LES`** follows pinned DOSBox-X: the segment word is read at `(offset+2) mod 64K`, as measured on 2E03, #1494. Under
this entry's SP guard, `[0004h,FFF8h]`, `BP+6` is at most FFFCh, so the wrap itself is unreachable here.

## Raw contract and stops

`pr_res_00ba_zero_test_23b7_cpu(ram, size, cpu)`:
- exactly 1 MiB of A20-off RAM;
- IP `23B7h`;
- entry SP in `[0004h,FFF8h]`.

| Stop | Pinned by |
| --- | --- |
| RETURNED | Unicorn 243/243; native 24/24 |
| TRANSFER (`RETF 4` popped another return address) | a guard; the body writes no return word |
| ARGUMENT | C test (asserts on): CPU and RAM untouched |

## Checks

The audit is `tools/overlay/res00ba_zero_test_23b7_cpu_audit.py` (`CHECKER_ROWS = ('RES_00BA_23B7',)`). The comparison
covers **CPU14 and all 1 MiB, with flags unmasked at the RETF**.

**Unicorn: 243 cases, every one equal.**
- **Coverage:** 13/13 instruction starts, 2/2 conditional outcomes, 235 independent statements (AL from the rule, AH
  kept, and the frame on return).
- **The field byte:** 00h, 01h, 7Fh, 80h, FFh, 10h and 03h.
- **Wraps:** the character offset wraps the +10Eh sum, and the character segment sits at the A20 wrap.
- **Machine state:** six incoming-flag values on both outcomes, and the SP edges.
- **Aliasing:** the field byte on each frame byte.
- **200 random cases.**

**Mutants: 8. Six are caught.**

| Mutant | Result |
| --- | --- |
| the field is at +10Fh | caught |
| the result is inverted | caught |
| CMP not computed (only ZF set) | caught |
| CMP parity from the wrong byte | caught |
| AL is the field byte | caught |
| RETF pops 2 | caught |
| **CMP leaves AF from SUB** | **survives, and is equivalent**: for `CMP x,0`, AF = (x ^ 0 ^ x) & 10h = 0 always |
| **LES reads its segment contiguously** | **survives, and is equivalent within the contract**: the SP guard keeps BP+6 ≤ FFFCh, where both reads coincide |

**C test:** `tests/recovered/test_resident_00ba_zero_test_23b7_cpu.c`, built with ASan/UBSan and asserts on. It covers:
- the zero and non-zero field, with CMP's flags;
- the top SP edge;
- atomic argument rejection.

**Native:** `tools/overlay/res00ba_zero_test_23b7_native.py`, DOSBox-X normal/386 (the host's packaged build, sha256 `99d74601…0df3`), with no masks and IF cleared. **24 of 24
cases pass, comparing 7,864,992 bytes.** They cover field values 00h/01h/80h/FFh/03h, the +10Eh offset wrap, four
incoming-flag values on both outcomes, the SP edges, and the field byte on each frame byte.

## `+0255` after the carve-out

`+0255`'s `resident_call` pushes the far frame exactly as before, then calls `pr_res_00ba_zero_test_23b7_cpu`. It
records the entry's status in `*callee_status` (`PR_U31_0255_RESIDENT`) and maps `ARGUMENT` to `CALLEE`.

**Re-run after the change:**
- `+0255`'s C test passes 158 checks (asserts on);
- `+0255`'s audit gives 174/174 original-byte cases equal (166 return, 4 CODE, 3 CALLEE, 1 TRANSFER), with the full 1 MiB and all 14 CPU
words; 58/58 starts and 6/6 outcomes; 286 resident 23B7 fetches through the shared entry.

There is an Evidence-note line in [U31_CANDIDATE_CHECK_0255_CPU.md](U31_CANDIDATE_CHECK_0255_CPU.md).

## What this does not show

- **No credit and no identity claim.**
- **Not yet glued into the hybrid.**
