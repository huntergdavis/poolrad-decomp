# U3B0 +08D7 and +0923: raw-CPU space scans

This PR adds raw-CPU, full-state models of two complete GAME.OVR unit
`0x3B0` routines. The VP assigned both to this lane on 2026-10-17, after
Astra confirmed they do not own them. In the census-1937 queue, each
routine's only callee is the SYSTEM string store at `0AF8:0B25`, and the
queue shows no inline coverage, model, branch or main file for either.
Both routines call the store as a real far call to the raw-CPU
[`pr_system_strstore_0b25`](SYSTEM_STRSTORE_0B25.md). That runtime model is
in #926, which this PR is stacked on. The timeboxed `deja` recall returned
nothing within 90 seconds, so nothing is reused.

- **`pr_u3b0_lead_spaces_08d7_cpu`** returns the index of the first
  non-space character of a Pascal string, scanning from 1.
- **`pr_u3b0_trail_spaces_0923_cpu`** returns the index of the last
  non-space character, scanning down from the length.

The classifier lists both as `helper_candidate`, grade B, provisional.
Identity and strict credit are the verifying lane's decision.

## Exact originals

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; expanded with `resident_loader_map.unpack` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U3B0 `+08D7` | GAME file `0x2D1A6..0x2D1F1` (unit `0x3B0` at `0x2C8CF`); **76 bytes, 31 instructions, two conditional branches**; `RETF 6`; SHA-256 `7435824950eaeaa9a6b6e294fccd55e01086b15aef717444f0a3f296b1bf8865` |
| U3B0 `+0923` | GAME file `0x2D1F2..0x2D237`; **70 bytes, 29 instructions, two conditional branches**; `RETF 6`; SHA-256 `f296aa6edff699b8b2c14b98188a8cf7442fe120f9fe4b3515597b8f96efde06` |
| Calls | `+08EB` and `+0937 LCALL 0AF8:0B25`; both segment words are fixup-backed |
| Callers | U3B0 `+0969` (sites `+0991`, `+09A6`) and `+0A08` (sites `+0A30`, `+0A45`), each calling `+08D7` and then `+0923` through `PUSH CS; CALL` |

## Complete ABI and ordered effects

Both routines are entered after a far call with `SS:[SP]` the return IP,
`[SP+2]` the return CS, `[SP+4]` an unread word and `[SP+6]/[SP+8]` a far
pointer S to a Pascal string. They return with `RETF 6`. Both callers
push S, then their own `[BP+6]` word, so the unread word is the static
link of a nested Turbo Pascal procedure.

**Shared prologue.**

1. `PUSH BP; MOV BP,SP; SUB SP,2Ch`.
2. `LES DI,[BP+8]` loads S. The routine pushes S, the address of the
   local `string[40]` at `SS:BP-2Ah`, and the length 28h.
3. `LCALL 0AF8:0B25` runs the string store. It copies min(S's length,
   40) characters and their length byte into the local.

**`+08D7`.** i := 1 in `[BP-2Bh]` and n := the copied length in
`[BP-2Ch]`. While i < n and the local's byte i is a space (20h), i rises
by one. A string of all spaces gives max(n, 1).

**`+0923`.** i := n in `[BP-2Bh]`. While i > 1 and byte i is a space, i
falls by one. A string of all spaces gives min(n, 1).

**Shared epilogue.** `[BP-1]` := i and AL := `[BP-1]`. AH is 0, from the
`MOV AX,28h` before the call and the loop's `XOR AH,AH`. Then
`MOV SP,BP; POP BP; RETF 6`.

**Registers.** BX, CX, DX, SI and ES are the string store's results:
BX is its entry SP, CX is 0, DX is the entry DS, SI points past the
source bytes it read, and ES is SS. DI is the last index the loop
tested. If the loop tested none, DI keeps the store's value, one past
the copy.

**Flags.** The final flags are the last `CMP` the loop ran: the bound
test or the space test. `INC`, `DEC` and `XOR AH,AH` writes are
overwritten before any read and are folded. DF is clear from the store's
`CLD`. `SUB SP,2Ch`'s flags are computed, because a `CODE` stop exposes
them. On every other path the store's `XOR CH,CH` overwrites them.

