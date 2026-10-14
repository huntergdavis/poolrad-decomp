# SYSTEM B98 concatenation native CPU/flags/RAM ABI

The new addressed C helper implements original SYSTEM AF8:B98, loaded
131C:B98, including length carry/clamping, actual alias order, pointer
arithmetic flags, forward REP MOVSB, DS restoration, and RETF 4. The saved
caller adapter composes the reached unit-6F +1999 and +19A3 calls and their
three intervening instructions. It stops unchanged at +19A8 before original
0802:02EB (loaded 1026:02EB), reporting return +19AD. No file-probe effect
or route beyond that measured boundary is supplied.

This distinct branch starts after PR #410 at main 513e93f. Before
reimplementation, bounded `deja "Poolrad SYSTEM AF8 B98 concat register
flags RAM alias overflow native 1999 CBC"` expired without reusable output.
Reused repository evidence is the immutable
[controlled Delete ABI map](SLOT_A_DELETE_ABI_MAP.md),
[+1974 continuation](SLOT_A_CONTINUATION_1974.md), and
[prior filename observation](SLOT_A_NATIVE_1983_FILENAME.md). Those supplied
addresses and frames to recheck. The value-only filename/string code is
never called. Bounded span and flag primitives are adapted from the
[merged 0356 instruction adapter](SLOT_A_STRING_0356_ABI.md); B98's own
instruction effects are checked separately. PR #410's documentation and existing Delete gate are unchanged.

## Original and fresh natural evidence

The same full START.EXE/GAME.OVR/save hashes as the preceding joins are
pinned. Original B98 START file B80E..B839 is 44 bytes, SHA-256
`c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64`.
All nineteen instruction boundaries are translated. Its loaded bytes are
identical to original input at every native stop. The 20-byte unit-6F
+1999..19AC span, including the gated file-probe call, hashes
`91bd3d3e5018ab2f3ee5b3a2cb0c32eb12a75e87b2ed8b186aa5915015907407`;
loaded span hash is
`1b8536d59827bf0c2a130552afc944077464d9c88058af2a16c95530702c9289`.
Exactly two SYSTEM segment words relocate AF8 to 131C and the probe's
resident word relocates 0802 to 1026.

The new ignored `slot-a-b98-abi-20261014` capture follows the ordinary
saved-A load without any guest memory/register edits. Sixteen complete
EV/RAM rows include both call entries, length additions, copies, returns,
and one partial REP iteration. All sixteen CPU fields and the full
655360-byte RAM images are pinned, along with the raw transcript.

| Ignored input/evidence | SHA-256 |
| --- | --- |
| Natural +1999 RAM | `6cc4831fcbb883b06fb20e7082b0452f50e7c7adb96cf4fffdda4020eecb1f01` |
| Natural +199E RAM | `525e143dab0072dbf7e9bf501af465f43b603232c234c3a45ef0e37f50eb0fc0` |
| Natural +19A8 RAM | `097e65f50e4c2066db2353e291b0ea2b751d59a001fc2b60ad4537cb23b904ec` |
| Raw sixteen-row transcript | `04ae0e9fd46294b29e20d6c4cae16bb19d33d99e65dc2c884599301db496db12` |

At first B98 entry SS:SP=1D16:37AC, the return is 2A8E:199E, source is
1D16:3BD4, destination is 1D16:3AB8. The second entry has the same SP,
return 2A8E:19A8, source 2A8E:15E9, and the same destination. RETF 4
consumes source, retaining the destination far pointer. At +19A8 SP=37B4,
BP=3DCA, AX=0004, BX=37AC, CX=0000, DX=DS=14A0, SI=15EE,
DI=3AD0, ES=SS=1D16, FS=GS=0000, FLAGS=7206, CS:IP=2A8E:19A8.
All fields are compared independently, including intermediate flags.

The first ADD at B98:+BAB changes only physical 20C18, increasing the
private Pascal length from 11 to 19. Its REP changes 20C24..20C2B. The
second length ADD changes the same length byte to 23. Complete
+1999-to-+19A8 net RAM change is 16 bytes, including actual stack frames.
No original payload is published.

The heavy debugger re-stopped at +BBD after one REP iteration: CX 8 -> 7,
SI 3BD5 -> 3BD6, DI 3AC4 -> 3AC5, exactly byte 20C24 changed. That row is
retained explicitly. Atomic C REP completes all iterations before C/Unicorn
comparison; the partial native row is checked separately against emulator
iteration state and is not counted as a C instruction-boundary match.

## C parity, alias behavior, and limits

The natural two-call composition matches **37 C instructions**, every CPU
field and full RAM after each instruction, plus **14 post-entry native C
checkpoints**. The separate original-byte partial REP comparison also
passes. From PR #410's actual nested Delete B98 entry, **16 C instructions**
match original bytes and its native 131C:CFF return CPU/RAM exactly. This
implements that helper's effects, not the surrounding Delete body.

Ten synthetic original-byte cases cover empty strings, exact capacity,
overflow, full destination, self-aliasing, forward/backward overlapping
copies, distinct-segment aliases, zero-count pointer wrap, and pointer ADD
carry preserved through INC. Both overflow branches are exercised. Together
with both native contexts, **219 C instruction steps** match Unicorn 2.1.4
CPU/full-RAM execution. Native snapshots supply assertions only and never
inject results or journals into C. The complete strict suite and all 219
instruction comparisons also passed with ASan/UBSan instrumentation.

The adapter reads lengths and payload in original order. It writes the
new destination length before REP; overflow clamps length to FF and copies
only FF minus the original destination length. REP uses forward iteration,
including reads of memory modified by earlier overlapping writes. The
native source/destination frames and aliases are not flattened into copied
host strings. Defined byte/word arithmetic flags are calculated; undefined
XOR AF is normalized to observed zero.

Strict C tests check immutable probe reports, stale dependencies, failed
far operands/call/return frames, failed REP spans, zero-count REP, DF-set
string steps outside reached CLD, and null arguments. Spans must fit the
host image <= 1 MiB and one 64 KiB segment; unsupported spans block
atomically at the current instruction. Individual pointer arithmetic wraps
to sixteen bits. Multi-byte segment/A20 wrap spans remain unsupported.

Delete remains blocked in its original adapter until its complete B57/B98/
B25 composition and controlled native audit pass in a separate PR. B98
runtime provenance remains unresolved library/game ownership. No game
identity/full-C count or playable-route credit is claimed.

- `target_function`: SYSTEM B98 effects plus the measured +1999/+19A3
  caller sequence through the +19A8 probe gate.
- `source_candidates`: system_b98_abi.c and its helper/caller APIs.
- `signals`: original/loaded byte identity and relocation; natural same-run
  full-state pair; nested Delete frame; instruction/native writer parity;
  alias/overflow/host-boundary checks.
- `score`: five checked signal categories, not a probability.
- `verdict`: bounded `match` for the measured paths and tested original-byte
  cases; `unclear` for library/game identity and unexecuted file-probe/route.
- `review_notes`: sixteen fresh rows include one emulator-only comparison
  at a partial REP iteration. The older Delete row is controlled evidence;
  its parent C Delete implementation is not inferred from this helper.

Run `bash tests/replay/check_system_b98_abi.sh` without media for strict C
checks. Full private reproduction:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_system_b98_abi.sh \
  resources/oracle-runs/slot-a-b98-abi-20261014 \
  resources/oracle-runs/slot-a-delete-abi-20261014
```

Both native roots stay ignored. The old #410 root is hash-validated without
modification. Optimized Python assertions are rejected. CMake registration
remains with the integration owner.
