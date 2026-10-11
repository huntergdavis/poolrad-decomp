# Gameplay path: program body, party menu, first script run (issue #56)

**Scope.** One concrete, address-only control path in the DOS v1.3 build from the program
body through the party menu to the first run of an area script. Inputs: `START.EXE` sha256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` (65,231 bytes) and
`GAME.OVR` sha256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
(206,105 bytes), both digest-checked before decoding. Resident addresses are loaded-image
offsets from the merged loader expansion (`tools/overlay/resident_loader_map.py`, run in
memory; Codex's [loader map](RESIDENT_LOADER_MAP.md) and [callable coverage](RESIDENT_CALLABLE_COVERAGE.md)
identify the program body `RPRO_000F6`). Overlay addresses are `unit + offset`; far-call
targets come from the fixup-confirmed call graph with the run-time stub addressing of
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) section 3. Only offsets, lengths, immediates and
counts appear here; no game text or executable bytes. An exact `deja` query for prior
path-tracing work returned no session. Two routines on the path are translated (section 6;
FUNCTION_LEDGER.md section 1.14).

## 1. The path

| step | caller site | callee | evidence |
| --- | --- | --- | --- |
| 1 | program body, image `0x00FD` | unit `0x36` entry 1 (`+0x082D`, the +0x800 alias of `+0x002D`) | the window and script-buffer allocator of FUNCTION_LEDGER.md section 1.7: allocates the three windows and the script buffer, zero-fills them, sets `[0x49F8] = 0`, `[0x49FF] = 0`, `[0x49ED] = 0x9900`, `[0x5376] = 1`, party x/y bytes `[0x6AAD] = 0x0F`, `[0x6AAE] = 1` |
| 2 | image `0x0102..0x011B` | runtime `0xAF8:0x16BC` then `0xAF8:0x0C10` | argument shape of ParamStr(1) into a 256-byte local, then a string compare against the constant at `cs:0x0000`; `je 0x0127` skips step 3 when the parameter is empty |
| 3 | image `0x011D`, `0x0122` (only with a command-line parameter) | unit `0x17` entry 1 (`+0x0627`), unit `0x19` entry 1 (`+0x08FD`) | off the default path; not traced further |
| 4 | image `0x0127` | unit `0x6c` entry 1 (`+0x0161`) | the party menu, section 2; returns only through the two paths in 2.3 |
| 5 | image `0x012C..0x0138` | unit `0x3bd` entry 1 (`+0x0155`) with byte arguments `0x0B, 0, 7, 6` | the view-parameter setter, translated (function 16) |
| 6 | image `0x013D` | unit `0x1c` entry 1 (`+0x32F8`) | the gameplay loop, section 3; unit `0x1c` has no overlay far callers, so this is its only entry route found |
| 7 | unit `0x1c + 0x3358` | unit `0x25` entry 4 (`+0x0341`) with the map id byte | the ECL block load, section 4; skipped when `[0x49F8] != 0` |
| 8 | unit `0x1c + 0x335D` | unit `0x25` entry 3 (`+0x01B6`) | the script-state initializer, translated as a model (function 17) |
| 9 | unit `0x1c + 0x3362..0x3367` | near `+0x3171` with `[0x49EB]` | the ECL run function (ECL_CONTRACTS.md section 2) on the fifth entry address, the initial-entry script: **the first gameplay transition** |
| 10 | image `0x0142` after the loop returns | resident `0x899:0x0000` | also the menu's exit path; the program body then falls to its epilogue |

## 2. The party menu, unit 0x6c + 0x0161 (bytes +0x0161..+0x04F0, 911 bytes to the `retf`)

### 2.1 Menu records in resident data

The routine indexes a table of 11 records of 42 bytes at `DS:0x0619` (record i at
`0x0619 + 0x2A * i`): a Pascal string of at most 40 characters followed by an enable byte at
record offset `0x29` (`DS:0x0642 + 0x2A * i`). The data segment paragraph is `0xC7C`
relative to the loaded image (record 0 at image `0xCDD9`, record 9 at image `0xCF53`),
established by matching the 11 length bytes at that stride. The routine prints the
enabled records one per line (`+0x0229..+0x02C9`: first character through `0xAF8:0x0C3B`
and the rest through `0xAF8:0x0B57`, each printed by resident `0x709:0x0626`).

### 2.2 Enable flags and key dispatch

- `+0x0168..+0x017A`: `[0x5376] = 3` (the ECL file number used by the block load, section
  4), local mode byte `[bp-0x10] = 4`, redraw flag `[bp-0xF] = 1`, `[0x470C] = 0`,
  `[0x49F3] = 0`.
- `+0x01AE..+0x0229`: on every redraw, if the active-character far pointer `[0x5D92]` is
  non-nil the flags of records 1, 2, 4, 5, 6, 8, 9 are set to 1, record 7's to 0, and record
  3's to 1 only when window 1 word `0x2A8` is non-zero or `[0x470C]` is set; with a nil
  pointer all of those are 0 except record 7's, which is 1. Record 0's and record 10's
  flags are not written here.
- `+0x02D0..+0x0300`: key input through unit `0x3b0` entry 2 (`+0x03BD`, section 2.4) into
  `[bp-2]` with an extended-key byte in `[bp-3]`; `+0x0305` branches on that byte to a side
  path (`+0x030B..+0x0331`: a key-in-set test through `0xAF8:0x0DC0` against the constant
  at `cs:0x00F6`, which has length 0, so the test never passes; a character redisplay) that
  loops without redraw.
- Compare chain on the key byte, each guarded by its record's enable flag:

| compare site | key | record / flag | action |
| --- | --- | --- | --- |
| `+0x034C` | `0x43` | 0 / `[0x0642]` | near `+0x056C` |
| `+0x035E` | `0x44` | 1 / `[0x066C]` | near `+0x1F37` |
| `+0x0370` | `0x4D` | 2 / `[0x0696]` | near `+0x209F` |
| `+0x0382` | `0x54` | 3 / `[0x06C0]` | near `+0x29AE` |
| `+0x0394` | `0x56` | 4 / `[0x06EA]` | unit `0x77` entry 5 (`+0x0AB3`) |
| `+0x03AC` | `0x41` | 5 / `[0x0714]` | near `+0x306D` |
| `+0x03BE` | `0x52` | 6 / `[0x073E]`, non-nil `[0x5D92]` | unit `0x6f` entry 4 (`+0x069E`), near `+0x3291` |
| `+0x03FB` | `0x4C` | 7 / `[0x0768]` | unit `0x6f` entry 8 (`+0x1602`); then `+0x040B`: if `[0x49F3] != 0`, `[0x49F8] = 1` and **return** (`+0x0417` to `+0x04ED`) |
| `+0x041D` | `0x53` | 8 / `[0x0792]`, non-nil party head `[0x5D96]` | unit `0x6f` entry 9 (`+0x1D58`) |
| `+0x0439` | `0x42` | 9 / `[0x07BC]`, non-nil party head `[0x5D96]` | `+0x044D..+0x0450`: `[0x49F3] = [bp-0x10]` (4); unit `0x3b0` entry 3 (`+0x06FB`); **return** (`+0x0458` to `+0x04ED`) |
| `+0x045E` | `0x45` | 10 / `[0x07E6]` | prompt through unit `0x3b0` entry 5 (`+0x10D1`) with `0x0E, 0x0A, 0x0F`; on `0x59`: save through unit `0x6f` entry 9 while `[0x557D] == 0` and the party head is non-nil, then resident `0x899:0x0000` (no return observed statically) |
| `+0x04AD` | `0x4A` | none | ParamStr(1) compared with the 5-character constant at `cs:0x0143`; on match `[0x470C] = 1` and resident `0xBA:0x1693` |

Every other outcome sets the redraw flag (`+0x04E6`) and loops to `+0x017F`. Each record's
string begins with the record's key letter (checked privately; the strings themselves are
not reproduced here).

### 2.3 The two returning paths

Only two sites reach the epilogue at `+0x04ED`: key `0x42` with record 9 enabled and a
non-nil party head (sets the mode byte `[0x49F3]` to 4), and key `0x4C` after a load whose
result left `[0x49F3]` non-zero (sets the restored flag `[0x49F8]`). The program body does
not test which; it proceeds to steps 5 and 6 either way. Unit `0x6f` entry 8 is the only
writer of `[0x5376]` besides the allocator and the menu, so a restored game can change the
ECL file number; a new game keeps 3.

### 2.4 The menu's key input, unit 0x3b0 + 0x03BD (bytes +0x03BD..+0x06FA, `retf 0x16`)

Arguments from the menu call at `+0x02D0..+0x02F8`: the prompt string at `cs:0x00E3`
(`[bp+0x18]`), the key-list string at `DS:0x05F0` (`[bp+0x14]`: 23 characters, the twelve
key letters of section 2.2 separated by spaces), row `0x0D` (`[bp+0x12]`), two zero bytes
(`[bp+0x10]`, `[bp+0x0E]`: together they clear the local `[bp-0x8F]`, so key `0x0D` is
**not** accepted here, `+0x05B2`), byte 1 (`[bp+0x0C]`, enables the digit path at
`+0x0691`), byte 0 (`[bp+0x0A]`), and the far pointer to the menu's extended-key byte
(`[bp+6]`). The loop `+0x048E..+0x06D5`:

- `+0x0566..+0x0585`: polls resident `0xA40:0x02FD` (loops while it returns 0) and reads a
  key through resident `0x899:0x0059`; a zero first byte reads once more (the scan code of
  an extended key) and, with `[bp+0xC] != 0`, sets the extended-key byte and returns. The
  resident image reaches the BIOS keyboard service through `int 16h` at loaded images
  `0xA552` (`ah = 1`) and `0xA558` (`ah = 0`), so the input source is the BIOS buffer.
- `+0x059C..+0x05AB`: `0x1B` returns 0 with the extended-key byte set.
- `+0x05C4..+0x0609`: `0x2C` and `0x2E` move the highlight index `[0x6B2B]` through the
  key list and redraw (near `+0x01DA`); they do not return.
- `+0x060C..+0x068F`: any other key is upcased (`0xAF8:0x182A`), tested against the set at
  `cs:0x037D` (`0x20` accepted as a selection), then matched against the key-list
  characters; a match sets the highlight and returns that letter.
- `+0x0691..+0x06CB`: with `[bp+0xC]`, keys in the set at `cs:0x039D` map through the
  table at `[di + 0x288C]` (and `0x5C` to `0x37`) and return with the extended-key byte.
- A timed branch (`+0x0432..+0x052D`, resident clock `0x709:0x0C3F`, window 0 word `0x1FF`,
  `[0x6ABE]`, `[0x6ABF]`) animates while waiting and does not consume keys.

Consequences for a scripted oracle: the selection keys are the twelve letters, in either
case; `0x0D` and the arrow keys do nothing on this menu (an arrow key returns as an extended
key, which the menu's side path discards); `0x2C`/`0x2E` only move the highlight. The PM's
oracle later confirmed the sequence `0x0D`, `l` reaches the saved-game chooser once the
configuration path was corrected.

## 3. The gameplay loop, unit 0x1c + 0x32F8 (bytes +0x32F8..+0x3507, 528 bytes, 164 instructions)

| offsets | what the bytes show |
| --- | --- |
| `+0x32FE..+0x331C` | `[0x4433] = 0`, `[0x4434] = 0`, `[0x4430] = 0`, `[0x4431] = 0`, `[0x4432] = 1`, `[0x49F3] = 4`, `[0x442F] = 0` |
| `+0x3321..+0x332A` | the map id: low byte of window 0 word `0xF2` (script address `0x49F2`) into `[0x84DC]` |
| `+0x332D..+0x3348` | if the map id is `0x19`, `0x1A` or `0x1B` and window 0 word `0xE6` is 0, `[0x49F3] = 3` |
| `+0x334D..+0x3358` | if `[0x49F8] == 0`: unit `0x25` entry 4 (`+0x0341`) with the map id (step 7) |
| `+0x335D` | unit `0x25` entry 3 (`+0x01B6`) (step 8) |
| `+0x3362..+0x3367` | push `[0x49EB]`; near `+0x3171`: run the initial-entry script (step 9) |
| `+0x336A..+0x3382` | if `[0x49F3] != 3`: when `[0x4432] == 1`, resident `0xBA:0x27D9` and `[0x4432] = 0`; unit `0x3b4` entry 1 (`+0x016D`, the view refresh) |
| `+0x3387..+0x3395` | window 0 word `0xF2` rewritten from `[0x84DC]`; then the input loop: unit `0x68` entry 2 (`+0x0989`) returns a key byte in `AL`, the active-character pointer is copied to `[0x4436]` |
| `+0x33B9..+0x3449` | the search/encounter branch: when window 1 word `0x2CA` is at most 1 the key must upcase (`0xAF8:0x182A`) to `0x45`; with `[0x49FF] == 0` either near `+0x2DD3` (key `0x45`) or the search entry `[0x49E5]` is run after setting window 1 word `0x2CA` to 1 (restored afterwards from `[0x84DD]`) |
| `+0x344C..+0x3466` | otherwise, with `[0x49FF] == 0`, the run entry `[0x49E3]` is run; `[0x442F] != 0` (set by the handler at `+0x0BD1`, which also writes the map id) routes to near `+0x31B4` |
| `+0x3473..+0x34F2` | movement bookkeeping: window 0 words `0xF0`/`0xF1` from the party x/y bytes `[0x6AAD]`/`[0x6AAE]`, unit `0x68` entry 1 (`+0x0D6D`), the view refresh, resident `0x8AE:0x00DC` on a position change, `[0x84E0] = 0`, `[0x84E1] = 1`, the search entry `[0x49E5]` run again |
| `+0x34F5..+0x3507` | loop to `+0x3395` while `[0x49FF] == 0`; otherwise `[0x49FF] = 0` and `retf` |

Writers of the map id word (window 0 word `0xF2`) across all 673 overlay routines: the two
rewrites above and the handler at unit `0x1c + 0x0BD1` (`+0x0BE9`). The allocator zero-fills
window 0 and no other writer precedes step 7, so a new game enters step 7 with map id 0.

## 4. The ECL block load, unit 0x25 + 0x0341 (bytes +0x0341..+0x0410, 208 bytes, 89 instructions)

`+0x0348..+0x0355`: fill the script buffer `[0x49DE]` (`0x1E00` bytes, value 0, runtime
`0xAF8:0x16A8`); `+0x035A..+0x037C`: `[0x557C] = 0`, unit `0x3b0` entry 3, a 21-character
constant at `+0x0322` shown through `0xAF8:0x0B0B` and resident `0x709:0x0626` (the wait
message); `+0x0381..+0x03C0`: the file name built from the 3-character constant at
`+0x0338`, the decimal of byte `[0x5376]` (runtime `0xAF8:0x1366`, argument shape of Str),
and the 4-character constant at `+0x033C`, i.e. the `ECL<n>.DAX` pattern of
ECL_CONTRACTS.md section 1; `+0x03C5..+0x03D3`: resident `0x802:0x0716` with the map id byte
and two locals (size `[bp-8]`, far pointer `[bp-0xC]`), the DAX block loader, not decoded
here; `+0x03D8`: a size below 2 jumps back to `+0x035F` (retry); `+0x03DE..+0x0401`: move
`size - 2` bytes from `pointer + 2` into the script buffer (`0xAF8:0x1685`, argument shape
of Move) and release the block (`0xAF8:0x0254`, argument shape of FreeMem); `+0x0406`: unit
`0x3b0` entry 3; `retf 2`. This is why the buffer addresses unpacked offset 2 (ECL_CONTRACTS.md
section 2). The ECL file with number 3 contains a block 0 (`tools/overlay/ecl_walk.py`),
consistent with the new-game map id.

## 5. The initializer, unit 0x25 + 0x01B6, and the run function

Section 1.14 of FUNCTION_LEDGER.md gives the instruction-level account of the initializer
(function 17). In short: fixed byte writes, `[0x49ED] = 0x9900`, the call-stack head and
compare flags cleared, four window words set, five operand loads with count 1 whose words
become `[0x49E3]`, `[0x49E5]`, `[0x49E7]`, `[0x49E9]`, `[0x49EB]` (the five block-head
entries), window 0 word `0xE6 = 1`, then either 42 window words cleared (new game) or the
restored flag consumed. The run function `+0x3171` (ECL_CONTRACTS.md section 2) stores its
argument to `[0x49ED]`, clears `[0x442E]`, and loops fetching the opcode through
`[0x49DE] + PC + 0x6700` into `[0x6F7F]` and calling the dispatcher `+0x2F2D` until
`[0x442E]` or `[0x49FF]` is set.

## 6. What is verified, what is inferred, what is missing

**Verified from the bytes:** every call site, target and branch condition in sections 1 to
5; the menu record stride and flag offsets; the two returning paths of the menu; that unit
`0x1c` has no overlay far callers; the writers of the map id word, of `[0x49F8]` (allocator,
initializer, menu) and of `[0x5376]` (allocator, menu, unit `0x6f + 0x1602`); the
translations of functions 16 and 17 (FUNCTION_LEDGER.md section 1.14).

**Inferred from argument shape only (runtime routines):** `0xAF8:0x16A8` fill,
`0xAF8:0x1685` move, `0xAF8:0x0254` release, `0xAF8:0x16BC` parameter string,
`0xAF8:0x0C10` string compare, `0xAF8:0x182A` upcase, `0xAF8:0x1366` number to string;
resident `0xA40:0x02FD` key-available and `0x899:0x0059` key-read from their use.

**Not established:** the meanings of `[0x49F3]` values 3 and 4 beyond their writers; the
resident DAX block loader `0x802:0x0716` and the exit routine `0x899:0x0000`; the bodies of
the menu's per-key routines; the semantics of the view bytes `[0x6A60..0x6A63]`.

**Runtime observation so far (DOS oracle, [docs/oracle/README.md](../oracle/README.md)).**
The oracle has loaded saved-game slot A from the menu (key `0x4C`, the restored path of
section 2.3) and reached the exploration screen at displayed position `15,4 N`; one
movement command advanced it to `14,4 N`. That exercises steps 4, 6 and the input loop of
section 3 on the **restored** path (`[0x49F8] == 1`, so step 7 is skipped and the script
buffer comes from the save). It does not yet observe the **new-game** path (key `0x42`),
the block load of step 7, or the memory values below; the position pair matches the party
x/y bytes `[0x6AAD]`/`[0x6AAE]` that the loop copies into window 0 words `0xF0`/`0xF1`,
which a memory read at those addresses could confirm.

**Still missing from the oracle, with a reproducible trace target.** Break at loaded-image offsets `0x0127`, `0x0138` and `0x013D` of the program body
(relative to the load segment) and, in the overlay, at unit `0x6c + 0x044D`, unit `0x1c +
0x3358`, `+0x335D` and `+0x3367` (the overlay units enter through the INT 3Fh stubs
`0x78:0x25`, `0x3D5:0x25` and `0x1F:0x25`). Expected on a new game after key `0x42`:
`[0x49F3] == 4`, `[0x5376] == 3`, `[0x84DC] == 0`, `[0x49F8] == 0`; after `+0x335D` returns:
`[0x49ED] == 0x9914`, and the words at `[0x49E3..0x49EB]` equal the five entry operands of
block 0 of the file with number 3; after `+0x3367` the dispatcher runs the initial-entry
script. The same breakpoints after key `0x4C` should show `[0x49F8] == 1` and step 7
skipped, which the slot A run now supports at the screen level but not yet at the memory
level. Those observations would confirm the map-id origin and the restored-game state,
which this static trace supports but cannot execute.

## 7. Reproduce (private; writes nothing into the repository)

```sh
G=resources/DOS/POOLRAD; O=/path/outside/repo
python3 tools/overlay/overlay_inventory.py --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/overlay_inventory.json --md $O/inv.md
python3 tools/overlay/callgraph.py --inventory $O/overlay_inventory.json --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/callgraph.json --md $O/cg.md
python3 tools/overlay/ecl_walk.py --dir $G --json $O/ecl_walk.json
# Expand the resident image in memory with resident_loader_map.unpack(exe)[0] and decode
# image 0x0006..0x0150 and the overlay routines above with Capstone 5 (16-bit); resolve
# far calls through the inventory's memory_segment and entries, and the call graph's
# fixup-confirmed edges keyed by stub paragraph and site offset.
```
