# U3B0 +00CB: raw-CPU item split composing SYSTEM string store, FillChar and set membership

`pr_u3b0_item_split_00cb_cpu` is a raw-CPU, full-state model of the
complete GAME.OVR unit `0x3B0` routine `+00CB`. The VP assigned it to this
lane on 2026-10-17 (i17629), after Astra cleared it with no in-flight work.
In the census regenerated on main `11c22a0`, its three callees are the
SYSTEM string store `0AF8:0B25` (PR #926), FillChar `0AF8:16A8` and set
membership `0AF8:0DC0` (PR #936). All three are runtime-covered, and the
routine has no inline coverage, model, branch or main file. The classifier
lists it as `helper_candidate`, grade B. Identity and strict credit are the
verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; expanded image at load segment 0 places SYSTEM at `0AF8` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U3B0 `+00CB` | GAME file `0x2C99A..0x2CA88`; **239 bytes, 93 instructions, four conditional branches**; `RETF 6`; SHA-256 `2103d70c0e552cd675212dd86eef4abea2af24d7d4573bb6dc7618a2af403c5a` |
| Far calls | `+00DF` `LCALL 0AF8:0B25`, `+00F4` `LCALL 0AF8:16A8`, `+012B` `LCALL 0AF8:0DC0`; each segment word is fixup-backed |
| Set constant | `+00AB..+00CA`, the 32-byte set the scan tests against; it holds the digits and the capital letters |
| Trailing bytes | `+01BA..+01D9` are not executed; the next routine starts at `+01DA` |
| Caller | U3B0 `+03BD` at site `+040C`, through `PUSH CS; CALL` |
| SYSTEM leaves | string store 36 bytes, SHA-256 `00995572…`; FillChar 20 bytes, `f1c5fade…`; set membership 33 bytes, `753159dd…` |

## ABI and ordered effects

The routine is a nested Pascal procedure. It is entered after a far call
with `SS:[SP]` the return IP, `[SP+2]` the return CS, `[SP+4]` the static
link L (the parent's BP) and `[SP+6]/[SP+8]` a far string pointer S. It
returns with `RETF 6`. After `PUSH BP; MOV BP,SP; SUB SP,2Eh`:

1. The string store copies S into the local `string[40]` at `BP-29h`
   (the cap is 28h, so a longer source is truncated).
2. FillChar clears the 40 bytes at `SS:L-8Dh` (value 0).
3. It sets k := 1 at `[BP-2Ah]`, n := the copied length at `[BP-2Ch]`,
   and the FOR bound `[BP-2Eh]` := n.
4. For i := 1 to n (i at `[BP-2Bh]`; skipped when n = 0), it tests the
   local byte `[BP+i-29h]` against the set at `CS:00AB` through the set
   membership test. The parent's start/end byte pairs are at
   `SS:L-8Fh+2j` and `SS:L-8Eh+2j`. For a member:
   - if start[k] = 0, then start[k] := i;
   - otherwise end[k] := i − 2, k := k + 1 and start[k] := i.
5. It sets end[k] := n and `L[-8Eh]` := k (the count, in end[0]). Then
   `MOV SP,BP; POP BP; RETF 6`.

FillChar clears only items 1–20. Without aliasing, start[k] = 0 holds only
at the first member: every later member finds start[k] = i ≥ 1, so it
closes the current item and opens the next. Items past 20 are written
beyond the cleared bytes. Every
operand is reread from RAM in source order. A pair store that aliases i,
k, n or the bound is therefore seen by later reads. AX ends as k with AH
from `SHL AX,1` of 2k. DL ends as n, and DI as L.

**Termination without a host cap.** The scan always ends within n − 1 ≤
254 loop-backs, so the model takes no cap and has no `LIMIT` status.

- i's slot is written only by `INC`, or by a start store, which writes i
  itself. A start store onto the bound sets it to i and ends the loop at
  that `CMP`.
- Pairs grow upward from L − 8Dh. If they start inside or just below the
  locals, FillChar clears the length byte (n = 0) or its own return frame
  (`TRANSFER`).
- Otherwise the pairs climb through the set-membership call frames just
  below the locals, which are rewritten every iteration. Two cases follow:
  - When the bound is a start slot, reaching it ends the loop.
  - When the bound is an end slot, the start slot below it is the high
    byte of `PUSH AX`, which is always 0 (`XOR AH,AH`). k therefore sticks
    there or lower, and i runs to n.

The Unicorn sweep below exercises both cases.

**Stops.**

- `CODE`: a reached `LCALL` whose opcode or entry word is not
  `9A 25 0B`, `9A A8 16` or `9A C0 0D` stops before it, with IP at the
  site. The sites move with the entry IP, and the set pointer's `00ABh`
  immediate stays absolute.
- `TRANSFER`: a SYSTEM callee's `RETF` popped a CS:IP other than the
  call's next instruction. FillChar onto its own frame does this.

**Flags.** The final flags are `ADD DI,AX`'s at `+01A2`, and DF is 0
(both copies `CLD`). At a `CODE` stop the flags are those of the last
instruction before the site:

| Site | Flags at a `CODE` stop |
| --- | --- |
| `+00DF` | `SUB SP,2Eh` |
| `+00F4` | `ADD DI,0FF73h` (FillChar does not touch them) |
| `+012B` | `XOR AH,AH`: CF = OF = SF = AF = 0, ZF = PF = 1 |

Every other flag write, including the bound `CMP`, is overwritten before
any stop and is folded. Offsets wrap at 16 bits and physical addresses at
1 MiB. The `LES` pair, `POP` and this routine's `RETF` frame are read
physically contiguous, and each SYSTEM leaf keeps its own contract. Null
RAM or CPU, or RAM that is not exactly 1 MiB, returns `ARGUMENT` before any
change. `*callee_status` receives the last callee's status, or −1 when no
callee ran.

## Validation

**Focused C test.** `test_overlay_u3b0_item_split_00cb_cpu.c` runs under
CTest with ASan/UBSan and compiles with `-Wconversion -Wsign-conversion
-Werror`. It uses no game bytes: only the three `LCALL` encodings and
random synthetic sets are placed in the code segment. An independent
reference of the copy, the clear and the split predicts all of the
following:

- the parent's pairs and count;
- the local string, k, i, n and the bound;
- every register;
- the final flags.

It passes 20,084 checks:

- 2,000 random strings (lengths 0–55, truncated at 40) over random sets;
- shapes: empty, no member, adjacent members, and 40 members running past
  the cleared bytes;
- a source that wraps its segment;
- each `CODE` site, including `SUB`'s and `XOR`'s flags at their sites;
- FillChar clearing its own frame (`TRANSFER`);
- a relocated entry;
- both aliasing endings at L = BP + 27h. With CS 8800h, k sticks at 25.
  With CS 8811h, the bound becomes i at 29;
- the argument guards.

**Unicorn differential.** `tools/overlay/u3b0_item_split_00cb_audit.py`
places the expanded START.EXE image at load segment 0, so the three SYSTEM
leaves run live from the routine's unrelocated far calls. It loads unit
`0x3B0`, including the set constant. It compares the entire 1 MiB RAM and
all 14 CPU words with no mask. `CODE` is decided from the live site bytes.

| Stops | Cases |
| --- | --- |
| Returns | 289 |
| `CODE` | 4 |
| `TRANSFER` | 87 |

Coverage is 93/93 instruction starts and 8/8 conditional outcomes. The
leaves are covered too: string store 16/16 and 2/2, FillChar 7/7, set
membership 13/13. The cases cover:

- 150 random strings with stale parent bytes;
- the text shapes and a 255-byte source;
- the parent link on the scan's locals at five offsets, each with two
  caller CS values;
- a sweep of every parent link from BP to BP + 60h with both caller CS
  values. These are the 87 `TRANSFER`s where FillChar meets its own frame;
- stacks at both segment ends, and SS and the source across A20;
- a source that wraps its segment, a random background and a relocated
  entry;
- four rewritten sites.

No case runs past 39 loop-backs (n − 1 for n = 40), and the audit asserts
at most 254 in every case.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **96
cases** with no mask. The cases produce item counts from 1 to 40.

- The COM copies unit `0x3B0` to paragraph `8800`, relocates the three
  `LCALL` segment words to `9000`, and copies the three SYSTEM leaves there
  at their own offsets.
- Each case compares all 14 CPU words and a 256-byte window. The window
  holds the locals, every call frame and the parent's pairs.
- The result SHA-256 is
  `68ef1a3d160d61c166a667f67754d6fc4a8a2e84bd892208b783df42c44aaba2`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u3b0_item_split_00cb_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u3b0_item_split_00cb_audit.py --unicorn --native --output resources/u3b0-00cb-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U3B0 `+00CB..+01B9`, composing SYSTEM `0AF8:0B25`, `0AF8:16A8` and `0AF8:0DC0` |
| `source_candidates` | `src/recovered/overlay_u3b0_item_split_00cb_cpu.c` with `src/recovered/system_strstore_fullstate_0b25.c` and `src/recovered/system_runtime_leaves_cpu.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with the SYSTEM leaves live and full instruction and branch-outcome coverage of the routine and its callees, every reachable stop kind, a termination sweep, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `helper_candidate`; identity is the verifying lane's call. The game meaning of the parent's pairs is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.
