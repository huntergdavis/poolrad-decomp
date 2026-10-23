# U38 +1276: active-character comparison against two code strings

Public entry 40 checks the active-character marker, compares its Pascal
string with up to two constants in the loaded U38 segment, and subtracts
four from DS:6822 on either equality. The complete caller composes the
merged raw SYSTEM +0C10 comparator and preserves CPU14, flags, live RAM
ordering and partial effects at a rejected call binding.

## Exact identity and calls

The complete executable body is **[1276,12B2), 60 bytes / 24 instructions**,
GAME.OVR file [0xFED1,0xFF0D), ending at RETF 0Ah at +12AF. Public entry
40 begins at +1276; the next entry begins at +12B2. This queue extent has
no trailing-data adjustment. The two Pascal constants it reads occupy
[1268,1276), the 14 data bytes excluded from the preceding +11F9 body.
Their contents remain in ignored owner media.

| Input/span | SHA-256 |
| --- | --- |
| START.EXE | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U38 [1276,12B2) | 0598c5bb03bf995e3aeba41c11d749c629fa65cd273bd6a00892bf9a5eaca01a |
| Code strings [1268,1276) | eb19eb8681d04b84bfe569f341ba3b2de215ff19ee7c51282198074a48c57ae4 |
| SYSTEM [0C10,0C3B) | 11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58 |

Both far calls target SYSTEM :0C10: sites +1290 and +12A2, loader fixups
+1293 and +12A5. The loaded segments are read independently and may differ.
C validates each reached LCALL opcode and offset before entering the
unchanged pr_system_strcmp_0c10 with its real far-call frame. The original
oracle verifies the hashes, entry, extent, both calls/fixups and RETF before
running the caller and the 43-byte / 20-instruction comparator together.

## ABI, ordering and contract

The entry takes five unused word arguments and returns with RETF 0Ah.
PUSH BP executes before the active pointer is loaded from DS:5D92/5D94.
If byte active+9Fh differs from 1, the routine returns with that comparison's
flags and the live ES:DI pointer.

Otherwise it reloads the pointer, pushes ES and DI, loads DI=1268, pushes
CS and DI, and calls the first loaded comparator. Equality branches directly
to the byte subtraction. Inequality reloads the active pointer again and
compares it against CS:1270 through the second independently loaded call.
A second inequality returns without subtraction. Both code strings are read
from live RAM; the implementation contains no copied game strings.

SYSTEM clears DF, reads both Pascal strings through the pushed pointers,
returns its complete CPU state and consumes RETF 8. On equality the caller
performs SUB BYTE DS:[6822],4, preserving the byte wrap and arithmetic flags.
On mismatch the last comparator's flags survive. On every path it finishes
with POP BP; RETF 0Ah, using the live saved BP and return frame.

The pointer can change between comparisons through stack aliases. In the
original fixture with SS=DS and entry SP=5DA0, the first far-call return
words overwrite DS:5D92/5D94. The second call therefore reads the changed
pointer. Other cases let the result byte alias the saved BP or caller
return IP, and let the active string overlap the outgoing argument frame.
All writes and subsequent rereads retain their original order.

The API requires exactly 1 MiB A20-off RAM, initial IP=1276 and
SP in [000E,FFF2], immutable original loaded code/constants/call bindings,
and CPU/status storage distinct from RAM. Effective offsets wrap at 16 bits
and physical addresses at 20 bits. Word, LES and return-frame reads use
physically contiguous bytes. Execution is synchronous; interrupts, faults
and arbitrary-PC resume are outside the profile. The comparator's flag
contract applies.

CODE stops before a malformed reached LCALL with its four argument words
already pushed. The last comparator status is retained: -1 before any call,
0 after a successful first call. ARGUMENT changes no CPU/RAM. callee_status
may be null. CALLEE and TRANSFER preserve failed or changed dependency
state as defensive guards; the current read-only leaf cannot produce those
stops under this profile. No stop rolls back prior effects.

## Measured evidence

- **543 original-chain cases**, CPU14 and all **1,048,576 RAM bytes** equal
  without masks: 537 returns and six CODE stops.
- **24/24 caller instruction starts and 6/6 conditional outcomes**;
  including SYSTEM, **44/44 starts and 12/12 Jcc outcomes**. These counts
  measure exercised code and branches, not an exhaustive input/alias proof.
