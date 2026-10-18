# Unit 0x25 +0x0F53: store string, hook and strict composed entries

`pr_u25_store_string_0f53_composed` is a strict, raw-CPU, full-state
candidate for the complete U25 public entry 12 (far stub `2B:005C`). It
runs over exactly 1 MiB of A20-off RAM and a fourteen-word CPU frame. Both
SYSTEM string stores run as real far calls into the raw-CPU
[`pr_system_strstore_0b25`](SYSTEM_STRSTORE_0B25.md) from #926, which this
PR is stacked on. The near `+06FB` address classifier runs in place from
its translation, as in the strict sibling
[`+0E0C`](U25_LOAD_WORD_0E0C.md).

The VP assigned this upgrade to this lane on 2026-10-17, after Astra
confirmed they do not own it. In the census-1937 queue, the string store
was the routine's only unready callee; the classifier is strict. The
timeboxed `deja` recall matched only this lane's current session. The body
translation is reused from this file's existing hook model on main.
Identity and strict credit are the verifying lane's decision.

The provisional hook entry `pr_u25_store_string_0f53` remains, with the
two stores as fail-closed call-site callbacks. Lane A's path model
`pr_rec_ecl_store_string` (`ecl_helpers.h`) is unchanged.

## Original boundary

Ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x25` starts at file `0x74D3`. The routine is exactly file
`[0x8426,0x8626)`, unit `+0F53..+1152`, **512 bytes / 181 instructions**,
SHA-256 `fb04baf47e5abfffab480e8494d662d5c49762e67ca4269b12128cc9e9fd3815`.
Its only return is `+1150 RETF 6`. It follows `+0E0C`'s `RETF 2`, and
`+1153`'s prologue follows directly. All jumps are internal.

The callers are the two far calls through `2B:005C`, both in unit `0x1C`:
`+02D7` inside `+0292`, and `+0A00` inside the INPUT STRING handler `+0994`
([U1C_INPUT_STRING_FULLSTATE_0994.md](U1C_INPUT_STRING_FULLSTATE_0994.md)),
where it is a callback. A decode-aligned scan of every ledger function found
no other far call, and START.EXE has none.

The near callee `+06FB..+0755` is the 91-byte, 28-instruction classifier,
SHA-256 `e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347`
([ECL_LOAD_WORD_CLASS4.md](ECL_LOAD_WORD_CLASS4.md)), ending in `RETF 2`.

The string store is SYSTEM `0AF8:0B25`, 36 bytes, SHA-256
`009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118`.
Both calls, `+0F69` and `+1010 LCALL 0AF8:0B25`, have fixup-backed segment
words.

## Behavior

Arguments are the far source string `[BP+6]` and the address word
`[BP+0xA]`. The frame is `SUB SP,0x104`: a local `string[255]` at
`[BP-0x100]`, then the class `[BP-0x101]`, length `[BP-0x102]`, index
`[BP-0x103]` and last index `[BP-0x104]`.

1. The string store copies the source into the local with cap `0xFF`.
2. `PUSH word [BP+0Ah]; PUSH CS; CALL +06FB` returns the class in AL. The
   classes are 0 for `4900..4CFF`, 1 for `6B00..6EFF`, 2 for `9700..98FF`,
   3 for `9900..B6FF`, and 4 otherwise.
3. The class still held in AL selects the store:
   - **Classes 0, 1 and 2** write each character, zero-extended, as a word at
     window far pointers `[49D2]`, `[49D6]` and `[49DA]`, with displacements
     `+6E00`, `+2A00` and `-2E00`, at `2*(address+i)`. They then write a zero
     word at `2*(address+length)`.
   - **Class 3** writes bytes at `[49DE]+6700+address+i`, then a zero byte.
   - **Address 6B00**, inside class 1, instead copies the local to far
     `[5D94]:[5D92]` with the string store and cap 15.
   - **Class 4** stores nothing.

Each loop sets last = `length-1` with a 16-bit `DEC` truncated to a byte,
then tests `CMP 0,last; JA`. That `JA` can never be taken, because 0 is never
above a byte. So every loop runs i = 0..last.

A **zero length** gives last = `0xFF`: all 256 elements are written from
local bytes `[BP-0xFF..BP+0]`. These are stale bytes past the copied string,
and the last one is the low byte of the saved BP. The terminator then
overwrites element 0 at `address+0`. Both entries keep this exactly.

Every pointer, local, argument and table word is re-read where the original
reads it, so store or alias writes take effect in source order. The
classifier writes only `SS:[BP'-1]` and always resumes at `+0F75`.

**Flags.** The final flags are exact on every path:

- **Word classes:** `XOR AX,AX` before the terminator.
- **Class 3:** `ADD DI,AX` before the terminator byte.
- **Class 4:** `CMP AL,3`.
- **Address 6B00:** the second store's `XOR CH,CH`.
- **Stops:** the `SUB SP` flags at the first site and the `CMP word,6B00`
  flags at the second, as `CODE` leaves them.

The `DEC`, `INC`, `SHL` and `ADD AX` flags are dead and omitted. `XOR`'s
AF is architecturally undefined; both oracles clear it, and so do both
entries. DF is clear after the first store's `CLD`. DX ends with the last
element (DL only in class 3).

## Composed strict entry

`pr_u25_store_string_0f53_composed(ram, size, cpu, max_loopbacks,
callee_status)` runs the same body:

- **Far calls.** Each reached site is read from RAM: `9A 25 0B` plus the
  loader-relocated segment word. The entry pushes CS and the next IP,
  relative to the entry IP, then runs the string store model at that
  segment. Other site bytes return `CODE`, with IP at the site and the
  five argument pushes done. A store that returns anywhere but the next
  instruction returns `TRANSFER`. That happens only when the 6B00 name copy's
  record pointer aliases the store's own return frame.
- **Near call.** The classifier's return IP is entry IP + 22h.
- **Loop cap.** An element store into the loop's own index or bound can make
  the original loop forever. `max_loopbacks` caps the taken `JNE`
  loop-backs, and the next one stops at that loop's `INC BYTE [BP-103h]`
  with `LIMIT`. That `INC` is at entry IP + 4Fh, DDh, 14Ah or 1B6h. Without
  aliasing a loop takes at most 255 loop-backs.
- **Addressing.** `LES` pairs and both `RETF` frames are read physically
  contiguous in both entries, as Unicorn reads them. Offsets wrap at 16
  bits and physical addresses at 1 MiB.
- **Arguments.** Null RAM or CPU, or RAM that is not exactly 1 MiB, returns
  `ARGUMENT` before any change. `*callee_status` receives the last store's
  status, or -1 when none ran.

## Verification

**Focused C tests.** Both run under CTest with ASan/UBSan and compile with
`-Wconversion -Werror`.

- `test_overlay_u25_store_string_0f53.c` (hook entry, unchanged) still
  passes with its synthetic store.
- `test_overlay_u25_store_string_0f53_composed.c` passes 5,380 checks. Its
  only game bytes are the two five-byte far-call sites. A byte-level
  reference builds every element, terminator and record byte, and all RAM
  outside the stack frame must match it. The cases cover:
  - all four classes at both range edges with lengths 0, 1, 3, 20 and 255;
  - six class-4 addresses;
  - another entry offset;
  - caps that stop each loop;
  - `CODE` at both sites;
  - a name copy onto the store's return frame (`TRANSFER`);
  - the argument guards.

**Hook-entry differential.** `check_overlay_u25_store_string_0f53.py` has
now run, after a one-line fix so its library links the string store
source. It passes **143 cases** with 209/209 instruction starts and 38/38
reachable conditional outcomes.

**Composed Unicorn differential.**
`tools/overlay/u25_store_string_0f53_composed_audit.py` places the expanded
START.EXE image at load segment 0, so the original string store runs from
both unrelocated far calls. It loads unit `0x25` and compares the entire
1 MiB RAM and all 14 CPU words with no mask. The original's behaviour
decides each stop: its return, a rewritten site, a store return elsewhere
or the loop cap. The C status must match.

| Stops | Cases |
| --- | --- |
| Returns, wherever `RETF 6` goes | 264 |
| `CODE` | 3 |
| `TRANSFER` | 1 |
| `LIMIT` | 15 |

| Code | Instruction starts | Reachable conditional outcomes |
| --- | --- | --- |
| `+0F53` body | 181/181 | 22/22 |
| `+06FB` classifier | 28/28 | 16/16 |
| SYSTEM string store | 16/16 | 2/2 |

The four never-taken `JA` edges never occur. The cases cover:

- every class at five addresses with five lengths;
- eight class-4 addresses and every name-copy length class;
- name copies onto, above and below the local, the store's frame and
  `+0F53`'s own frame;
- 120 random worlds;
- element stores into the local, the bound, the index, the length, the
  address argument, the saved BP, the return frame and the pointer table;
- stacks at both segment ends, SS, S and a window across A20;
- a random background, small caps and three rewritten sites.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **100
cases** with no mask: classes 0 to 4 have 19, 25, 21, 21 and 14, and 11
are 6B00 name copies. Each case compares all 14 CPU words, a 288-byte stack
window and a 1 KiB element window. The COM copies unit `0x25` to paragraph
`8800` and the 36 string-store bytes to `9000:0B25`. It relocates both
fixup-backed segment words to `9000`, loader-style. The result SHA-256 is
`e8ab65c28a216c660f7c626c33fe247d11c236dc7eb451fae0002b551cf4347b` with
COM segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u25_store_string_0f53 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u25_store_string_0f53_composed_audit.py --unicorn --native --output resources/u25-0f53-parity/report.json
(cd tests/recovered && PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:../../tools/overlay python3 check_overlay_u25_store_string_0f53.py)
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U25 `+0F53..+1152` (public entry 12) with `+06FB` in place and SYSTEM `0AF8:0B25` composed |
| `source_candidates` | `src/recovered/overlay_u25_store_string_0f53.c` (composed entry) with `src/recovered/system_strstore_fullstate_0b25.c` |
| `signals` | Exact hashes and CFG, fixup-backed far calls, whole-RAM/CPU differential with the store live, full instruction and reachable-outcome coverage of the body, classifier and store, every stop kind, independent C tests, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | The classifier is `game_verified` for `+0F53`. Natural saved-game reach and caller-level native parity remain open. |

No classifier, queue or count ledger is changed. Identity and strict credit
remain the verifying lane's decision.
