# OVL_0025_0341 in a real native process: forced-call capture, whole-trace store replay, C composition (run 039)

`OVL_0025_0341` (unit 0x25 `[0341,0411)`, 208 bytes, 89 instructions, the block loader into a fixed 1E00h buffer; model and earlier
evidence in `docs/re/OVL_0025_0341_CPU.md`) had only synthetic differentials. This note records one native invocation of the body
in the real game process under DOSBox-X, a byte-exact account of everything the call changed, and a composition of the existing C
model from the native entry state. **Evidence only: no parity, no function identity or count and no credit is claimed.** Any DOS
`INT 21h` binding below is a **declared hypothesis** (the callback internals are not instruction-traced; the binding is supported only
by byte equality), the keyboard writer in section B is the same kind of hypothesis, and the C models are unverified beyond the measured
scope. Media, RAM images and traces stay in ignored `resources/`; this note holds hashes, addresses and counts only. No `src/` or
`include/` file changes.

Base `origin/main` `d68e6aff3f4abc01f6cb12f17065baafda3cc03a`; branch `t574-ovl-0025-0341`. Files: this note,
`tests/replay/audit_slot_a_ovl0341_native_t574.py`, `tests/replay/check_slot_a_ovl0341_native_t574.sh`.

## What was captured: a FORCED call, not natural reach

Recipe of the earlier native runs (reviewed controller under `poolrad-heavy.sh`, `TERM=xterm-256color`, screenshot-gated keys, saved slot A
loaded to the playable view). A breakpoint at the unit-0x25 entry-4 stub `084F:0034` (it holds a far jump to the resident body at run-time
`308A:0341`; file segment + 0x0824) saw **no stop** on the natural path to the playable view. The call was therefore forced from the
debugger: a far-call frame was pushed (return `1264:0302`, a debugger-chosen stop address whose code is unrelated; block id `14h`, the
value of `DS:[84DC]`), registers were set (`SR`), `CS:IP` was set to `308A:0341`, a return breakpoint was set at the stop, and one armed
`LOGL` traced the single pass (**165,373 lines**, the last being the return state). CPU14 and the ten-segment 640 KiB RAM image were
captured at entry and at return. The state is the real game's, the call is not: the return address and the id word are synthetic,
the natural caller (unit 0x1C) was not observed calling the body, and **why the natural path does not call it is not established here**.

| Run-039 file (ignored `resources/oracle-runs/t4492-slot10-ovl0341-039/`) | SHA-256 |
| --- | --- |
| `LOGL-CALL-308A-0341.TXT` (165,373 lines) | `8a97b6060b394336b2426c81d77135dd7953376617ed8a6179773a3ed402342b` |
| `RAM-308A-0341.BIN` (entry) | `12f3a96ca8ee49a46ed1546f0ac4ee87f900614e3d9198fbf0b19c6573381d71` |
| `RAM-1264-0302.BIN` (return) | `d35faaa931f7e5a3b686a499205fa44905b0d0e5e6add05124078a077e72e65c` |
| `CPU-308A-0341.TXT` | `e05b334fe232f03a9e9c51d9580c888ca7157841e356a434ee1b2bdb8c1d2b0b` |
| `CPU-1264-0302.TXT` | `e3c1264ea9db4bf4e34c09acf1f952c37b62017c54433831b49838cc0717148d` |
| `MANIFEST-CAPTURE.sha256` (258 entries, all verify) | `b3bf4d70917f0b1a9c185db63a8ce70e716aae2a7812992887a0983309019054` |

The audit locks the five run files by hash and `GAME.OVR` by hash (`872964a3…`). Both CPU files are manual `EV` transcriptions; all 14
fields (flags included) equal the first and last trace lines.

## A. Independent re-derivation of the body (not from the model)

A fresh capstone decode of the 208 bytes at `GAME.OVR` offset 30740 (SHA-256 `b8d29a86…`): the decode tiles the span, 89 instructions, 12
far calls, one conditional jump (`JB` at `03DC`), one `RETF` at `040E`; the 12 far-call sites equal the call-site table. The body loaded in the
native entry image equals `GAME.OVR` except the 12 far-call segment words, each exactly the file word + `0x0824`. The trace executes each of
the 89 instruction starts **exactly once**: a single pass, the `JB` not taken, **no retry iteration occurred natively** (the retry loop is not
natively exercised; it stays covered only by the earlier synthetic cases).