- **64 DOSBox-X normal/386 returning cases**, 25 with a match. CPU14, the
  256-byte stack window and all 65,536 DS bytes match without masks.
- **7,186 semantic checks pass ASan/UBSan**. Focused CTest: **1/1 passed**.
- Native RESULT.BIN SHA-256:
  9ec42f111cabebbac39fa0883f4009b43a4c7c644abaedb3dad884b7ccdc8bc5.

The original corpus includes every marker-byte value and every initial
result-byte value across first/second/mismatch/empty/prefix/extension/long
string cases; both independently relocated comparator segments; malformed
opcode/offset bytes at both call sites; unreachable bad bindings; odd and
boundary stacks; SS=DS and A20 wrapping; wrapped names; randomized RAM;
and the active-pointer, argument-frame, saved-BP and return-IP aliases.
It checks the expected comparator diagnostic on every differential case.

Semantic tests use synthetic seven- and five-character constants. They
independently enumerate all 256 result values on both equality paths and
mismatch/empty paths, calculate subtraction flags through signed arithmetic
and parity, enumerate all marker values, and check pushed frames, aliases
and argument guards. Native replay uses the actual private constants with
synthetic entry frames, IRQs masked during execution. It covers returning
paths; bad bindings and aliases are measured by the original differential
and semantic tests. Natural gameplay reach is not established.

The native COM reserves offset 3A50h for SYSTEM 0AF8:0C10 and copies the
caller plus its adjacent constants to 8800:1268. Original media, extracted
strings/code, generated assembly/COM and captures stay under resources/.

With owner media under resources/DOS/POOLRAD and Capstone/Unicorn installed:

    python3 tools/overlay/u38_character_compare_1276_audit.py --unicorn --native --output resources/u38-1276/report.json
    cmake -S . -B resources/u38-1276/build -DBUILD_TESTING=ON
    cmake --build resources/u38-1276/build --target poolrad_overlay_u38_character_compare_1276_cpu_tests
    ctest --test-dir resources/u38-1276/build -R '^poolrad_overlay_u38_character_compare_1276_cpu_tests$' --output-on-failure

Private receipt resources/evidence.json SHA-256:
b30a6cbf6eae55ae3adcc2bdc8182c668ff08f0c6aad640670d2654f11b07547.
It pins 19 public build/tool/queue inputs, 19 private artifacts,
media/tool binaries and 14 unchanged dependency files against main
5438ae2f027244b0a258e9ecca5a1826de3c5a02. The document is excluded from its own receipt.

## Reuse, queue caveat and author assessment

Pre-implementation deja "U38 1276 SYSTEM string compare" found indexed
Claude session 281c8700-5ac (poolrad-decomp-claude-lane), which identified
SYSTEM +0C10 as the dependency that completes U38 +11F9/+1276. Current main
independently confirms the shared raw comparator. The unpublished branch
codex/fullc-u38-1276 at ba5d612175b80063e361fd5f5c1c4cd9a167d0f5 supplies
body/fixup/ABI provenance; its inline comparator is not reused.

Source memory/flags/composition helpers follow merged
[U38 +0679/+0947](U38_STRING_MATCH_CPU.md) and the unchanged
[SYSTEM string/move leaves](SYSTEM_STRING_MOVE_LEAVES.md). Oracle and native
capture adaptation also reuse the author branch for
[PR #1068](https://github.com/huntergdavis/poolrad-decomp/pull/1068);
this PR has no build dependency on that pending branch.

The schema-8 successor of the requested schema-4 queue ranks this target 74,
next after submitted #1066/#1067/#1068. Its --no-git export omitted the old
unpublished branch; explicit remote-branch/all-state PR checks found it and
no existing +1276 PR. The caveat was reported to the VP before authoring.

- target_function: OVL_0038_1276, public entry 40, complete [1276,12B2).
- source_candidates: overlay_u38_character_compare_1276_cpu.c composing
  unchanged pr_system_strcmp_0c10.
- signals: original body/constants/helper hashes, entry/bounds, both call
  fixups, marker gate and live pointer reload, CPU14/full-RAM differential,
  native replay, independent semantic checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match within the stated profile; independent review pending.
- review_notes: synthetic native frames, no natural-game reach claim,
  immutable code/constants, conservative stack bounds and defensive stops.
  No classifier, acceptance, accounting or credit changes.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0038_1276 --require-current-main
```
