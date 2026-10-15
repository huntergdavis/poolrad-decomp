# Actual saved-A allocation-return caller setup

The separate unit-6F adapter translates twelve instructions from loaded
2A8E:19BA through :19D6. It stores actual returned DX:AX in
SS:[BP-1CD]/[BP-1CB], prepares the record-loader arguments and pushes CS.
It blocks unchanged before the near CALL at +19D7 to unit-6F +0AEC,
return +19DA. The return-word push and all record-loader effects remain
outside this adapter. No allocator result is injected into a continuous
replay. No full-C, playable-route, identity or count credit is claimed.

The branch starts at merged main
217e809448be096fcc48b2413714856f5b165284, after PR #497. Before debugging,
bounded deja "Poolrad 19BA 19D7 0AEC" expired without reusable output.
Reused repository leads are the
[earlier native allocation/record return](SLOT_A_NATIVE_19BA_RECORD_RETURN.md)
and [preceding allocation setup](SLOT_A_ALLOCATION_19B1_ABI.md).
Original bytes and every instruction's live CPU/RAM were independently
rechecked. Allocator, real-hook callback and printer adapters are unchanged
and are not linked into this caller adapter.

## Pinned original and untouched native input

| Ignored input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original +19BA..19D9, GAME file 1F9A0..1F9BF | 32 | de51fef7ceacba00f9619b52c0c06c2ae33c04d12f7f27a151c1cee23b4d0e2c |
| Raw thirteen-row native transcript | — | 5829da0b43fd5e4a654190833b6063f390ba2b3fd1ed94dacd8e4405eb5f8948 |

The entire original span, including the gated near CALL, matches loaded
RAM at every captured stop without relocation. It contains twelve translated
instructions plus the blocked CALL. The audit pins all thirteen 655360-byte
RAM images, raw rows and matching sixteen-field CPU JSON records.

The fresh ignored slot-a-caller-19ba-setup-20261014 capture runs the
unaltered saved-A route with no guest RAM/register edits and breaks at
actual GetMem return +19BA: DX:AX=449A:0008. This ordinary allocation
route is separate from controlled heap growth/exhaustion and nonzero
callback cases. All subsequent instruction boundaries are captured in
order in that process, closed at +19D7 before the loader CALL.

Throughout, CS=2A8E, DS=14A0, ES=42AB, SS=1D16, FS=GS=0000,
BP=3DCA, BX=44AC, CX=0008, DX=SI=449A and FLAGS=7202. The table
lists the remaining changing registers and physical RAM writers. Each
listed address pair changes both bytes in this native run.

| Stop before instruction | AX | DI | SP | Writer since preceding stop |
| --- | --- | --- | --- | --- |
| 19BA | 0008 | 0000 | 37B8 | Natural allocation-return input |
| 19BE | 0008 | 0000 | 37B8 | 20D5D..20D5E: pointer offset |
| 19C2 | 0008 | 0000 | 37B8 | 20D5F..20D60: pointer segment |
| 19C6 | 0008 | 3BD4 | 37B8 | None: LEA |
| 19C7 | 0008 | 3BD4 | 37B6 | 20916..20917: PUSH SS |
| 19C8 | 0008 | 3BD4 | 37B4 | 20914..20915: PUSH DI |
| 19CC | 0008 | 3BD4 | 37B2 | 20912..20913: PUSH pointer segment |
| 19D0 | 0008 | 3BD4 | 37B0 | 20910..20911: PUSH pointer offset |
| 19D2 | 0001 | 3BD4 | 37B0 | None: MOV AL,1 preserves AH |
| 19D3 | 0001 | 3BD4 | 37AE | 2090E..2090F: PUSH AX |
| 19D5 | 0001 | 3BD4 | 37AE | None: MOV AL,1 preserves AH |
| 19D6 | 0001 | 3BD4 | 37AC | 2090C..2090D: PUSH AX |
| 19D7 | 0001 | 3BD4 | 37AA | 2090A..2090B: PUSH CS |

There are eighteen changed bytes overall. The pointer locals contain
0008/449A after the ordered stores. The seven words at SS:37AA are
2A8E, 0001, 0001, 0008, 449A, 3BD4, 1D16. Source explicitly pushes CS
before the near CALL; that CS word is included, while return IP 19DA is
not pushed. The immutable dependency report names caller/callee unit 6F,
call +19D7, callee +0AEC, return +19DA and call_is_near=1. No record
bytes, filename text or file effects are published or inferred.

## Differential and strict gates

