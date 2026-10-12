# Unit 0x68 internal +0x073B (party coordinate writer): path-complete model

`src/recovered/overlay_u68_coordinate_writer.c` models DOS v1.3 `GAME.OVR`
unit `0x68` internal routine `+0x073B..+0x07F3`. Entry 1 (`+0x0D6D`) reaches
it by `push cs; call +0x073B` at `+0x102E` when its local movement flag
`[bp-1]` is nonzero. It is the routine that **stores the stepped party
position**: it adds a facing-indexed step from two data-segment byte tables to
`[0x6AAD]` / `[0x6AAE]`, wraps the result into 0..15, then refreshes three
lookup results and notifies unit `0x7d`. Classification: **path-complete
model** of the 185 code bytes with the two unmerged far callees as explicit
host hooks. **No game-function credit is claimed**; the PM decides the
accounting. This note does not claim the full slot-A route.

`deja "Poolrad unit 0x68 +0x073B coordinate writer entry1 0x102E"` found
only this session's own prompt. Reused with citation and re-checked here:
the Up preflight note [U68_UP_PREFLIGHT_068D.md](U68_UP_PREFLIGHT_068D.md)
(party bytes, the two direction tables and their 9-byte skew, window-1 word
semantics, unit provenance); the merged unit `0x3bd` entries 2 and 4
([U3BD_NIBBLE_ENTRY2.md](U3BD_NIBBLE_ENTRY2.md),
[U3BD_CELL_ENTRY4.md](U3BD_CELL_ENTRY4.md)); GAMEPLAY_PATH.md for the
entry-1 movement bookkeeping row (resident `0x8AE:0x00DC`, party bytes).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x68`: code at file offset `0x166CB`, run-time segment `0x72`. Decoded
with Capstone (16-bit).

| Item | Value |
| --- | --- |
| code extent | `+0x073B..+0x07F3` = file `0x16E06..0x16EBE`, **185 bytes, 66 instructions**, one `retf` at `+0x07F3`; SHA-256 `342da33f91df7b31ba2aa2819f3e2fc6508f9d0b6634f7f22df616788d675fc5` |
| trailing data | `+0x07F4..+0x0817`, **36 bytes** = file `0x16EBF..0x16EE2`, SHA-256 `068ac240cc0bd037563adc671ff59de0b83af56efd85dc6b25a6b98374bd8d69`: a Pascal string constant (length byte 35, then 35 printable bytes), not code. The next routine `+0x0818` pushes `cs:+0x07F4` at `+0x0916` and calls runtime `0xAF8:0x0B0B` with it. Its text is not reproduced here. 185 + 36 = 221, the PM's bound; the ledger's own bound is 219 |
| frame | `push bp; mov bp, sp` and **no locals** (`pop bp; retf` at the end, no `mov sp, bp`); no arguments; no result read |
| branches | 5 forward conditionals (`+0x076A jge`, `+0x0776 jle`, `+0x0782 jge`, `+0x078E jle`, `+0x07D8 jbe`) and one `+0x07E5 jmp`; no loops |
| near callers | one: `+0x102E` (entry 1). A scan of every `call rel16` in the unit whose target is `+0x073B` finds only that site, preceded by `push cs` |
| far calls | 5 (table below) |
| DS reads | bytes `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]`; word `[0x2628]`; `[di+0x2736]`, `[di+0x273F]`; far pointer `[0x49D6]` |
| DS writes | `[0x6AAD]`, `[0x6AAE]` (each three times: add, then up to two wrap stores), `[0x6AB0]`, `[0x6AB1]`; DS `0x6DDE..0x6DE0` through the runtime fill |
| ES reads | word `es:[di+0x594]` (window 1); **no ES write** in this span |

## Instruction paths (in order)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x073E..+0x0742` | `push word [0x2628]; lcall 0x8AE:0x00DC` | resident call, one word; unresolved (RESIDENT_FUNCTION_INVENTORY.md lists the segment's routines as unresolved): host hook `resident_8ae_00dc(word_2628)` |
| `+0x0747..+0x0752` | `di = zx [0x6AAF]; al = [di+0x2736]; add byte [0x6AAD], al` | `x = (uint8_t)(x + dx_table[facing])` (8-bit add, wraps mod 256) |
| `+0x0756..+0x0761` | `di = zx [0x6AAF]; al = [di+0x273F]; add byte [0x6AAE], al` | `y = (uint8_t)(y + dy_table[facing])` (dy table 9 bytes after dx) |
| `+0x0765..+0x0778` | `cmp byte [0x6AAD], 0; jge; mov [0x6AAD], 0xF; cmp [0x6AAD], 0xF; jle; mov [0x6AAD], 0` | signed byte: `<0 -> 15`, then signed `>15 -> 0` |
| `+0x077D..+0x0790` | the same on `[0x6AAE]` | same |
| `+0x0795..+0x07A6` | push x, y, facing bytes (AH is 0, set by `xor ah, ah` at `+0x074A` and never changed); `lcall 0x3D5:0x2A`; `[0x6AB0] = al` | `pr_u3bd_entry2(x, y, facing)` (unit `0x3bd` entry 2, merged), result to `lookup_b0` |
| `+0x07A9..+0x07B5` | `push ds; push 0x6DDE; push 3; push 1; lcall 0xAF8:0x16A8` | runtime fill, (pointer, count, value) by argument shape only (same inference as GAMEPLAY_PATH.md): `ds_6dde[0..2] = 1` |
| `+0x07BA..+0x07C7` | push x, y bytes; `lcall 0x3D5:0x34`; `[0x6AB1] = al` | `pr_u3bd_entry4(x, y)` (unit `0x3bd` entry 4, merged) |
| `+0x07CA..+0x07D8` | `les di,[0x49D6]; ax = es:[di+0x594]; and ax, 1; or ax, ax; jbe +0x07E7` | bit 0 of window-1 word `0x594` (script word `0x6DCA`) |
| `+0x07DA..+0x07E5` | bit set: push 1; push 2; `lcall 0x8C:0x25`; `jmp +0x07F2` | unit `0x7d` entry 1 (`+0x03D5`) with (1, 2) |
| `+0x07E7..+0x07ED` | bit clear: push 1; push 1; `lcall 0x8C:0x25` | unit `0x7d` entry 1 with (1, 1) |
| `+0x07F2..+0x07F3` | `pop bp; retf` | |

**Stubs.** The stub segment `0x3D5` is unit `0x3bd` (stub stride 5 from `0x25`
for entry 1, so `0x2A` = entry 2 and `0x34` = entry 4, matching the nibble and
cell notes' caller rows). Stub segment `0x8C` is unit `0x7d` (the inventory
row `0x7d`, stub paragraph `0x8C`); `0x8C:0x25` is its entry 1 at `+0x03D5`
(ledger: 165 bytes, 8 far callers). It is **not translated** in this tree.
Resident `0x8AE:0x00DC` is also untranslated and unresolved. Both are
behind hooks; neither's effects are asserted. They are the call boundaries.

**State effects, exactly.** Stored: `[0x6AAD]`, `[0x6AAE]` (the stepped,
wrapped position), `[0x6AB0]`, `[0x6AB1]`, `DS:0x6DDE..0x6DE0 = 1,1,1`.
Everything else is read-only here. The position stores are real: unlike the
Up preflight (`+0x068D`, which only clamps and raises window-1 word `0x5AA`),
this routine commits the step. VP's trace is confirmed: `+0x0752` is the x
add and `+0x0761` the y add. The wrap differs from the preflight's clamp:
`16..127 -> 0` and `128..255 -> 15`, so a step off either edge reappears on
the opposite edge. Which arm of entry 1 lets an out-of-range candidate reach
this routine is not established here. Facing is never written. The result is not
returned in a register that a caller reads (the caller falls to `+0x103C`).

**Coordinate data source.** The step bytes are DS:`0x2736 + facing` (x) and
DS:`0x273F + facing` (y), added as raw bytes; the facing is `[0x6AAF]`
zero-extended. The same two tables feed the preflight. Their live contents
remain **unestablished**: the image bytes at the computed data-segment
position of `START.EXE` do not form a unit-step table (re-checked here; the
Turbo Pascal data segment is presumably rewritten at run time), so they are
host state in the model. The writers of `[0x6AAF]` keep it in `{0, 2, 4, 6}`
(U3BD_NIBBLE_ENTRY2.md); other facings are modelled, not assumed
unreachable.

## Entry 1's caller context (the gate)

Entry 1 `+0x0D6D`: `[bp-1] = 0` at `+0x0D74`; with `[0x49FA] == 1` and
window-1 word `0x592` below 0xFF it pushes the party x, y and facing bytes
and calls unit `0x3bd` entry 3 (`+0x0D9E`). **Result 1 sets `[bp-1] = 1`
(`+0x0DA7`) and jumps straight to `+0x1027`**, where `cmp [bp-1], 0; je
+0x1033` is not taken and `push cs; call +0x073B` runs at `+0x102E`; then
`+0x1031 jmp +0x103C` and resident `0xBA:0x290B`. Other results branch to other arms
(`+0x0DB5`, `+0x0EED`) and the key paths later in the entry; the writes of
`[bp-1]` from the helper routines `+0x0294`, `+0x04DF`, `+0x0648` (return values stored at
`+0x100C`, `+0x1024`, `+0x0ECD`, `+0x0EDA`, `+0x0EE7`) reach the same test. When `[bp-1] == 0` the else arm calls the same resident routine
`0x8AE:0x00DC` with the word `[0x2622]` instead of `[0x2628]`, then the
resident `0xBA:0x290B`. So the coordinate store runs only when the move was
reported allowed by one of those arms; for the one-Up-key path of
U68_UP_PREFLIGHT_068D.md, entry 3's result (the merged field lookup) decides.
Which result values the entry-3 lookup takes at a given saved cell, and the
helper arms' return values, are not computed here.

This is a connection **inside unit 0x68 only**. The slot-A route to ECL
`0x996B`, the minute writer and the live direction tables remain open and
belong to the other lane.

## Minute bytes (VP's question): not found in this span

**Update after the unit 0x7d note:** the span itself still writes no window-0 byte, but its last far call (`0x8C:0x25`, unit 0x7d entry 1) rewrites window 0 words `+0x018C..+0x0199`; with `idx` 1 or 2 that is `+0x018E` or `+0x0190`. See [U7D_ENTRY1.md](U7D_ENTRY1.md).

This span writes no window-0 byte and never loads `[0x49D2]`; it has no
`+0x18E`/`+0x190` write. Its only ES access is the read of window-1 word
`0x594` (script word `0x6DCA`, the word the preflight note lists next to
`0x6DC9`). Callees'
effects are the merged entries 2 and 4 (read-only) and the two hook targets,
whose bodies are outside this lane.

## Verification

`tests/recovered/test_overlay_u68_coordinate_writer.c` fabricates the buffer,
map id, window 1, direction tables and hook recorders (no game data) and
compares the model with an independent raw-byte restatement over all x bytes,
a dense y sampling, ten facings (the four live ones, odd ones, 8 and 255),
two map ids and both values of the window bit. It checks the stored position,
`[0x6AB0]`/`[0x6AB1]`, the 1,1,1 fill, hook order (resident first, unit `0x7d`
last), the hook words (`[0x2628]`; `(1, 2)` or `(1, 1)`), that window 1 is
untouched, pinned wrap cases (15+1 → 0, 0−1 → 15, 127+1 → 15, 0x10+1 → 0)
and that every ARGUMENT guard leaves state untouched with no hook run.
