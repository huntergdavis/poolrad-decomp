# U1C +2B14 raw clear-box composition

This extends the existing 31-byte caller with a raw CPU/bus entry.
It increments the live script pointer at DS:49ED, pushes left1/top1/right15/
bottom15 with incoming AH preserved, calls resident 0709:0DD3, and sets
DS:84DE to 1 only after the expected callee return. Clear composes the
existing rectangle, page-select, EGA reset and bus FillChar implementations.
No function effects are substituted by callbacks.

The original callback body and its API prefix remain unchanged.
Higher-level ECL/gameplay callback routing is not migrated by this change.
No classification, acceptance, accounting, merge or credit change is included.

## Selection and provenance

The regenerated schema-8 successor of the schema-4 queue selects
OVL_001C_2B14 at rank 338, score 29, with one composable resident callee.
Earlier higher-ranked candidates already have raw bodies, including
U3B0 +06FB merged in PR #1116 and U1C +2E7D. The existing +2B14 body is a
callback boundary; its dependency now has a merged raw ABI.

VP ownership approval i18733 permits extending the existing pair and
preserving the callback ABI. A pinned main-tree source/header scan finds
the existing callback implementation, not another raw caller body.
Checked-in queue identity is game_verified/verified; that existing metadata
is not new evidence or author-side credit. Independent review owns any
acceptance/classification decision.

Dunking Sheep query: "poolrad U1C 2B14 clear box callback 0DD3 raw CPU
ownership". Codex session 01a1281b-85e3-7e23-bdf6-1ea8af4d69de and Claude
session 9742b6e1-d504-40a0-92b9-7d2536d3f756 supplied historical
callback/source pointers only. Execution evidence is newly measured.
The existing +2B14 physical access and INC-flag helpers are reused, as are
the unchanged raw clear/rectangle/page/reset/FillChar/bus implementations.
Original audit mechanics come from res0709_clear_0dd3_cpu_audit.py;
native plain/boundary capture mechanics come from the U3B0 +06FB harness.

START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

| Body | Half-open span | Bytes | Instructions | SHA-256 |
| --- | --- | ---: | ---: | --- |
| U1C caller | 2B14–2B33, GAME file offset 14426 | 31 | 15 | 69641d023ee5478d48a8dd54832eda981a2d9cb1c87aeff0d08435e27ae3c0bf |
| Resident clear | 0709:0DD3–0F82 | 431 | 190 | 0ca48149f5b50fcb93ed265bb72528f33ae5ec659cd134f205f6f312c50905f2 |
| Rectangle | 0709:00B7–0171 | 186 | 89 | f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5 |
| Page select | 0709:00A0–00B7 | 23 | 10 | 6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d |
| EGA reset | 0709:0000–0071 | 113 | 58 | 5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5 |
| FillChar | 0AF8:16A8–16BC | 20 | 7 | f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7 |

Static closure: 804 bytes / 369 instructions across six bodies.
Caller FAR CALL2B27 returns at2B2C; final store2B2C, POP BP2B31, RETF0
at2B32. Source mapping, spans, hashes and instruction counts are checked
against owner-supplied original media, kept under ignored resources/.

## Entry and stop contract

Exactly1MiB A20-off bus with attached EGA, entry IP2B14, SP004C..FFFC.
DS/SS whole segment windows exclude EGA; code and bindings are immutable
outside EGA and writes avoid code. Reached far-return frames do not cross
SS:FFFF. CPU, bus, EGA and diagnostic are separate host objects outside RAM.
Effective offsets wrap at16 bits; bytes inside word operands are physically
consecutive with20-bit wrap. Other aliases remain live.

The initial PUSH BP precedes the DS:49ED read/increment, so aliasing that word
with saved BP changes the value incremented and later restored. INC preserves
CF. Four MOV AL/PUSH AX pairs preserve AH and FLAGS. A call opcode/offset
guard rejects before pushing the far return frame; the live segment operand
selects the loaded resident body.

The raw clear dependency executes with real CPU/RAM/EGA state. Its two loop
budgets pass through unchanged; all nested diagnostics and partial state
are preserved. A successful return to an unexpected CS:IP stops as TRANSFER
before the DS:84DE store. After an expected return, that byte store can
legitimately alter saved BP or the outer return pair; both are read live.
There is no invented cleanup beyond the real callee RETF8 and caller RETF0.

