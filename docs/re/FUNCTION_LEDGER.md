# Function ledger: recovered DOS v1.3 functions

**Issue:** #8 (lane A), completed by PRs #11 and #15; loader follow-up #16. **Target:** `GAME.OVR` sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` (206,105 bytes) with
`START.EXE` sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
**Code:** `src/recovered/ecl_helpers.c`, `include/poolrad/recovered/ecl_helpers.h`,
`tests/recovered/test_ecl_helpers.c`. All five originals live in overlay unit `0x25`
(file offset 29,907, 8,730 code bytes, 25 public entries, run-time stub segment `0x2B`).

**Delivered so far:** three **full translations** of leaf functions (functions 1, 4, 5),
two **extracted behavior cores** (functions 2 and 3), one **recovered state-transition
core** (function 6, the operand loader, issue #16), and the engine string windows with two
further path-complete translations and two cores (section 1.7, issue #22). A full translation reproduces every
instruction path of the original with the same inputs and outputs. An extracted core
reproduces a verified decision table or decoding loop but replaces engine memory (script
buffer, PC, Pascal string buffers, runtime calls) with explicit arguments and omits the
side effects; it is evidence toward a later full translation, not one itself.

Status vocabulary: **verified** = disassembly read instruction by instruction and an
independent check agrees with the part that is claimed; **provisional** = a stated aspect
rests on inference. Decompiler renderings were used as a reading aid only and are not
committed.

## 1. The five functions (1 to 3 from PR #11, 4 and 5 from PR #15)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 1 | `pr_rec_ecl_char_remap` = GAME.OVR 0x07BA6, unit 0x25 + 0x06D3 (internal, not a public entry) | 40 bytes, 16 instructions | far Pascal: one word argument at `[bp+6]`, result in `AL`, `retf 2`; no globals, no calls | **full translation** | **verified** |
| 2 | `pr_rec_ecl_operand_count` = GAME.OVR 0x09379, unit 0x25 + 0x1EA6, public entry 22 (called as `0x2B:0x8E`, 6 sites, all in the IF-family handler of unit 0x1c) | 267 bytes, 133 instructions | far Pascal, no arguments, `retf`; reads `[0x49DE]` (script buffer far pointer) and `[0x49ED]` (PC), writes `[0x6F7F]` (opcode latch); near-calls the operand loader (unit 0x25 + 0x0000) with a count in `AL`, else `PC += 1` | **extracted core**: the opcode-to-count table only; the fetch, latch, loader call and PC advance are documented, not translated | table **verified**; role as "skip next statement" **verified** by its call sites; the prefetch-versus-wire distinction below is a measured finding |
| 3 | `pr_rec_ecl_unpack_text` = GAME.OVR 0x08626, unit 0x25 + 0x1153 (internal; near-called by the loader for tag `0x80` operands) | 424 bytes, 166 instructions | far Pascal: `[bp+6]` string-buffer index (byte), `[bp+8]` packed length (byte), `retf 4`; reads bytes through `[0x49DE]`/`[0x49ED]` and advances the PC once per byte; clears and appends to the Pascal string at `0x6FC8 + 256 * index` through four runtime calls at `0xAF8:0x0B0B`, `0xAF8:0x0C3B`, `0xAF8:0x0B98`, `0xAF8:0x0B25` (string load, char-to-string, concat, store, unidentified); near-calls function 1 | **extracted core**: decodes from a caller buffer; PC advance and string-runtime side effects omitted | decoding **verified**; the 255-character cap is **provisional** (section 1.4) |

### 1.1 Character remap (function 1)

Compare the argument with `0x1F`; if not above, add `0x40` in `AX` (high byte cleared
first) and return the low byte; otherwise return the argument. The test checks all 256
inputs against this rule and the six-bit alphabet boundaries. Lane B's text decoder
(`src/formats/ecl.c`) applies the same mapping, so the two layers agree.

### 1.2 Operand-count dispatcher (function 2)

The function fetches the byte at `buffer + PC + 0x6700`, latches it as the current opcode,
and runs seven compare chains. Each chain ends by pushing a count and near-calling the
operand loader at unit offset 0; the final `else` only increments the PC. Transcribed table
(opcodes not listed take no prefetched operand):

| prefetch count | opcodes |
| --- | --- |
| 1 | 0x01 0x02 0x0A 0x0E 0x11 0x12 0x1D 0x20 0x2D 0x32 0x34 0x36 0x38 0x39 0x3C |
| 2 | 0x03 0x08 0x09 0x0C 0x0F 0x10 0x1F 0x22 |
| 3 | 0x04 0x05 0x06 0x07 0x0B 0x21 0x28 0x2A 0x2F 0x30 0x35 0x37 0x3B |
| 4 | 0x14 0x23 |
| 5 | 0x2E |
| 6 | 0x1E 0x2C |
| 8 | 0x27 |
| 14 | 0x29 |
| 0 | 0x00 0x0D 0x13 0x15 0x16–0x1B 0x1C 0x24 0x25 0x26 0x2B 0x31 0x33 0x3A 0x3D and every value above 0x3D |

The test holds an independent dense transcription of the chains (checksum 117) and checks
all 256 opcode values.

**Measured finding: prefetch count is not the wire format.** Running the ECL linear sweep
(`tools/overlay/ecl_walk.py --sweep`) with this table decodes only 33,812 of 175,627 body
bytes (19.3%) before the first error, against 107,060 (61.0%) with the reference
interpreter's table. The two tables differ only at 0x0C (2 vs 3), 0x34 (1 vs 2) and 0x36
(1 vs 2); the shipped scripts carry the larger counts, so those three handlers read one
further operand themselves after the prefetch. The dispatcher's six call sites are all in
the IF-family handler: when a test fails, the handler calls this function to consume the
next instruction, which is exactly a "skip next statement" primitive. Consequence: a
re-implementation must treat the prefetch table as the engine's behavior and the wire
table as the script format; both are now in `ecl_walk.py`.

### 1.3 Six-bit text unpacker (function 3)

State machine with phase 1 to 4 over the consumed bytes: phases 1 to 3 read one new byte
each (previous byte kept), phase 4 reuses the last byte. Codes: phase 1 `cur >> 2`;
phase 2 `(prev & 3) << 4 | cur >> 4`; phase 3 `(prev & 0xF) << 2 | cur >> 6`; phase 4
`cur & 0x3F`. Every non-zero code is remapped with function 1 and appended; **zero codes are
skipped, not terminators** (there is no exit from the loop on a zero code). After the loop,
if the phase counter stands at 4 the fourth code of the last group is emitted when non-zero.
The loop runs until exactly `packed_len` bytes were consumed, so the length byte of the
`80 LL` operand is authoritative.

Checks:

- synthetic vectors in the test: `AB ` and `ABCD` (the same vectors lane B uses), a
  two-byte group (`AB`, the partial third code is not emitted), a one-byte group, a group
  with zero codes between characters (`AC` from codes 1,0,0,3), empty and null inputs,
  capacity handling, and the 255-character cap;
- the Gold Box Explorer reference decoder (`DaxEclFile.cs`) also skips zero codes and uses the
  same bit layout;
- a data check on the owner's scripts: of 1,582 inline text frames reached by the linear
  decode, 359 contain a zero code and **none** has a non-zero code after a zero, so the
  difference between "skip" and "stop at first zero" is not observable in the
  reached shipped frames. Lane B adopted the engine's skip rule in PR #13.

### 1.4 Evidence on the unpacker's 255-character cap (provisional)

What the bytes show: for every emitted character the original performs the Turbo Pascal
`s := s + chr(c)` sequence, and the final store call is preceded by `mov ax, 0xFF; push ax;
lcall 0xAF8:0x0B25`, i.e. the engine passes 255 as the destination's maximum length. The
loader clears the string buffers with a 256-byte stride (`shl di, 8` then a zero length
byte at `0x6FC8 + 256*k`), consistent with `string[255]` slots. What is not proven: the
routine at `0xAF8:0x0B25` has not been read, so "characters beyond 255 are dropped" rests
on the documented behavior of the Turbo Pascal string-store routine rather than on this
build's bytes. Input bounds cannot settle it either way: the packed length is one byte, so
a frame can hold up to 340 codes; the shipped scripts' longest frame is 154 packed bytes
(at most 205 characters), so the cap is never reached in the owner's data. The cap stays
provisional until the runtime routine is identified or a synthetic script is run on the
DOS oracle.

### 1.5 Two further full leaf translations (merged PR #15)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 4 | `pr_rec_ecl_operand_word` = GAME.OVR 0x07B82, unit 0x25 + 0x06AF, public entry 9 (called as `0x2B:0x4D`, 27 fixup-confirmed sites in unit 0x1c, plus a near call from the operand loader) | 36 bytes, 16 instructions | far Pascal: two byte arguments, first at `[bp+8]`, second at `[bp+6]`, result in `AX`, `retf 4`; no globals, no calls | **full translation** | **verified** |
| 5 | `pr_rec_ecl_address_class` = GAME.OVR 0x07BCE, unit 0x25 + 0x06FB (internal, 4 near callers in unit 0x25) | 91 bytes, 28 instructions | far Pascal: one word argument at `[bp+6]`, result in `AL`, `retf 2`; no globals, no calls | **full translation** | **verified**; the *meaning* of ranges 0 to 2 is **provisional** (not asserted by the code) |

**Function 4.** Paths: exactly one (straight-line, 16 instructions, no branches). Side
effects: none; it writes only its own stack frame and returns in `AX`. Zero-extends the
second argument, shifts it left by 8, and adds the zero-extended first argument. The loader calls it with the two value bytes of an operand
slot (`[0x7085+i]` first, `[0x7045+i]` second), so the result is the little-endian operand
word. Every path is covered; the test checks all 65,536 input pairs and the little-endian
property.

**Function 5.** Paths: four independent two-compare tests (eight conditional branches)
plus the default; every one is represented by the four `if` statements and the initial
value. Side effects: none; it writes only its own stack frame and returns in `AL`. Starts a
local result at 4, then tests the argument against four inclusive ranges in order, each
rewriting the result when it matches: `0x4900..0x4CFF` to 0,
`0x6B00..0x6EFF` to 1, `0x9700..0x98FF` to 2, `0x9900..0xB6FF` to 3. The ranges do not
overlap, so order does not matter. The test checks every boundary and all 65,536 inputs
against an independent range table. Data evidence on the owner's scripts: range 3 is
exactly 7,680 bytes and the largest shipped block unpacks to 7,679 bytes; of 2,900 in-block
jump and call targets reached by the walk, all are in range 3; of 8,638 tag-01 word
operands, 1,882 lie in range 0, 2,551 in range 1, 510 in range 2, 3,078 in range 3 and 617
in none. What this shows: the engine treats `0x9900..0xB6FF` as the script window (which
fixes the maximum script block at 7,680 bytes, a tighter bound than the 65,535 the lane B
reader allows) and three further windows as addressable engine data; which engine
structures the first three windows denote is not established here.

Throughput: function 4 took 10 minutes to read, translate and test; function 5 took 20
minutes plus 10 minutes for the data check. Both were found by a leaf scan over all 673
prologue functions (no calls, bounded size, few globals), which listed 71 candidates, 4 of
them in the ECL helper unit.

### 1.6 Operand loader state transition (issue #16, branch `claude/ecl-loader`)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 6 | `pr_rec_ecl_load_operands` = GAME.OVR 0x074D3, unit 0x25 + 0x0000, public entry 2 (called as `0x2B:0x2A` from 42 fixup-confirmed handler sites in unit 0x1c, and near-called by the skip dispatcher, function 2) | 310 bytes, 119 instructions | far Pascal: one word argument at `[bp+6]`, the operand count in its low byte; `retf 2`; no result | **recovered core** with a caller-owned state object | all paths of the loader itself **verified**; the 0x81 string-by-address side effect is **unresolved** for engine-window addresses; the inline-string cap is **provisional** (section 1.4) |

**Globals read and written (engine addresses, all modelled in `pr_rec_ecl_loader_state`):**
`[0x49DE]` script buffer far pointer and `[0x49ED]` PC (read, PC written); tag array
`[0x7005+i]`, first value byte `[0x7085+i]`, second value byte `[0x7045+i]` for i = 1..count
(written); string buffers `0x6FC8 + 256*k` for the k-th string operand of this call
(cleared and filled). The PC is a **virtual address**: every fetch is `es:[buffer + PC +
0x6700]`, so virtual `0x9900` is buffer offset 0 and the buffer pointer addresses unpacked
block offset 2. GOTO writes an operand word straight into the PC (contracts section 5).

**Paths (all represented):**

| path | engine effect | translation |
| --- | --- | --- |
| `count < 1` | skip the loop; `PC += 1` | same |
| per operand, entry | `tag[i] = [PC+1]`, `low[i] = [PC+2]`, `PC += 2` | same, with a bounds check |
| tag 1, 2 or 3 | `PC += 1`, `high[i] = [PC]` | same |
| tag 0x80, length > 0 | `k = ++strings`; function 3 consumes `length` bytes (PC advances before each read), clearing then filling buffer k | same via `pr_rec_ecl_unpack_text`; `PC += length` |
| tag 0x80, length 0 | `k = ++strings`; buffer k length byte = 0 | same |
| tag 0x81 | `k = ++strings`; `PC += 1`; `high[i] = [PC]`; word = function 4(low, high); call unit 0x25 + 0x12FB (word, k) | word computed; the callee is translated for script-window addresses only (below) |
| any other tag (including 0x00) | nothing further | same |
| loop exit | when `i == count`; then `PC += 1` | same |

**Callee unit 0x25 + 0x12FB (GAME.OVR 0x087CE, 594 bytes, 229 instructions), string by
address.** Arguments: address word, string index. It clears buffer k, classifies the address
with function 5, then by class copies characters until a zero element, appending each with
the runtime concat/store sequence (maximum length 0xFF passed to the store): class 0 reads
words from the structure at far pointer `[0x49D2]` (index `address - 0x4900`, low byte
appended); class 1 with address exactly `0x6B00` assigns the string at far pointer
`[0x5D92]`, otherwise reads words from `[0x49D6]` (index `address - 0x6B00`); class 2
reads words from `[0x49DA]` (index `address - 0x9700`); class 3 reads **bytes** from the
script buffer at `address - 0x9900` (a plain zero-terminated byte string inside the block,
not six-bit text); class 4 leaves the buffer empty. Translated: classes 3 and 4. Recorded
as unresolved with the address kept in `string_address[k]`: classes 0 to 2, because the
three engine structures and the `[0x5D92]` string are not modelled.

**State reuse:** a string slot's length, recorded address and unresolved flag are cleared
whenever the slot is allocated by a call, so a state object reused across instructions never
reports a previous instruction's string metadata (regression test added after PR #20
review).

**Additions over the engine:** bounds checks on every script read (`PR_REC_ECL_LOADER_BOUNDS`,
state partially updated as the engine would have been), a slot limit of 15
(`PR_REC_ECL_LOADER_COUNT`, state unchanged; the engine's arrays are 64 bytes apart and it
is never called with more than 14), and the provisional 255-character cap of function 3.

**Checks.** Synthetic tests: all tag paths with slot positions and PC after each operand;
count 0; an unknown tag; empty inline string; string by address for classes 3, 2, 0 and 4;
truncated word, truncated inline string, class-3 string without terminator, operand bytes
below the base; count limit; and that per-operand advances agree with the operand span sizes
documented for lane B's reader (2, 3, 3, 3, 3, 2 + LL), checked against a hard-coded
expected total rather than by calling that API. Call-site evidence: the 42 handler sites push an immediate count at
39 sites (`mov al, n; push ax`), and a run-time value at 3 (0x1D, 0x2B second call, 0x39).
Data checks on the owner's scripts (counts only, limited to the instructions reached by the
walker's linear decode, about 61% of script bytes): all 195 tag-0x81 operands reached
address classes 1 (6) and 2 (189), none the script window, so the unresolved branch is the
one every reached operand uses; among reached instructions no IF-family opcode is
immediately followed by 0x0C, 0x34 or 0x36, the three opcodes where the skip dispatcher's
prefetch count differs from what the handler itself loads (3, 1 and 2 at their loader call
sites), so that disagreement is latent in the decoded portion. Neither statement is a
corpus-wide proof.

**Verdict:** recovered core, not a full translation. Remaining dependencies: the three
engine structures behind the 0x81 windows and the `[0x5D92]` string; the runtime string
routines at `0xAF8:0x0B0B`, `0x0B98`, `0x0B25`, `0x0C3B` (cap); and dynamic confirmation on
the DOS oracle.

Throughput: about 70 minutes for the loader and its callee (reading 904 bytes of code,
translation, 5 tests, 2 data checks).

### 1.7 Engine string windows behind tag 0x81 (issue #22, branch `claude/ecl-string-windows`)

**Fields.** The three windows are heap arrays of 16-bit words, each reached through a far
pointer global; all three are allocated, zero-filled and never re-pointed by one function.

| window | script addresses | far pointer global | allocation | words | readers (`les` sites / functions) | writers of the pointer |
| --- | --- | --- | --- | ---: | --- | --- |
| 0 | 0x4900..0x4CFF | `[0x49D2]` (segment `[0x49D4]`) | `GetMem(0x800)` | 1024 | 86 / 32 | unit 0x36 + 0x002D only |
| 1 | 0x6B00..0x6EFF | `[0x49D6]` (`[0x49D8]`) | `GetMem(0x800)` | 1024 | 143 / 47 | same |
| 2 | 0x9700..0x98FF | `[0x49DA]` (`[0x49DC]`) | `GetMem(0x400)` | 512 | 9 / 7 | same |
| 3 (script) | 0x9900..0xB6FF | `[0x49DE]` (`[0x49E0]`) | `GetMem(0x1E00)` | 7,680 bytes | — | same |

Initializer: unit 0x36 + 0x002D (GAME.OVR 0x0E600; first 1,079 bytes read). It calls the
runtime allocator `0xAF8:0x023F` (size pushed, far pointer returned in DX:AX) for the four
buffers above and three others, then the runtime fill `0xAF8:0x16A8` (pointer, count, value)
with value 0 over all four, sets the PC `[0x49ED]` to 0x9900, and clears `[0x5D92]`. **Verified**
from the bytes; the allocator and fill identities rest on their argument shapes and effects
(provisional naming of runtime routines, as before).

Element arithmetic, identical in all four accessors below: `shl ax,1; les di,[base]; add
di,ax` and an addressing bias of `+0x6E00`, `+0x2A00`, `-0x2E00` for windows 0, 1, 2, which is
`-2 * window_base` modulo 65,536, so the element is `window[address - base]`. **Verified.**
String format in a window: one character per word (the store zero-extends the byte, the
read uses the low byte), terminated by a zero word. **Verified.**

**Calls.** All in unit 0x25; far Pascal convention; arguments listed first-pushed first.

| function | identity | size | arguments / result | translated as | status |
| --- | --- | ---: | --- | --- | --- |
| string by address | +0x12FB, GAME.OVR 0x087CE | 594 bytes, 229 insns | (address word, string index byte), `retf 4` | `pr_rec_ecl_string_by_address` | **all paths translated**: classes 0-2 read words to a zero word; class 3 reads script bytes to a zero byte; class 4 leaves the slot empty; address 0x6B00 copies the active character's name (a `string[15]` at `[0x5D92]`, modelled as a state field; unresolved only when that field is not valid). Runtime concat modelled with the provisional 255 cap. |
| store string | +0x0F53, GAME.OVR 0x08426, entry 12 (`0x2B:0x5C`; used by the INPUT STRING handler) | 512 bytes, 181 insns | (far pointer to a Pascal string, address word), `retf 6` | `pr_rec_ecl_store_string` | **all paths translated**: copies into a local `string[255]`; classes 0-2 write one word per character then a zero word; class 3 writes bytes then a zero byte (needs `script_rw`, else reported unresolved); 0x6B00 assigns the active name with maximum 15; class 4 stores nothing. |
| load word | +0x0E0C, GAME.OVR 0x082DF | 327 bytes, 125 insns | (address word), result AX, `retf 2` | `pr_rec_ecl_load_word` | **core**: classes 0-3 translated (class 3 returns the zero-extended byte); class 1 first calls +0x07BC (not read) which may override the window value, reported as HOOK; class 4 pseudo-addresses (0xB1, 0xFB, 0xFC, 0x33D, 0x35F and 0xC04B+0..4, 0xE, mapping to `[0x6F6E]`, `[0x6F6A]`, `[0x6F6C]`, `[0x6AAF]`, `[0x6AAD]`, `[0x6AAE]`, `[0x6AB0]`, `[0x6AB1]`) not modelled, reported unresolved. |
| store word | +0x0C77, GAME.OVR 0x0814A, entry 10 (`0x2B:0x52`, 35 sites) | 405 bytes, 141 insns | (address word, value word), `retf 4` | `pr_rec_ecl_store_word` | **core**: classes 0-3 translated (class 3 stores the low byte into the script); class 0 addresses 0x49FD/0x49FE also set `[0x84E8]` (not modelled); class 1 then runs the hook +0x0A74 (read: mirrors 0x6B20..0x6B70 into character-record bytes at `[0x5D92]+0x18..`, 0x6BB8 and 0x6BBB..0x6BC3, 0x6BF7, 0x6BF9, 0x6C00, 0x6C0C into further record fields and flags, 0x6E12 to `0x802:0x094A`, 0x6E22/24/26 to `0x3D5:0x003E`), reported HOOK after the store; class 4 pseudo-addresses (0x3DE, 0xB8, 0xB9, 0xBF68+0xE3/E4/E5/F1/F7 into `[0x84CC]`, `[0x84CE]`, `[0x84D0]`, `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]`, `[0x6F68]`, `[0x6F69]` with flags) not modelled, reported unresolved. |

**State model additions.** `pr_rec_ecl_loader_state` now carries the three windows,
the active name fields and an optional writable script pointer; `pr_rec_ecl_windows_clear`
mirrors the initializer's zero fill and its nulling of `[0x5D92]` (the active name becomes
invalid and empty). The loader's 0x81 path now reads through
`pr_rec_ecl_string_by_address`, so an engine-window string resolves whenever the window
holds it; only the active-name path can remain unresolved.

