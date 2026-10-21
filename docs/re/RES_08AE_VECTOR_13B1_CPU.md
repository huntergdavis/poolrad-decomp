# RES_08AE +13B1: translated, evidenced driver-internal timer-hook remover (never a credit)

This raw CPU body undoes [RES_08AE_137C](RES_08AE_VECTOR_137C_CPU.md): it restores the INT 08h
vector from the saved copy, resets the PIT channel-0 count, clears the speaker gate and
silences four PSG channels. It is a candidate **`driver_internal`** member
([DRIVER_INTERNAL_CLASS.md](DRIVER_INTERNAL_CLASS.md)): translated and evidenced to the
credited-body bar, **never a credit**. This author change proposes **no strict row, no identity
row, no manifest row and no member-table row**; admission is the reviewer's act.

## Media and executable extent

START.EXE SHA-256: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Loader-expanded image: 68,592 bytes, SHA-256
a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e.

The decoded span is **08AE:[13B1,13EA), 57 bytes / 27 instructions**, physical
[09E91,09ECA), ending in the only return, `RETF` at 13E9. Body SHA-256:
`0fa5df2cbe185bdb43d4ff225e14b5b23a6893e85721aac20a99f86b34a0b019`. The span is **straight-line**:
no conditional branch, no call. The checker asserts the hash, the 27 starts, the single return, the
absence of any branch or call, the `INT 21h` site (13C6), the seven `OUT` sites (13CC, 13D0, 13D6,
13DA, 13DE, 13E2, 13E6) and the one `IN` (13D2), that **exactly one loader relocation falls inside
the span, at CS:13B3**, and that the two loads from the slot are 13B8 and 13BD.

## What the body does

1. Save DS. DS := the relocated segment word at CS:13B3; save that DS too.
2. **Load DX from CS:1378 and AX from CS:137A; DS := AX.** These four bytes are outside the span
   (see below): they are the old timer vector that 137C saved.
3. `INT 21h` with AX=2508h (set vector 08h to DS:DX): the vector goes back to the saved pointer.
4. Restore DS. `OUT 40h` twice with FFh (the body's literal `MOV AX,FFFFh`, split into bytes).
5. `IN AL,61h`; `AND AL,FCh`; `OUT 61h,AL`: clear bits 0 and 1 of port 61h (the speaker gate and
   timer-2 gate bits). The `AND` also sets the final flags from the masked byte (SF, ZF, PF; CF=OF=0;
   AF cleared), which is the only observable use of the byte read.
6. `OUT C0h` with 9Fh, BFh, DFh, FFh, in that order. Restore DS; `RETF`. AX is FFFFh on return.

## The slot is an INPUT from outside the span

CS:1378..137B is **read** here and **written by 137C**, four bytes before 137C's entry and 56 bytes
before this one. They are in no decoded body span (a data word pair in the 08AE segment; the same
finding as the 137C note). The differential and the native run treat them as input state and
compare the whole 1 MiB, so a model that read the wrong word, the wrong order or stale bytes fails.
A linear decode of the 08AE segment finds exactly these decoded references to the slot: 137C's two
stores (1388, 138D), this body's two loads (13B8, 13BD) and `LJMP CS:[1378]` at 08AE:1468, inside
the handler region after 13EA (the chain to the old timer vector). A linear sweep proves nothing
about references it cannot decode.

## Raw contract and the declared environment

`pr_r08ae_vector_13b1_cpu(ram, size, cpu, env)`: exactly 1 MiB of A20-off ordinary RAM, IP=13B1,
SP in [000A,FFFC], distinct CPU/RAM storage, immutable code, no MMIO, no interrupts or faults, no
arbitrary-IP resume. Word accesses keep A20 wrapping. The relocated segment word at CS:13B3 is read
live. The body writes only the stack; the environment writes only its vector table.

**Declared environment** (an input, scripted identically on both sides, and not DOS, the PIT, the
PPI or the PSG), named exactly:

- `INT 21h`: hardware frame (FLAGS, CS, next IP) pushed with IF and TF cleared; then the documented
  DOS "set interrupt vector" service **AH=25h only** (stores DX offset and DS segment into
  table[AL]; AL := a scripted value); return as DOS does, `RETF 2`, with a scripted flags word.
  **AH=35h is not named for this body and is refused.**
- `OUT`: ports 40h, 61h, C0h only; each write is logged.
- `IN`: port 61h only; the byte read is a scripted value, **swept over all 256 values**, never a
  guessed default.
- **Fail closed:** any other AH, port or `IN` is a named refusal (ENV_DOS, ENV_PORT), never a
  default. The C test asserts each refusal and its exact stop state (ENV_DOS after the frame push
  with IP at the INT; ENV_PORT before the access with nothing executed; a missing `IN` hook refuses
  at 13D2). The Unicorn side asserts it never saw an unlisted access.
- The values are the game's own: the vector restored is whatever the slot holds (read from RAM, never
  scripted), and the count and PSG bytes are the body's literals.

Mechanical choices, declared: Unicorn does not dispatch a real-mode `INT` through the IVT, so the
Unicorn hook and the C model both perform the hardware frame push and the `RETF 2` return
themselves, identically. The native run uses a **real `INT 21h`** into a stub handler.

### Observed constraints (not modelling choices)

