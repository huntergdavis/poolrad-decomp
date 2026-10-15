# Actual allocator callback B0B join through +0064

The new dispatcher binds callback 398A:+005F's far call to the existing
SYSTEM B0B C helper. It composes the unchanged real-hook prefix and helper
against one live CPU/RAM state, reaching callback +0064 before its printer
call. No snapshot replaces a blocked state or supplies a hook outcome.
This is bounded ABI integration with no full-C, function-count, identity,
continuous allocator or playable credit. Nonzero hook return/retry/error
remains explicitly blocked.

The branch starts from current main
`e4fce07b04df1c4b067df0b905b55214fcb97255` after #495 and merged #490.
Bounded `deja "Poolrad allocator hook 398A 005F SYSTEM B0B 0064 callback
length 28 native helper return"` expired without reusable output before
implementation. Reused evidence is [the actual-hook ABI](SYSTEM_GETMEM_REAL_HOOK_ABI.md)
and [the existing B0B native/helper audit](SLOT_A_NATIVE_U6F_1965_SYSTEM_COPY.md).
The new C source delegates to `system_getmem_real_hook_abi` and
`slot_a_saved_copy_1965`; neither implementation or header is modified.

Before review the branch is restacked onto main
`217e809448be096fcc48b2413714856f5b165284` after #497. The tested
dispatcher, delegated helpers, headers and oracle files retain identical
hashes; the focused audit is rerun on the final restacked head.

## Pinned original and native inputs

| Input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SAVGAMA.DAT | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original/loaded SYSTEM B0B..B24 | 26 | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| Original/loaded callback source, Pascal prefix plus payload | 29 | `ad7768f38fbce2c9c16fad3b116ed210b41a094354d6275085a12548d6272fc2` |
| Complete 17-row native transcript | — | `39ef8adb748c91ff5e0e960cb0703229a9194cede407764f17e4e092acee4cc8` |

B0B bytes come from START file +B781. Callback source bytes at loaded
398A:0000 match GAME file +0008. Source contents, RAM dumps and native
media stay ignored; public output contains only hashes, lengths and addresses.
The #490 callback-body identity, six +0824 relocations and loaded thunk
are rechecked against every new capture. Original SYSTEM +0308 and +0491
code identities are also pinned.

Private root: `resources/oracle-runs/slot-a-getmem-hook-b0b-join-20261014`.
Seventeen same-run rows pin sixteen CPU fields and all 655360 RAM bytes.
One row is untouched saved-A +0308 after the confirmed +19B5 caller.
The other sixteen rows form an explicitly controlled exhaustion case:
heap head and limit are set to the measured end 6652:0008, changing five
RAM bytes. All CPU fields and the actual DS:[4414] hook pointer remain
unchanged at that controlled input. No returned AX/AL is edited.

## Real frame, writers and return

The actual hook prefix reaches callback +005F with SS:SP=1D16:3774,
BP=37A6. Its far CALL pushes the real B0B frame at SS:3770:
`0064, 398A, 0000, 398A, 3786, 1D16, 000E` (return, source,
destination and retained word). The new frame changes four RAM bytes.

B0B clears DF, saves DS in DX, loads the destination with LES and the
source with LDS, reads Pascal length **28**, and stores that prefix at
SS:3786. MOV CL,AL leaves stale CH intact (CX=441C); XOR CH,CH produces
CX=001C before REP. The observed full flags word becomes 7246, including
AF=0. Native and raw original-byte comparisons retain that whole word.

STOSB changes one byte; REP MOVSB stores and changes 28 bytes at
SS:3787..37A2. The full 29-byte local matches the pinned source and the
adjacent callback flag at SS:37A3 remains zero. The existing helper's
forward-copy semantics and atomic REP step are retained. The +005F-to-
+0064 interval changes 33 RAM bytes, including the four-byte far frame.