**Checks.** Tests: zero fill; store then read round trips in windows 0, 1 (last three words)
and 2; low-byte-only character extraction; class 3 read and write through `script_rw`; class
4; word load and store with the HOOK and UNRESOLVED statuses; a string running off the end
of window 2 (bounds, where the engine would overrun its heap block); unterminated window
and script strings; the 0x6B00 name path before and after a name is set, including the
15-character limit; argument checks. Data audit on the owner's scripts (instructions reached
by the walker's linear decode only, counts only): tag-0x81 operands occur in SAVE (113),
PRINT (47), COMPARE (14), ENCOUNTER MENU (9), INPUT STRING (6) and PRINTCLEAR (6); they name
17 distinct addresses, led by 0x982C (108), 0x9890 (44), 0x985E (11), 0x9800 (7) and 0x6B00
(6); no overlay unit and no resident code references 0x982C, 0x9890, 0x985E or 0x9800 as an
immediate, so the scripts themselves populate window 2 strings through SAVE and INPUT STRING.

**Unresolved semantics, stated plainly:** the class-1 load hook +0x07BC was not read; the
class-4 pseudo-address tables of load and store are listed but not modelled; the SAVE handler
(0x09) was not read, so the exact path by which a 0x80 or 0x81 source reaches `+0x0F53` is
inferred from the operand usage, not traced; the runtime routines remain identified by
argument shape only (allocator, fill, string load/concat/store). A targeted DOS experiment
that would settle the cap and the SAVE path: run a script that SAVEs a 200-character inline
string to 0x982C and PRINTs 0x982C.

