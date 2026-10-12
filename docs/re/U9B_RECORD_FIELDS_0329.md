# Unit 0x9B:+0x0329 record field leaf

`pr_u9b_record_fields_0329` covers every terminating path of the
call-free DOS v1.3 body at unit `0x9B:+0x0329..+0x045D`. It reads
three record bytes and three DS tables, then writes up to eight record
bytes. Their game meanings remain unknown.

Before implementation, `deja "Poolrad u9B 0329 conditional entry routine
byte parity control dependencies"` found no exact match.
`deja "u9b 0329"` recovered the prior [native u9B dispatch
trace](U9B_RUNTIME_DISPATCH_0000.md). This work reuses that trace,
the [u6F to u9B entry map](U6F_U9B_TRANSFER_1574.md), and the
[adjacent +0x0255 RAM/frame model](U9B_RECORD_TABLE_0255.md).
The ignored DOS binaries were checked independently.

| Source | Verified bound |
| --- | --- |
| START.EXE | SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| u9B `+0x0329..+0x045D`, GAME file `0x2ADB0..0x2AEE4` | 309 bytes, 125 instructions, seven branches, no calls; SHA-256 `f5ff12924988b708edcf3756213532a2c5ba1b4860a5f63e11ffcd229297f3d6` |
| Exit | `RETF 4` at `+0x045B`; next entry begins at `+0x045E` |

The hash-locked `+0x0000` body near-calls this leaf at `+0x01A6`
only if its signed record `+0x9C` gate is positive at
`+0x0197..+0x019D`. The captured saved first insertion has that
field zero after its reached `+0x0255` call and skips `+0x0329`.
This is a complete isolated leaf translation, not proof of execution
on that saved route.

## Frame and ordered effects

The API starts after the caller's near `CALL`: SS RAM already contains
return IP, caller CS, and the far record argument. At `+0x0329`, the
leaf pushes BP, reserves six bytes, loads the far argument from
`SS:[BP+6]`, and caches its offset and segment at `[BP-5]` and
`[BP-3]`. Its loop index at `[BP-1]` starts at one and ends when a
fresh read equals eight. Every later `LES` reloads the cached words.

For each index, `+0x0341..+0x035D` reads signed record `+0x2E` and
tests signed DS byte `+0x3D3B + 8*record[+0x2E] + index`. On a
negative table byte, `+0x035F..+0x03A2` compares its absolute value
to unsigned DS byte `+0x3CF3 + 8*record[+0x9C] + index`; an absolute
value greater than that byte clears record `+0x76 + index` at
`+0x03AE`. The other path adds the signed first table byte and the
unsigned second byte and stores their low byte at `+0x0400`.
For indices below six, `+0x040A..+0x044B` adds signed DS byte
`+0x3D4E + 5*record[+0x13] + index` to the just-written record byte
and stores the low byte again. Values are reread at each instruction
site: a record, DS table, local, or cached pointer may alias a prior
store.

The C model uses one A20-off 1 MiB RAM for SS, DS, ES, tables, and
record. The original `LES` and `RETF 4` word operands read
contiguous physical bytes even when their starting offset is
`FFFF`. The model exposes returned general registers, segment
registers, BP/SP, CS:IP, and flags. The final `+0x044F` compare
against eight supplies the terminating arithmetic flags. Null CPU
or RAM shorter than 1 MiB is rejected before any write. A
self-sustaining alias loop remains nonterminating as in DOS.

## Verification

The strict synthetic CTest covers host guards, the eight-index loop,
negative clear and nonclear gates, signed and unsigned sums, index
below-six bonus, and return-frame aliasing. The optional hash-locked
Unicorn audit executes the ignored original bytes for 140
deterministic synthetic cases. It compares **every byte of 1 MiB RAM
and every returned register** with portable C, including random
disjoint records, zero tables, DS/table and cached-pointer aliases,
a return-frame alias, and far argument and return offset crossings.
All 140 pass. A saved native
`+0x0329` capture does not exist; the captured insertion skips it.

```sh
cmake -S . -B resources/build-u9b-0329 -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u9b-0329
ctest --test-dir resources/build-u9b-0329 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u9b_0329_parity_audit.py --unicorn
```

The audit reports only offsets, counts, and hashes. Original code,
RAM, and game media remain in ignored `resources/`. Project-wide
strict and identity counts are unchanged pending PM review.
