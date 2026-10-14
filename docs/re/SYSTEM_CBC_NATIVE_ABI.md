# SYSTEM CBC Delete complete-body native ABI

The normal saved-A instruction adapter now executes the resident +03FC
far CALL, all 43 original SYSTEM CBC instruction boundaries, its two B57
substring calls, B98 concatenation, B25 assignment, and the actual RETF 8.
It computes CPU, flags, stack and RAM effects from live state. Native
snapshots remain assertions and never supply C results. The historical
prefix-only entry point preserves the pre-Delete gate for the PR #410 audit.

This is a distinct follow-on to merged B98 PR #419. Before implementation,
a bounded `deja` search for SYSTEM CBC Delete, its signed count, B57/B98/B25
composition and native frame expired without reusable output. Reused
repository evidence is the [PR #410 controlled ABI map](SLOT_A_DELETE_ABI_MAP.md),
the [0356 instruction adapter](SLOT_A_STRING_0356_ABI.md), and the
[B98 native ABI](SYSTEM_B98_NATIVE_ABI.md). The #410 document is unchanged;
this report supplies the separate complete-body evidence it required.

## Original code and controlled native state

Full START.EXE, GAME.OVR and saved-A hashes remain pinned by the inherited
input audit. Original SYSTEM AF8:CBC, loaded 131C:CBC, is the 86-byte span
at START file B932..B987 with SHA-256
`14be20a0c6b74485ec3dcc683266dc07bc2c6e130d9ddcb2f83434b926996a33`.
Original and loaded bodies agree in all eighteen native RAM images.
The original/relocated resident +03FC call and B57, B98, B25 and C10 bodies
are independently hash-checked before replay.

The ignored #410 capture has eighteen full CPU/RAM rows: the natural
+196A input, its controlled counterpart, resident +03FC, and fifteen
post-call checkpoints through resident +0401. Only five bytes differ
between the natural and controlled inputs, within the six requested
synthetic source/needle bytes. CPU and every other RAM byte are unchanged.
The capture is controlled Delete reachability, not an ordinary saved-A
Delete execution. The natural source pointer is 3C82 at the input; the
later nested source pointer is SS:36A2.

| Evidence | SHA-256 |
| --- | --- |
| Raw eighteen-row native transcript | `ac6e7f6e47fb8e1758687e20b90737dee67e2d4ad54c6a11176b5d018ba53c30` |
| Resident +03FC RAM | `ffed4537737c02607b84c395b070d75feb0001670518ed85a2a2a73a0d1a8501` |
| SYSTEM CBC entry RAM | `0413cdde392f828e2c92cdcc55c249695c63e1bab5ccde3efaf288be83505a20` |
| Resident +0401 RAM | `9b9098f7607d4915d69cdf9b6a24b7e422b9390f186f598a8070ef4dbe62488c` |

At +03FC, CS:IP=1026:03FC, SS:SP=1D16:3496, BP=37A2,
AX=0001, BX=3492, CX=0000, DX=DS=14A0, SI=34A0, DI=36A2,
ES=SS, FS=GS=0000, FLAGS=7246. Arguments are count=1, index=1,
source=1D16:36A2. The real far CALL pushes 1026:0401 at SS:3492;
the prologue establishes BP=3490 and SP=3290, with prefix/suffix
scratch buffers at SS:3390 and SS:3290.

The first B57 call has SP=3280, return 131C:0CE1, count=0 and index=1;
the second has SP=327C, return 131C:0CFB, count=255 and index=2.
Both read SS:36A2. B98 at SP=3284 returns to 131C:0CFF, appending
SS:3290 to SS:3390. B25 at SP=3282 returns to 131C:0D0C and copies
SS:3390 to SS:36A2 with capacity 255. Each near CALL plus PUSH CS
constructs the actual far return frame consumed by its helper.

RETF 8 returns to resident +0401 with SP=349E, BP=37A2, AX=0002,
BX=3282, CX=0000, DX=14A0, SI=3393, DI=36A5, FLAGS=7246;
segment registers are unchanged from their expected final values.
The target length writer changes physical 20802; the final forward copy
changes 20803 and 20804. A trailing old byte is retained. The complete
+03FC-to-+0401 net change is 31 RAM bytes, including stack and scratch.
No original payload or private capture is published.

## Complete-body and composition gates

The independent CBC API first passed **125 C instruction comparisons**
against original-byte Unicorn execution and **15 native post-call
CPU/full-RAM checkpoints**. Thirteen synthetic cases then passed, including
signed counts 0000, FFFF and 8000. Those three take eight instructions,
return without dereferencing an intentionally invalid source pointer, and
exercise the signed JLE path. Positive-count cases cover middle deletion,
suffix truncation, an index past the end, index zero, the signed index
boundary, 16-bit index addition wrap, maximum-length strings, deleting the
last maximum-length character, and both scratch-buffer source aliases.

The unchanged historical eighteen-stop ABI audit passed before enabling
the normal adapter's Delete dispatch. The normal composed API subsequently
matched **266 C instructions** from controlled +196A through +0401 and
**16 post-controlled-entry native CPU/full-RAM checkpoints**, including
+03FC. The fifteen CBC checkpoints are a subset of these sixteen, not
additional independent stops. Each instruction compares all sixteen CPU
fields and all 655360 RAM bytes. The thirteen synthetic cases total 1239
instructions; unified, standalone and synthetic runs total **1630** C
instruction comparisons. The same complete suite passed ASan/UBSan.

The normal 0356 adapter continues after +0401 through its existing code to
the explicit +196F -> 0802:0425 ABI gate. Its separate 0425 adapter reaches
the +1974 continuation gate. The continuation adapter still gates at
+1999; the separate merged B98 caller adapter reaches +19A8 and blocks
unchanged before 0802:02EB, return +19AD. This PR supplies no file-probe
effects or dispatch connecting those later gates.

Media-free strict checks cover failed source/frame/stack spans, delegated
copy spans, invalid instruction boundaries, null state/RAM and oversized
RAM. Unsupported operations preserve CPU/RAM at the current instruction.
Pointer arithmetic wraps to sixteen bits; host spans must fit one 64 KiB
segment and the RAM image <= 1 MiB. Multi-byte segment/A20 wrap and DF-set
string execution outside the reached CLD remain unsupported. Code identity
is an audited input prerequisite, not a decoder implemented by the C API.

The existing focused runners link the new CBC/B98 dependencies. Historical
Delete audit calls the explicitly named prefix API; its old map and stop
remain reproducible. Updated synthetic match/empty-needle cases now finish
Delete and reach their respective later gates. CMake registration remains
with the integration owner; these replay sources had no shared registration.

The complete 0425 regression exited successfully: one natural and seven
synthetic cases, **8/8 complete and zero pending**, totaling **50965**
instruction comparisons. Natural execution contributes 6968 instructions
and three post-entry native checkpoints at 1026:0425, 1026:0426 and
2A8E:1974. Synthetic counts are empty source 1532, ASCII boundaries 6981,
prefix truncation 11614, source/output alias 8974, binary table miss 5612,
composed Delete 3760, and live table/local alias 5524. No loop bound failed.
The original run was not interrupted; its output was buffered through a
pipe. Per-case wall times were not captured, so no particular case is
identified as the cause of the elapsed time.

The audit now flushes case starts, every thousand-step counter and results.
Each case has a default 300-second deadline, checked at C instruction
boundaries and during emulator REP iterations; the existing 200000 C-step
and 65537 REP-iteration limits also remain. The runner adds a 1200-second
process timeout, followed by forced termination after five seconds if
needed. `POOLRAD_0425_CASE_TIMEOUT` and `POOLRAD_0425_TOTAL_TIMEOUT` may
adjust those seconds. Expiry fails the audit and supplies no parity credit.
The focused composed-Delete case is rechecked under these guards; the
full eight-case result above uses identical runtime C sources.

- `target_function`: complete SYSTEM CBC Delete body, dependencies and the
  measured resident +03FC far-call/return frame.
- `source_candidates`: system_cbc_abi.c and the composed 0356 entry point.
- `signals`: original/loaded byte identity; controlled native full state;
  instruction/flag/writer parity; signed-count and alias cases; atomic bounds.
- `score`: five checked signal categories, not a probability.
- `verdict`: bounded `match` for audited original-byte and controlled native
  behavior; `unclear` for library/game ownership and ordinary saved-A Delete
  reachability. Zero game identity/full-C count or playable-route credit.
- `review_notes`: eighteen captured rows include the input edit pair;
  sixteen are post-controlled-entry C checkpoints. Positive Delete behavior
  is intentionally controlled evidence; negative counts are synthetic.

Run `bash tests/replay/check_system_cbc_abi.sh` without media for strict C
checks. Full private reproduction:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_system_cbc_abi.sh \
  resources/oracle-runs/slot-a-delete-abi-20261014
```

Private inputs remain ignored and hash-validated without modification.
The audit requires Unicorn 2.1.4 and rejects optimized Python assertions.
