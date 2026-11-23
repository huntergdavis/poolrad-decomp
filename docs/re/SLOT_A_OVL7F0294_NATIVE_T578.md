# OVL_007F_0294 in a real native process: forced-call capture, whole-trace store replay, C composition (run 040)

`OVL_007F_0294` (unit 0x7F `[0294,0441)`, the game's numeric entry line editor; model and earlier evidence in
`docs/re/U7F_NUMBER_0294_CPU.md`) had synthetic and composed-closure evidence only. This note records one native invocation of
the body in the real game process under DOSBox-X, a byte-exact account of everything the call changed, and a composition of
the existing C model from the native entry state, with a mutant triage. **Evidence only: no parity, no function identity or
count and no credit is claimed.** The keyboard, timer and interrupt effects below are declared host effects, each supported
only by byte equality. No DOS call occurs in this call, so no DOS binding is declared. Media, RAM images and traces stay in
ignored `resources/`; this note holds hashes, addresses and counts only. No `src/` or `include/` file changes.

Base `origin/main` `d68e6aff3f4abc01f6cb12f17065baafda3cc03a`; branch `t578-ovl-007f-0294`. Files: this note,
`tests/replay/audit_slot_a_ovl7f0294_native_t578.py`, `tests/replay/check_slot_a_ovl7f0294_native_t578.sh`.

## What was captured: a FORCED call, not natural reach

Reviewed controller under `poolrad-heavy.sh`, `TERM=xterm-256color`, screenshot-gated keys, saved slot A loaded to the playable
view. The editor was then entered from its cold stub (a forced call to the stub, which stopped on the Esc key), and the body was
entered by a direct far call from the debugger: `CS:IP = 2F6B:0294`, `SP = 3DF0`, a frame whose return address is `9FE0:0100`,
colour `0Ah`, a prompt pointer `9FE0:0000` (the prompt text was written by the debugger and is not reproduced here) and `max
64h`. One armed `LOGL` traced the call from the entry to the stop at the `RETF 8`. Three breakpoint stops at `2F6B:02FB` (the
loop head before each key read) let the debugger inject the keys `4`, `2` and Enter into the BIOS keyboard buffer. The natural
caller of the body was not observed, and why the natural path does not reach it is not established.

Run `t578-ovl7f0294-040` (ignored `resources/oracle-runs/`). The audit pins five inputs:

| Artifact | SHA-256 |
| --- | --- |
| `LOGCPU.TXT` (134,363 lines, the single armed log) | `36259e90d7ef853fc6526e87791b5c8a5dddd76834916849228732f4533184b0` |
| `RAM-2F6B-0294.BIN` (entry, 640 KiB) | `4c6280dd2fb66da131cc1a6fd790b0b23ba5ae6d5fc9d114da57543aca904762` |
| `RAM-2F6B-043E.BIN` (return stop, 640 KiB) | `19778b2cee78f6c1ded2ce5afad2c55b5bbe2caf2cff53f1c9e4b44df5c9567d` |
| `CPU-2F6B-0294.raw.txt` (EV output at entry) | `d9d8d865e60144d1a203a701756169de0eb1a1b4efab74fd2f3ea8e065f0ae25` |
| `CPU-2F6B-043E.raw.txt` (EV output at the return stop) | `3b822a44b76c42959a36f86a27da165e7464d07024b0a978b95fc6586357a892` |

The CPU files are machine output of the debugger's register command, not hand transcriptions. The thirteen register words and
the flags of both files equal the trace's first and last rows.

The action log (`actions.log`) records the chain: the entry capture, the armed log, the three injections, the three breakpoint
stops, the return breakpoint and the return capture.

## A. Re-derivation of the body (not from the model)

The 429 bytes at `GAME.OVR` unit offset `0294` match the pinned SHA-256 and decode to 188 instructions, tiling the span. The unit
has 17 loader fixup words (far-call segment words). The loaded image at `2F6B:0294` equals the file bytes except those words.
Of the 188 instruction starts, **137 execute in this call and 51 do not**. The unexecuted starts fall in four paths this input
does not take: the clamp (`0371` to `0392`), the non-digit letters path (`0311`), the Backspace path (`03AF` to `03F0`) and the
Esc result (`0419` to `041E`).

## B. Whole-trace store replay and the per-byte account

The audit replays every executed row over the native entry image: capstone decode, effective addresses from the logged
registers, shadow memory, string operations chunked by the `CX`/`DI` change. It compares the result with the native return image.