Throughput: about 60 minutes for the four functions and the initializer (2,900 bytes of
code read, two translations, two cores, 2 tests, 3 data checks).

### 1.8 Short ECL handlers: GOTO, GOSUB, PRINT RETURN and the call-frame push (issue #34)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 7 | `pr_rec_ecl_push_call_frame` = GAME.OVR 0x08E24, unit 0x25 + 0x1951, public entry 17 (called as `0x2B:0x75`, 2 sites) | 138 bytes, 47 instructions | far Pascal: one word argument (slot index in its low byte), `retf 2` | **path-complete callee model with a stated heap limit** (the 6-byte heap node allocation through `0xAF8:0x023F` is replaced by a bounded 64-entry array with status `STACK_FULL`) | control flow and effects **verified**; not counted as a full translation |
| 8 | `pr_rec_ecl_op_goto` = GAME.OVR 0x00E3A, unit 0x1c + 0x00F4, opcode 0x01 | 29 bytes, 13 instructions | far Pascal, no arguments, `retf` | **path-complete handler model with a stated dependency** (composes with the loader core, function 6) | handler body **verified** instruction for instruction; not counted as a full translation |
| 9 | `pr_rec_ecl_op_gosub` = GAME.OVR 0x00E57, unit 0x1c + 0x0111, opcode 0x02 | 21 bytes, 10 instructions | far Pascal, no arguments, `retf` | **path-complete handler model with stated dependencies** (functions 6 and 7) | handler body **verified** instruction for instruction; not counted as a full translation |
| 10 | `pr_rec_ecl_op_print_return` = GAME.OVR 0x03831, unit 0x1c + 0x2AEB, opcode 0x33 | 41 bytes, 13 instructions | far Pascal, no arguments, `retf`; no calls | **full translation** | **verified**, no dependencies |

**Paths and side effects.**

- Function 7: two branches, list empty or not; both allocate a 6-byte node, make it the head
  `[0x6F70]`, link it to the previous head (null when empty) and store the current PC in word
  0; then PC := word(slot low `[0x7085+slot]`, slot high `[0x7045+slot]`) through function 4.
  Modelled: `call_stack[depth++] = pc; pc = word`. Written: `[0x6F70]` (head), the node,
  `[0x49ED]`; temporaries `[0x6F74..0x6F77]` hold the previous head and are not modelled
  separately.
