# Saved-A resident +0356 CPU/stack/string ABI join

`pr_slot_a_string_0356_step` starts at the already reached native call
`2A8E:196A -> resident 1026:0356` and models its measured route through
return to `2A8E:196F`. It translates addressed instructions in the resident
body and its reached SYSTEM Copy, substring, and compare helpers. It does
not call or infer register/memory behavior from the existing value-only
`pr_rec_res_0802_0356` implementation.

The measured saved-A route performs no Delete. A reached resident call at
`+03FC -> SYSTEM 0AF8:0CBC` therefore returns typed **BLOCKED_DELETE** before
its far-call frame is pushed. On the measured return, the next caller at
`+196F -> resident 0802:0425` returns typed **BLOCKED_0425_ABI**, also before
any CPU/RAM change. These dependencies use original code addresses; the
runtime relocations are validated separately.

## Recall, ownership, and native inputs

PR #381 was merged at main `1e0b2dd`. This distinct branch
`codex/saved-string-0356-abi` started there and was rebased to main `280dee6`
before review. Before implementation,
`timeout 10s deja "poolrad resident 0802 0356 native CPU RAM 196A 196F ABI Unicorn"`
expired; no prior-session result was reused. Reused repository evidence:
[the actual native resident call/return and controlled writer trace](SLOT_A_NATIVE_U6F_196A_RESIDENT_RETURN.md),
[the previous +1965 ABI join](SLOT_A_SAVED_COPY_1965_JOIN.md), and
[the resident source/ABI inventory](RESIDENT_STRING_0356.md).
The latter supplied addresses to recheck, not a substitute for instruction
or native-state comparison. Key dispatch, art, UI, resident value-only code,
and shared CMake files are not edited.

Actual live CPU and full-RAM captures were copied from ignored run
`slot-a-native-u6f-196a-048`; no new native capture was needed. The original
live second caller and return are distinguished from the independently
restored same-state controlled writer trace. The existing native checker
verifies their identity, restoration equality, argument frames and deltas.

| Ignored native evidence | Bytes | SHA-256 |
| --- | ---: | --- |
| Caller `+196A` RAM | 655360 | `4e25d55dfe20735950fb26c9115b5ec2d8d59990e1b0c62dc4b4173207c17f17` |
| Return `+196F` RAM | 655360 | `1baad8fc261b2bbf80d5434e34ea1637e2c2ced7985aff77a706deb113cbe816` |
| Live numeric CPU transcript | pinned | `1ec062b5e5bddd5e7811133516a723d1d2bec9a0c68fe9bd02933185ce768309` |
| Controlled numeric CPU transcript | pinned | `2e12550d09d8d9048a810a4ea7bbb940965eb9dc2f2ffecef58a0337cb7baeaf` |

The host also pins START.EXE (65231 bytes) to
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR (206105 bytes) to
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and SAVGAMA.DAT (13137 bytes) to
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

## Original bytes and callback frames

| Original span | Size | SHA-256 |
| --- | ---: | --- |
| Caller `GAME.OVR[1F950..1F954]`, `+196A` | 5 | `965606c688f124e8166f41f356e580d9d2d0897521f1a7db054b55733834822d` |
| Resident `START.EXE[083AD..0847B]`, `0802:0356..0424` | 207 / 82 instructions | `ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889` |
| SYSTEM `0B25` Copy | 36 / 16 instructions | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |
| SYSTEM `0B57` substring | 65 / 29 instructions | `18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa` |
| SYSTEM `0C10` compare | 43 / 20 instructions | `11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58` |

The resident's loaded 207-byte hash is
`2d2a3260b63ef4f99decb51f44ec543a83d5c1ec4f8c854da949a376e56b722e`.
Exactly six far-call segment words relocate original `0AF8` to `131C`.
All three reached SYSTEM bodies are byte-identical between original input
and loaded RAM. The caller targets original `0802:0356`, loaded `1026:0356`.
Thus the differential executes hash-linked original instructions, including
their documented relocations, rather than synthetic replacement code.

At native `SS:SP=1D16:37A8`, the three far pointers are needle
`1D16:3BB6`, selected source `1D16:3CAB`, and output `1D16:3AB6`.
The call pushes return `2A8E:196F`; resident PUSH BP and SUB SP,0304
create the local frame. SYSTEM calls receive and consume the actual stack
arguments: Copy uses cap/destination/source with RETF 10; substring uses
count/index/source with RETF 8, retaining destination; compare consumes
its two pointers with RETF 8. No callback journal or expected output state
is injected: these runtime bodies execute as C instruction translations.

