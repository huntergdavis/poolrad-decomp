# Resident 0709:0DBE and CRT Delay 0A40:029C: raw-CPU forms (composed)

`pr_r0709_delay_wrapper_0dbe_cpu` is a raw-CPU, full-state model of the
START.EXE resident routine `0709:0DBE..0DD2`. It composes
`pr_a40_delay_029c_cpu`, a raw-CPU model of the Turbo Pascal CRT `Delay`
at `0A40:029C..02C8`. They are the CPU/stack-effect forms of two merged
models:

- [`resident_0709_delay_wrapper_0dbe.h`](RES_0709_DELAY_WRAPPER_0DBE.md)
  takes the A40 callee as a host callback;
- [`resident_a40_delay_029c.h`](RES_A40_DELAY_029C.md) is bounded and reads
  its BIOS byte and count through callbacks.

`0709:0DBE` is a direct callee of `RES 00BA:16EC`, U38 `+1545` and U1C
`+271F`, so this makes it a ready raw-CPU callee for all three. It is the
first step of the 16EC closure the VP assigned on 2026-10-18 (i18054,
approved in i18063). The rest of that closure is port-bound; see
[the port-harness spec](RES_00BA_16EC_PORT_HARNESS.md).

## Exact original

| Span | Size | SHA-256 |
| --- | --- | --- |
| `0A40:029C..02C8`: Delay `029C..02BC` and DelayLoop `02BD..02C8` | 45 bytes, 19 instructions | `d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb` |
| `0709:0DBE..0DD2` | 21 bytes, 10 instructions | `a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d` |

START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. The
spans come from the image expanded by the hash-locked loader map.

## Behaviour

**Delay** (`RETF 2`) is entered after a far call with the millisecond count
at `[SP+4]`.

1. `MOV BX,SP; MOV DX,SS:[BX+4]; OR DX,DX`. A zero count returns.
2. `MOV AX,40h; MOV ES,AX; XOR DI,DI; MOV AL,ES:[DI]` loads the BIOS byte
   at `0040:0000`.
3. For each millisecond, `MOV CX,DS:[84FCh]` loads the count and `CALL
   02BD` runs passes of `MOV BX,4; DEC BX; JNE` followed by `CMP
   AL,ES:[DI]; LOOPE 02BD`. That is up to CX passes (65,536 when CX is 0)
   while the byte is unchanged. Then `RET; DEC DX; JNE 02B0`.

**The wrapper** (`RETF`) runs `PUSH BP; MOV BP,SP; MOV AL,DS:[49E2h]; XOR
AH,AH; MOV DX,0E1h; MUL DX; PUSH AX`. It then calls `LCALL 0A40:029C`, so
the delay is AL × 225 ms, and ends with `POP BP; RETF`.

**After a return** from a nonzero delay, AL is the BIOS byte, and BX, CX,
DX and DI are 0. ES is `0040h`, and the flags are `DEC DX`'s with the last
`CMP`'s CF. After a zero delay, BX is the delay's entry SP, DX is 0, and
the flags are `OR DX,DX`'s.

**Aliasing.** Every operand is reread in source order. Each millisecond's
`CALL` writes its return word `02B7h` at `SS:SP-2`. If that slot aliases
the BIOS byte or the count, the loop changes as on the CPU:

- The test and the audit put the slot on `0040:0000`, so `LOOPE` ends after
  one pass with CF set.
- The audit also puts it on `DS:[84FCh]`, so the second millisecond
  rereads `02B7h` passes.

**MUL's flags.** MUL leaves DX = 0 and CF = OF = 0, since a byte times 225
fits. Its architecturally undefined SF, ZF and PF are set from AX, with AF
clear, as Unicorn does. They only show at a `CODE` stop, because Delay's
`OR DX,DX` overwrites them.

**Loop cap.** `max_loopbacks` (64-bit) caps the taken backward jumps
(`JNE 02C0`, `LOOPE 02BD`, `JNE 02B0`) and stops at the target with
`LIMIT`. A millisecond of CX passes takes 4 CX loop-backs.

## Composition and stops

| Wrapper status | Meaning |
| --- | --- |
| `OK` | returned through `RETF` |
| `ARGUMENT` | null pointers, RAM other than 1 MiB, or IP not `0DBEh`; nothing changed |
| `CODE` | the `+0DCC` bytes are not `9A 9C 02`; stops there with the count pushed |
| `CALLEE` | the delay stopped (its `LIMIT`); `*callee_status` holds its status |
| `TRANSFER` | the delay's `RETF 2` popped a CS:IP other than `0DD1` in the wrapper's CS (a guard that is never exercised) |

