# Unit 0x68 internal +0x068D (Up preflight): path-complete model with the entry 3 lookup merged

`src/recovered/overlay_u68_up_preflight.c` models DOS v1.3 `GAME.OVR` unit
`0x68` internal routine `+0x068D..+0x073A`, the routine entry 2 (`+0x0989`,
the exploration input loop's key handler of
[GAMEPLAY_PATH.md](GAMEPLAY_PATH.md)) reaches by `push cs; call +0x068D`
at `+0x0AB2` when its prompt returned an extended key with scan code
`0x48`. It reads the party position bytes and the facing byte, clears
window-1 word `0x5AA`, asks unit 0x3bd entry 3 for the field at that cell,
adds a facing-indexed step from two data-segment byte tables, and clamps
the candidate to the 0..15 grid, raising the window-1 word when it does.
The candidate is never stored when it stays in range. Classification:
**path-complete model** of the 174 bytes with the one far callee merged
(the translated `pr_u3bd_entry3`, [U3BD_FIELD_ENTRY3.md](U3BD_FIELD_ENTRY3.md))
and the two direction tables as host state. **No game-function credit is
claimed**; the PM decides the accounting.

`deja "unit 0x68 +0x068D Up preflight bounds direction table 0x2736"`
found no prior session. Reused with citation and re-checked here: the
U3BD_FIELD_ENTRY3.md caller row for this site; entry 2's key paths from
[U3BD_NIBBLE_ENTRY2.md](U3BD_NIBBLE_ENTRY2.md); the window table of
FUNCTION_LEDGER.md §1.7 (window 1 = script addresses `0x6B00..0x6EFF`,
word index = byte offset / 2); the party bytes `[0x6AAD]`/`[0x6AAE]` and
facing `[0x6AAF]` from GAMEPLAY_PATH.md; the ledger row `0x68:0x068d`
(grade B, bound 174, 55 instructions, one far overlay call, `retf`) and
the classifier row (`game_evidenced`, `win1`).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x68`: code at file offset 91,851 (`0x166CB`), run-time segment
`0x72`, public entries `+0x10A7`, `+0x0D6D`, `+0x0989`. The routine is
not a public entry; it has no fixup-confirmed far caller and one near
caller. Decoded with Capstone 5.0.7 (54 bytes of the span were also
checked against the PM's read-only scan).

| Item | Value |
| --- | --- |
| extent | `+0x068D..+0x073A`, **174 bytes, 55 instructions**, one `retf` at `+0x073A`; the next prologue is `+0x073B`, so the ledger bound is exact |
| span SHA-256 | `54bf1c34b310e1645d1b7c855a1fc17630282901b9fa64fb01b7ee7e33ca34aa` (file `0x16D58..0x16E05`) |
| frame | `sub sp, 6`; no arguments; `[bp-2]` = x word (`cbw` of `[0x6AAD]`), `[bp-4]` = y word (`cbw` of `[0x6AAE]`), `[bp-6]` = facing word (`xor ah,ah` of `[0x6AAF]`, 0..255) |
| branches | 5 forward: `je +0x0737`, `jle +0x06F5`, `jge +0x070B`, `jle +0x0721`, `jge +0x0737`; no loops |
| call sites | 1: `+0x06C0` `lcall 0x3D5:0x2F`, fixup-confirmed, unit 0x3bd entry 3 (`+0x0350`, merged) with three byte arguments pushed as words (x, y, facing) |
| DS reads | bytes `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]`; far pointer `[0x49D6]` (five `les` sites); table bytes `[di+0x2736]` and `[di+0x273F]` with `di` = facing word |
| DS writes | `[0x6AAD]` = 15 or 0, `[0x6AAE]` = 15 or 0, each only on its clamp |
| ES writes | window-1 word `es:[di+0x5AA]` (script word `0x6DD5`): 0 at `+0x06AF`; 1 at `+0x06EE`, `+0x0704`, `+0x071A`, `+0x0730` |
| result | none read: the caller sets `[bp-2] = 1` and jumps to its common exit `+0x0B9B` |

## Instruction paths (local effects in order)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x0693..+0x06A6` | `[bp-2] = cbw [0x6AAD]; [bp-4] = cbw [0x6AAE]; [bp-6] = zx [0x6AAF]` | `x_in`, `y_in` signed; `facing` 0..255 |
| `+0x06A9..+0x06AF` | `les di,[0x49D6]; es:[di+0x5AA] = 0` | flag word cleared first, unconditionally |
| `+0x06B4..+0x06C0` | push x byte, y byte, facing byte; `lcall 0x3D5:0x2F` | `pr_u3bd_entry3(map, buffer, residue, x, y, facing)`; the merged entry 3 wraps out-of-range bytes except on map 0/10, where it returns 0 |
| `+0x06C5..+0x06C7` | `or al,al; je +0x0737` | lookup 0 → return; position and flag untouched (flag is 0) |
| `+0x06C9..+0x06D1` | `di = [bp-6]; al = [di+0x2736]; cbw; [bp-2] += ax` | `x += dx[facing]` (signed byte) |
| `+0x06D4..+0x06DC` | `di = [bp-6]; al = [di+0x273F]; cbw; [bp-4] += ax` | `y += dy[facing]`, the dy table 9 bytes after dx |
| `+0x06DF..+0x06EE` | `cmp [bp-2],15; jle; [0x6AAD] = 15; flag = 1` | clamp x high |
| `+0x06F5..+0x0704` | `cmp [bp-2],0; jge; [0x6AAD] = 0; flag = 1` | clamp x low (on the candidate word, not the byte just written) |
| `+0x070B..+0x071A` | same on `[bp-4]` → `[0x6AAE] = 15` | clamp y high |
| `+0x0721..+0x0730` | same → `[0x6AAE] = 0` | clamp y low |
| `+0x0737..+0x073A` | epilogue, `retf` | |

