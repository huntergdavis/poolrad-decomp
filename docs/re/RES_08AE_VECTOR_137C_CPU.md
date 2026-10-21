# RES_08AE +137C: translated, evidenced driver-internal timer-hook installer (never a credit)

This raw CPU body saves the current INT 08h vector, installs the driver's own handler at
`08AE:13EA` through DOS, and programs the PIT. It is a candidate **`driver_internal`**
member ([DRIVER_INTERNAL_CLASS.md](DRIVER_INTERNAL_CLASS.md)): translated and evidenced to the
credited-body bar, **never a credit**. This author change proposes **no strict row, no
identity row, no manifest row and no member-table row**; admission is the reviewer's act.

## Media and executable extent

START.EXE SHA-256: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Loader-expanded image: 68,592 bytes, SHA-256
a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e.

The decoded span is **08AE:[137C,13B1), 53 bytes / 24 instructions**, physical
[09E5C,09E91), ending in the only return, `RETF` at 13B0. Body SHA-256:
`2dbbe197d8c276b5cb7dc04dc19e0156b06d1772f880396de147fc244470696d`. The span is **straight-line**:
no conditional branch, no call. The checker asserts the hash, the 24 starts, the single return,
the absence of any branch or call, the two `INT 21h` sites (1386, 139E), the three `OUT` sites
(13A3, 13A8, 13AC), and that **exactly two loader relocations fall inside the span, at CS:137F and
CS:1394** (the two segment immediates, both `08AE` in the loaded image).

## What the body does

1. Save DS, ES. DS := the relocated segment word at CS:137F.
2. `INT 21h` with AX=3508h (get vector 08h): ES:BX := the old timer vector.
3. **Store BX at CS:1378 and ES at CS:137A.**
4. DS:DX := (relocated segment word at CS:1394):13EA; `INT 21h` with AX=2508h (set vector 08h):
   the handler is `08AE:13EA`.
5. `OUT 43h, B6h`; `OUT 40h, B1h`; `OUT 40h, 13h`. The values are the literal `MOV AX,13B1h`
   split into bytes. 13B1 is also the address of the next routine, so the number may be a
   coincidence or an address used as a divisor; **it is the body's own constant either way**,
   and it is taken as such, not interpreted.
6. Restore ES and DS; `RETF`.

## THE BODY WRITES BEFORE ITS OWN ENTRY (outputs outside the span)

Step 3 writes **four bytes at 08AE:1378..137B, which are before the entry at 137C** and outside
the 53-byte span. A model that compared only the span's bytes would miss this output and pass.
This evidence compares them explicitly:

- the differential compares the whole 1,048,576-byte RAM image, so CS:1378..137B is compared
  in every case, and the checker additionally asserts, per case, that those four bytes equal the
  (BX, ES) the environment returned (the old vector);
- the native run captures those four bytes in every record.

Whose bytes are they? In the loaded image they are **four zero bytes at the end of a data
region** (the bytes before 137C decode only as runs of `ADD [BX+SI],AL` and are word-table data, not instructions). They lie **inside no
decoded body span** and no credited or declared body. They are the saved old vector. A linear
decode of 08AE[0000,1330) and [137C,1800) finds exactly these decoded references to the slot:
the two stores in this body (1388, 138D); the two loads in **13B1** (13B8, 13BD), which restores
the vector; and **`LJMP CS:[1378]` at 08AE:1468**, which is inside the handler region after 13EA
and chains to the old timer vector. Those are decoded facts from a linear sweep, not a proof
that no other reference exists. The 1468 reader belongs to the handler this body installs; it is
not modelled here.

## Raw contract and the declared environment

`pr_r08ae_vector_137c_cpu(ram, size, cpu, env)`: exactly 1 MiB of A20-off ordinary RAM, IP=137C,
SP in [000C,FFFC], distinct CPU/RAM storage, immutable code, no MMIO, no interrupts or faults, no
arbitrary-IP resume, and **the stack (all pushes, including both INT frames) must not overlap
CS:1378..13B0 or the environment's vector-table entry**: an overlap would corrupt the live INT
return frame when the environment stores the vector, which is the stack's fault, not the body's.
Word accesses keep A20 wrapping. The two relocated segment words are read live.

**Declared environment** (an input, scripted identically on both sides and not DOS or the PIT):

- `INT 21h`: the hardware frame (FLAGS, CS, next IP) is pushed with IF and TF cleared; the
  environment then performs the **documented DOS vector-table service only**. AH=35h loads
  ES:BX from table[AL]; AH=25h stores DX (offset) and DS (segment) into table[AL] and sets AL to a
  scripted value; the handler returns as DOS does, `RETF 2`, with a **scripted flags word**. The
  table is the environment's own; in the differential it is the real IVT region (physical 0).
- `OUT`: only ports 43h and 40h are listed; each write is logged.
- **Fail closed:** any other AH, any other port, or any `IN` is a **named refusal** (ENV_DOS,
  ENV_PORT), never a default value. The C test asserts each refusal and its exact stop state
  (ENV_DOS after the frame push with IP at the INT; ENV_PORT before the OUT with nothing
  executed). The Unicorn side asserts it never saw an unlisted access.
