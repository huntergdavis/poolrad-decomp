# U38 +133F and raw resident00BA:2637

U38+133F now composes a raw CPU/RAM version of its resident target-reset
callee. The earlier resident scalar/callback API remains available unchanged;
it cannot carry the live stack/register state required by this caller.
The resident identity was already recovered: this is an ABI upgrade, not a
claim for a new resident identity or new credit. Independent review owns
verification and accounting. References: tracker #1019 and VP assignment i17989.

## Identity and call chain

| Body | Extent | Size / instructions | SHA-256 |
| --- | --- | --- | --- |
| U38+133F | GAME.OVR file[65434,65456), unit[133F,1355) |22 B /9|f5d32541e8f8b2ce0e4d08ed887a5ddf9c52829f449f74b8c1553a6b0bbfe8ce|
| RES00BA:2637 | expanded image[12759,12829), segment offsets[2637,267D) |70 B /20|045d3c16b446425bd6ec344cfb54b83028fc2ed83c46f02ae561401fad734721|

The caller is public U38 entry43, original stub0041:00F7. The hash-pinned
U38+31A2 installer binds it to types31,51,52,53. The LCALL at1348 points to
00BA:2637, with loader fixup at134B. Both targets and the caller's RETF10
are checked from the original bytes. The resident ends with RETF4, has no
callees, and reloads its argument and linked pointer before every byte store.

Media: START.EXE (65,231 B), SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd;
GAME.OVR (206,105 B), SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Installer SHA-256:
4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519.

## Raw contracts

Both APIs use exactly 1 MiB A20-off RAM and the standard14 CPU words:
AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS. CPU is outside RAM; code and
bindings remain immutable. Interrupts/faults are excluded. Words and LES
pairs read physically contiguous bytes after the effective offset wraps;
physical addresses wrap at1MiB. All reached return frames remain nonwrapping,
including aliases that overwrite them.

The resident API enters at IP2637, SP[0004,FFF8], with return IP/CS and a
character offset/segment. It pushes BP, allocates two bytes with SUB SP,2,
and follows the live character+108 pointer independently before each zero
store at target+3,+0,+7,+6. It then writes local[BP-1]=1, loads AL preserving
AH, restores the live frame and RETF4. Its final FLAGS are the SUB SP,2 result.
There are no memory callbacks or invented null checks. Pointer aliases can
redirect later writes; frame aliases can change BP or the actual far return.

The caller API enters at IP133F, SP[0010,FFF2], with return IP/CS, three unused
words and the character offset/segment. It pushes the two character words,
checks the reached LCALL opcode/offset, reads the resident segment from the
live fixup, constructs the real far-call frame and invokes the raw resident.
A changed resident return stops TRANSFER with exact partial state. Otherwise
OR AL,AL sets the original flags, POP BP restores the caller frame, and RETF10
consumes the five handler words. Logical AF follows normal/386 zero.

The JE at134F targets1351, which is also its fallthrough. The resident always
loads AL=1 after writing its own local, so this corpus observes ZF=0 and the
not-taken direction. There is only one distinct successor address. The C does
not invent a second observable path or count an unreachable taken outcome.
CODE stops at1348 before the call. Invalid host arguments change nothing.
The CALLEE status cannot arise within the declared parent SP/entry profile.

## Measured evidence

- Caller original-byte differential:209 cases,207 returns,one changed-return
  stop,one code-binding stop; all29 combined instruction starts (9 caller,20 resident).
- Resident direct differential:111 cases,all20 instruction starts.
- Every differential compares all14 CPU words and all1,048,576 RAM bytes;
  there are no flag masks or undefined-flag exceptions.
- Native DOSBox-X normal/386:100 complete caller/resident executions,CPU14,
  SS1KiB,DS64KiB and an unchanged heap64KiB capture; no flag masks.
- Synthetic C:3,599 ASan/UBSan checks; focused CTest1/1.

Original cases include all256 AH values in synthetic C, randomized registers,
nil pointers, alias-driven pointer retargeting, live saved-BP/return changes,
record and target offset wrap, A20, odd/minimum/maximum SP, relocated resident
code, seeded RAM backgrounds, and a reached invalid call binding. The changed
inner-return case clears both return IP's low byte and CS's high byte; it stops
at0000:1300 instead of falsely continuing the caller.

Native fixtures place records/targets in captured DS and include offset-alias
retargeting, target-offset wrap and record-link effective-offset wrap. Original
caller bytes are relocated only at the verified resident segment word to7000;
the resident is copied unchanged to7000:2637 and caller to8800:133F. Generated
original bytes, assembly, COM, disk input and captures stay ignored. LES pairs that cross a64KiB segment boundary have Unicorn-only evidence;
native fixtures use noncrossing operands. The
fixtures are synthetic; natural gameplay reachability is not proved. Native
captures are after the caller's OR, so standalone resident SUB flags are checked
by direct original-byte differential, not separately by native entry capture.

Native RESULT.BIN SHA-256:
32e2249832056b9bc4e8b25171f2b0351c912c5f0214d95d8548d4a1208364d2.
Combined measured receipt SHA-256: d330267a1ef25c849b5e8d18ca045a530a4457315b056e846247c1b915881e4b.

## Reproduction and reuse

    PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u38_target_reset_133f_audit.py --unicorn --native --output resources/u38-133f-combined.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_overlay_u38_target_reset_133f_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_overlay_u38_target_reset_133f_cpu_tests$' --output-on-failure

Recall query "poolrad U38 133F resident 2637" timed out while the previously
started deja-health --repair rebuild was active. No recalled session was reused.
[RESIDENT_CHARACTER_TARGET_RESET.md](RESIDENT_CHARACTER_TARGET_RESET.md) supplies
the scalar behavior, original loader address and game-caller provenance. Source
search found only its scalar/callback API. The raw implementation and its frame
semantics are checked against the original bytes. Native capture follows #1020's
disk-initialized oracle; the shared media/CPU helpers come from the U86 audit.
The primary slot lead comes from #1019/#1020's inventory and is rechecked here.

## Parity review record

- target_function: U38+133F and raw resident00BA:2637.
- source_candidates: overlay_u38_target_reset_133f_cpu.c and
  resident_00ba_target_reset_2637_cpu.c. The scalar resident source is a semantic
  reference, not a replacement for CPU/stack effects.
- signals: exact body hashes, public entry, installer stores, relocation,
  Pascal ABI, live pointer/frame aliases, complete instruction coverage,
  full-state original execution and native caller composition.
- score: high for the declared two-body chain; natural-game route unmeasured.
- verdict: likely_match, pending independent reviewer verification.
- review_notes: inspect LES physical contiguity, SUB versus OR flag contracts,
  alias-induced transfer handling, the JE's coincident successors, and native
  capture scope. No self-credit or canonical accounting changes.