**Composition.** The far call is read from RAM: `9A 25 0B` plus a segment
word, the loader-relocated SYSTEM segment. The model pushes CS and the
next IP, relative to the entry IP, and runs the string store model. If
the site holds other bytes, the result is `CODE`, with IP at the site and
the five argument pushes done. A `TRANSFER` guard checks that the store
returns to the next instruction. It cannot trigger with this store, which
writes only the 41-byte local above its own return frame.

Every operand is reread from RAM in source order. Offsets wrap at 16 bits
and physical addresses at 1 MiB. `LES` pairs, `POP` and the `RETF` frames
are read physically contiguous. Null RAM or CPU, or RAM that is not
exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u3b0_space_scan_cpu.c` runs under CTest
with ASan/UBSan and compiles with `-Wconversion -Werror`. Its only game
bytes are the two five-byte far-call sites. A byte-level reference of
both scans builds the expected 1 MiB buffer and every CPU word for each
case. It passes 22,634 checks:

- every copied length 0 to 40, with every leading-space count for
  `+08D7` and every trailing-space count for `+0923`, and the other end
  randomized;
- empty, single-space and all-space strings, and caller lengths above
  40;
- another entry offset, a stack whose local wraps below offset 0, a
  source that wraps in its segment and a source equal to the local;
- `CODE` stops for a rewritten opcode and a rewritten offset byte, with
  exact `SUB` flags;
- the argument guards and a null status pointer.

**Unicorn differential.** `tools/overlay/u3b0_space_scan_audit.py` places
the expanded START.EXE image at load segment 0, so the original string
store runs from both routines' unrelocated far calls. It loads unit
`0x3B0` and compares the entire 1 MiB RAM and all 14 CPU words with no
mask:

| Body | Returning cases | `CODE` stops | Instruction starts | Conditional outcomes |
| --- | --- | --- | --- | --- |
| U3B0 `+08D7` | 330 | 3 | 31/31 | 4/4 |
| U3B0 `+0923` | 330 | 3 | 29/29 | 4/4 |
| Composed string store | | | 16/16 | 2/2 |

For each routine the cases cover:

- every copied length 0 to 40 against five space counts at the scanned
  end;
- caller lengths from 29h to FFh;
- 80 random worlds with space densities from 0 to 1 and random FLAGS,
  registers and stacks;
- 19 sources inside the routine's own frame, so the pushes and the copy
  reshape the string;
- eleven stacks near both segment ends, SS and S across A20, and a random
  background;
- three rewritten call sites.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **122
cases**, 61 per routine, with no mask. Eighteen place S inside the stack
frame. Each case compares all 14 CPU words, a 128-byte stack window and a
256-byte source window. The COM copies unit `0x3B0` to paragraph `8800`
and the 36 string-store bytes to `9000:0B25`. It relocates both
fixup-backed segment words to `9000`, loader-style. The result SHA-256 is
`078ffe372c21be46256e5fc838134dcfbc0f856e3538d1e3a146be98d3b2269f` with
COM segment `0814`. Original bytes and generated images stay under
ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u3b0_space_scan_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u3b0_space_scan_audit.py --unicorn --native --output resources/u3b0s-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U3B0 `+08D7..+0922` and `+0923..+0968`, each composing SYSTEM `0AF8:0B25` |
| `source_candidates` | `src/recovered/overlay_u3b0_space_scan_cpu.c` with `src/recovered/system_strstore_fullstate_0b25.c` |
| `signals` | Exact hashes and CFGs, fixup-backed far calls, whole-RAM/CPU differential with the store live and full instruction and branch-outcome coverage for both bodies and the store, `CODE` stops, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Both are `helper_candidate`; identity is the verifying lane's call. Their callers `+0969` and `+0A08` also need U3B0 `+0000`, `RES_0709_0626` and SYSTEM `runtime:2903`. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed. Identity and strict credit
remain the verifying lane's decision.
