# Actual saved-A loader prefix and first file-open gate

The independent adapter executes the actual near CALL at loaded
2A8E:19D7 into unit-6F +0AEC, then the reached nonzero-flag loader setup.
It composes the unchanged SYSTEM string helpers and resident +0425 using
live call frames and real far returns. It stops unchanged before +0C26's
far CALL to original resident 0802:025D, return +0C2B, including that
call's return-frame push. No file-open result, file bytes, allocator result
or continuation snapshot is supplied. This is a bounded prefix, with no
full-C, game identity, playable-route or count credit.

The branch begins at fresh main
217e809448be096fcc48b2413714856f5b165284, after PR #497. Before debugging,
bounded deja "Poolrad 0AEC record loader ABI body 19D7" expired without
reusable output. Reused leads are the
[continuation dependency map](U6F_PARTY_CONTINUATIONS.md),
[earlier allocation-return watch](SLOT_A_NATIVE_19BA_RECORD_RETURN.md),
[loaded loader identity](SLOT_A_NATIVE_SECOND_SLOT_INSERTION.md), and
[approved caller evidence](https://github.com/huntergdavis/poolrad-decomp/blob/b1d313c2e0da75cb51096eaf5a152bc19388a9c9/docs/re/SLOT_A_CALLER_19BA_SETUP.md).
All original/loaded bytes and the new live frame were independently rechecked.
The caller branch and callback-B0B branch are separate review packages;
this branch has no dependency on either unmerged implementation.

## Pinned images and code identity

| Ignored input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original near CALL +19D7..19D9 | 3 | ff1ff561b775606fee8583f4c2dd90dd4a53946eddf634c60474104b122d4907 |
| Original loader +0AEC..0EC3, GAME file 1EAD2..1EEA9 | 984 | 715424407210de3824c3a0218cc29033f04bb357ec718aa24a7995d9452d1e36 |
| Loaded loader 2A8E:0AEC..0EC3 | 984 | 4da70731d0fe4afe0387c8df693aea3420b8c9c1916be920e79f16e04ad41ccc |
| Raw 131-row native transcript | — | facac7976b2a533f95744d59b9ad59da4fcb0236b238bca4a0ee5b7cdd08badb |
| Native +19D7 RAM | 655360 | af272158b4330292fd40626b91b1c428a58059ecabab74bcd19c6af6c040f34d |
| Native +0AEC RAM | 655360 | 94315d738a79183059f2002fae91205092c72bf30954f8c75c09c68ee1872028 |
| Native +0C26 RAM | 655360 | 517973b07ec36bacee930f131a45604a76f49079d6878d9f2c0a584477f7b686 |

All 47 loader far-call segment words relocate by +0824, changing 94
bytes. The complete loaded body has the independently known hash above;
checking that identity does not claim translation of its remainder. Every
captured image matches that loaded body and the original near CALL.
The unchanged resident/string byte-check functions pin +0425, +0356,
B0B/B25/B57/B98/C10/C3B, uppercase helper +182A and Delete/CBC dependencies.
All 131 RAM hashes and transcript-derived sixteen-field CPU values are
checked by the new audit; original media and strings stay private.

## Fresh untouched native path

The ignored slot-a-loader-0aec-prefix-20261014 run loads the pinned saved
A without any guest register/RAM edits. It begins at the actual +19D7
frame after ordinary GetMem returned 449A:0008. It does not use controlled
heap exhaustion, callback outcomes or the disjoint growth result 6652:0008.
The owned process is closed at +0C26 before file-open.

At +19D7, SS:37AA contains seven words:
2A8E, 0001, 0001, 0008, 449A, 3BD4, 1D16. The caller already pushed CS;
the near CALL adds 19DA at SS:37A8. At loader +0AEC the eight words are
19DA, 2A8E, 0001, 0001, 0008, 449A, 3BD4, 1D16. PUSH BP stores 3DCA
at SS:37A6; MOV BP,SP selects that frame. SUB SP,03BA yields SP=33EC
and defined FLAGS=7212. LES DI,[BP+0E] reads the basename pointer 1D16:3BD4.
Both argument low bytes are 1, selecting the two measured nonzero paths.
Zero paths +0B81/+0BE7 stay explicit unsupported-path gates.

The new native watch records all 93 reached caller/loader instruction
starts (including input and final gate), all fifteen starts in the first
B25 call, all twelve starts in the first B0B call, and eleven further
helper-entry checkpoints. This gives 131 ordered full-state rows, with
130 distinct post-input checkpoints. Resident +0425 returns to the actual
+0B2E frame; no returned state is installed by the adapter.

| Boundary | SP | BP | AX | FLAGS |
| --- | --- | --- | --- | --- |
| +19D7 before near CALL | 37AA | 3DCA | 0001 | 7202 |
| +0AEC loader entry | 37A8 | 3DCA | 0001 | 7202 |
| +0AF3 after SUB | 33EC | 37A6 | 0001 | 7212 |
| +0B02 before first B25 | 33E2 | 37A6 | 00FF | 7212 |
| +0B07 after B25 | 33EC | 37A6 | 0008 | 7246 |
| +0B29 before resident +0425 | 33E0 | 37A6 | 000B | 7246 |
| 1026:0425 entry | 33DC | 37A6 | 000B | 7246 |
| +0B2E after resident +0425 | 33E4 | 37A6 | 0008 | 7246 |
| +0C26 before file-open | 33DE | 37A6 | 0000 | 7202 |

At +0C26, CS=2A8E, DS=DX=14A0, ES=SS=1D16, FS=GS=0000,
BX=33E0, CX=0000, SI=3624 and DI=3626. The five words at SS:33DE are
3626, 1D16, 0000, 35EE, 1D16: a file-variable pointer, zero mode word
and filename pointer. The native filename at SS:35EE has length 19 and
SHA-256 f86f598eac842da88485f8bb38d7070ba1543692dc26bacdd5fab15c0bbb2e14
including its Pascal length byte. A separate length-23 string exists at
SS:34ED; it is not the pointer supplied to this first open call. No
filename text, file-variable semantics or file success is inferred.
There are 185 net changed RAM bytes from +19D7 to +0C26.

## C parity and negative gates

The uninterrupted natural run matches **7,227 original instructions**:
92 new caller/loader steps (the near CALL and 91 body instructions), plus
7,135 steps delegated to unchanged helpers. It passes all **130 distinct
post-input native CPU/full-RAM checkpoints**, with no state replacement
between entry and the file-open gate.

Independent synthetic entries exercise AH/DF/stale flags, a wrapped near
frame and BP-relative locals, and own Pascal basename lengths 0/1/255.
Those cases select the first zero-flag path and stop explicitly at +0B81
after verifying the initial copy; they do not claim either zero-path body.
A separate input at the native +0BCB boundary selects and stops at +0BE7.
These inputs are independent cases, never chained into the natural run.
Twenty-nine further original-byte comparisons check SUB borrow/overflow,
zero/sign/parity branch flags, all thirteen loader far-call frames with
SP=0000, and AH preservation in the two reached AL operations.

Overall the complete audit passes **7,408 full-state C/original-byte
instruction comparisons**: 198 new caller/loader comparisons and 7,210
unchanged helper comparisons. Every comparison checks all sixteen CPU
fields and all 655360 RAM bytes. Native states, original execution and C
agree on the complete final pre-open frame and 185 net RAM changes.
No oracle flag masking, allocator-result override or returned-state
injection occurs. Native and synthetic counts are reported separately.

The media-free strict suite checks actual near/far frames, defined SUB
flags, LES word spans, arithmetic wrapping, invalid source/destination
bounds, the complete immutable file-open report, alternate-path and
post-open gates, wrong CS/PC, oversized/missing RAM and null arguments.
New source/tests pass strict C11/Wconversion. Unchanged dependencies use
their established C11 strict warning flags. RAM must be <=1 MiB and each
span must fit one 64 KiB segment; individual address/SP arithmetic wraps
to sixteen bits. State/report/RAM must not overlap. Failed steps preserve
CPU/RAM atomically; null arguments also preserve report.

The media-free strict suite and complete 7408-comparison differential also
pass ASan/UBSan with the same bounds and all native checkpoints.

Each differential has a 200000-instruction/300-second bound and REP
deadline checks. The process guard is 1200 seconds plus five-second kill
grace, with progress every 1000 instructions. Existing 0425 guards and
all imported helper files are unchanged. The immutable PR #410 map,
shared CMake, scoreboard and other lanes' sources are untouched.

- target_function: unit-6F +0AEC reached nonzero setup, actual +19D7
  near-call entry and +0C26 file-open boundary.
- source_candidates: slot_a_loader_0aec_prefix.c with unchanged existing
  resident/SYSTEM helper adapters.
- signals: independently pinned original/loaded code; untouched consecutive
  native full states; real caller/return frames and final writers;
  adversarial arithmetic/wrap/branch/string cases and atomic gates.
- score: four checked signal categories, not a probability.
- verdict: bounded match for the measured loader prefix; unclear for
  file effects, loader remainder, zero-flag paths and continuous saved-A C parity.
- review_notes: all native rows are natural; helper instruction comparisons
  are included explicitly and do not create new helper identity/count credit.

Two disjoint follow-ons remain: bind actual +0C26 -> 0802:025D file-open
ABI/effects before the subsequent +0C44 SYSTEM 14FB read; coordinate the
callback +0064 -> original 0709:0BF5 printer join with the printer owner.
Neither effect is implemented or credited here.

Strict suite without media:

~~~sh
bash tests/replay/check_slot_a_loader_0aec_prefix.sh
~~~

Complete private audit (Unicorn 2.1.4, assertions enabled):

~~~sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" bash tests/replay/check_slot_a_loader_0aec_prefix.sh resources/oracle-runs/slot-a-loader-0aec-prefix-20261014
~~~

For native reproduction, prepare a fresh ignored run with the pinned save
and original media using tools/oracle/prepare_run.sh. Launch the private
heavy debugger with a 1200-second cap. Arm 2A8E:19D7 and :0C26 before
load/A. At each reached caller/loader instruction and listed helper
boundary, record EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS
and MEMDUMPBIN 0000 0000 A0000, preserving each CPU/RAM pair before
resuming. Freeze the raw transcript and close the owned process at +0C26.
All media, filenames, screenshots, RAM and raw debugger streams remain
under ignored resources/.
