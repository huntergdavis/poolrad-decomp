# Unit 0x25 +0x0F53: provisional full-state store string

`pr_u25_store_string_0f53` translates the complete U25 public entry 12
(far stub `2B:005C`) over caller-owned A20-off 1 MiB RAM and a
fourteen-word CPU frame. It composes its sole near callee, the `+06FB`
address classifier, from that routine's original bytes. Its two SYSTEM
StrStore calls remain explicit fail-closed call-site callbacks. Lane A's
path model `pr_rec_ecl_store_string` (`ecl_helpers.h`) is unchanged. **No
strict full-C, identity or count credit is requested; whole credit remains
for PM review.**

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

The near callee `+06FB..+0755` is the 91-byte classifier, SHA-256
`e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347`
([ECL_LOAD_WORD_CLASS4.md](ECL_LOAD_WORD_CLASS4.md)), ending in `RETF 2`.
The merged copy is static in another owner's file, so this file carries its
own.

SYSTEM `AF8:0B25` StrStore is decoded from `START.EXE`. Its 36 bytes match
the SHA-256 recorded in [RESIDENT_STRING_0356.md](RESIDENT_STRING_0356.md).
It copies the source Pascal string under the low byte of the cap word and
returns with `RETF 0xA`.

## Behavior

Arguments are the far source string `[BP+6]` and the address word
`[BP+0xA]`. The frame is `SUB SP,0x104`: a local `string[255]` at
`[BP-0x100]`, then the class `[BP-0x101]`, length `[BP-0x102]`, index
`[BP-0x103]` and last index `[BP-0x104]`.

1. StrStore copies the source into the local with cap `0xFF`.
2. `PUSH CS; CALL +06FB` returns the class in AL. The classes are 0 for
   `4900..4CFF`, 1 for `6B00..6EFF`, 2 for `9700..98FF`, 3 for
   `9900..B6FF`, and 4 otherwise.
3. The class still held in AL selects the store:
   - **Classes 0, 1 and 2** write each character, zero-extended, as a word at
     window far pointers `[49D2]`, `[49D6]` and `[49DA]`, with displacements
     `+6E00`, `+2A00` and `-2E00`, at `2*(address+i)`. They then write a zero
     word at `2*(address+length)`.
   - **Class 3** writes bytes at `[49DE]+6700+address+i`, then a zero byte.
   - **Address 6B00**, inside class 1, instead StrStores the local to far
     `[5D94]:[5D92]` with cap 15.
   - **Class 4** stores nothing.

Each loop sets last = `length-1` with a 16-bit `DEC` truncated to a byte,
then tests `CMP 0,last; JA`. That `JA` can never be taken, because 0 is never
above a byte. So every loop runs i = 0..last.

A **zero length** gives last = `0xFF`: all 256 elements are written from
local bytes `[BP-0xFF..BP+0]`. These are stale bytes past the copied string,
and the last one is the low byte of the saved BP. The terminator then
overwrites element 0 at `address+0`. The translation keeps this exactly.

Every pointer, local, argument and table word is re-read where the original
reads it, so callee or alias writes take effect in source order. The
classifier writes only `SS:[BP'-1]` and always resumes at `+0F75`.

Flags are exact wherever the StrStore hook or the return can observe them:

- **First StrStore:** the `SUB SP` flags.
- **Name StrStore:** the `CMP word,6B00` flags.
- **Word classes:** `XOR AX,AX` before the terminator.
- **Class 3:** `ADD DI,AX` before the terminator byte.
- **Class 4:** `CMP AL,3`.

The `DEC`, `INC`, `SHL` and `ADD AX` flags are dead and omitted. `XOR` uses
the reference emulator's cleared AF, which is architecturally undefined.
DX ends with the last element (DL only in class 3).

## Verification

`test_overlay_u25_store_string_0f53.c` uses a synthetic StrStore with the
decoded effect. It checks:

- the three word classes at range edges, including terminator placement
  over an existing word, and the final flags and AX
- the byte class and DX
- the `6B00` copy capped at 15
- class 4 with final flags
- the zero-length case: 256 stale elements, the saved-BP byte at element 255,
  and the terminator at element 0
- an abort at both call sites
- argument rejection

It passes strict C11 `-Wall -Wextra -Wpedantic -Werror` and ASan+UBSan.

`check_overlay_u25_store_string_0f53.py` pins:

- the file, body, classifier and StrStore hashes
- both call sites, the single near call and internal jumps
- the neighbouring `RETF 2` and prologue
- the four never-taken `JA` edges

Its source pins have run statically. Its Unicorn differential has **not**
run and waits for a PM serial slot. When run, it covers 143 cases:

- 17 edge and out-of-range addresses, with strings of length 0, 1, 3, 20
  and 255 and stale or constant local buffers
- SS/DS alias and wrapped-frame cases
- 40 seeded cases

For each case it compares the C against original-byte execution of both
routines, over all 1 MiB of RAM, the CPU words and every call trace. It
requires every instruction start and every reachable conditional outcome to
be hit, and asserts that none of the four unreachable `JA` edges ever occurs.

No reached native `+0F53` frame was searched. Runtime and caller native
parity remain open.