## B. Whole-trace store replay and the per-byte account

The audit replays every store of the trace over the native entry image (capstone decode, effective addresses from the logged registers, shadow
memory; string operations chunked from the `CX`/`DI` change) and compares the result with the native return image byte for byte.

| Quantity | Value |
| --- | ---: |
| byte stores journaled | 60,746 |
| trace read annotations checked against the shadow memory | 55,110 matches, **0 mismatches** |
| native delta bytes, entry to return | 15,902 |
| explained by a replayed writer | **15,902** |
| writer with the wrong value / **unattributed** / spurious | 0 / **0** / 0 |
| replayed image vs native return image | **0 differing bytes** |
| display stores (`>= A0000`) counted, not compared, no bytes invented | 6,464 |

Last writer of the 15,902 delta bytes, by code: the loader call 15,792, row clears 45, string loads 30, the `print` 14, the body itself 6, `FreeMem` 6,
`Str` 5, `FillChar` 2, `StrCat` 2. By class: game stores 8,094, DOS read data 7,713, DOS callback 74, INT frames 18, keyboard 2, BIOS tick 1.

Declared host writers (not CPU stores of the traced code): the DOS `INT 21h` callback (nine saved registers below the caller's `SP` and the PSP stack
pointer; `AH=3Dh/3Eh/42h/3Fh` bound to the real `ECL2.DAX` in the game directory: **3 opens, 3 closes, 8 seeks, 14 reads, 7,853 bytes**, the seek
positions checked against the trace), the BIOS tick (+1), one hardware interrupt frame, and the `INT` instruction's own 3-word frames (28
`INT 21h` instructions in game code). `INT 1Ch`, `INT 15h`, idle callbacks and the keyboard callback are store-free except the next point.

**One declared hypothesis was needed to reach 0 unattributed.** The first replay left exactly two bytes, the BIOS shift-state bytes `0417`/`0418`.
The trace shows the keyboard interrupt handler running once, with scancode `B8` (left Alt released: the debugger break-in key) in `AL`; the audit declares
that this host callback clears `0417` bit 3 and `0418` bit 1. The binding is supported by byte equality only; the mutant `kbd-skip` fails without it.

## C. The C model composed from the native entry state

From the native 640 KiB entry image (upper 384 KiB zero: unobserved) and the transcribed CPU, the existing `pr_u25_entry4_0341_cpu` runs with its
composed callees (`FillChar`, `Str`, `StrLoad`, `StrCat`, `Move`, `FreeMem`, the `0709:0626` printer, the unit-0x3B0 row clear, `0802:0716` load by id)
under the declared file service for the real `ECL2.DAX` (first handle taken from the native trace) and an EGA bus:

| Check against native | Result |
| --- | --- |
| status | `RETURN`, 12 callee calls |
| CPU14 at return | **equal, 14 of 14 fields** |
| RAM below A0000 vs native return image | 94 differing bytes, **all inside the 104 declared host-writer bytes** (the INT frames, DOS callback stores, tick, keyboard bytes); **0 differing bytes** once those declared host effects are applied |
| DOS call sequence | 28 events, equal (open/close/seek/read with handles and positions) |
| `OUT` sequence | 3,434 events, equal to the 3,434 `out dx,al` rows of the trace's game code |
| `SS:SP` at each of the 28 `INT 21h` | equal to the trace |
| display writes | 6,464, equal to the replayed display stores (count only) |

This is a same-state composition: it shows the model reproduces the native effect of this one pass under the declared bindings. It does not show the
model for any other state, and the display contents themselves were not observed.

## D. Mutants

Twenty-four mutants of the audit's branches (`replay`/`comp-*` perturb the store replay and the composition's declared environment) and of the
C model's source (`m-*`, built from a perturbed copy of `overlay_u25_entry4_0341_cpu.c`; a model mutant counts only if it built and the audit then
failed its own check, and a Python traceback never counts). **19 are killed, 5 survive.**