- The values are the game's own: the vector installed is the pointer the body loads (relocated
  segment : 13EA), the divisor bytes are the body's literal, and the old vector is an arbitrary
  input because the body never interprets it. No game value is guessed; no case was skipped as
  UNKNOWN.

Mechanical choices, declared: Unicorn does not dispatch a real-mode `INT` through the IVT, so the
Unicorn hook and the C model both perform the hardware frame push and the `RETF 2` return
themselves, identically (three stale words are left below the live stack). The native run uses a
**real `INT 21h`** into a stub handler, so it checks that frame mechanics independently.

Named stops: RETURN (the original return), ENV_DOS, ENV_PORT, ARGUMENT (null/size/IP/SP out of
contract; CPU and RAM untouched). There are no conditional branches, so no successor accounting
beyond straight-line starts.

## Evidence

**Original-byte Unicorn differential: 318 cases, 0 mismatches**, comparing all **14 CPU words and
all 1,048,576 RAM bytes with no mask of any kind**, plus the environment event log (1,590 events:
two `INT 21h` and three `OUT` per case, in order). Starts **24/24 (100%)**, conditional edges 0,
stops **318 RETURN**. Cases: 300 random (registers, nine initial flag words, scripted handler
flags and AL, random vector tables, relocated segments equal and unequal), SP at both bounds,
nine low-stack positions ending immediately above the table slot, A20-wrapped stacks, CS=FFFF and a
relocated table base.

**Sensitivity.** Five deliberate model mutations were run. Four were caught (storing ES instead
of BX at CS:1378, reversing the FLAGS/CS push order, dropping the handler's flags, and
`MOV AL,AH` replaced by a no-op). One was **not** caught: popping into ES instead of DS at
13A0. It is an equivalent mutant: the same stack word is consumed, ES is overwritten by the
`POP ES` at 13AE and DS by the `POP DS` at 13AF, and neither register is used in between.

C test: **30 semantic checks**, clean under `-fsanitize=address,undefined` (manual build with
`-Wall -Wextra -Wpedantic -Werror`) and 1/1 in the Debug CTest build.

### Native corroboration (partial, with a declared stub and shadow table)

DOSBox-X normal core, 386, **32 synthetic direct calls**, original bytes relocated to segment
8800. INT 21h is a **real `INT`** into a stub handler in the COM that implements the same
documented service over a **shadow vector table** in the COM's own memory, returning with `RETF 2`
and scripted flags and leaving no stack residue of its own. The real IVT[08] is deliberately not
written: it would point the emulated timer at the driver's handler while interrupts are off, and
break the host timer when they come back on. All 14 CPU words, a 256-byte stack window, the four
bytes CS:1378..137B and the shadow vector entry match the C model.

**The shadow table means the native run corroborates FRAME MECHANICS only (the hardware INT
frame, `RETF 2`, flags and stack residue, and the body's register and memory effects); it does
NOT corroborate the vector installation itself**, which is the stub's own implementation of the
contract. Port writes are **not observed** natively (DOSBox-X offers no port trap here), so the
OUT sequence is corroborated by the Unicorn log only. **1,048,312 of 1,048,576 RAM bytes are not
compared natively;** the full comparison is the Unicorn differential. The native run used the host
`/usr/bin/dosbox-x` (SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`), **not** the pinned
`5a9c7c53` build. RESULT.BIN SHA-256:
`606afefcb2ea96720004c2951c47c01cc505bc72f0275f1246157705aafd5ad8`. Frames are synthetic;
this establishes neither natural reachability nor real DOS or PIT behaviour.

## What this does NOT establish

- **A scripted vector swap proves the routine's register and memory effects around the calls. It
  proves nothing about the real handler at 08AE:13EA** (including its `LJMP CS:[1378]` chain at
  1468), and nothing about real DOS, the PIT or the speaker. The green result must not be read as
  covering them.
- No wall-clock or interrupt behaviour; no arbitrary-IP resume.
- Ownership: the body is in the 08AE driver set; this note makes no claim about authorship, which
  is the class's premise.
- The prior-session memory tool (`deja`) was unavailable, so nothing was reused from earlier
  sessions; the 006C and 181A checkers and native harnesses were reused as templates and every
  figure above was measured on this change.

## Reproduction

    python3 tools/overlay/res08ae_vector_137c_cpu_audit.py --unicorn --native --output resources/137c-evidence.json
    python3 tools/overlay/res08ae_vector_137c_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --all --require-current-main
    cmake -S . -B resources/build137c -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/build137c --target poolrad_resident_08ae_vector_137c_cpu_tests
    ctest --test-dir resources/build137c -R '^poolrad_resident_08ae_vector_137c_cpu_tests$' --output-on-failure

Use the audit Python environment (Capstone, Unicorn). Original bytes, generated assembly/COM and
capture receipts stay under ignored `resources/`.