MOV DS,DX restores DS=14A0. RETF 4 returns to **398A:0064** with
SS:SP=1D16:3778, BP=37A6, ES=1D16, AX=001C, BX=3770, CX=0000,
DX=14A0, SI=001D, DI=37A3, FS=GS=0000 and FLAGS=7246. AX/AL is the
helper's temporary source length at this point; the allocator callback
has not returned. The remaining argument words are `3786, 1D16, 000E`.

The dispatcher blocks at +0064 before original 0709:0BF5 (loaded
0F2D:0BF5), return +0069. It reports a far-call dependency and leaves
CPU/RAM unchanged, before any printer frame or effect. Native execution
also closes at this boundary.

## Verification and limits

The uninterrupted controlled hook case executes **29 joined C instructions**
from SYSTEM +03C3 through callback +0064, with **14 distinct post-input
native full-state checkpoints**. Thirteen of those instruction steps are
the new far-call context and existing helper; the other sixteen delegate
to the unchanged #490 hook prefix. All CPU fields and RAM bytes compare
against original-byte execution after every joined instruction.

Seven independent synthetic helper-input cases bring the total to **120
full-state C/original-byte instruction comparisons**. They cover incoming
DF/stale flags, AH preservation, wrapped far frames, own strings of lengths
0/1/255 and forward overlap. They add no native reachability or hook-outcome
claim. The original-only 30-instruction core prefix matches its captured
pre-hook endpoint and is excluded from C totals; it never bridges a C gate.

Strict C11/Wconversion, media-free checks and complete ASan/UBSan validation
pass. The wrapper preserves the existing helper's atomic preflight failures,
unknown-hook target guard and wrapper boundary. SYSTEM +03C7 stays blocked
with CPU/RAM unchanged for all 256 AL values. Nonzero callback return,
retry/error behavior, printer effects and whole allocator parity remain
unproved. No full-C credit is assigned to this ABI draft.

The audit initializes its oracle only before each independent input, with
no mid-case correction or state replacement. Joined cases are bounded by
100 instructions/ten seconds; the original-only prefix by 500/ten seconds;
the process by 120 seconds plus five-second kill grace. Private inputs and
builds must stay under this clone's ignored `resources/`.

Run `bash tests/replay/check_system_getmem_hook_b0b_join.sh` without media.
With pinned captures and Unicorn 2.1.4, run
`POOLRAD_UNICORN_PYTHON=/path/to/python bash tests/replay/check_system_getmem_hook_b0b_join.sh resources/oracle-runs/slot-a-getmem-hook-b0b-join-20261014`.
Assertions must remain enabled. The native capture uses the #490 startup
and caller-specific +19B5-to-+0308 procedure, followed by the labeled
head/limit edits and stops at the actual B0B boundaries and +0064.

## Adversarial parity review and next gates

- target_function: actual callback +005F SYSTEM B0B context through
  callback +0064, composed with the existing real-hook prefix.
- source_candidates: `system_getmem_hook_b0b_join.c` dispatcher and the
  two unchanged existing adapters.
- signals: original/loaded code and data identity; real far frame and
  stack consumption; exact copy writers; full-state oracle/native checks;
  negative bounds and blocked-outcome tests.
- score: 5/5 checked categories above, ordinal evidence coverage rather
  than a probability or full-function completion score.
- verdict: match for the bounded context join; full callback and allocator
  outcome semantics remain unclear.
- review_notes: one untouched baseline and sixteen controlled rows remain
  separate; helper AX/AL is a temporary length, not a supplied hook result.
  No snapshot bridge, literal publication, function identity or count credit.

Follow-on A requires printer-owner coordination for the actual +0064
0709:0BF5 context before continuing the nonzero callback path. Follow-on B
remains caller +19BA pointer stores/setup through +19D7's near CALL +0AEC
(return +19DA), stopping before the loader. Keep callback/runtime and caller
files disjoint. This change adds six uniquely named files; the old helpers,
prior gates, immutable #410 map, CMake and key/printer lane files stay intact.