| Quantity | Value |
| --- | ---: |
| byte stores journaled | 21,560 |
| trace read annotations matching the shadow memory | 45,740, with 0 mismatches |
| keyboard reads (INT 16h AH=00) matching the buffer word | 3, with 0 mismatches |
| keyboard peeks (INT 16h AH=01) with ZF = (head == tail) | 3, with 0 mismatches |
| `IRET` frames whose popped IP, CS and FLAGS equal the next trace row | 8 of 8 |
| native delta bytes, entry to return | 71 |
| explained by a replayed writer | **71** |
| writer with the wrong value / **unattributed** / spurious | 0 / **0** / 0 |
| replayed image vs native return image | **0 differing bytes** |
| display stores (at or above A0000) counted, not compared | 6,848 |

The 71 delta bytes are the body's and its callees' stores (50), the debugger's key and buffer-tail writes (7), the keyboard
head and the tick (2, one byte each), the interrupt frames (6 bytes changed) and the IRQ0 handler's pushes (6).

Declared host effects, each supported only by byte equality, and each with its own check:

* **Debugger key injections** (6 writes): the three key words and the buffer tail, written at the three stops. The key values are
  the callback results the trace shows (`0534`, `0332`, `1C0D`; these are key scan and ASCII words, not game bytes). The tail
  starts from the entry image.
* **INT 16h AH=00** (callback at `F000:CFC1`): reads the buffer word at the head and advances the head by 2 (wrap `1E` to `3E`).
  The value read must equal the AX the trace shows after the callback (checked for all three reads). **AH=01** is store-free;
  its ZF must equal (head == tail) (checked for all three peeks).
* **IRQ0 landing** (callback `F000:FEA6`, one landing): the BIOS tick word increases by one. The hardware frame (IP, CS and
  FLAGS at the post-push stack pointer) is written at landing, and the handler's `IRET` must pop the same three words (checked).
  The rest of the handler is traced ROM code.
* **INT 1Ch** (callback `F000:CC00`): store-free.

No DOS `INT 21h` call occurs in this trace, so none is declared.

## C. The C model composed from the native entry state

`pr_u7f_number_0294_cpu` (the merged model, reached through the existing `u7f_number_0294_cpu_audit` closure) runs from the
native entry state: CPU from the entry EV output, the 640 KiB native image with zeros above A0000 (not observed), and the keys
delivered in the trace's three typing groups.

| Check against the native return | Result |
| --- | --- |
| status | `RETURN` |
| CPU14 | **equal, 14 of 14 fields**, after the post-`RETF` state is derived (see below) |
| keyboard calls (function and value) | 6, **equal** |
| `OUT` events | 3,638, **equal** to the trace's game-code `out` rows |
| display writes | 6,848, **equal** to the replayed display stores |
| RAM below A0000 | 27 differing bytes, **all in declared host classes** (BIOS keyboard and tick bytes, INT frames, the IRQ0 frame and the ROM pushes, which the C model does not execute); **0 differing bytes** once those effects are applied |

The return stop is taken **before** the `RETF 8`, so the native trace does not contain the post-`RETF` state. The expected post-`RETF`
state is derived from the frame words the return image holds at the stack top (checked: `0100` and `9FE0`) and the instruction's own
semantics: `IP` and `CS` popped, `SP` advanced by 4 + 8, giving `9FE0:0100` and `SP = 3DFC`. The C model returns to that state.

This is a same-state composition for one pass with one key sequence. It shows the model reproduces the native effect of this call
under the declared host effects; it says nothing about other states.

## D. Mutants and their triage

Baseline passes. Nineteen mutants were run, each as a fresh audit: nine of the C model's source (`m-*`, each built from a copy with
one anchored edit, the anchor asserted to occur exactly once), three in the composition (`comp-*`), and seven in the replay
(`kbd-skip`, `tick-skip`, `no-hw-frame`, `int-frame-skip`, `host-inject-skip`, `host-tail-skip`, `pair-dup`).

**Thirteen are killed**, each by a checked failure, with no traceback:

| Mutant | Killed by |
| --- | --- |
| `kbd-skip` | keyboard value mismatch (the head is not consumed) |
| `tick-skip` | unaccounted delta bytes (the tick) |
| `no-hw-frame` | an `IRET` pops a frame that disagrees with the next row (the IRQ0 frame is consumed by the handler's `IRET`) |
| `int-frame-skip` | trace read annotations disagree with the replayed memory (12 rows) |
| `host-inject-skip` | keyboard value mismatch (the buffer is empty) |
| `host-tail-skip` | the empty-buffer check (head == tail) fires although the value read equals the trace value; the message wording says "value", but the cause is the head == tail check |
| `comp-keys-merged` | display write count 6,656 against 6,848 |
| `comp-out-drop` | OUT sequence differs from the trace |
| `m-enter-0e` (Enter compared against 0Eh) | C status `CALLEE`, not `RETURN` |
| `m-result-plus1` (result plus one) | RAM differs outside the declared host effects |
| `m-space-027` (clear to column 27h) | display write count 6,784 against 6,848 |
| `m-colour-0e` (print colour 0Eh) | RAM differs outside the declared host effects |
| `m-frame-size` (frame 243h not 244h) | RAM differs outside the declared host effects |

**Six survive.** Each survivor is classified below from the artifacts; none is classified by assumption.

| Survivor | Class | Evidence |
| --- | --- | --- |
| `pair-dup` (execute both copies of each stop-duplicated row) | **equivalent** (idempotent duplicate) | All six stop-duplicate rows are the same instruction at `2F6B:02FB`, a `call 10BD:0059`, with identical registers and identical instruction bytes. A call's stores depend only on the registers and the return address, so the second copy writes the same bytes to the same addresses. The account and the annotation checks are unchanged. |
| `comp-entry-cf` (flip the entry carry flag) | **equivalent** (dead input) | The first three executed rows are `push bp`, `mov bp,sp`, `sub sp,0244`. The subtraction writes the carry flag from its own result before any instruction reads a flag, so the entry carry never reaches the model's outputs. The final flags still equal the native return. |
| `m-clamp-always` (the `over = 0` branch becomes `over = 1`) | **equivalent over the valid input domain** (unreachable branch) | The edited line is the branch taken only when `0 - hi` is nonzero and has `SF = OF`, where `hi` is the high word of the value. Over all 65,536 high words that branch is taken exactly for `hi` in `8000h` to `FFFFh`, and for no other value. The editor's text is at most six digits, so the value is at most 999999 and `hi` is at most `000Fh`. The branch cannot execute for any valid input, so the edit is inert there. |
| `m-esc-1c` (the in-loop Esc test becomes `1Ch`) | **gap** (not exercised by this call) | The compare executes, but no Esc key (`1Bh`) is delivered: the keys are `34h`, `32h` and `0Dh`, for which both versions behave identically. Only a call that delivers Esc distinguishes them. Not equivalent. |
| `m-bs-inc` (Backspace moves the column the other way) | **gap** (not exercised by this call) | No Backspace key (`08h`) is delivered; the path is unexecuted (`03AF` to `03F0`). Not equivalent. |
| `m-limit-plus1` (the key budget check is one higher) | **gap** (not exercised by this call) | The loop runs three times against a budget of 64; the budget check fires for neither value. Only a call near the budget boundary distinguishes them. Not equivalent. |

The three gaps are limits of this one call, not evidence about the model's statements there; the model's Esc, Backspace and budget
paths remain unobserved natively and are covered only by the earlier synthetic and composed-closure evidence.

## E. Existing apparatus, re-run on this base

* The C unit test `tests/recovered/test_overlay_u7f_number_0294_cpu.c`, built with the audit's source list at `-Werror` under
  AddressSanitizer and UndefinedBehaviorSanitizer: **13 semantic checks pass**.
* The closure's compile gate, `u7f_number_0294_cpu_audit.py --compile-only`: **passes**, with the sources built at `-Werror`.

The Unicorn and native closure runs of `U7F_NUMBER_0294_CPU.md` were **not** re-run in this slice.

## Limits (not claims)

* A forced call: a debugger-written frame, return address `9FE0:0100`, colour `0Ah`, prompt and max `64h` chosen by the debugger.
  The natural caller was not observed, and natural reach is not established.
* One pass, one key sequence (`4`, `2`, Enter). Backspace, Esc, the clamp, the letter keys and the key budget are unexercised
  natively (section D).
* 51 of 188 instruction starts do not execute in this call.
* The keyboard, timer and interrupt effects are emulator callbacks and declared host effects, supported only by byte equality. The
  debugger injects keys at stops, not through IRQ1. The trace halts the guest at each stop, and `LOGL` timing is not excluded.
* The upper 384 KiB is never observed; the composition uses zeros there. Display contents are never observed: display stores and
  writes are counted, not compared.
* The return state is pre-`RETF`; the post-`RETF` state is derived, not observed.
* Missing from run `t578-ovl7f0294-040`: `MANIFEST-CAPTURE.sha256`, `CAPTURE-NOTES.txt`, and the fourteen-field `CPU-*.TXT`
  transcriptions the earlier runs produced. The audit reads the raw EV output instead, and the manifest and notes were not
  regenerated.
* The audit needs the private ignored `resources/` and is not CI-wired. No CI has run for this head.
* No parity, no function identity or count, and no credit follows from this note. The C models are unverified beyond the
  measured scope.

## Reproduce

`tests/replay/check_slot_a_ovl7f0294_native_t578.sh` (needs the ignored run directory and `GAME.OVR`; a full pass takes about 40
minutes; run under `poolrad-heavy.sh`).
