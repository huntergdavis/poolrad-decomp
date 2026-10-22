# U20 +11AC list walk

Author evidence for the complete `OVL_0020_11AC` body: raw CPU model, original-byte
differential over the full 1 MiB, and the +3291 / SYSTEM FreeMem compositions.
**This file claims no credit.** The author lane does not self-credit; par-e accepts,
and the ledger rows stay untouched until then.

## Extent, and why the row's size was wrong

| Field | Value |
| --- | --- |
| Span | `[11AC,12A8)` = **252 bytes, 77 instructions**, final `RETF` at `12A7` |
| Body SHA-256 | `87b20686c110afeba83ea8f33c74623e20b187cf2d9d74b69d29e6dbd295e851` |
| GAME.OVR file offset | `26410` (the queue's `ovr_offset` **is** the body's file offset) |
| Queue row said | `316` bytes, `104` instructions |
| The extra 64 bytes | `00 1b 20 74 61 6b 65 73 20 61 …` - **a text string**, not code |

Linear-decoding the queue's 316 bytes does not terminate; the last 27 "instructions"
wander through `" takes a …"` as `imul` / `popaw` / `outsb`, and one of them invents a
far call. The convention that settles this without guessing: decode exactly
`[ovr_offset, ovr_offset+bytes)` and require the last instruction to be a
**terminator**. (`2E7D` = `15299` and `0EC4` = `126634` both match their documents
under the same rule.)

The 64-byte tail's own hash: `8021972cf76a755834431c501f193e11523604afb0c59972173c9f964c451a00`.

