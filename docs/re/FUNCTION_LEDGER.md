# Function ledger: recovered DOS v1.3 functions

**Issue:** #8 (lane A), continued on branch `claude/full-leaves` with section 1.5. **Target:** `GAME.OVR` sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` (206,105 bytes) with
`START.EXE` sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
**Code:** `src/recovered/ecl_helpers.c`, `include/poolrad/recovered/ecl_helpers.h`,
`tests/recovered/test_ecl_helpers.c`. All three originals live in overlay unit `0x25`
(file offset 29,907, 8,730 code bytes, 25 public entries, run-time stub segment `0x2B`).

**Delivered so far:** three **full translations** of leaf functions (functions 1, 4, 5) and
two **extracted behavior cores** (functions 2 and 3). A full translation reproduces every
instruction path of the original with the same inputs and outputs. An extracted core
reproduces a verified decision table or decoding loop but replaces engine memory (script
buffer, PC, Pascal string buffers, runtime calls) with explicit arguments and omits the
side effects; it is evidence toward a later full translation, not one itself.

Status vocabulary: **verified** = disassembly read instruction by instruction and an
independent check agrees with the part that is claimed; **provisional** = a stated aspect
rests on inference. Decompiler renderings were used as a reading aid only and are not
committed.

## 1. The three functions

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
  difference between "skip" (engine) and "stop at first zero" (lane B's current decoder)
  is not observable in the shipped data. Lane B may keep its contract or adopt the engine
  rule; this ledger records the engine rule.

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

### 1.5 Two further full leaf translations (branch `claude/full-leaves`)

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

## 2. Examined, not translated

| identity | what was learned | why not translated |
| --- | --- | --- |
| operand loader, unit 0x25 + 0x0000, entry 2 (called as `0x2B:0x2A`, 42 sites) | For operand i = 1..count: tag byte at PC+1 to `[0x7005+i]`, next byte to `[0x7085+i]`, PC += 2; tags 1, 2, 3: PC += 1 and the byte there to `[0x7045+i]`; tag `0x80`: string index += 1, length = the second byte, if non-zero call function 3 (index, length) else clear string `0x6FC8 + 256*index`; tag `0x81`: string index += 1, PC += 1, high byte to `[0x7045+i]`, then near-calls unit 0x25 + 0x06AF (entry 9) and unit 0x25 + 0x12FB to copy a string from script memory. Final PC += 1. | Depends on two further functions and on engine memory layout; slot layout recorded for the contracts doc. |
| unit 0x25 entry 20 (+0x1CC3) and entry 21 (+0x1DF0) | Entry 20 is arithmetic over the runtime's software-float routines (`0xAF8:0x119A`, `0x1186`, `0x119E`); entry 21 walks a linked list of party records (head at struct +0xC8, link at +0x2A, field +0x37 compared with 0xFF and 0x18), rolls `0xB0:0x48` (1..100) against an adjusted chance and calls `0xBA:0x1509`. Neither is an operand helper. | Behavior depends on unidentified runtime and resident routines. |

## 3. Correction to merged documents

PR #6 resolved far calls with a linear file model (`seg*16+off` in the packed START.EXE
image). This pilot showed that overlay entry stubs are addressed at run time as
`segment:(0x20 + 5*j)` in a paragraph-aligned 32-byte-header layout whose segment is
recorded in the next unit's descriptor (offset +14). Under that model 1,154 fixup-confirmed
far calls resolve to overlay entries (1,150 on exact prologues) against 202 before, and
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

Measured count for the battle card: 3 fully translated functions with passing behavior
checks (functions 1, 4, 5), 2 extracted cores (functions 2, 3); total verified functions is
not claimed beyond that.

Time spent on the addressing model that these functions depend on: about one hour, which
also rewrote the call-graph resolution. Obstacles that remain for the next functions: the
unnamed Turbo Pascal runtime (1,787 call sites), the resident game units' file mapping,
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
