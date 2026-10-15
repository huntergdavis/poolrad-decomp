# Actual saved-A +025D and Assign ABI through the Reset gate

The separate adapter enters original resident 0802:025D from the actual
loaded 2A8E:0C26 far CALL, copies both retained string arguments, and
executes its reached near CALL to resident +02EB. It composes the existing
probe/B25 instructions, implements the actual +0310 far CALL and SYSTEM
AF8:13E2 Assign memory writes, and consumes the real RETF 8 frame.
It stops unchanged before resident +031F -> SYSTEM AF8:1410 Reset,
return +0324, including that far-frame push. No DOS call, file-open result,
file read, returned probe AL or mid-case snapshot replacement is supplied.
No full-C, game identity, playable-route or count credit is claimed.

This fresh local branch starts at main
217e809448be096fcc48b2413714856f5b165284. Before debugging,
deja "Poolrad resident 025D file open ABI C26 14A8 DOS" found no indexed
match. Reused leads are the
[existing bounded nested-open model](RESIDENT_DAX_NESTED_OPEN.md),
[probe ABI](SLOT_A_FILE_PROBE_02EB_ABI.md), and
[corrected loader frame](https://github.com/huntergdavis/poolrad-decomp/blob/304e672180d35111330266d108a5f11c88d4044d/docs/re/SLOT_A_LOADER_0AEC_PREFIX.md).
Bytes, argument roles, CPU/RAM and writers were independently rechecked.
The existing probe API keeps its Assign/Reset/Close/IOResult gates;
this separate entry point executes only the measured Assign composition.

## Pinned media and original code

| Ignored input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Resident +025D..02EA, START file 82B4..8341 | 142 | eeea6613012d3db99c7ad85788fdd3ff5b9888d7cc6701e9ce7a612195210dc6 |
| Resident +02EB..0355, START file 8342..83AC | 107 | 141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474 |
| Assign +13E2..140F, START file C058..C085 | 46 | 94e00b1b9e01bc846824e0f7eb23bc8d3c80c28b089995524ce99045af95ffa7 |
| Raw 91-row native transcript | — | d3c2758bed18bfb91ca2212f96a49fefd77819758880ee3425595473240af1d3 |
| Final native +031F RAM | 655360 | ae69a61ea0d7b30e2bd218f0d0bb57393d7796965812a0208e5a7e7fc8564f45 |
| Native assigned temporary record | 128 | 3a79efb3f12aca41c6414e392e436b9b92d203d962798973e43ae490df7dbd8c |

The audit checks every loaded resident image against the original with
its four/five SYSTEM segment relocations. Assign has no relocated words;
all 46 original bytes match loaded 131C:13E2..140F in every snapshot.
The original/loaded +0C26 call and existing B25 body are checked separately.
Hashing whole resident spans does not claim their unexecuted remainder.
All 91 RAM digests and transcript-derived sixteen-field CPU JSON records
are pinned in the audit; media, strings and captures remain private.

## Fresh untouched native call and memory effects

The ignored slot-a-open-025d-assign-20261014 run loads saved A without
any guest CPU/RAM edits. It stops at the actual +0C26 frame, then captures
ninety consecutive post-input full-state checkpoints through nested
Assign and its real return. The owned process closes at +031F before
Reset. This input follows the ordinary native allocation route, not a
controlled callback result or a translated allocator/probe bridge.

The complete pre-call frame at SS:33DE is seven words:
3626, 1D16, 0000, 35EE, 1D16, 34ED, 1D16. The final two words are a
retained string pointer. After far CALL/PUSH BP, resident BP=33D8.
The first LES at +0263 reads [BP+10] -> 1D16:34ED, candidate length 23;
the second at +0276 reads [BP+0C] -> 1D16:35EE, extension/error length 19.
Their capped copies occupy SS:33AE and SS:3385 respectively. This fresh
native observation confirms both argument roles in the corrected loader
note. Mode [BP+0A] is zero and [BP+06] names caller file variable 1D16:3626.
That caller variable is not initialized by the first nested Assign.

The near CALL +028F -> +02EB returns to +0292, but C never reaches that
return because Reset is blocked inside the selector. The nested frame
has BP=337A and temporary record 1D16:32D0. The actual Assign far-call
frame is at SS:32C4 with return 1026:0315, source 1D16:3350 and destination
1D16:32D0. Assign saves DS=14A0 at SS:32C2 before loading live source/dest
pointers. Its ordered stores write leading word 0000, word D7B0, twenty-two
zero words, twenty-three name bytes at record offset 30h and a terminating
zero. It touches 72 record bytes; the remaining 56 bytes stay unchanged.
There are 44 net changed record bytes and two changed saved-DS stack bytes
at physical 20422/20423: 46 net Assign entry-to-return RAM changes.
The full +0C26 -> +031F prefix has 144 net changed RAM bytes.

After Assign RETF 8, CS:IP=1026:0315, SP=32D0, BP=337A, AX=CX=0000,
DS=DX=14A0, ES=SS=1D16, BX=32C4, SI=3368, DI=3318, FLAGS=7246,
FS=GS=0000. At the final +031F gate, SP=32CA, AX=0080 and DI=32D0;
the other fields are unchanged. The three argument words are
0080, 32D0, 1D16: size 128 and the initialized temporary record pointer.
The typed gate reports original caller 0802:+031F, callee AF8:1410 and
return +0324. No DOS handle, error code or successful file open is inferred.

## Parity, adversarial cases and bounds

The uninterrupted natural path passes **118 C/original-byte instruction
comparisons**, comprising 50 new entry/resident/Assign instructions and
68 unchanged probe/B25 instructions. It matches all **90 distinct
post-input native checkpoints** from the 91 untouched rows. No in-flight
CPU/RAM replacement occurs between +0C26 and the Reset gate.

Synthetic full-prefix entries cover incoming AH/DF/stale flags and own
candidate lengths 0/1/40/255, preserving the two distinct argument roles
and exercising the 40-byte copy cap. Independent Assign-entry cases cover
lengths 0/79/80/255, its 79-byte clamp, wrapped stack/return frames, source
bytes cleared by the header stores, and source/destination aliasing.
An independent REP-boundary case proves forward overlap propagation.
These independent inputs are not linked together or installed into the
natural run; Assign-only cases stop at their real +0315 return before
executing another instruction.

Total: **870 full-state C/original-byte comparisons**, consisting of
461 new-code and 409 unchanged helper comparisons. Every comparison
checks all sixteen CPU fields and all 655360 RAM bytes. Strict new-source/
test C11/Wconversion checks, media-free bounds tests and the complete
ASan/UBSan differential pass. Existing helper sources retain their
established strict flags and remain unchanged.

Strict tests preserve the old probe's Assign gate and check the new actual
far frames, cross-segment/truncated LES and word spans, whole-REP preflight,
DF-set rejection, real-return spans, immutable Reset report, post-probe and
wrong-PC gates, invalid RAM sizes and null arguments. Spans must fit one
64 KiB segment and ram_size <= 1 MiB; individual address/SP arithmetic
wraps to sixteen bits. State/report/RAM must not overlap. Failed steps
preserve CPU/RAM at the current instruction; null arguments also preserve
report. Reached CLD and existing AF=0 logic rules are preserved without
oracle flag masking.

Cases have a 300-instruction/ten-second bound; the process has a
120-second guard plus five-second kill grace. Existing 0425 guards,
immutable PR #410 map, CMake, scoreboard, key/printer sources and the
bounded value-model files are unchanged. The six new files are an
independent replay adapter, not a replacement for those models or gates.

- target_function: actual unit-6F +0C26 call, resident +025D initial copies
  and nested +02EB prefix, plus reached SYSTEM Assign +13E2..140D.
- source_candidates: slot_a_open_025d_assign_abi.c with unchanged probe/B25 helpers.
- signals: original/loaded identity; untouched complete native states and
  retained arguments; ordered record writers/real return; clamp/wrap/alias
  cases and atomic unsupported-effect gates.
- score: four checked signal categories, not a probability.
- verdict: bounded match through the first Reset boundary; unclear for
  Reset/DOS, outer +025D continuation and continuous saved-A C parity.
- review_notes: Assign modifies the nested temporary record only; successful
  opening, original caller-file state and record content are not supplied.

Two disjoint follow-ons remain: actual +031F -> SYSTEM AF8:1410 Reset and
its DOS/open/error effects; callback +0064 -> original 0709:0BF5 printer
join coordinated with its owner. Neither is implemented or credited here.

Strict media-free check:

~~~sh
bash tests/replay/check_slot_a_open_025d_assign_abi.sh
~~~

Complete private audit (Unicorn 2.1.4, assertions enabled):

~~~sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" bash tests/replay/check_slot_a_open_025d_assign_abi.sh resources/oracle-runs/slot-a-open-025d-assign-20261014
~~~

For native reproduction, prepare a fresh ignored run with pinned save and
originals. Launch the private heavy debugger with a 1200-second cap and
arm 2A8E:0C26 before load/A. After reaching that actual caller frame,
retain 1026:031F as a safety breakpoint. Record EV CS IP DS ES FS GS SS
SP BP AX BX CX DX SI DI FLAGS and MEMDUMPBIN 0000 0000 A0000 at the
listed resident, B25 and Assign instruction boundaries. Preserve each
pair before resuming, freeze the raw transcript and close only the owned
process at +031F. All media, strings, screenshots, RAM and streams stay
under ignored resources/.