Killed: `regsave-order`, `read-skip`, `tick-skip`, `kbd-skip`, `int-frame-skip`, `psp-skip`, `read-pos`, `seek-origin` (the replay: each leaves bytes
unattributed, a trace read annotation mismatching, or the seek position wrong), `rep-short` (killed incidentally: the shortened string operation corrupts
a file name the replay then cannot find), `comp-handle-5`, `comp-entry-ds`, `comp-flags-if`, `comp-host-narrow`, `comp-out-drop` (the composition), and the
model mutants `m-fill-len` (1E01h fill length), `m-skip-freemem`, `m-add2` (source `+3`), `m-retf-pop`, `m-jb-invert`.

Survivors, triaged (none is a kill, none is hidden):

| Survivor | Class | Why it survives |
| --- | --- | --- |
| `no-hw-frame` (drop the hardware interrupt frame stores) | **equivalent for this trace** | the frame is 6 bytes at `20D0C..20D11`; every one is overwritten afterwards by a game `rep stos` in the loader call (22 and 1,366 later non-frame stores), so the return image and every trace read annotation are unchanged |
| `comp-flags-cf` (flip entry `CF`) | **equivalent for every entry state** | the first flag writer, `SUB SP,112h` at `0344`, rewrites all six arithmetic flags (`CF PF AF ZF SF OF` = mask `08D5`) before anything reads one; `PUSH BP` and `MOV BP,SP` read none |
| `m-557c` (skip `MOV byte [557Ch],0`) | **state-indistinguishable, not equivalent** | `DS:[557C]` is already 0 in the native entry image, so a store of 0 changes nothing here; a state with a nonzero byte would distinguish it |
| `m-digit-ds` (read the digit from `DS:[5377]`) | **state-indistinguishable, not equivalent** | `DS:[5376]` and `DS:[5377]` are both `02` in the native entry image |
| `m-dec-once` (copy length `size-1` instead of `size-2`) | **state-indistinguishable, not equivalent** | the extra byte copied is `block[size]` into `buffer[size-2]`; both are `00` here (`size = 1DFFh`, so the copy is 7,677 bytes of a 1E00h buffer), so the over-copy is invisible |

The three state-indistinguishable survivors are limits of one native state, not proof that the model's corresponding statements are right; the earlier
synthetic `--mutate` apparatus (15 of 15 caught) is where those statements are probed, and nothing here replaces it.

## E. Existing apparatus re-run on this base (reproduction of earlier claims, rc 0)

`ovl_0025_0341_cpu_audit.py`: `--compile-only`, `--unicorn` (75 cases, 970 far calls composed, 59 buffer cases checked, 89/89 starts), `--demo` (the
documented aperture divergence demo), `--native` (24 cases, 7,361,626 bytes compared), `--mutate` (15 of 15 caught) all exit 0. The C unit test
`tests/recovered/test_overlay_u25_entry4_0341_cpu.c` built with the audit's source list at `-O1 -Werror` passes (15,449 semantic checks, no `NDEBUG`).

## Limits (not claims)

Forced call with a synthetic return address and id word, not natural reach, and the natural cause is not established; one pass, no retry iteration
and no failure path natively; one block id, one saved slot, one `ECL2.DAX` file; manual `EV` CPU transcriptions; breakpoint-display stop receipts only;
the debugger halts the guest at each stop and `LOGL` timing is not excluded (the hardware interrupt landing, the tick and the keyboard release include
those pauses); the upper 384 KiB and the display contents were never observed (display stores are counted); the DOS callback, the tick, the
interrupt frames and the keyboard writer are declared host writers supported by byte equality; host-time noise; the audit needs private
ignored `resources/` and cannot run on a clean public checkout (not CI-wired); no CTest registration changes. A same-trace or same-state check
establishes **no parity, no function identity or count, and no credit**.

## Reproduce

`tests/replay/check_slot_a_ovl0341_native_t574.sh` (needs the ignored run directory and `GAME.OVR`; run under `poolrad-heavy.sh`).