Natural execution passes twelve C/original-byte comparisons and twelve
distinct post-input native checkpoints. Eight independent synthetic inputs
cover zero/all-bits offsets, preserved AH and stale flags including DF,
wrapping/odd-high SP, wrapping BP-relative addresses and stack writes
that overwrite either pointer local before its later PUSH reads. Each
operand is read at its original instruction, preserving alias effects.
Total: **108 full-state C/original-byte instruction comparisons**, checking
all sixteen CPU fields and 655360 RAM bytes after every instruction.
No mid-case state replacement, allocator return override, oracle flag
masking or loader execution occurs.

Media-free strict tests check ordered stores, AH/flags, the complete gate
report, source/destination aliasing, address wrap, each PUSH's invalid
word span, invalid source words, host bounds, wrong CS/PC, excessive RAM
size, stale reports and null arguments. A word must fit one 64 KiB
segment and ram_size <= 1 MiB. Individual address/SP arithmetic wraps to
sixteen bits. Failed steps preserve CPU/RAM atomically; null arguments
also preserve report. Audited code identity and nonoverlapping
state/report/RAM are caller preconditions.

Strict C11/Wconversion and the complete differential pass ASan/UBSan.
Each case has a thirteen-call/ten-second bound; the process has a sixty-
second guard plus five-second kill grace. The merged 0425 guards,
immutable PR #410 map, CMake, scoreboard and other lane sources are unchanged.

- target_function: unit-6F +19BA..+19D6 and +19D7 near-loader boundary.
- source_candidates: slot_a_caller_19ba_setup.c.
- signals: pinned original/loaded identity; untouched native full states;
  ordered writers/ABI; synthetic alias/wrap/AH/flags and atomic gates.
- score: four checked signal categories, not a probability.
- verdict: bounded match for twelve setup instructions; unclear for loader
  effects and continuous saved-A C parity.
- review_notes: actual ordinary allocation return only; controlled growth
  and nonzero callback states are disjoint and do not supply this input.

Two disjoint follow-ons remain: measure the reached unit-6F +0AEC loader
ABI/body from its actual frame; coordinate callback +0064 -> original
0709:0BF5 printer join with the printer owner. Neither is implemented here.

Strict suite, without media:

~~~sh
bash tests/replay/check_slot_a_caller_19ba_setup.sh
~~~

Complete private audit (Unicorn 2.1.4, assertions enabled):

~~~sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_caller_19ba_setup.sh \
  resources/oracle-runs/slot-a-caller-19ba-setup-20261014
~~~

For native reproduction, prepare a new ignored run with the pinned save
and originals using tools/oracle/prepare_run.sh. Launch the private heavy
debugger with a 1200-second cap. Arm 2A8E:19BA and :19D7 before load/A.
At each listed boundary, record EV CS IP DS ES FS GS SS SP BP AX BX CX
DX SI DI FLAGS and MEMDUMPBIN 0000 0000 A0000, preserving CPU/RAM
before resuming. Freeze the transcript and close the owned process at
:19D7. Media, captures and raw streams stay under ignored resources/.

## Per-stop private RAM digests

| Stop | SHA-256 |
| --- | --- |
| 19BA | 7efd3b6a696f7072f9e767beda9037dc1704ce64e6cd3103fdb9063a877cc9de |
| 19BE | bd0e356328cfee9bb26388be7ef12ac4dcbc80c791b5b27281686ee53b6f212a |
| 19C2 | b27119f2f92e72bde24aa1d0061e3a90e7f3b0829eb235e5c89d138547ca81a1 |
| 19C6 | b27119f2f92e72bde24aa1d0061e3a90e7f3b0829eb235e5c89d138547ca81a1 |
| 19C7 | 90929e5e1f6870616ddb88baf9a82ce079513a82f506e9f6c330a5433e58f2a3 |
| 19C8 | 2d9ba43b34db8f8f0a7af9b826542b62f77eb08e4ac59573f91ae75a460c19e2 |
| 19CC | 9c3f6676845db74bfd3bfc85249b4226d5b0bfdaefb745eb6d11b71a2b157320 |
| 19D0 | 9d6ba8241366332e60192eb36df301ef586a088ecd79759e1b2189e503084b49 |
| 19D2 | 9d6ba8241366332e60192eb36df301ef586a088ecd79759e1b2189e503084b49 |
| 19D3 | 8cee4f03d458ead70183281c14333d3e7d780a48262cc8bf2ce09c1067c486e4 |
| 19D5 | 8cee4f03d458ead70183281c14333d3e7d780a48262cc8bf2ce09c1067c486e4 |
| 19D6 | f45b1a2af471dd032cb9c3471adbb7f53991d440ce13483a86208510dc4b7a9a |
| 19D7 | c5c02ac4b727ef2f0c5f0243ca6f18cb707fbf8a66bd8547e92e78da2dd512e4 |
