# Actual loader CAA / DAA selector contexts

The bounded prefix API executes unit6F +0C8F/+0D8F sidecar setup, real
SYSTEM B0B copy and B98 append, then actual +0CAA/+0DAA far CALL through
resident02EB/B25 to the immutable Assign0310 dependency. A separate return
API starts from independently captured +0CAF/+0DAF native states and follows
OR AL,AL / conditional branches to allocator, printer, or next-context stops.
No native result updates an in-flight C state, and no runtime/DOS effects
connect these four independent fragments.

Base is main 217e809448be096fcc48b2413714856f5b165284. Only six new replay
files change. All B0B/B98/02EB/B25 helpers and their historical gates remain
unchanged. No general dispatcher, caller 0292 continuation, printer body,
scoreboard, function identity/count, or playable-route credit is supplied.

## Provenance and pins

The bounded prior-session query "Poolrad loader 0CAA 0DAA 02EB B25" expired
without reusable output. Reused repository leads, rechecked against original
and fresh native code, are the [selector call-site map](RESIDENT_DAX_SELECTOR_02EB.md),
[loader continuation map](U6F_PARTY_CONTINUATIONS.md),
[older party-loop watch](SLOT_A_NATIVE_PARTY_LOOP_EXIT.md),
[existing probe ABI](SLOT_A_FILE_PROBE_02EB_ABI.md), and
[approved Close/IOResult evidence](https://github.com/huntergdavis/poolrad-decomp/blob/52aee0899389528ca9bf6f80d5d6d5da96f534f0/docs/re/SLOT_A_CLOSE_IORESULT_ABI.md).
That preceding branch is unmerged and is not a C dependency here.

Composition reuses slot_a_saved_copy_1965.c for B0B, system_b98_abi.c for
B98, and slot_a_file_probe_02eb_abi.c for 02EB/B25. CPU layout/comparer,
pinned-media reader and original-instruction stepping reuse the existing
0356/delete audit helpers. Their source is unchanged.

An untouched original saved-A session reached the first loader occurrence
through actual C26 and original file processing. All 36 CPU/full-RAM states
were frozen before writing this C implementation. The four fragment inputs
are C8F, CAF, D8F, DAF; 32 subsequent native checkpoints are checked.
Original media, RAM, native log, screenshots and builds remain ignored in
resources/oracle-runs/slot-a-loader-sidecar-context-20261014.

| Original artifact | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Unit6F C8F..CAE | 32 | a90b1d46e356c4fbe0553e89da3637e1da7697c98f8d7ad53c5c332cb55b14b1 |
| Unit6F D8F..DAE | 32 | d10e9abf5707ebeaa3277d66e3707cc7f8b49d747a6a1ed3f37d98ee6ef2282f |
| Unit6F CAF..CBE | 16 | 6a6980b142121eccd959ff3ea69319c89a5a3e843ab69af6f26cc16721cffae1 |
| Unit6F DAF..DBE | 16 | ac34b4c897a9bbd92fa302c56f8c06cf0b501d3591b457ec1f5b384596abdc7c |
| Unit6F E8D printer call only | 5 | ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356 |
| Native raw transcript | local | 4f2a7c7374b2447d9b0bbb5908698ca4e23b1fb4338a30021801c6a94c68c76a |

The audit also pins original SYSTEM B0B (26), B98 (44), B25 (36) bytes and resident
02EB (107) bytes. Every native RAM image must contain all expected loaded
bodies. Far-call segments relocate by 0824: AF8->131C,802->1026,899->10BD.
The source body and relocation checks include the unexecuted dependency call
sites; no implementation of those dependency effects is inferred.

## Native retained-pointer ABI

Both setup inputs have BP=37A6, SS=1D16, DS=14A0, SP=33EC. The source Pascal
string is SS:36A6 (BP-100), length 19. The reused destination is SS:34ED
(BP-2B9). C computes both pointers in original order and pushes them.
At C9B/D9B B0B entry SP=33E0, frame words are
[CA0/DA0,2A8E,36A6,1D16,34ED,1D16]. B0B copies the length and 19 bytes,
executes real RETF4, and retains destination [34ED,1D16] at SP=33E8.

CA5/DA5 calls B98 with source 2A8E:0AE2/0AE7. Its SP=33E0 frame is
[CAA/DAA,2A8E,0AE2/0AE7,2A8E,34ED,1D16]. The four-byte suffix is appended,
producing a 23-byte name; RETF4 again retains the destination far pointer.
Filename bytes stay private. On the second path, native AH=3E survives the
string helpers (AX=3E13 after B0B and 3E04 after B98); all CPU fields are
compared, so low-byte operations cannot silently erase that high byte.

The actual CAA/DAA call enters loaded 1026:02EB at SP=33E4 with
[CAF/DAF,2A8E,34ED,1D16]. The prologue establishes BP=33E2 and SP=3338.
Its actual B25 copy caps the name at 40 and copies 23 bytes to SS:33B8.
C then prepares Assign's source/record arguments and stops at 1026:0310,
SP=3330, before pushing the far-call frame to original AF8:13E2, return 0315.
All four unknown Assign/Reset/Close/IOResult dependencies remain uncomposed.

Both first-occurrence selector returns were AX=0001, DS:49CE=0000,
SP=33EC, BP=37A6, FLAGS=7206. Independently seeded C return fragments execute
OR AL,AL (FLAGS=7202), select the nonzero branch, load size 003F/0009,
and push it, reaching CBA/DBA at SP=33EA. They block before SYSTEM AF8:023F,
return CBF/DBF. Allocation and intervening file processing are unknown effects.

The older party-loop watch reports DS:49CE=0067 for a later slot 4 DAA
occurrence. That is a different input/occurrence, not a contradiction or a
replacement state for this first-occurrence measurement. This result makes
no statement that every later loader selector succeeds.

## Verification and bounded verdict

~~~sh
bash tests/replay/check_slot_a_loader_sidecar_abi.sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
 bash tests/replay/check_slot_a_loader_sidecar_abi.sh \
 resources/oracle-runs/slot-a-loader-sidecar-context-20261014
~~~

The audit passes 1002 complete CPU/RAM instruction comparisons: 199 exercise
new loader code and 803 exercise unchanged helpers. Native prefixes each
contribute 73 comparisons (12 new), and independent returns each contribute
four (all new): 154 natural comparisons total. All 32 post-input native
checkpoints match exactly, with no flags mask. These are repeated-case
comparison counts, not counts of new functions or distinct instructions.

Controlled cases independently copy native inputs: base lengths 0/1/40/255
with initial DF set, identical/physically aliased/forward-overlapping B0B
source/destination pointers, and both return sites with AX=0000/0001/0100/0101/
8000/8001/FF00/FFFF under a full flag pattern. This checks low-AL branching,
AH preservation, arithmetic flags, copy/append caps and original alias order.
No controlled case supplies a runtime return into an active prefix run.

For AL=0, the separate return API stops at D8F (next-context boundary) or
E8D (before original 0899:0104 printer call, return E92). The positive paths
stop before CBA/DBA allocation. All blocked gates preserve CPU and the entire
RAM image; the next-context boundary has no fabricated call report. The
prefix API can separately start from an actual D8F input, with a new C state.
The printer implementation is untouched. No original false-return path was
measured in this first-occurrence run; those branch cases are synthetic.

Each case is bounded at 200 instructions / 20s; the process at 180s plus 5s
termination allowance. New source/tests compile with C11 Wall/Wextra/
Wpedantic/Wconversion/Werror. Existing helpers keep their established strict
warning flags. Strict tests cover actual far frames, failed spans, stale
reports, full flags, null arguments, separate prefix/return entry APIs, and
unchanged Assign/allocator/printer/next-context stops. The complete strict and differential suites also pass ASan/UBSan, including
all 1002 comparisons and 32 native checkpoints.

- target_function: actual loader C8F/CAA and D8F/DAA setup and selector
  prefixes, plus independently seeded CAF/DAF return branches.
- source_candidates: slot_a_loader_sidecar_abi.c with unchanged B0B/B98/
  resident02EB/B25 instruction helpers.
- signals: pinned original/loaded code, retained-pointer call frames, fresh
  same-run CPU/RAM checkpoints, copy/append lengths, full flags, independent
  return inputs, and cap/alias/high-byte branch tests.
- score: 4/5 ordinal evidence strength for these fragments; not probability.
- verdict: match, bounded to the four stated fragments; complete loader unclear.
- review_notes: unsupported I/O/runtime/heap/printer effects separate all
  fragments. Earlier 025D caller 0292, file processing between contexts, later
  party-slot outcomes, full-C ownership and playable saved-A parity stay open.