- Function 8: `loader(1)`, then PC := word(`[0x7086]`, `[0x7046]`), i.e. slot 1. One path.
  Side effects are the loader's (slot 1, PC) and the final PC write.
- Function 9: `loader(1)`, then function 7 with slot 1. One path.
- Function 10: `PC += 1`; if `[0x84DF] != 0`: `[0x5E24] = 1`, `[0x5E25] += 1`, `[0x84DF] = 0`;
  else `[0x5E24] = 1`, `[0x5E25] += 1`. Two paths, both represented; the three bytes are
  modelled by address (`b84df`, `b5e24`, `b5e25`) because their meaning is not asserted.

**Checks.** Tests transcribed independently from the handler bytes: GOTO sets the PC from a
word operand and consumes exactly one operand; GOTO with a byte operand yields
`word(low, 0)`; truncated operands propagate the loader's bounds status; GOSUB saves the PC
that follows its operand (the loader's final increment) and jumps; nested GOSUB orders the
saved PCs; the push alone uses the given slot and the current PC; argument and model-bound
statuses; `windows_clear` empties the list as the initializer does; PRINT RETURN on both
flag values, byte wraparound of `[0x5E25]`, and no script access.

**Count, stated strictly.** Function 10 (PRINT RETURN) is a full translation with no
dependencies. Strict full translations in this ledger's numbering: 10 (functions 1, 4, 5, 10, 11, 12, 13,
14, 15 and 16). Overlay-only reviewed full C count on main, which also includes the sixteen
unit 0x38 entries of PRs #63, #69 and #70 and later reviewed batches (documented outside
this numbering): 32 on main d3cde1f (PM's reviewed record); the issue #102 PR adds
two models (functions 26 and 27) and two every-instruction translations of runtime-calling
helpers (functions 24 and 25, credit pending the PM's final review) and claims no count
change here. Project-wide reviewed strict count (the PM's record,
overlay 32 plus resident 9): 41 on main d3cde1f, not changed by this ledger.
Functions 7, 8, 9, 17, 18, 19, 20, 21, 22, 23, 26 and 27 are path-complete models: their
own instruction paths are translated and verified, but 8, 9, 17, 21, 22, 23, 26 and 27 compose with the operand loader core (function 6, whose tag-0x81
unset-active-name path and runtime string cap remain unresolved) and 7 replaces the heap
node allocation with a bounded array. They are not counted as full translations.

### 1.9 Character-effect dispatch entry 8 (issue #39)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 11 | `pr_rec_u38_entry8` = GAME.OVR 0x0EFD2, unit 0x38 + 0x0377 (bytes +0x0377..+0x03AB), public entry 8, run-time stub `0x41:0x48` | 53 bytes, 15 instructions | far Pascal: five word arguments pushed by the dispatcher and discarded (`retf 0xA`); no result | **full translation** | **verified**: all three branches and both side effects; no calls, no dependencies |

**Provenance.** `GAME.OVR` sha256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
unit 0x38 at file offset 60,507; routine at unit offset 0x0377, followed by the sibling at
0x03AC. Classification row: grade A (public entry), class `game_evidenced` before this
translation.

**Instructions and branches.** Prologue; load the active-character far pointer `[0x5D92]`
three times and compare the byte at record offset 0xA0 with 2, 5 and 8 in turn (`je` to the
modify block on equality, `jne` past it after the third compare); modify block: `add byte
[0x6816], 2` then `sub byte [0x6822], 2`; epilogue `pop bp; retf 0xA`. Paths: three ways into
the modify block, one way past it; all represented by the C condition and the two byte
updates (wrapping modulo 256 as the originals do).

**Caller and role evidence.** Unit 0x38 + 0x31A2 (public entry 1, 1,799 bytes) fills a table
of far pointers at `DS:0x682C` onward with unit 0x38 entries in slot order and stores this
entry's pointer (`0x41:0x48`) at `0x6848` and again at `0x68DC`, i.e. items 8 and 45 of the
table whose base `0x6828` the engine dereferences in its `lcall [di + 0x6828]` dispatch; the
sibling entry 9 (`0x41:0x4D`) sits at items 9 and 46. Every item is called with the same five
word arguments. `[0x6816]` has 16 writers and `[0x6822]` 24 writers, all but one in unit
0x38. No direct far call targets entry 8 (far_in 0): it is reached through the table.

**Contract in C.** `pr_rec_modifier_state` holds the two engine bytes; `record` points at a
full 285-byte character record (the verified frame in `docs/formats/DOS_CHARACTER.md`);
`applied` reports whether the modify block ran. A null record or state returns
`PR_REC_EFFECT_ARGUMENT` with no change, where the engine would read through a null far
pointer.

**Tests.** All 256 values of record byte 0xA0 (exactly 2, 5, 8 modify, with +2 and −2
observed); byte wraparound (0xFF + 2, 0x01 − 2); accumulation over repeated calls;
insensitivity to other record bytes; null contracts.

**Uncertainty, stated plainly.** The meaning of record byte 0xA0 is not documented in the
character-record format doc; the value sets {2,5,8} here and {0,3,6} in the sibling match
the evil and good columns of the nine AD&D alignments in standard order, which is a reading,
not a verified field. The meanings of `[0x6816]` and `[0x6822]` (modifier accumulators by
their use pattern) and of dispatch items 8 and 45 are not established. None of this affects
the translation's completeness: the routine's behavior depends only on the byte compared and
the two bytes updated.

Throughput: 20 minutes to select (candidate scan over the classification CSV), 10 to read,
15 to translate and test, 15 to gather caller evidence.

### 1.10 Character-effect dispatch entry 9 (issue #43)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 12 | `pr_rec_u38_entry9` = GAME.OVR 0x0F007, unit 0x38 + 0x03AC (bytes +0x03AC..+0x03E0), public entry 9, run-time stub `0x41:0x4D` | 53 bytes, 15 instructions | far Pascal: five word arguments discarded (`retf 0xA`); no result | **full translation** | **verified** independently of the sibling: all branches and both side effects; no calls, no dependencies |

**Independent verification** (hash-checked overlay, Capstone decode from the entry offset,
not copied from section 1.9): the unit's entry table maps slot 9 to +0x03AC; the routine
spans 53 bytes and 15 instructions; it loads `[0x5D92]` three times and compares the byte at
record offset 0xA0 with 0, 3 and 6 (`je` to the modify block twice, `jne` past it after the
third); the modify block is `add byte [0x6816], 2; sub byte [0x6822], 2`; epilogue `pop bp;
retf 0xA`. A byte-level comparison with entry 8 shows exactly three differing bytes, the
three compared immediates (indices 12, 24, 36). Unit 0x38 + 0x31A2 stores this entry's
pointer (`0x41:0x4D`) at `0x684C` and `0x68E0`, items 9 and 46 of the dispatch table based at
`0x6828`. Direct callers: none (far_in 0), consistent with table dispatch.

**Contract and tests.** Same contract as function 11. Tests transcribed from the bytes: all
256 values of record byte 0xA0 (exactly 0, 3, 6 modify); the two siblings never both fire for
the same value; wraparound (0xFE + 2, 0x00 − 2); accumulation; insensitivity to other record
bytes; null contracts with no change.

**Uncertainty.** As for function 11: record byte 0xA0 is undocumented in the character-record
format doc (the {0,3,6} / {2,5,8} sets match the good and evil columns of the nine AD&D
alignments, a reading only); the two accumulator bytes and dispatch items 9 and 46 have no
established meaning. None of this affects completeness.

Throughput: 10 minutes to re-verify, 10 to translate and test, 10 to document.

### 1.11 Character-effect dispatch entry 10 (issue #46)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 13 | `pr_rec_u38_entry10` = GAME.OVR 0x0F03C, unit 0x38 + 0x03E1 (bytes +0x03E1..+0x03FD), public entry 10, run-time stub `0x41:0x52` | 31 bytes, 13 instructions | far Pascal: five word arguments discarded (`retf 0xA`); no result | **full translation** | **verified**: both paths and all three byte effects; no calls, no record access, no dependencies |

**Instructions and branches** (hash-checked overlay, decoded from the entry offset):
prologue; `mov al,[0x6819]; and al,2; or al,al; je skip`; effect block `mov al,[0x6818]; xor
ah,ah; shr ax,1; mov [0x6818],al` then `add byte [0x6816],3`; `skip: pop bp; retf 0xA`. Two
paths. The shift acts on the zero-extended byte, so the stored byte is the old value divided
by two with no carry-in; the add wraps modulo 256.

**Caller and role evidence.** Unit 0x38 + 0x31A2 stores this entry's pointer (`0x41:0x52`)
at `0x6850`, item 10 of the far-pointer dispatch table based at `0x6828` (`lcall [di +
0x6828]`); unlike entries 8 and 9 it appears in the table once. No direct far callers.
`[0x6818]` is also halved by unit 0x38 + 0x2AE6 and cleared by + 0x264B under
character-record conditions (section 2 candidates), and `[0x6816]` is the accumulator the
sibling entries add to.

**Contract and tests.** The two new bytes `b6818` and `b6819` join `pr_rec_modifier_state`;
entries 8 and 9 do not touch them (tested). Tests: all 256 values of `[0x6819]` (only bit 1
selects the effect; other bytes untouched); all 256 values of `[0x6818]` halve correctly;
wraparound of the +3; repeated halving to zero; null state rejected.

**Uncertainty.** The meanings of `[0x6819]` bit 1, `[0x6818]` and `[0x6816]` are not
established (by use pattern, a flag byte, a quantity that effects halve, and an
accumulator). Dispatch item 10's role is not named. Completeness is unaffected.

Throughput: 10 minutes to decode and confirm the role, 15 to translate, test and document.

### 1.12 Character-effect dispatch entry 31 (issue #49)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 14 | `pr_rec_u38_entry31` = GAME.OVR 0x0F870, unit 0x38 + 0x0C15 (bytes +0x0C15..+0x0C25), public entry 31, run-time stub `0x41:0xBB` | 17 bytes, 6 instructions | far Pascal: five word arguments discarded (`retf 0xA`); no result | **full translation** | **verified**: straight line, no branches, no calls, no record access |

**Instructions** (hash-checked overlay, decoded from the entry offset): prologue; `sub byte
[0x6822], 4`; `sub byte [0x6816], 4`; `pop bp; retf 0xA`. One path. Both subtractions wrap
modulo 256.

**Caller and role evidence.** Unit 0x38 + 0x31A2 stores this entry's pointer (`0x41:0xBB`)
at `0x68B8`, item 36 of the far-pointer dispatch table based at `0x6828` (`lcall [di +
0x6828]`); no direct far callers. It writes the same two accumulator bytes that entries 8, 9
and 10 modify, so it belongs to the same effect family. Selection: among the untranslated
table entries, the 7-byte ones are empty stubs (prologue and `retf 0xA` only; entry 116 is
installed in nine items as the default), the 14-byte ones call out, and five 15-to-17-byte
entries (30 at +0x0C06, 31 here, 2 at +0x010D, 59 at +0x171F, 36 at +0x10C1) are
straight-line byte updates; 31 was chosen for using both known accumulators.

**Tests.** All 256 starting values of each byte with the other byte complementary (both
subtract 4, the two unrelated state bytes untouched); wraparound through zero and
accumulation; composition with entry 8 (+2 then −4); null state rejected.

**Uncertainty.** As before, the meanings of `[0x6816]`, `[0x6822]` and dispatch item 36 are
not established; completeness is unaffected.

Throughput: 15 minutes including the candidate scan.

### 1.13 Character-effect dispatch entry 30 (issue #51)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 15 | `pr_rec_u38_entry30` = GAME.OVR 0x0F861, unit 0x38 + 0x0C06 (bytes +0x0C06..+0x0C14), public entry 30, run-time stub `0x41:0xB6` | 15 bytes, 6 instructions | far Pascal: five word arguments discarded (`retf 0xA`); no result | **full translation** | **verified**: straight line, no branches, no calls, no record access |

**Instructions** (hash-checked overlay, decoded from the entry offset): prologue; `inc byte
[0x6816]`; `inc byte [0x6822]`; `pop bp; retf 0xA`. One path. Both increments wrap modulo
256 (the flags they set are dead: nothing reads them before the return).

**Caller and role evidence.** Unit 0x38 + 0x31A2 (the table installer) stores this entry's
pointer at +0x335F..+0x336B: `mov ax, 0xB6; mov dx, 0x41` (segment word fixed up) into
`0x68B4`/`0x68B6`, item 35 of the far-pointer dispatch table at `0x6828` (`lcall [di +
0x6828]`), immediately before the store of entry 31 into item 36. The public
classification row for this routine records zero fixup-confirmed far callers, so the
table is the only identified entry route (that does not exclude an indirect caller not yet
found). It updates the same two accumulator bytes as entries 8,
9, 10 and 31 (here +1 each, the inverse direction of entry 31's −4), so it belongs to the
same effect family. Selection: the remaining 15-to-17-byte straight-line entries from the
issue #49 scan are 2 (+0x010D, adds 5 to `[0x6825]` and increments `[0x6822]`), 59
(+0x171F, sets `[0x681E]` to 1 and subtracts 4 from `[0x6822]`) and 36 (+0x10C1, halves
`[0x681A]`); 30 was taken first because it uses only the two already-modelled bytes.

**Tests.** All 256 starting values of each byte with the other byte complementary (both
increment, the two unrelated state bytes untouched); wraparound at 0xFF and accumulation;
composition with entry 31 (+1 then −4 nets −3); null state rejected.

**Uncertainty.** As before, the meanings of `[0x6816]`, `[0x6822]` and dispatch item 35 are
not established; completeness is unaffected. `deja` recall for this entry returned no
prior session.

Throughput: 10 minutes.

### 1.14 Two routines on the first-gameplay path (issue #56)

Context: [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) traces the program body through the party
menu (unit 0x6c + 0x0161) to the gameplay loop (unit 0x1c + 0x32F8) and the first run of
the initial-entry script. `deja` recall for this path returned no prior session.

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 16 | `pr_rec_u3bd_entry1` = GAME.OVR 0x2EA4C, unit 0x3bd + 0x0155 (bytes +0x0155..+0x0173), public entry 1, run-time stub `0x3D5:0x25` | 31 bytes, 12 instructions | far Pascal: four byte arguments at `[bp+0xC]`, `[bp+0xA]`, `[bp+8]`, `[bp+6]` (first pushed to last), `retf 8`; no result | **full translation** | **verified**: straight line, no branches, no calls, no reads of globals |
| 17 | `pr_rec_ecl_script_init` = GAME.OVR 0x07689, unit 0x25 + 0x01B6 (bytes +0x01B6..+0x0321), public entry 3, run-time stub `0x2B:0x2F` | 364 bytes, 135 instructions | far Pascal, no arguments, `retf` | **path-complete model with stated dependencies** (functions 6 and 4; two runtime fills replaced by direct zeroing) | every instruction path **verified**; not counted as a full translation |

**Function 16.** Instructions (hash-checked overlay, decoded from the entry offset):
prologue; `mov al,[bp+0xC]; mov [0x6A60],al`; `mov al,[bp+0xA]; mov [0x6A61],al`; `mov
al,[bp+8]; mov [0x6A62],al`; `mov al,[bp+6]; mov [0x6A63],al`; `pop bp; retf 8`. One path.
Callers: the program body at loaded image 0x012C..0x0138 (`mov al,imm; push ax` four times
with 0x0B, 0, 7, 6, then the far call through stub `0x3D5:0x25`) right before the gameplay
loop, and the fixup-confirmed site at unit 0x3b4 + 0x0207 inside the view refresh
(`+0x016D`), which pushes the four bytes back after recomputing them. The classification
row for `0x3bd:0x0155` records one fixup-confirmed far caller (that site); the resident
caller is outside the overlay call graph. Since the callee reads only the low byte of each
pushed word, the C function takes four `uint8_t`. Uncertainty: the meanings of the four
bytes (the view refresh derives the first from a window-0-indexed table at `[di + 0xC26]`
and the others from three near helpers) are not asserted; completeness is unaffected.

**Function 17, paths and side effects** (offsets inside unit 0x25):

- `+0x01BC..+0x01DA`: `[0x84E0] = 0`, `[0x84E2] = 0`, `[0x84D2] = 0`, `[0x84D3] = 0`,
  `[0x6F68] = 0x41`, `[0x6F69] = 9`, `[0x84E5] = 1`.
- `+0x01DF..+0x01EB`: runtime `0xAF8:0x16A8` with (`DS:0x84C8`, 2, 0); `+0x01F0..+0x01FB`:
  `[0x6F83] = 8`, `[0x49ED] = 0x9900`, `[0x70C6] = 0`; `+0x0200..+0x020D`: the four words
  `[0x6F70..0x6F76]` zeroed (call-stack head nil and the two temporaries of function 7);
  `+0x0210..+0x021C`: runtime `0xAF8:0x16A8` with (`DS:0x6F78`, 6, 0), the compare flags.
- `+0x0221..+0x0248`: window 1 words `0x2E1 = 0xFF`, `0x2D2 = 0`, `0x2D3 = 0`; window 0 word
  `0xE5 = 0` (script addresses 0x6DE1, 0x6DD2, 0x6DD3, 0x49E5).
- `+0x024D..+0x02B8`, five times: `mov al,1; push ax; call +0x0000` (the operand loader,
  function 6, count 1), then `push [0x7086]; push [0x7046]; call +0x06AF` (function 4, slot
  1) and the word stored to `[0x49E3]`, `[0x49E5]`, `[0x49E7]`, `[0x49E9]`, `[0x49EB]` in
  that order. With the PC at 0x9900 this consumes the five 4-byte block-head entries
  (opcode, tag 0x01, word), leaving the PC at 0x9914.
- `+0x02BB`: window 0 word `0xE6 = 1` (address 0x49E6).
- `+0x02C6`: if `[0x49F8] == 0`: `+0x02CD..+0x02F0` zero window 0 words 0x100..0x11F (the
  compiler emits `(i + 0x49FF) * 2 + 0x6E00` with 16-bit wrap for i = 1..32: script
  addresses 0x4A00..0x4A1F) and `+0x02F2..+0x0315` zero window 1 words 0x379..0x382
  (`(i + 0x6E79) * 2 + 0x2A00`, i = 0..9: addresses 0x6E79..0x6E82); else `+0x0319`:
  `[0x49F8] = 0`. Both paths reach the epilogue at `+0x031E`.
- The bytes at `+0x0322..+0x0340` after the `retf` are the string constants of entry 4
  (section 4 of GAMEPLAY_PATH.md), not code.

Modelled: the runtime fills as direct zeroing (FillChar's argument shape, inferred); the
loader and word assembly through the existing C functions, so bounds statuses propagate
and the loader's stated limits apply; the heap list head as `call_depth = 0`. Written
engine state is held by address in the loader state (`entry_addr[5]`, `restored`,
`cmp_flags[6]`, `w6f74`, `w6f76` and eleven bytes). Nothing is read except the script.

**Tests.** Function 16: the program body's constant call, all 256 values through each
argument with distinct patterns, the identity round trip the view refresh performs, null
state. Function 17: a synthetic five-entry block head (new game: the five words, the PC at
0x9914, every fixed write, the two cleared ranges and nothing else in the three windows;
restored game: ranges kept, flag consumed), a truncated head (bounds status after three
entries, the later write absent), null state.

Throughput: 70 minutes for the trace, 25 for the two translations.

### 1.15 ECL block load (issue #68)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 18 | `pr_rec_ecl_load_block` = GAME.OVR 0x07814, unit 0x25 + 0x0341 (bytes +0x0341..+0x0410), public entry 4, run-time stub `0x2B:0x34` | 208 bytes, 89 instructions | far Pascal: one byte argument (block id) at `[bp+6]`, `retf 2`; no result | **path-complete model with explicit boundaries** (resident DAX loader `0x802:0x0716`, FreeMem, row clear and wait message as callbacks; fill and Move inlined) | every instruction path **verified**; not counted as a full translation |

Evidence, paths, the resident boundary and the measured limits are in
[ECL_BLOCK_LOAD.md](ECL_BLOCK_LOAD.md). Side effects: the script buffer (fill, then
`size - 2` bytes from block offset 2), `[0x557C] = 0`, the two display calls, the block
release. Model-only statuses: `RETRY_LIMIT` (the engine loops without bound while
`size < 2`) and `OVERFLOW` (the engine copies past the buffer without a check; all 29
shipped blocks fit). Tests are listed in the note's section 6. Throughput: 45 minutes
including the resident decode.

### 1.16 ECL run loop and opcode dispatcher (issue #75)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 19 | `pr_rec_ecl_run` = GAME.OVR 0x03EB7, unit 0x1c + 0x3171 (bytes +0x3171..+0x31B2; the byte 0x00 at +0x31B3 before the next prologue at +0x31B4 is inter-function padding, not body), internal (near-called from +0x32F8 at +0x3367, +0x3410, +0x3458, +0x34E7, from +0x2DD3 twice and from +0x31B4 three times) | 66 bytes, 22 instructions | far Pascal: one word argument (entry address) at `[bp+6]`, `retf 2` | **path-complete model with a callback boundary** (the dispatcher's handler bodies) and a step bound | every instruction **verified**; not counted as a full translation |
| 20 | `pr_rec_ecl_dispatch` = GAME.OVR 0x03C73, unit 0x1c + 0x2F2D (bytes +0x2F2D..+0x3170), internal (near-called from +0x3171 only) | 580 bytes, 273 instructions | far Pascal, no arguments, `retf`; reads `[0x6F7F]` | **path-complete model**: the 61-case compare chain as a 62-slot table (`pr_rec_ecl_handler_offset`) with handler callbacks | every case **verified** against the bytes and against ECL_CONTRACTS.md section 3; not counted as a full translation |

**Run function, paths.** `+0x3174..+0x317A`: `[0x49ED] = [bp+6]`, `[0x442E] = 0`. Loop head
`+0x317F`: exit when `[0x442E] != 0` or `[0x49FF] != 0`. Body `+0x318D..+0x31A5`: `[0x6F7E] =
[0x6F7F]`; `al = es:[di + 0x6700]` with `es:di = [0x49DE] + [0x49ED]` (so virtual address
`0x9900` is buffer offset 0, as ECL_CONTRACTS.md section 2 states); `[0x6F7F] = al`; near
call `+0x2F2D`; `jmp +0x317F`. Exit `+0x31AA`: `[0x442E] = 0`; `retf 2` at `+0x31B0..+0x31B2`, the last body bytes. Two paths out
(either stop byte); the model adds `FETCH_BOUNDS` (PC outside the buffer, where the engine
reads whatever follows) and `STEP_LIMIT`, and ends on any non-OK dispatcher result.

**Dispatcher, paths.** `+0x2F30`: `al = [0x6F7F]`; then 61 `cmp al, imm` cases in ascending
order with `je` ladders for the shared handlers; every match is `push cs; call handler; jmp
+0x316F`; the epilogue at `+0x316F` is `pop bp; retf`. The 49 distinct handler offsets are
exactly those of ECL_CONTRACTS.md section 3 (checked value by value from the bytes).
**Opcode 0x1F and every value above 0x3D take no case**: the chain falls through to the
epilogue having done nothing, the PC is not advanced, and the run loop would fetch the same
byte again, so the engine spins on an unknown opcode until a stop byte is set elsewhere.
The model reports `NO_CASE` at that point. Handler slots that are NULL report
`UNMODELLED`; the three handlers already in C (GOTO, GOSUB, PRINT RETURN) bind through
adapters.

**Tests.** Offset map: the gap, every other slot non-zero, twelve spot values, the six
shared groups, 49 distinct offsets. Dispatcher: all 256 latch values (62 slots hit once
each, `NO_CASE` for 0x1F and 0x3E..0xFF with no handler call and no PC change), unmodelled
slot, failing handler, arguments. Run loop: PC set and stale stop byte cleared on entry,
fetch bias, latch and previous-opcode bookkeeping across four opcodes, the stale latch as
first previous opcode, stop by either byte with `[0x442E]` cleared on exit and `[0x49FF]`
left set, a pre-set stop byte (no dispatch), step bound, fetch bounds, 0x1F in the stream,
unmodelled and failing handlers, arguments. Composition: block load, initializer, then the
initial entry and the first head entry through the real GOTO handler and an EXIT stub.

Throughput: 40 minutes.

### 1.17 Slot value and the SAVE handler on the new-game route (issue #84)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 21 | `pr_rec_ecl_slot_value` = GAME.OVR 0x07609, unit 0x25 + 0x0136 (bytes +0x0136..+0x01B5), public entry 1, run-time stub `0x2B:0x25` (52 fixup-confirmed sites in unit 0x1c) | 128 bytes, 57 instructions | far Pascal: one word argument (slot index, low byte) at `[bp+6]`, result in `AX`, `retf 2` | **path-complete model** (composes with the load-word core, function "load word", and function 4) | all four paths **verified**; not counted as a full translation |
| 22 | `pr_rec_ecl_op_save` = GAME.OVR 0x00FD8, unit 0x1c + 0x0292 (bytes +0x0292..+0x02DF), opcode 0x09, called only from the dispatcher case at +0x2F86 | 78 bytes, 30 instructions | far Pascal, no arguments, `retf` | **path-complete model** (loader core, function 21, store-word core, store-string translation) | both paths **verified**; not counted as a full translation |

**Function 21, paths** (offsets inside unit 0x25): `+0x013C..+0x015C` read `[0x7085+slot]`,
`[0x7045+slot]`, `[0x7005+slot]`; `+0x0160..+0x016C` tag 0: result = low byte; `+0x016E..
+0x0194` tags 1, 3, 0x80: result = near `+0x0E0C` (load word) at `(high << 8) + low`
(assembled inline, the same arithmetic as function 4); `+0x0196..+0x01AA` tags 2, 0x81:
result = function 4 (`+0x06AF`); any other tag reaches `+0x01AD` and returns the local
`[bp-2]`, which no path wrote (modelled as UNRESOLVED, value 0). No globals written.

**Function 22, paths** (offsets inside unit 0x1c): `+0x0298..+0x029B` loader with count 2;
`+0x02A0..+0x02AD` dest = function 4 of slot 2 (`[0x7087]`, `[0x7047]`); `+0x02B0` branch on
`[0x7006] >= 0x80`; word path `+0x02B7..+0x02CD`: value = function 21 of slot 1, then store
word (`0x2B:0x52`) with (dest, value); string path `+0x02CF..+0x02D7`: store string
(`0x2B:0x5C`) with (string buffer 1 at `DS:0x70C8`, dest); epilogue `+0x02DC`. Side effects
are the loader's (slots 1 and 2, PC, string buffer 1) and the store's. Model deviations:
an UNRESOLVED value (class-4 source) skips the store the engine would make with an
unmodelled value; the HOOK statuses of the cores are passed through (store done, hook not
run).

**Tests.** Slot value: tag 0 ignores the high byte; tag 1 reads a window-0 word; tag 3 reads
a script byte; tag 0x80 reads at the length-byte address; a 0x6B00 read reports HOOK with
the value; tags 2 and 0x81 return the word; a class-4 source is unresolved; an unknown tag
is unresolved with value 0; slot 0, slot 16 and nulls. SAVE: the new-game shape (byte to a
window-0 word, PC advanced by 6, no other word touched), immediate word, variable to
variable across windows, hook on a 0x6B00 source and on a 0x6B00 destination, class-3
destination into a writable script, class-4 destination and class-4 source (nothing
stored), an empty inline string (one zero word), a three-byte packed string (four words of
0x40 then zero), truncated operands, null. Composition: block load, initializer, then a run
from the first head entry through two SAVEs, a GOTO and PRINT RETURN to an EXIT stub, with
the window words checked.

Throughput: 35 minutes including the route trace.

### 1.18 LOAD FILES / LOAD PIECES handler on the initial-entry route (issue #92)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 23 | `pr_rec_ecl_op_load_files` = GAME.OVR 0x019BA, unit 0x1c + 0x0C74 (bytes +0x0C74..+0x0DEE), opcodes 0x21 and 0x37 (dispatcher cases +0x3054 and +0x3058), no near calls, 11 far-call sites | 379 bytes, 129 instructions | far Pascal, no arguments, `retf`; branches on the latch `[0x6F7F]` | **path-complete model with callback boundaries** (unit 0x3bd entries 6 and 7, unit 0x3ce entry 1, resident `0xBA:0x27D9` and `0xBA:0x0767`; loader core and function 21 as dependencies) | every branch **verified**; not counted as a full translation |

**Paths** (offsets inside unit 0x1c). `+0x0C7A..+0x0C82`: loader with count 3; `[0x4431] = 1`.
`+0x0C87..+0x0CA9`: loop i = 1..3, `[bp-4+i]` = low byte of function 21 (slot i). `+0x0CAB`:
`cmp [0x6F7F], 0x21`. Opcode 0x21, `+0x0CB2..+0x0D1D`: `[0x4434] = 1`; if v1 is not 0xFF, not
0x7F and window 0 word 0xE6 (`es:[di+0x1CC]`) is non-zero: window 0 word 0xC5
(`es:[di+0x18A]`) = v1, `0x3D5:0x43` (unit 0x3bd entry 7) with v1, window 1 word 0x2C9
(`es:[di+0x592]`) = 0; if v3 is not 0xFF and word 0xE6 is zero: the 6-character constant at
`cs:0x0C6D` through `0xAF8:0x0B0B`, then `0x3E9:0x25` (unit 0x3ce entry 1) with (constant, 0,
0x28). Opcode 0x37, `+0x0D20..+0x0DBD`: `[0x4433] = 1`; v1 == 0x7F: `0x3D5:0x3E` (unit 0x3bd
entry 6) with (0, 1); else words 0xE7 and 0xE8 (`es:[di+0x1CE]`, `es:[di+0x1D0]`) both
non-zero: entry 6 with (v1, 1) and (v3, 3); else loop i = 1..3: entry 6 with (vi, i) when vi
is not 0xFF, otherwise the words `[0x6AAE + 4i]` and `[0x6AB0 + 4i]` = 0xFFFF. Tail
`+0x0DBF..+0x0DE6`: when `[0x4432]`, `[0x4433]` and `[0x4434]` are all non-zero: `0xBA:0x27D9`,
then `0xBA:0x0767` with `[0x5D92]`, then `[0x4432] = 0`. Epilogue `+0x0DEB`. Writers and
readers of the four flags are all in unit 0x1c: the gameplay loop (`+0x32FE..+0x3312`)
clears them and sets `[0x4432]`; `+0x0BD1` clears `[0x4431]`; `+0x2CCF` and `+0x31B4` test
`[0x4431]`.

**Model deviations.** An UNRESOLVED or BOUNDS slot value returns before the branch-specific
effects (the operand load and `[0x4431] = 1` have already happened; the engine continues
with an unmodelled byte); HOOK from a class-1 source is returned after the effects. Each
callback is required at the point its site is reached: a missing one returns UNRESOLVED
after the writes that precede the site and before the effects that follow it (for the gate,
`[0x4432]` stays set), so a run cannot report a completed route with DOS work omitted; a
branch that reaches no site needs none. The callbacks carry no text (the 6-character
constant is passed by identity only).

**Tests.** 0x21 wallset branch with the window words and flags checked and nothing else in
window 0 touched; 0x21 with word 0xE6 zero (text only); v1 of 0xFF and 0x7F; v3 of 0xFF
with word 0xE6 zero; 0x37 with v1 0x7F; 0x37 with words 0xE7/0xE8 set; the default loop
with a 0xFF slot; the three-flag gate across a 0x21 then a 0x37 and its one-shot clearing;
values through a window-0 variable and a class-1 source (HOOK); a class-4 source (no
branch-specific effect); each reached boundary without its callback (wallset after the
word 0xC5 write and before the word 0x2C9 clear, text, the three pieces paths with the
earlier slot's 0xFFFF words kept, and both resident boundaries with `[0x4432]` kept); the
branches that reach no boundary with every callback absent; truncated operands; null
arguments. Composition in the
initial-entry shape (four SAVEs, 0x21, 0x37, GOTO, PRINT RETURN, EXIT) through block load,
initializer and run loop with `[0x4432]` set.

Throughput: 40 minutes.

### 1.19 COMPARE, its flag helpers and the IF family (issue #102)

| # | identity | size | convention | kind | status |
| --- | --- | ---: | --- | --- | --- |
| 24 | `pr_rec_ecl_compare_words` = GAME.OVR 0x08DBE, unit 0x25 + 0x18EB (bytes +0x18EB..+0x1950), public entry 16, run-time stub `0x2B:0x70` | 102 bytes, 36 instructions | far Pascal: two word arguments, `a` at `[bp+8]` (pushed first), `b` at `[bp+6]`, `retf 4`; writes `[0x6F78..0x6F7D]` | **every instruction translated**; the one runtime call (`0xAF8:0x16A8` with `DS:0x6F78, 6, 0`) was decoded independently by the PM as FillChar (loaded image `0x0C628`: `rep stosb`, `retf 8`) | **verified**; strict-count credit is the PM's final review |
| 25 | `pr_rec_ecl_compare_strings` = GAME.OVR 0x08CE8, unit 0x25 + 0x1815 (bytes +0x1815..+0x18EA), public entry 15, run-time stub `0x2B:0x6B` | 214 bytes, 86 instructions | far Pascal: two far string pointers, `a` at `[bp+0xA]` (pushed first), `b` at `[bp+6]`, `retf 8`; writes the six flags | **every instruction translated**; the runtime calls were decoded independently by the PM (`0xAF8:0x0B25` at loaded `0x0BAA5`: copy `min(length, limit)` bytes, `retf 0xA`; `0xAF8:0x0C10` at `0x0BB90`: compare lengths, `repe cmpsb` unsigned, then length compare; the fill as above) and the C follows that order | **verified**; strict-count credit is the PM's final review |
| 26 | `pr_rec_ecl_op_compare` = GAME.OVR 0x00E6C, unit 0x1c + 0x0126 (bytes +0x0126..+0x01A2), opcode 0x03, dispatcher case +0x2F59 | 125 bytes, 50 instructions | far Pascal, no arguments, `retf`; `0x204` bytes of locals | **path-complete model** (loader core, function 21, functions 24 and 25) | both paths **verified**; not counted as full |
| 27 | `pr_rec_ecl_op_if` = GAME.OVR 0x01898, unit 0x1c + 0x0B52 (bytes +0x0B52..+0x0BC7), opcodes 0x16..0x1B, dispatcher ladder +0x3009..+0x3022 | 118 bytes, 41 instructions | far Pascal, no arguments, `retf` | **path-complete model** (the skip through unit 0x25 entry 22 uses function 2's count table and the loader core) | all seven paths **verified**; not counted as full |

**Function 24, paths.** `+0x18EE..+0x18FA`: fill `[0x6F78]`, 6, 0. Then six compares of `a`
against `b`, each `mov ax,[bp+8]; cmp ax,[bp+6]; jcc skip; mov byte [flag],1`: `jne` for
`[0x6F78]` (equal), `je` for `[0x6F79]` (not equal), `jae` for `[0x6F7A]` (below), `jbe` for
`[0x6F7B]` (above), `ja` for `[0x6F7C]` (below or equal), `jb` for `[0x6F7D]` (above or
equal). The conditions are unsigned. Epilogue `+0x194D`.

**Function 25, paths.** `+0x181C..+0x183F`: the two strings copied into `[bp-0x100]` and
`[bp-0x200]` with limit 255 **before** `+0x1844..+0x1850` clears the flags (so an input that
aliases the flag bytes is compared as it was on entry; the C copies first too, with an
alias test); `+0x1855..+0x18E0`: six calls of `0xAF8:0x0C10` with (`a`, `b`) followed by
the same six conditional stores. The ordering is Turbo Pascal's (unsigned bytes, first
difference decides, a shorter prefix is less), as the PM's independent decode of the
runtime routine confirms.

**Function 26, paths.** `+0x012D..+0x0130`: loader with count 2. `+0x0135..+0x0141`: if
`[0x7006] >= 0x80` or `[0x7007] >= 0x80` (either tag a string): `+0x0143..+0x0177` copies
string buffer 1 (`DS:0x70C8`) and buffer 2 (`DS:0x71C8`) into two 255-byte locals and calls
function 25 with them in that order. Otherwise `+0x017E..+0x019A`: function 21 of slot 1
into `[bp-2]`, of slot 2 into `[bp-4]`, then function 24 with (`[bp-2]`, `[bp-4]`). One
reading worth stating: with a single string operand in the second position, the loader
fills buffer 1 (the k-th string operand goes to buffer k), so the engine compares that
string against whatever buffer 2 holds from an earlier instruction; the model keeps the
same buffers. Reached-boundary rule (as in PR #97): in the word path a slot value that
is not OK (HOOK from the class-1 load hook, UNRESOLVED, BOUNDS) returns at once; after a
slot-1 stop slot 2 is not read, and in every such case the six flags are left as they
were. The engine itself continues through the hook, so a run that stops there is
reported as `RUN_HANDLER`/HOOK rather than completed.

**Function 27, paths.** `+0x0B55`: `PC += 1`. `+0x0B59`: the latch selects the case: 0x16
tests `[0x6F78]`, 0x17 `[0x6F79]`, 0x18 `[0x6F7A]`, 0x19 `[0x6F7B]`, 0x1A `[0x6F7C]`, 0x1B
`[0x6F7D]`; a non-zero flag falls to the epilogue; a zero flag calls `0x2B:0x8E` (unit 0x25
entry 22, `+0x1EA6`), which fetches the opcode at the PC into `[0x6F7F]`, then either calls
the loader with that opcode's prefetch count (function 2's table) or increments the PC
(`+0x1FAB`). A latch outside the family reaches the epilogue after the increment.

**Tests.** Function 24: equal, below, above, the unsigned corner cases, flags always 0 or
1, null. Function 25: equal, prefix, differing byte, empty against non-empty, both empty,
case, high bytes, both inputs limited to 255, an input aliasing the flag bytes in either
position, nulls. Function 26: immediates, a window-0
variable against an immediate word, a class-1 source in slot 1 (HOOK at once, flags
untouched, slot 2 provably not read) and in slot 2 (HOOK before the compare), a class-4
source (flags untouched), two inline strings equal and differing, a lone string in second
position against a stale buffer 2, truncated operands, null. Function 27: for each of the
six opcodes, flag set (PC past the IF only, latch kept) and flag clear (a 6-byte SAVE
skipped, latch holds its opcode); skipping a zero-operand statement; skipping past the
script end; a latch outside the family; null. Composition: COMPARE then IF-not-equal with
a SAVE in its shadow, through the run loop, both outcomes.

Throughput: 45 minutes.

## 2. Examined, not translated

| identity | what was learned | why not translated |
| --- | --- | --- |
| operand loader, unit 0x25 + 0x0000, entry 2 | now function 6 (section 1.6) | translated as a recovered core |
| unit 0x25 entry 20 (+0x1CC3) and entry 21 (+0x1DF0) | Entry 20 is arithmetic over the runtime's software-float routines (`0xAF8:0x119A`, `0x1186`, `0x119E`); entry 21 walks a linked list of party records (head at struct +0xC8, link at +0x2A, field +0x37 compared with 0xFF and 0x18), rolls `0xB0:0x48` (1..100) against an adjusted chance and calls `0xBA:0x1509`. Neither is an operand helper. | Behavior depends on unidentified runtime and resident routines. |

## 3. Correction to merged documents

PR #6 resolved far calls with a linear file model (`seg*16+off` in the packed START.EXE
image). This pilot showed that overlay entry stubs are addressed at run time as
`segment:(0x20 + 5*j)` in a paragraph-aligned 32-byte-header layout whose segment is
recorded in the next unit's descriptor (offset +14); the final unit's 0x428 segment is
inferred from descriptor spacing and corroborated by seven relocated calls. Under that model
1,161 fixup-confirmed far calls resolve to overlay entries (1,157 on exact prologues) against 202 before, and
the previously "misaligned" sites disappear. Consequences for earlier statements:

- `0x2B:0x25`, `0x2A`, `0x2F`, `0x34`, `0x8E` are unit 0x25 entries 1, 2, 3, 4 and 22, not
  20 to 24. The ECL contracts doc and the feasibility report are corrected in this PR.
- Segment `0xB0` (311 calls) is overlay unit 0x9e, not a resident RNG unit; `0x3E0`,
  `0x3C5`, `0x85`, `0x8F`, `0x96`, `0x7C`, `0x68` are overlay units too (see the inventory).
- Resident segments remain `0xAF8` (runtime), `0xBA`, `0x709`, `0x464`, `0x3F1`, `0x802`,
  `0x899`, `0x8AE`, `0xA40`; their file offsets are not claimed. For `0x709` and `0x464` all
  26 distinct targets land on Pascal prologues at file offset `0x200 + seg*16 + off - 455`,
  which is a candidate rule for the resident game units, not yet applied by the tools.

## 4. Throughput and obstacles

| function | reading and translation | checks | notes |
| --- | ---: | ---: | --- |
| 1 | 10 min | 5 min | trivial leaf, full translation |
| 2 | 35 min | 25 min | extracted core; table transcription plus the data check that exposed prefetch versus wire counts |
| 3 | 40 min | 20 min | extracted core; runtime string calls abstracted; cap provisional |

Measured count for the battle card: strict full translations in this ledger's numbering 10
(functions 1, 4, 5, 10, 11, 12, 13, 14, 15 and 16); overlay-only reviewed full C count on
main 32 on d3cde1f (10 from this ledger's numbering plus the reviewed unit 0x38 batches,
PM's record), unchanged by the issue #102 PR pending the PM's
ruling on functions 24 and 25; project-wide strict count 41 on d3cde1f (overlay 32 plus resident
9, PM's record); path-complete models with stated dependency, heap limits or external
boundaries 12 (functions 7, 8, 9, 17, 18, 19, 20, 21, 22, 23, 26, 27); path-complete translations whose runtime string calls carry the provisional cap 2
(string by address, store string); cores 5 (functions 2, 3, 6, load word, store word); total
verified functions is not claimed beyond that.

Time spent on the addressing model that these functions depend on: about one hour, which
also rewrote the call-graph resolution. Obstacles that remain for the next functions: the
unnamed Turbo Pascal runtime (1,790 call sites), the resident game units' file mapping,
and the absence of DOS oracle traces (issue #3) for dynamic confirmation. The NEWECL
handler's loader call (`0x7C:0x57`) resolves to unit 0x6f entry 11 under the run-time model;
PR #6 named it unit 0x7f entry 1.

## 5. Build and CMake targets (applied on main in 6d3b7da)

Without CMake: `tests/recovered/build_and_test.sh` compiles the library and the test with
`-std=c11 -Wall -Wextra -Wpedantic -Werror` and runs it. With CMake the target
`poolrad_recovered_tests` runs under `ctest`. The targets in `CMakeLists.txt` are:

```cmake
add_library(poolrad_recovered STATIC src/recovered/ecl_helpers.c)
target_include_directories(poolrad_recovered PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(BUILD_TESTING)
    add_executable(poolrad_recovered_tests tests/recovered/test_ecl_helpers.c)
    target_link_libraries(poolrad_recovered_tests PRIVATE poolrad_recovered)
    add_test(NAME poolrad_recovered_tests COMMAND poolrad_recovered_tests)
endif()
```