**State effects, exactly.** On lookup 0: window-1 word `0x5AA` = 0, nothing
else. On lookup ≠ 0 with the candidate inside 0..15 on both axes: window-1
word `0x5AA` = 0, nothing else; the candidate is not stored anywhere. On
a clamp: the clamped axis's byte becomes 15 or 0 and the word becomes 1.
No other memory is written. The arithmetic is signed throughout: a stored
byte ≥ 0x80 is a negative coordinate (entry 3 wraps it to 15 or 0 for its
own lookup; the clamp here uses the un-wrapped candidate).

**The hidden stack byte.** Entry 3's unwritten `[bp-0xF]` (U3BD_FIELD_ENTRY3.md)
is, relative to this routine's BP, `BP-33`: below this frame's locals
(`BP-6`), the three pushed words (`BP-8..BP-12`), the far return
(`BP-16..BP-13`) and entry 3's saved BP (`BP-18..BP-17`). It is residue
from earlier stack use and is proven not to change entry 3's result; the
model passes it through as `field.entry2_frame_residue`.

**Direction tables.** `dx` is the DS byte at `0x2736 + facing`, `dy` the
byte at `0x273F + facing`, both sign-extended; with facing up to 255 the
reads reach `0x2835` and `0x283E`. The bytes at the computed data-segment
image position of `START.EXE` (paragraph `0xC7C`) do not resemble a
unit-step table, so the live table contents are not established here
and the model takes them as host state (`direction_table`, dx at index
`facing`, dy at `facing + 9`). The writers of `[0x6AAF]` keep it in
{0, 2, 4, 6} (U3BD_NIBBLE_ENTRY2.md); other facings are modelled, not
assumed unreachable.

## Caller (one near call) and the gate p5 asked about

Entry 2 `+0x0989`: clears window-1 word `0x592` (`+0x0995`); requires
`[0x49FA] == 1` (else `+0x0BAC`); prompts through `0x3C5:0x2A` with the
DS string `0x4B6`, receiving a key byte in `[bp-3]` and an extended flag
in `[bp-4]`. With the flag set (`+0x0AAA`): key `0x48` → `push cs; call
+0x068D` (`+0x0AB1..+0x0AB2`), `[bp-2] = 1`, `jmp +0x0B9B`; keys `0x50`,
`0x4B`, `0x4D` rotate the facing and call entry 3 themselves
(U3BD_NIBBLE_ENTRY2.md). The common exit `+0x0B9B` calls resident
`0xBA:0x290B` and loops to `+0x09A8` when `[bp-2] == 0`, else leaves to
`+0x0D1C`. So one Up key runs this routine and nothing else in entry 2
before the exit.

Script word `0x6DD5` is window-1 byte offset `0x5AA` (`(0x6DD5 − 0x6B00) × 2`).
On Up it is **cleared at `+0x06AF` before the lookup and set to 1 only on
a clamp**; an in-range step leaves it 0. Other writers of the same word
seen so far: unit 0x1c `+0x31C2` (clears it; U3BD_CELL_ENTRY4.md). Entry
2's own writes are to words `0x592` and `0x594` (`+0x0995`, `+0x0A72`,
`+0x0A8D`), not `0x5AA`.

## Independent check of the saved slot-A gate word (address-only)

Window 1 is saved whole at `SAVGAM<slot>.DAT` offset `0x0801` (ten block
transfers of unit 0x6f entry 8; see the loader at `+0x17D5..+0x18DA`), so
script word `0x6DD5` is file offset `0x0DAB`. In both owner-held saves
(A, `abca2483…c08f1`; J, `4b64213a…6ca9`) that word is **0**; the
neighbouring words `0x6DC9` (`0x592`) and `0x6DCA` (`0x594`) are 0 too.

Every `es:[di+0x5AA]` instruction in `GAME.OVR`, found by scanning for
the displacement with an ES prefix across all 34 units:

| Site | Routine | Effect |
| --- | --- | --- |
| unit 0x1c `+0x31D0` | `+0x31B4` (U3BD_CELL_ENTRY4.md) | `mov es:[di+0x5AA], ax` with AX = 0 (clears) |
| unit 0x68 `+0x06AF` | this routine | clears |
| unit 0x68 `+0x06EE`, `+0x0704`, `+0x071A`, `+0x0730` | this routine | sets 1 on a clamp |