The delay itself returns `RETURNED`, `ARGUMENT` or `LIMIT`. The `LCALL`
follows the merged U7D `+0565` convention: the opcode and offset are
checked at the site, and the segment is read from RAM.

## Validation

**Focused C test.** `test_resident_0709_delay_wrapper_0dbe_cpu.c` runs
under CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. It passes **24** hand-predicted checks:

- Delay(2) with three passes per millisecond takes exactly 23 loop-backs;
  with cap 22 it stops at `LIMIT` on the last pass's `JNE 02C0`;
- Delay(0);
- the aliased BIOS byte;
- the wrapper's 899 loop-backs for factor 1;
- `CALLEE` carrying the delay's `LIMIT`;
- factor 0;
- `CODE` with MUL's flags;
- the guards.

**Unicorn differential.** `tools/overlay/res0709_delay_0dbe_cpu_audit.py`
loads both original spans at their own addresses. It compares the whole
1 MiB RAM and all 14 CPU words with **no mask**.

- **Cases: 153** (148 return, 1 `CODE`, 4 `LIMIT`).
- **Coverage.** 29/29 instruction starts and 8/8 conditional outcomes.
- **Inputs.**
  - Delays entered directly, and through the wrapper.
  - Counts of 0 (65,536 passes, 262,143 loop-backs in one case) and 1–12.
  - Factor `FFh` (229,499 loop-backs).
  - Both aliasing cases.
  - `LIMIT` on each of the three backward jumps.
  - SS across A20, DF set, several flag seeds, random SPs and a random
    background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **32
cases**, 24 of them with a nonzero delay, with no mask. Each compares all 14
CPU words, the stack window and the whole data segment.

- The `LCALL` reaches the Delay bytes at absolute `0A40:029C`.
- The harness records the live BIOS byte `0040:0000` (`F8h`) for the C
  replay, and keeps interrupts off around the call.
- The result SHA-256 is
  `1cf4770bd827b6ce34dea12caad8bf41ab8eb663d86d8d3afeec479751a6fa25`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_resident_0709_delay_wrapper_0dbe_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1500s \
  python3 tools/overlay/res0709_delay_0dbe_cpu_audit.py --unicorn --native --output resources/res0709-delay/report.json
```

`deja "Poolrad A40 029C CRT delay 0709 0DBE raw CPU full state"` found no prior
session. The decoding was checked against the merged callback models'
notes, and nothing else is reused.

No classifier, queue or count ledger is changed. Crediting `RES_0709_0DBE`
and recording `0A40:029C` as covered runtime are the verifying lane's
calls.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py RES_0709_0DBE --require-current-main
```

## Pinned MULW flags: model fixed (2026-10-23, census #1464 family 3)

Pinned DOSBox-X `MULW` (`instructions.h:605-619`) keeps SF and AF (`FillFlagsNoCFOF`), sets ZF from AX **alone**, PF to the even parity of **all four bytes of DX:AX** (`FLAG_PF` is the constant mask 4), and CF = OF = (DX != 0). The model took PF from AL only and SF from AX bit 15. Fix: the model follows pinned MULW.

- **Did the evidence depend on these flags?** Only at the audit's artificial code-check stop right after the MUL, where the old model agreed with Unicorn rather than with pinned. `res0709_delay_0dbe_cpu_audit.py --unicorn` now uses the shared declared split (MULW rule): it passes, with the split used at **1** stop. The DOSBox-X native run passes (32 cases, all 14 CPU words, no mask), but it compares at returns, after `OR DX,DX`, so it cannot see this bit. The C test asserted the AL-only PF at the call site (`0x7202` for AX=`01C2h`); it now asserts the pinned `0x7206` and passes with asserts on.
- **Caller-observable?** No. The `MUL` is followed immediately by `LCALL 0A40:029C`, whose `OR DX,DX` at `+02A2` overwrites ZF, PF, SF and AF before any instruction reads them (walked over the original bytes). At every real return these flags are already gone.
- **Evidence for the verdict (par-e decides):** the difference is not observable, and the evidence now holds the model to pinned, so we expect the credit to stand.
