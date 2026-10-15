# Saved-A file-probe return +19AD caller ABI

This distinct follow-on to [probe PR #440](https://github.com/huntergdavis/poolrad-decomp/pull/440)
implements exactly two original unit-6F caller instructions: `OR AL,AL`
at +19AD and `JE +19EA` at +19AF. Actual returned AL=1 takes the natural
fall-through to +19B1, before allocation setup. AL=0 selects +19EA,
before the loop gate. Both targets block unchanged; no allocation,
record-load, insertion or loop effects are supplied.

Before implementation, bounded `deja "Poolrad 19AD OR AL AL 19AF JE 19EA
19B1 native caller return flags ABI"` expired without reusable output.
Reused repository leads are the
[prior selector return](SLOT_A_NATIVE_19AD_SELECTOR_RETURN.md) and
[first-loop branch observation](SLOT_A_NATIVE_19EA_FIRST_LOOP.md).
The flag convention follows the existing addressed instruction adapters.
Original/loaded bytes and natural full state were checked independently.
This branch starts separately from main b749eb9 and shares no changed
source, header, test or evidence paths with PR #440.
The review branch is restacked onto fresh main
`34fd820547686478fe044bc1d6d5c8e967fa6c80`; its standalone C source and
audited four-byte caller span are unchanged.

## Pinned input and same-run native continuation

| Ignored input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SAVGAMA.DAT | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original and loaded +19AD..19B0, GAME file 1F993..1F996 | 4 | `cb1c7828f77549fd0e0620fbb5a261abb5966a8188dfd3a50e242ef4e08376ca` |
| Raw three-row native transcript | — | `aae3f921ce34bcc4279b68db74a6a3f2aad3188caf80a0529af2769ba73c3a51` |
| Each native 655360-byte RAM image | 655360 | `e8356e1107adcd130ea03a79033ca764cebd67f91375485efe2632c1bddbae77` |

The native session continued directly from the actual +19AD return in
the fresh natural probe capture. No guest register or RAM edits occurred.
The new transcript is the subsequent three-row debugger stream, saved in
the separate ignored root `slot-a-probe-return-19ad-abi-20261014`.
Its first RAM/CPU stop agrees exactly with the preceding probe's return.
The native process was closed after +19B1, without executing allocation.

All sixteen CPU fields and the entire RAM image are checked at every stop.
CS=2A8E, DS=14A0, ES=SS=1D16, FS=GS=0000, SP=37B8, BP=3DCA,
AX=0001, BX=0006, CX=0000, DX=3734, SI=379C and DI=3704 throughout.
At +19AD FLAGS=7206; at +19AF and +19B1 FLAGS=7202. `OR AL,AL`
clears the prior parity bit for AL=1; ZF remains zero, and the JE falls
through. AX, all other registers and all 655360 RAM bytes remain unchanged.
There is no frame push or writer in these two instructions.

## C parity, exhaustive branch inputs and limits

The natural caller pair matches **two C instructions** against original
bytes and **two post-entry native CPU/full-RAM checkpoints**. The three
captured rows comprise one input and those two checkpoints. Synthetic
cases cover all **256 AL values**, each through both instructions and
its blocked target, for **514 total C instruction comparisons** including
the natural pair. Synthetic AH=A5 is retained; incoming CF, OF, AF, ZF
and SF are set to stale values, while alternate cases set DF. The adapter
replaces defined OR flags correctly, preserves unrelated flags including
DF, and does not modify either half of AX. All nonzero AL values select
+19B1, including negative signed-byte values; only zero selects +19EA.

Undefined logic AF is normalized to zero, as observed in this native
case and original-byte Unicorn execution. Its behavior is not promoted
to an architectural guarantee. Every comparison checks sixteen CPU fields
and all RAM bytes. The media-free strict suite and complete differential
also pass ASan/UBSan. Strict checks cover unsupported PCs/segments, both
continuation stops, null arguments/RAM and the image size limit.

The API requires live audited return state and matching original/loaded
code. It supplies no selector return value and does not compose across
PR #440's unsupported Assign/Reset/Close/IOResult effects. It is not a
snapshot bridge. Original executable/media/captures remain ignored; the
merged 0425 timeout guard and immutable PR #410 document are untouched.
Tests allow three API calls/ten seconds per case and 120 seconds per
process with a five-second kill grace. All blocked outcomes preserve CPU
and RAM. No playable-route, game identity or full-C count credit is claimed.

- `target_function`: unit-6F +19AD/+19AF caller test and branch only.
- `source_candidates`: slot_a_probe_return_19ad_abi.c.
- `signals`: four-byte original/loaded identity; natural full-state stops;
  exhaustive AL/flag parity; immutable continuation boundaries.
- `score`: four checked signal categories, not a probability.
- `verdict`: bounded `match` for these two instructions; `unclear` for
  complete file-probe C parity and either continuation's effects.
- `review_notes`: the natural input is the real native return, not a C
  file-I/O result. The zero branch is synthetic original-byte evidence.

Run `bash tests/replay/check_slot_a_probe_return_19ad_abi.sh` for media-free
strict checks. Full private reproduction:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_probe_return_19ad_abi.sh \
  resources/oracle-runs/slot-a-probe-return-19ad-abi-20261014
```

Unicorn 2.1.4 and non-optimized assertions are required. All six files are
new lane-owned paths; shared CMake registration remains with integration.