No overlay instruction reads the word directly; the scripts reach it
through the ECL operand classes (window 1 is class 1 of the operand
loader, FUNCTION_LEDGER.md). Therefore, on the in-range one-Up path from
15,4 N to 14,4 N: the word is 0 in the file, this routine clears it again
at `+0x06AF`, no clamp fires because the observed destination 14,4 is
inside 0..15 (whatever the live step bytes are, the result shows the
candidate was in range), entry 2 writes only `0x592`/`0x594`, and no other
overlay code writes it. **The word is still 0 when the gameplay ECL
compare at `0x9934` runs**, so the equal branch to `0x996B` is taken and
the `CALL` at `0x9945` is bypassed, unless something on the resident side
or in the scripts themselves writes it between the key and the compare;
that tracing (ECL `0x996B`, the actual x/y and minute writers) is p5's
and is not claimed here.

**Minute bytes (VP question, address-only): not found.** The span's only
memory writes are DS bytes `[0x6AAD]`/`[0x6AAE]` (`+0x06E5`, `+0x06FB`,
`+0x0711`, `+0x0727`) and window-1 word `es:[di+0x5AA]` (the five sites
above); it never loads `[0x49D2]` (window 0) and touches no window-0 byte,
so `+0x18E`/`+0x190` are not written here. Its only call is `+0x06C0` to
unit 0x3bd entry 3, whose merged translation (U3BD_FIELD_ENTRY3.md) writes
nothing outside its frame, makes no far calls, and only reads `[0x84DC]`
and the `[0x6A5C]` buffer through its internal leaf and entry 2. No path
of this routine writes the minute bytes or invokes a likely minute writer.
The x/y writer at `+0x073B` is outside this span and not covered.

## C contract and tests

`pr_u68_up_preflight(state, report)` runs in place on `state` (position
bytes, facing, host window 1, host direction table, and the merged entry
3's inputs). Statuses: `OK`; `ARGUMENT` for a null state or a window 1
shorter than 0x5AC (nothing written), for entry 3's own `ARGUMENT` (null
buffer at a read; the flag word is already 0), or for a table shorter
than `facing + 10` when the table is reached (flag 0, lookup done,
position untouched). The report carries the signed inputs, the lookup,
the step, the candidate, the four clamp bits, the flag and entry 3's
report.

`tests/recovered/test_overlay_u68_up_preflight.c`: all 65,536 (x, y)
byte pairs for ten facings (0, 2, 4, 6, 1, 3, 5, 7, 8, 0xFF) and three
map ids (0, 10, 1) against an independent restatement that includes entry
3's rule, checking the position bytes, the flag word, that no other
window-1 byte changes, and that an in-range candidate is never stored;
pinned cases: (15,4) facing 6 with a synthetic (−1, 0) step stays (15,4)
with the flag cleared from a non-zero value; y = 0 stepping to −1 clamps
to 0 with the flag; x = 15 stepping to 16 clamps to 15; a negative
stored byte (−16) clamps to 0; lookup 0 on the map gate and on a zero
field; an unrecognised facing reading table bytes 7 and 16; optional
report; bounds: null state, short and null window 1, short and null
table, null buffer on a recognised selector.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2` (gcc 15.2.0)
builds the model, the two merged unit 0x3bd sources and the test with no
diagnostics and prints `unit 0x68 up preflight model tests passed` in
under a second; the same with `-fsanitize=address,undefined
-fno-sanitize-recover=all` passes; the CMake Release build has no
warnings; `ctest` passes 42 of 42 targets on main 1d0607d (after PRs
#136/#137, rebased with no conflicts); the hygiene
workflow's three checks pass locally.

## Limitations, stated plainly

- The live direction-table bytes are not established; the parity test
  uses synthetic steps. The oracle's observed 15,4 N → 14,4 N is consistent
  with a (−1, 0) step for facing 6 but this note does not assert the table.
- The routine does not move the party; where the in-range candidate is
  applied is outside this span (PR #136 recovers a position step in unit
  0x25 and is not touched here).
- What the window-1 word `0x5AA` means to the scripts is for the ECL lane;
  this note only records its writers.
- No game-function credit is claimed; the accounting is the PM's.

## Reproduce

Added 2026-10-22 by par-e (VP i21177). Until then this document described its checks but
gave no command to rerun them. The evidence kind is recorded in
[credit_evidence_kind.csv](credit_evidence_kind.csv) (labels: [CREDIT_EVIDENCE_KIND.md](CREDIT_EVIDENCE_KIND.md)).

Synthetic C tests. These are hand-derived checks of the C, with no DOS bytes, so they are not
an original-byte differential:

```sh
cmake -S . -B build
cmake --build build --target poolrad_overlay_u68_up_preflight_tests
ctest --test-dir build -R '^poolrad_overlay_u68_up_preflight_tests$' --output-on-failure
```

| Credit | Exercised by |
| --- | --- |
| `OVL_0068_068D` | the whole `poolrad_overlay_u68_up_preflight_tests` executable |

Measured on `ee07bd68`: `poolrad_overlay_u68_up_preflight_tests` passed (1/1 under ctest).
