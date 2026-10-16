# U6F +1446 full-state composition and cold-dispatch boundary

`pr_u6f_1446_step` executes the whole 313-byte insertion body through
its actual generic RETF 4, its two SYSTEM helpers, the warm patched jump,
and the existing reviewed U9B +0000 composition. A cold INT 3Fh path
requires an explicit full CPU/RAM dependency event; the resident handler
and loader are not translated by this adapter. This remains a bounded
composition with no new whole-native, game-identity, or strict-count credit.

Source identity is DOS v1.3 GAME.OVR, 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and START.EXE, 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| U6F span | GAME file span | Bound | SHA-256 |
| --- | --- | --- | --- |
| +1446..157E executable | 1F42C..1F564 | 313 bytes / 99 instructions | `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` |
| +157F..1601 following data | 1F565..1F5E7 | 131 bytes, excluded from code | `1dfa5f64639ccd4fec61babcf5419475c3c8317f0d3ba3b69f5ab0731dd192ee` |
| +1426..1445 membership data | 1F40C..1F42B | 32 bytes, read from live CS RAM | `d22f8e3450655784853879c7ecfee4ff36192352f6ffe0d40593dee56f583350` |

Four recorded caller frames are supported: U6F +1416→1419 and
+19E3→19E6 use PUSH CS/near CALL; U6C +318A→318F and +322B→3230
far-call public entry 10 at original stub 007C:0052. The incoming pointer
is read from SS:[BP+6/+8]. The saved outer CS:IP is never hardcoded.

The source-ordered path preserves initial record +BF=FF, head/active
global writes, append traversal, stack-backed occupancy, the live CS set,
slot selection, window-word increment, and helper/callee frames. There is
no host list traversal cap. Exactly 1 MiB of A20-off RAM is required;
effective offsets wrap to 16 bits and word/far reads use consecutive
physical bytes with A20 wrap, following the reused insertion/U9B models.
Undefined logic/shift AF and multibit shift OF normalize to zero.

The SYSTEM FillChar +16A8..16BB is 20 bytes/seven instructions;
membership +0DC0..0DE0 is 33 bytes/13 instructions. Static original-byte
disassembly checked all 99 caller and 20 helper table boundaries.
Loaded segments are explicitly bounded to U6F 2A8E, SYSTEM 131C,
stub 08D0, and U9B 2CCD, with host-audited loaded code/data identity.

At +1574 the actual destination is the +0000 descriptor, not isolated
U9B +01AF. The warm stub must contain a far jump to 2CCD:0000; then the
reviewed `pr_u9b_entry_zero_0000` supplies all three nested paths. Its
14-field CPU result is copied explicitly; FS/GS survive unchanged.
Changed nested return targets are exposed as transfers.

The cold callback owns INT frame writes, handler/loader state, code
patching, and restored CPU. Missing callback blocks before INT execution.
Old-byte chains, patched target, normal SS:SP restoration and projected
INT return frame are checked before any RAM/CPU mutation. A patch-only
event is insufficient. Only a normal return to the patched stub is
supported; arbitrary cold-handler diversions remain outside this boundary.
No successful load, allocator effect or native route is invented.

Validation uses fabricated RAM and preserved captures:

```sh
bash tests/replay/check_u6f_1446_fullstate.sh
bash tests/replay/check_u6f_1446_fullstate.sh --sanitize
bash tests/replay/check_u6f_1446_fullstate.sh --native-prefix \
  resources/oracle-runs/slot-a-native-first-insert-cold-037
```

Six cases compare every byte of 1 MiB against the separate existing
SS-prefix model, including 300 finite nodes, all eight occupied slots,
SS occupancy/record alias and a window/record alias. Four complete
fabricated-state invocations cover the four caller frames, warm execution,
signed U9B gates, and an explicitly synthetic cold event. U9B's returned
14 CPU fields and every RAM byte match its separate reviewed API; FS/GS
and the outer cleanup are checked. Cold-event rejection covers stale old
bytes, bad target/PC, excess count and omitted INT frame. Physical word
and return-frame A20 crossings are also exercised. These are source-level
composition checks, not independent original-byte execution.

The optional run-037 gate hashes the pinned original inputs, numeric EV
transcript, entry/stop RAM and loaded insertion/helper code. Starting at
actual `2A8E:1446`, 92 C steps reach the actual cold stub `08D0:0025`:
all 16 CPU fields and all 655,360 captured bytes agree, with zero
differences. Output SHA-256 is
`16eeb185b0e03b4293085b34ad7090661b8ae19a5b1f9a9e738b3f005a7c6383`.
The initialized upper 384 KiB remains zero; no native comparison is claimed
there. This stops before INT 3Fh and says nothing about cold-handler
completion. It reads existing private captures without a new emulator or
original-byte oracle. Full cold dispatch, whole-invocation native parity,
and independent branch/alias original-byte gates remain open.

The bounded recall query timed out without reusable indexed output.
Reused evidence: [SS model](U6F_PARTY_INSERT_1446_SS.md),
[correct thunk destination](U6F_U9B_TRANSFER_1574.md),
[actual cold CPU stops](SLOT_A_NATIVE_FIRST_INSERT_COLD_CPU.md),
[U9B composition](U9B_ENTRY_ZERO_COMPOSITION.md), and
[U9B saved entry/return](SLOT_A_NATIVE_U9B_ENTRY_RETURN_PARITY.md).
The preceding +02AE adapter and disjoint message/UI plan are unchanged.

Parity review: `target_function=OVL_006F_1446`;
`source_candidates=u6f_1446_fullstate.c` with reviewed U9B dependencies;
`signals=exact executable/data extents, caller/callee graph, static source
boundaries, synthetic full-state comparisons, preserved native prefix`
(five evidence categories, not a probability). `verdict=match` only for
the pinned pre-dispatch native prefix; `unclear` for the complete cold
invocation. Callback-supplied effects, fixed loaded segments and unexecuted
original-byte/whole-native gates prevent strict promotion.