**Dated note (2026-10-28), because a reviewer and a free model both tripped on it:** the
queue row carries three different numbers and only one of them is the body's file
offset. For this row `ovr_offset` = **26410** (the body's file offset), `offset` =
**4524** (= `0x11AC`, the in-unit address) and `unit` = **32** (= `0x20`, the unit
index). The unit's own base is a fourth number, **21886**, and `21886 + 0x11AC =
26410`. So "the queue's `ovr_offset` is the body's file offset" is true while
"`ovr_offset` is the unit base" is also true in a different sense - of the *unit*,
not of the row. Cite the field explicitly.


## Identity

- `lcall 0078:002A` at `1239` -> the loaded stub `EA 91 32 <seg>` -> **U6C +3291**,
  already strict-credited (`U6C_RECORD_UNLINK_3291_CPU.md`).
- `lcall 0AF8:0254` at `126C` -> **SYSTEM FreeMem**, strict-credited.
- The +3291 closure records its own fixup-backed callers as
  `INCOMING=[(0x1C,0x394),(0x20,0x6EA),(0x20,0x748),(0x20,0x1239),(0x33,0x1DCF),(0x6A,0x1935),(0x6A,0x19E5)]`.
  **`(0x20,0x1239)` is this call site**, so the caller relation comes from fixup
  provenance rather than from a queue label, and `u['memory_segment']==0x78` confirms
  the stub segment.

## Behaviour

Clears the word at `window(+590h)` where `window` is the far pointer at `DS:49D6`, then
walks the linked list whose head is `DS:5D96/5D98`, following each node's far link at
`+108h` to test it and continuing at `+104h`:

- node `+13h == 1` **or** node `+10Eh == 1`: if `+10Dh != 1`, increment `window(+590h)`;
  set `DS:5D92/5D94` to the node; save `node+104h` in `BP-8/BP-6`; `LCALL 0078:002A`
  with **B6 = 1** and **B8 = (successor+13h == 1)**; continue at the saved `node+104h`.
- otherwise, if either half of `node+108h` is nonzero: `FreeMem(offset node+108h,
  segment node+10Ah, size 16h)` and zero both halves; continue at `node+104h`.

The walk ends when the current far pointer is nil; `DS:5D92/5D94` are then set to the
head, and the routine returns with `RETF`.

## Measured differential

`tools/overlay/u20_list_walk_11ac_cpu_audit.py`, original bytes in Unicorn against the
raw C model, **every case comparing all 14 CPU words and the entire 1 MiB** (983,040
ordinary bytes; `A0000..AFFFF` excluded by the harness's `ram[:0xA0000] + ram[0xB0000:]`,
flag masks 0):

```
cases 25 | returns 22 | CALLEE 2 | LIMIT 1
```

**The two CALLEE stops are `freed-*` fixtures and they are FreeMem `RunError`s, because
a block outside the heap is an error.** The expectation of OK in the first version of
the harness was wrong, not the code. This is written down because a future reader will
otherwise treat a CALLEE stop as a model failure and go looking in the body.

**The LIMIT case is the per-case walk budget, and it exposed a harness bug: the budget
reached the Unicorn hook but never the model.** A budget that silently does not apply
is how a "pass" gets recorded for the wrong reason, so the case is kept as a guard on
the budget plumbing rather than as a semantic test.

## The defect this differential found

At `1232` the original is `JNE 1235`, so the `INC AX` at `1234` runs only when the
`122B` compare was **EQUAL**. The first model incremented when it was **not** - a
**one-bit inversion**. That flipped `B8` from 0 to 1, and `B8` is exactly the flag the
credited +3291 model uses to decide whether to decrement `window+67C`.

So the symptom was **two RAM bytes with a correct return value**, which is why it read
as a callee disagreement for three rounds:

```
original: window+67C 0x1234 -> 0x1233      model (before fix): 0x1234 -> 0x1234
```

Resolved by the credited model used as an oracle: run alone, it decrements on `B8=0`
and leaves the word on `B8=1`, and the original agrees. So the credited model was
never in question - the composition was.

## Bugs found, and by what

| Bug | Kind | Found by |
| --- | --- | --- |
| `1232` inverted branch | **model** | differential (2 bytes) |
| ctypes diagnostic struct mirroring a nested C ABI (segfault) | harness | differential crash |
| `EA`-stub check applied to the FreeMem site, which is a **direct** call | harness | differential |
| FreeMem segment word missing the loader relocation (installed at `0x9000`) | harness | differential |
| call-boundary trace read `entry_sp+4/+6` when `entry_sp` predates the `CS/IP` pushes | trace | differential |
| `freed-*` expected OK where FreeMem correctly raises `RunError` | expectation | differential |
| per-case budget passed to the hook but not the model | harness | LIMIT case |

**One model bug, four harness/trace bugs, one wrong expectation - and the differential
found all of them; my narration found none.** Three plausible theories (wrong-node
`+13h` read, `B8=1` argument, `b8=3120`) were each refuted by the next measurement.

**Standing rule, earned here:** *an instrument's reading must be verified on a
known-good case before it is used to explain a failure.* Four instrumentation bugs, each
producing a plausible wrong conclusion, is normal behaviour of instrumentation; the
defence is a positive control, not more care.

**Low-memory agreement, which the cases were not written to test:** when the successor
is nil, `ES:DI = 0000:0000`, so `ES:[DI+13h]` reads `0000:0013` - **the interrupt vector
table, byte `0x31`**. Both sides read the same byte, so the harness demonstrably
installs and compares low memory rather than only the regions the fixtures touch.

## Declared limits

- **Native replay is not in this revision.** The differential above is original-bytes
  vs C only; no DOSBox-X capture is claimed for this body yet.
- The 3291 and FreeMem closures' own contracts apply to their reached callees, including
  3291's heap well-formedness limit and FreeMem's `[HeapOrg,HeapPtr)` block rule.
- Walk budget: `max_walk_loopbacks` counts taken `1293 -> 11CA` edges and stops at `11CA`
  with earlier effects kept.
- `RETF 4` and `RETF 6` pop the **argument words as well as the 4-byte return frame**;
  the model checks the resulting SP. Getting this wrong earlier made a correct callee
  look like a `TRANSFER`.
- Three harness bugs were the same class as the tool-level defects of this project: the
  harness **restated** a rule instead of inheriting it from the world's owner (a stub
  check applied to a direct call, a relocation the loader owns, a budget that only
  reached the observer). The fix in each case was to inherit from the closure's own
  builder.
- Excluded from comparison: the `A0000..AFFFF` aperture (declared above), and nothing
  else. Code is immutable; IRQs, faults and self-modification are out of scope.

## Reproduce

```sh
export PYTHONPATH=resources/pythondeps:tools/overlay
export PYTHONDONTWRITEBYTECODE=1
python3 tools/overlay/u20_list_walk_11ac_cpu_audit.py --compile-only   # CHECKER_BUILD_OK
python3 tools/overlay/u20_list_walk_11ac_cpu_audit.py --output resources/u20_11ac.json
```