Invalid host arguments preserve CPU/RAM/EGA and reset optional diagnostics
to -1. Null diagnostic is supported. DS/SS accesses use the legacy ordinary
RAM helpers under the non-EGA segment precondition; code reads use the bus.
The renderer's existing FLAGS/device/budget contracts apply unchanged.
No new undefined-FLAGS normalization. Port hooks only observe/refuse without
mutation. No arbitrary-PC resume, interrupts, faults or self-modifying code.
Native hardware scope is EGA, not CGA/Tandy.

## Tests and reproduction

Use a Python environment with Capstone and Unicorn:

    python tools/overlay/u1c_clear_box_2b14_cpu_audit.py --unicorn --output resources/original.json
    python tools/overlay/u1c_clear_box_2b14_cpu_audit.py --native --output resources/native.json

The original audit executes the actual closure and compares CPU14 unmasked,
all983040 ordinary RAM bytes outside the EGA aperture, ordered OUT events
including CS:IP, and VRAM write operands. Guard/budget/refusal hooks stop
immediately before the corresponding original instruction. Unicorn is not
the plane oracle; native EGA captures test all four planes separately.

Native DOSBox-X uses machine=ega, video mode0Dh, core=normal, cputype=386.
Normal runs are uninstrumented except eight loaded far-segment bindings:
the caller, six clear-to-FillChar sites, and rectangle-to-FillChar.
Caller/resident/SYSTEM are loaded at4000/8800/6000; near calls are unchanged.
Selected stop cases use a flags-neutral far jump at the pre-call/pre-OUT
boundary. CPU14, SS1024, DS65536 and all4x65536 EGA plane bytes are unmasked.
No native OUT-trace, budget-stop, A20, wrapped-frame or gameplay claim.
Packed-mode ordinary RAM comparisons do not establish packed hardware.

The synthetic C test independently verifies all65536 INC results/defined
flags before a call guard, the whole-plane rectangle and untouched
surroundings, live AH-bearing argument words, script-pointer wrap,
completion-byte suppression at partial stops, completion-byte/return-frame
aliases, saved-BP/script-pointer aliases, and host-argument atomicity.
The legacy callback, resident clear and rectangle tests are included in
focused CTest. Exact measured results and receipt details follow below.

## Parity review

target_function: U1C [2B14,2B33), raw extension of the existing callback body.
source_candidates: overlay_u1c_clear_box_2b14.c and the unchanged raw clear,
rectangle, page-select, EGA reset and FillChar closure.
signals: pinned span/hash; exact15-instruction shape; script pointer and
completion byte; AH-preserving arguments; original call/return frames;
original CPU/RAM/port/write comparisons; independent native planes and
synthetic boundary/alias checks.
score: seven independent signal categories, scoped behavioral match rather
than an identity or hardware probability.
verdict: match within the documented memory, device, entry and stop profile.
review_notes: inherited dependency limits remain; static body coverage is
not dynamic path coverage; gameplay routing and independent acceptance are
separate. Closure paths not executed by this fixed caller are listed
explicitly in the ignored original report.

## Measured result

- Original:119 cases,62 returns,12 caller code guards,45 nested stops.
  Caller15/15 starts and closure368/369 starts; only0709:0EF5 is unexecuted.
  No conditional branch in the caller. CPU14 and983040 ordinary RAM bytes
  per case unmasked,2162 OUT events and162150 VRAM write operands matched.
- Two packed-output/stack aliases return nested clear status5 (TRANSFER),
  outer status3 (CALLEE), with CS:IP0000:0000 and DS:84DE stillA5. CPU,
  diagnostics and RAM were compared against original execution. The
  top-level unexpected-successful-clear-return status4 remains unmeasured.
- Native:35 cases,21 returns,10 pre-call stops,4 pre-OUT stops.
  CPU14 plus328704 RAM/plane bytes per case, unmasked.
  RESULT.BIN SHA-256:
  95f3edc5683cbb628a407fb007c7cbcb0bc93153807c796c4ae1af50618a253e.
- Strict C11 warnings-as-errors with ASan/UBSan:2228299 checks passed.
- Focused CTest:4/4 passed (raw caller, unchanged callback, clear, rectangle).

Tested and integration base:7281bef845670dc4f88fea60443d0a903bfb26fe.
No execution dependency was modified. The ignored resources/evidence.json
pins all public inputs, legacy prefixes, original/native reports, tool/media
hashes, native INPUT/RESULT and sanitizer/CTest logs. Native INPUT is
independently reconstructed before receipt creation. Receipt public hashes
are checked against the final committed head. Independent acceptance and
credit remain with the reviewer/VP.
