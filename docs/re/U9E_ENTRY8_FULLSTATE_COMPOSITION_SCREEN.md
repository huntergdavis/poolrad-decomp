# U9E entry8 full-state composition source gate

This pre-edit gate maps the merged bounded sources onto the original
75-byte U9E entry8 function before adding any composition C. It reuses
[U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md)
for the U7F/B0:0048 frame and each linked slice note for its local
contract. A targeted deja query returned no indexed session. All five
merged original/loaded audit scripts passed locally against ignored
DOS v1.3 media; no emulator or natural caller was run.

GAME.OVR is 206105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Unit U9E base is file 0x2AEEA. Entry8 is [0x2BCCD,0x2BD18),
75 bytes / 29 starts, SHA-256
7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba.
START.EXE is SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

## Exact source and temporal ownership

| Original offsets | Bytes | Merged owner | Normal stop |
| --- | ---: | --- | --- |
| +0DE3..+0E0D | 43 | fullstate_prefix_0de3 | count zero: +0E1F; otherwise: +0E0E |
| +0E0E..+0E12 | 5 | system_call_0e0e, using SYSTEM +11C0 and +120F | +0E13 only on a normal SYSTEM return |
| +0E13..+0E1E | 12 | fullstate_postcall_0e13 | equal index/count: +0E1F; otherwise: +0E05 |
| +0E05..+0E0D | 9, already inside prefix | fullstate_loop_0e05 on subsequent iterations only | +0E0E |
| +0E1F..+0E2D | 15 | fullstate_tail_0e1f | live caller CS:IP after RETF4 |

The non-overlapping first-pass spans sum to 75 bytes. The loopback
re-executes nine prefix bytes and is a separate temporal owner, not an
additional function extent. +0E2D is the second immediate byte of
RETF4, not a new instruction. The original JNE at +0E1D targets +0E05.
The exact +0E0E far call targets SYSTEM 0AF8:11C0; SYSTEM calls +120F
before its range-zero check and returns with RETF2.

A smallest future composition can start at loaded U9E CS:+0DE3, call
prefix, then loop through call adapter and postcall; postcall +0E05
uses loopback before the next call; +0E1F uses tail. The same live
CPU14 and 1 MiB A20-off RAM must pass between every component.
Do not substitute the aggregate pr_u9e_entry8_roll helper: its detached
seed and count/range values cannot preserve stack or seed aliases.
B0:0048 stub dispatch and any U7F caller transition are outside this
composition's entry boundary.

## State and transfer conditions before C editing

* The U9E entry frame has return IP, CS, range WORD and count WORD at
  SS:SP +0,+2,+4,+6. Prefix PUSH BP and four-byte local reservation
  place count at [BP+8], range at [BP+6], count snapshot at [BP-4],
  sum at [BP-3] and index at [BP-2].
* Each nonzero iteration rereads live SS:[BP+6], pushes a range WORD,
  and calls SYSTEM. SYSTEM reads/writes the DS seed DWORD at 4428/442A
  in the same RAM. A seed write may alias range, locals, later stack
  words, or return words. A fixed copied range or seed is invalid.
* The first-pass XOR AH,AH leaves AF undefined; the prefix source
  retains its incoming AF by convention. Loopback INC BYTE defines
  AF on low-nibble carry, then XOR retains that AF by the same
  convention. Postcall CMP defines all six arithmetic flags on
  every normal nonzero iteration. Zero count reaches tail after
  prefix CMP; tail preserves FLAGS.
* The +0E0E adapter reports argument, nested SYSTEM failure,
  changed SYSTEM transfer, and changed outer return separately;
  none can flow into +0E13 as though normally returned. Each slice
  rejects writes into pinned loaded instruction bytes at its own
  boundary. If a later rejection occurs, preceding live effects
  remain; a composition cannot roll them back.
* Tail POP BP and RETF4 read each SS word at a separately wrapped
  16-bit offset; each word's high byte is physically contiguous
  across the 1 MiB boundary. Tail returns the actual live CS:IP,
  even if an alias changed the caller frame.
* The byte index, count snapshot and caller range may all be
  modified by aliases. A future host loop cap must return a distinct
  limit status with the live state. It is not an original CPU branch
  and cannot be counted as a full-function execution.
* The five C CPU14 structs list the same 14 uint16_t fields, but
  they are distinct C types. A new wrapper must copy state by named
  fields or prove representation compatibility; casting between
  unrelated struct pointers would violate strict aliasing.

## Gate result

The original/loaded prefix audit passed count 0, 1 and a high-byte
count case; the +0E05 byte/JNE gate passed; the call audit passed normal
and wrapped frames; postcall passed equal/unequal loop conditions;
tail passed ordinary and BP=FFFC/FFFD wrapped RETF4 frames. These are
controlled C/source checks, not original-instruction or native parity.
The source ownership screen is clear for a bounded loaded-entry
composition that preserves explicit transfer/limit statuses. It does
not establish B0:0048 dispatch, a natural U7F frame, or strict full-C
credit. Wait for PM review of this exact source gate before editing the
composition C.
