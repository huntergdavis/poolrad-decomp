# U38 +3134 raw gated message caller

This adds a raw CPU body for unit 0x38 public entry 114 (`+3134`). It composes the
unchanged SYSTEM string load `0AF8:0B0B` and the resident display routine
`00BA:1693`: unless a gate byte or a range byte blocks it, it clears one byte
through far pointer A, copies a 24-character Pascal-string literal into its own
frame and shows it as a transient message. No callee is replaced by a callback.
This PR supplies the body and evidence only. It edits no manifest, ledger,
classification, checkpoint or credit file; acceptance belongs to the reviewer.

## Selection, ownership and scope

Queue input: `resources/strict-candidate-queue/queue.json` (read-only, not
regenerated), SHA-256
`c0b0ce1b6d82c91261902976728492297f3dc41e7639512dc78171b4c4f96c1b`, 455 rows.
Row `OVL_0038_3134`: 70 bytes, zero unready callees (callee list: resident
`RES_00BA_1693` ready, runtime `0AF8:0B0B` covered). Authored on main
`f04d610efdd5757ade32bca4346edf2cfec2e188`, rebased onto `c1af4cbf` (see commit
range; the checker, sources and media were unchanged by the rebase).

Ownership comes from the unit, not from a caller scan. `+3134` is public entry
**114 of 117** in unit 0x38 (`unit['entries'].index(0x3134) == 114`), alias stub
`STUB_0041_025A`. Its only loader fixups are the two far-call segment words at
`+316D` and `+3172`. The queue records **zero** fixup-backed and zero near
callers: like other dispatch-table entries it is reached through the unit's entry
table, not by a direct call. That establishes game-specific ownership of the
bytes and their two outgoing bindings; it does **not** show the entry is reached
in natural play, and no such claim is made. A caller-based reachability argument
remains open (see `U9E_DISPATCH_HANDLER_BACKLOG.md`).

## Original identity

START.EXE SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The loader-expanded START.EXE image is 68,592 bytes, SHA-256
`a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e` (resident and
SYSTEM bodies are hashed from this image, never from raw file offsets).

| Body | Span | Bytes | Instr. | SHA-256 |
| --- | --- | ---: | ---: | --- |
| U38 entry 114 | `3134`–`317A`, GAME.OVR offset 73,103 | 70 | 26 | `9c024ea9f2fa5fa8b113d6bafce1d7dea90603488dff98a7c6362bd38978481f` |
| SYSTEM string load | `0AF8:0B0B`–`0B25` | 26 | 12 | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| Display | `00BA:1693`–`16EC` | 89 | 44 | `6db9222ad42386393622a6614d2ab37cc1f2eec0a7d65b2e943bbc2e9593581a` |

The span is decoded from the GAME.OVR bytes to its single RETF (`3177`, `RETF 0Ah`);
the queue's 70 B equals the decoded span, so **no metadata repair** applies here.
The 25 bytes at `CS:311B` (length byte 24, hash
`a625eab24ae403c80fc07c89550e2f4462d4100f902d59010a4a0f5017a44eea`) precede the
entry exactly and are the string literal read by `0B0B`; their text is not
recorded. The complete executed closure is **2,246 bytes / 941 instructions in 14
bodies**. The checker pins all of them. There is no indirect call, indirect
jump or INT in the 26 instructions.

## Raw contract

Entry IP `3134`, exactly 1 MiB A20-off RAM, attached EGA, SP in `[0192,FFF2]`.
The lower bound is 1693's own entry minimum `016A` plus this frame (`2 + 1Eh + 8`
bytes); the upper bound keeps all 16 frame bytes below `SS:FFFF`. Stack frame:
`[SP+4]` gate word (only its low byte is tested), `[SP+6/8]` far pointer A,
`[SP+10/12]` far pointer B; `RETF 0Ah`.

Behaviour: nonzero gate byte returns; otherwise a byte `ES:[B+10h] >= 13h`
returns (both with the compare flags, A untouched); otherwise byte
`ES:[A+34h] := 0`, the literal is copied to `SS:BP-1Dh` through `0B0B`, and
`1693` displays it. Pointers A and B are **unrestricted**: their single-byte
accesses go through the bus, so video-aperture and aliased targets are live
(measured below), including A aliasing the literal's length byte, the saved BP,
the return frame and the local buffer, and B aliasing the return frame.