- **The stack must not overlap the environment's vector-table entry.** The `INT 21h` frame sits just
  below the live stack; when the service stores the vector over a frame that overlaps the table
  entry, the return address is overwritten. This was *observed*, not argued: a stack sweep ending
  at SS:SP=0:002A in the 137C checker made the original bytes, run under Unicorn, return to a garbage address
  (offset `5240`, outside the span); the C model pops the same RAM words, so it would follow. The low-stack sweep here
  therefore stops just above the entry (lowest stack word 0x24). Do not widen it.
- **The stack may overlap the saved-vector slot.** The pushes then overwrite the slot before the
  loads at 13B8/13BD, and the body reads what they left. Six such cases (SS=CS, SP 137A..1384) are in
  the differential and agree.

Named stops: RETURN, ENV_DOS, ENV_PORT, ARGUMENT (null/size/IP/SP out of contract; CPU and RAM
untouched). No conditional branches, so no successor accounting beyond straight-line starts.

## Evidence

**Original-byte Unicorn differential: 480 cases, 0 mismatches**, comparing all **14 CPU words and
all 1,048,576 RAM bytes with no mask of any kind**, plus the environment event log (4,320 events:
one `INT 21h`, seven `OUT` and one `IN` per case, in order) and, per case, that table[8] holds the
(DX, DS) the `INT` carried and that, when the stack does not overlap the slot, it equals the saved
vector. Starts **27/27 (100%)**, conditional edges 0, stops **480 RETURN**. Cases: **all 256 values of
the `IN 61h` byte**, 200 random (registers, nine flag words, scripted handler flags and AL,
relocated segment equal and unequal, random vector tables), SP at both bounds, nine low-stack
positions above the table entry, six slot-overlap stacks, A20-wrapped stacks, CS=FFFF and a
relocated table base.

**Composed round trip, original bytes of both bodies: 64 cases.** The checker runs the ORIGINAL
137C then the ORIGINAL 13B1 (not the C models) under the union environment (the named set plus
AH=35h and port 43h): the vector table entry 8 is back to its initial value, CS:1378..137B holds
that vector, DS and ES and SP are restored, and the `INT 21h` sequence is exactly 35h, 25h, 25h. This
tests the pair, not two models of it.

**Sensitivity.** Six deliberate model mutations were run and **all six were caught**: AF left set by
the `AND`, the slot words swapped, the `AND` mask changed to FEh, a `POP DS` redirected to ES at each
of 13C8 and 13E8, and one PSG byte changed. Unlike 137C there is no equivalent mutant here: the
`AND` sets flags that are the body's final flags, so AF *is* observable.

C test: **43 semantic checks**, clean under `-fsanitize=address,undefined` (manual build with
`-Wall -Wextra -Wpedantic -Werror`) and 1/1 in the Debug CTest build.

### Native corroboration (partial, with a declared stub, shadow table and one masked flag group)

DOSBox-X normal core, 386, **32 synthetic direct calls**, original bytes relocated to segment 8800.
`INT 21h` is a real `INT` into a stub handler implementing the same AH=25h service over a
**shadow vector table** in the COM's own memory (returning with `RETF 2` and scripted flags, no stack
residue of its own). The real IVT[08] is not written (it would redirect the emulated timer while
interrupts are off). All 14 CPU words, a 256-byte stack window, CS:1378..137B and the shadow vector
entry match the C model, **except that SF, ZF and PF are masked in the flags word**: `IN AL,61h`
reads live emulated hardware, which cannot be scripted, so the `AND` flags differ. An unmasked
native run was attempted and fails exactly there (native case 0: C=0046, DOSBox-X=0006).

**The shadow table means the native run corroborates FRAME MECHANICS only (the hardware INT frame,
`RETF 2`, stack residue, and the body's register and memory effects), not the vector restoration
itself**, which is the stub's own implementation of the contract. Port writes are **not observed**
natively, so the port sequence is corroborated by the Unicorn log only. **1,048,312 of 1,048,576
RAM bytes are not compared natively**; the full comparison is the Unicorn differential. The native
run used the host `/usr/bin/dosbox-x` (SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`), **not** the pinned
`5a9c7c53` build. RESULT.BIN SHA-256:
`013c9de20fdb41c171706bb1a8b21770649748ba6c910d25a2946492b9f3d224`. Frames are synthetic; this
establishes neither natural reachability nor real hardware behaviour.

## What this does NOT establish

- **A scripted vector swap proves the routine's register and memory effects around the calls. It
  proves nothing about the real handler at 08AE:13EA** being removed or running, nor about real DOS,
  the PIT, the PPI port 61h or the PSG at C0h. The green result must not be read as covering them.
- No wall-clock or interrupt behaviour; no arbitrary-IP resume.
- Ownership: the body is in the 08AE driver set; this note makes no claim about authorship, which is
  the class's premise. 13B1 and 137C compose into an install/remove pair; neither is credited.
- The prior-session memory tool (`deja`) was unavailable, so nothing was reused from earlier sessions;
  the 137C and 006C checkers and native harnesses were reused as templates and every figure above
  was measured on this change.

## Reproduction

    python3 tools/overlay/res08ae_vector_13b1_cpu_audit.py --unicorn --native --output resources/13b1-evidence.json
    python3 tools/overlay/res08ae_vector_13b1_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --all --require-current-main
    cmake -S . -B resources/build13b1 -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/build13b1 --target poolrad_resident_08ae_vector_13b1_cpu_tests
    ctest --test-dir resources/build13b1 -R '^poolrad_resident_08ae_vector_13b1_cpu_tests$' --output-on-failure

Use the audit Python environment (Capstone, Unicorn). Original bytes, generated assembly/COM and
capture receipts stay under ignored `resources/`.