The resident returns with SP=`37B0`, retaining output `1D16:3AB6` for the
next call. All 16 captured CPU fields and all 655360 RAM bytes agree with
the computed C state at that boundary; the complete net RAM delta remains
the native trace's 30 bytes. String payloads are never printed or committed.

## Differential and explicit limits

Unicorn **2.1.4** executes the hash-verified loaded original instructions
from the same captured CPU/RAM. After each C instruction step, the checker
compares every CPU field and the entire RAM image with Unicorn. REP is one
atomic C step; Unicorn advances through all iterations of that instruction
before comparison. The measured path completes **621 C instruction steps**
and matches **13 live/controlled native CPU/full-RAM checkpoints**, including
the actual return. Expected snapshots supply comparisons only, never output
registers, writes, callback journals, or string results to the adapter.

Five synthetic original-byte differentials cover a longer no-match source,
a longer needle, an empty source, an actual-match Delete boundary, and an
empty-needle Delete boundary. The first three return to the typed +0425
blocker; the last two match original CPU/RAM up to the unsupported Delete
call and block there unchanged. Synthetic caller frames are explicitly
separate from native-route inputs.

The strict C suite additionally checks unchanged typed blockers, stale
dependency invalidation, incomplete far-pointer reads, failed call/REP
spans, forward overlap, zero-count REP and outside-slice DF. Host spans must
fit the supplied RAM image and one 64 KiB segment. Unsupported spans block
before instruction effects, including all of REP. Individual pointer
increments retain 16-bit wrapping; multi-byte segment/A20 wrap spans are
outside this adapter. Logic-operation AF is undefined in x86 and is
normalized to the measured zero; defined arithmetic flags are computed.
The strict suite and the complete native/synthetic differential also passed
with the C adapter instrumented by AddressSanitizer and UBSan.

The instruction tables include 82 resident and 65 reached-helper code
instructions; this does not mean every branch has native evidence or that
Delete is implemented. Caller/helper addresses use this measured relocation.
The host must check code identity before supplying another snapshot.

## Next dependency and checker verdict

The next original caller `+196F -> 0802:0425` has five-byte SHA-256
`fe81385603a3fc55df47de14941cf1bdbf72d0bd7c936edaecb4ceda0c23d52e`;
loaded `+196F -> 1026:0425` hashes
`78cec5fcdde872ab5e4c4add6d5debd1f4cf3d3430f3e12b1a051c4ce061d147`.
The differential verifies both before accepting the typed report's callee
and return `+1974`. The retained output pointer and live conversion table
are inputs to that next ABI; its existing value-only C implementation does
not supply its complete native CPU/stack effects. The alternate `+03FC`
Delete dependency remains separate and explicit.

Upstream native save-close and loader effects remain prerequisites for
reaching this input boundary from a raw save. No caller frame is constructed
from saved coordinates or a desired tuple. This is neither playable one-Up
nor a new strict full-C/game-routine identity.

- `target_function`: measured unit-6F `+196A` resident-call ABI through
  native return `+196F`, including reached SYSTEM instruction bodies.
- `source_candidates`: new `slot_a_string_0356_abi.c` and addressed tables.
- `signals`: complete source/loaded byte identity with documented relocation;
  actual native arguments and callback frames; instruction-level original
  Unicorn/C CPU and RAM differential; live and controlled native endpoints;
  immutable unsupported-branch and host-boundary checks.
- `score`: five checked signal categories, not a probability.
- `verdict`: `match` for the measured path and tested prefixes; `unclear`
  for full runtime behavior, unimplemented Delete, and unsupported spans.
- `review_notes`: the existing string-value helper is never invoked. Native
  and controlled checkpoints share a restored starting state and are not
  independent end-to-end playthroughs. Source identity alone gives no full-C
  credit, and alternate-route blockers are retained.

## Reproduction

`bash tests/replay/check_slot_a_string_0356_abi.sh` runs strict synthetic C
checks without game files or Python dependencies. For the hash-pinned
original/Unicorn/C and native comparison, install Unicorn 2.1.4 in an ignored
virtual environment and run from this clone:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_string_0356_abi.sh resources/string0356-private
```

The native root contains only ignored copied inputs, RAM dumps and numeric
transcripts from the existing `slot-a-native-u6f-196a-048` evidence. The
runner reuses `check_slot_a_u6f_196a_resident_native.py` to pin every original
and controlled capture, then checks the additional runtime/next-call spans.
Optimized Python assertions are rejected. Shared CMake registration remains
coordinated with the integration owner; this slice changes only new owned
replay source/header/test files and this evidence note.