Each far CALL checks opcode `9A` and its offset before pushing its frame; a
mismatch stops with CODE and IP at the call. The live segment word selects the
loaded body. CALLEE keeps the nested stop (`callee` 0 = string load, 1 =
display, with 1693's own nested diagnostic); TRANSFER keeps an unexpected
successful return; ARGUMENT rejects atomically. Budgets are 1693's per-callee
loop limits, passed unchanged.

## Exclusions, each with the state that would make it feasible

1. **SP below `0192` or above `FFF2`** (402 + 13 = 415 of 65,536 values). Feasible
   if a caller enters with a nearly exhausted stack or with frame words past
   `SS:FFFF`. Excluded because 1693's raw contract refuses SP below `016A` and a
   frame crossing `SS:FFFF` would wrap. The early-return paths do not need the
   lower bound; the single range is a deliberate simplification, not a proof.
2. **CS, DS or SS segment windows overlapping `A0000..AFFFF`** (segments
   `9001..AFFF`, 8,191 of 65,536 values). Feasible if code, data or stack were
   mapped into video memory. Excluded because `0B0B` and 1693 read the string and
   frames from flat RAM. Pointers A and B are not subject to this exclusion.
3. **A write onto executed code** (the 2,246 bytes above). Feasible if A points
   into any closure body: the original would then execute modified instructions,
   which the C composition does not re-decode. The checker asserts that no case
   mutated a pinned body. Writes onto the literal's *length byte* are **not**
   excluded and are measured (`a-literal-*`, `literal-*`).
4. **Trap flag and interrupts**: flags with TF set raise a single-step trap
   (Unicorn reports `UC_ERR_EXCEPTION`); no interrupt, fault or
   self-modification is modelled. No flags are masked: every other flag
   pattern in the cases is compared exactly.
5. **VGA write mode 3 on a write through A into the aperture.** Feasible if the
   graphics controller was left in mode 3 by earlier code; outside the EGA
   contract of the independent device model (it refuses mode 3).
6. **Arbitrary-PC resume, return frames that wrap, CGA/Tandy hardware.**

## Masking and comparison scope (doc truth)

- **Python/Unicorn differential: nothing is masked.** CPU14 (all 14 words,
  flags included, no flag mask), the **entire 1,048,576-byte backing RAM** including
  the unchanged backing bytes under `A0000..AFFFF`, ordered OUT events with
  CS:IP, every EGA write operand, and the full device state (four 64 KiB planes,
  latches, sequencer and graphics registers; 262,164 bytes). The aperture is
  served by an independent Python EGA through MMIO, not by RAM. Excluded
  ranges: **none**. This differs from the older resident/overlay audits (such as
  `+06FB`) that compared 983,040 ordinary-RAM bytes and masked the aperture.
- Unicorn is the CPU oracle for the original bytes; the Python EGA is an
  independent oracle for planes, written from the documented truth tables.
- **Native DOSBox-X is partial** (below): only the captured windows are compared and
  every other backing byte is masked; the report lists the masked ranges.

## Differential result

164 cases, **zero mismatches**, all unmasked. Original bytes of all 14 bodies
executed; 66,104 OUT events, 211,067 EGA write operands and 6 aperture reads
compared.

- **Root coverage: 26/26 instruction starts; 4/4 successor outcomes**
  (`314A→314E`, `314A→314C`, `3156→3174`, `3156→3158`), every one feasible.
- Closure coverage: 934/941 starts, 84/96 outcomes. The 7 unexecuted starts
  (`8000:025B`, `8800:021A`, `021D`, `022D`, `0235`, `05E2`, `064D`) and 12
  outcomes lie in already-delivered callees (clear, printer, rectangle) and are
  inherited from their own checkers; this PR claims root coverage only.

Stops, named and **not summed** (case counts):

| Stop | Cases |
| --- | ---: |
| RETURN/message | 69 |
| RETURN/message/altered-frame (return frame rewritten by the copy or by A) | 7 |
| RETURN/gate | 26 |
| RETURN/b-gate | 23 |
| CODE@316A (0B0B guard) | 2 |
| CODE@316F (1693 guard) | 2 |
| CALLEE/display, nested 1693 root CODE at 16A7 / 16B8 / 16CB / 16D0 / 16E1 | 1 each |
| CALLEE/display/first-clear: CODE 01F1, CODE 020C, LIMIT 01D2 | 1 each |
| CALLEE/display/first-clear/rectangle: CODE 0140, LIMIT 014F | 1 each |
| CALLEE/display/first-clear/rectangle: PORT 015C | 2 |
| CALLEE/display/first-clear/rectangle/nested 2@0008 / 000E | 2 each |
| CALLEE/display/first-clear/rectangle/nested 2@0014 / 006E | 1 each |
| CALLEE/display/printer: CODE 063C, CODE 0691, LIMIT 069F | 1 each |
| CALLEE/display/printer/glyph: CODE 0207, LIMIT 045B, PORT 03AD | 1 each |
| CALLEE/display/printer/glyph: PORT 03D5 | 8 |
| CALLEE/display/delay-wrapper: CODE 0DCC | 1 |
| CALLEE/display/delay-wrapper/CRT: LIMIT 02C0 | 2 |

The string-load callee has no failing status (its only non-return is an
ARGUMENT for a non-1 MiB host buffer, which this API prevents), so there is no
`CALLEE/strload` case; ARGUMENT is covered by the C test.

## Native corroboration

DOSBox-X (`machine=ega`, video mode 0Dh, `core=normal`, `cputype=386`) runs the
original 3134, 0B0B and the complete 1693 closure at rebound segments
(overlay 3000, SYSTEM 6000, resident 8200/8000/8400/8800) with the original
bytes. Segment words are relocated exactly as the loader would; **no successful
callee is stubbed (0 substitutions)**. A stop case replaces the reached
instruction with a flags-neutral far JMP back to the harness (declared trap) and
reports that original boundary; ordinary cases are uninstrumented.

**45 native cases, zero mismatches**: 31 returns and 14 stops (CODE at `316A` and
`316F`; nested 1693 CODE at `16A7`, `16B8` ×3, `16CB`, `16D0`, `16E1`;
clear `01F1`, `020C`; printer `063C`; glyph `0207`; delay `0DCC`). Each case
compares CPU14 (flags unmasked), a 1,024-byte stack window, the full 64 KiB DS,
font and string segments and all four 64 KiB EGA planes. The set covers
gate/B-byte paths, six flag patterns, delay factors, active pages, modes 3/255,
no-font, aperture-page and bad-font cases, A aliasing the literal's length byte
and first character, A wrap, **A written inside the video aperture** and **B
read inside it**. RESULT.BIN SHA-256:
`c68cbbe64f2bbf85bcfa721708f3e2a403be97b7626a4416fc1858b83c36b41e`.

**Native scope is partial and masked.** Captured RAM is 197,632 bytes
(`16D40..1713F` stack window, and the DS, string and font segments at
`50000`, `40000`, `70000`); **850,944 bytes of backing RAM are not compared
natively**: `00000..16D3F` (93,504), `17140..3FFFF` (167,616), `60000..6FFFF`
(65,536) and `80000..FFFFF` (524,288). Those bytes are compared in the Unicorn
differential only. Declared harness limits: the DOS harness cannot vary the
string literal per case, so the seven literal-length cases are Unicorn-only;
the sequencer map mask is restored to `0F` after the per-plane fill so both sides
start from the reset state (an earlier run differed on exactly that, in the
harness, and was corrected before this result); controller state beyond the
planes is not captured natively; no OUT trace and no budget-stop, A20 or
wrapped-frame claim.

## C semantic test and reproduction

`tests/recovered/test_overlay_u38_message_3134_cpu.c`, built with strict C11
`-Wall -Wextra -Wpedantic -Werror` and ASan/UBSan (no recovery), passes:
**5,242,818 semantic checks, exit 0**. It checks the gate and B-byte exits (flags,
A untouched, frame cleanup), the message path (A byte cleared, the literal copied
to `SS:BP-1Dh`, 1693's first nested stop and its diagnostic), A aliasing the
literal, A offset wrap, A and B inside the EGA aperture (through the bus, not flat
RAM), all four code guards with their exact SP, and atomic host rejection (seven
bad states and null/size/limits arguments). The full-closure behaviour is the
Unicorn differential's job; this test pins the composition seams.

    python tools/overlay/u38_message_3134_cpu_audit.py --compile-only
    python tools/overlay/u38_message_3134_cpu_audit.py --unicorn --output resources/original.json
    python tools/overlay/u38_message_3134_cpu_audit.py --native --output resources/native.json

Run under `nice -n 15` and the shared heavy-job wrapper.

## Reused evidence and limits

Reused code, unchanged: `resident_00ba_display_1693_cpu` and its closure, the
SYSTEM string leaves, the 1693 audit's independent Python EGA, fixtures and
oracle hooks, and the composition pattern of `U3B0 +06FB` and `U7D +05F0`.
`deja` was unavailable (the MCP server failed to start), so no prior-session
recall was used. The row-clear `+06FB` doc is reused only as a template.

Limits the evidence relies on, stated here as required: synthetic frames and
fixtures only, no naturally reached game route; entry reachability through the
dispatch table is not shown; the literal is treated as live data (its length
byte may be changed) but the pinned code is immutable; the Python EGA is not a
display-timing or VGA model; closure coverage above root is inherited.

## Parity review

target_function: U38 public entry 114 `[3134,317A)`.
source_candidates: `overlay_u38_message_3134_cpu.c`, composed with the unchanged
0B0B and 1693 closure.
signals: pinned body hash, entry slot and relocations; gate and range-byte
structure; exact frame and call/return handling; unmasked CPU/RAM/OUT/EGA
differential against the original bytes; native EGA corroboration (below).
verdict: match within the documented entry, memory, device and stop profile.

## Acceptance (reviewer only)

Proposed `partial_callee_models.csv` line, text only:

    OVL_0038_3134,full,docs/re/U38_MESSAGE_3134_CPU.md

Strict credit and the identity decision are the reviewer's.
`game_evidenced`/provisional is the queue's label; this PR changes none of it.
