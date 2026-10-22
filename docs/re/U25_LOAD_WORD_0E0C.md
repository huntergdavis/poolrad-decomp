# Unit 0x25 +0x0E0C: raw-RAM ECL load word

The new pr_u25_load_word_0e0c executes the load-word entry, its address
classifier, and the complete class-1 load hook using caller-owned A20-off
1 MiB RAM and a fourteen-word CPU frame. Its two party-list calls reuse the
merged [party-index translation](U25_PARTY_INDEX_0756.md). The existing
pr_rec_ecl_load_word core API is unchanged. This is an isolated routine
implementation, not a playable-route integration. **No identity or strict-C
credit is promoted; whole-entry credit remains for PM review.**

## Source and reused evidence

The bounded prior-session query "poolrad u25 load word C04F store word 06AF
full C ABI aliases" timed out after 15 seconds; no completed finding was reused.
Reused repository evidence:
[ECL_LOAD_WORD_CLASS4.md](ECL_LOAD_WORD_CLASS4.md),
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md),
[U25_LOAD_HOOK_6B18_NATIVE_RECORD.md](U25_LOAD_HOOK_6B18_NATIVE_RECORD.md), and
the [load-hook classifier](ECL_LOAD_HOOK_CLASSIFIER.md). The assignment's
ECL_LOAD_HOOK_6B18_NATIVE_RECORD.md spelling does not exist; the U25-prefixed
note is the published native evidence.

GAME.OVR is 206,105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The DOS START.EXE build is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Unit 0x25 begins at GAME file offset 29,907.

| Routine | Span and instructions | SHA-256 |
| --- | --- | --- |
| Load word | +0E0C..+0F52, 327 bytes / 125 instructions | e8f999e62ee3f04b194908f90ce7c0e15b0478a2af16a3843a1a076c86c67ce5 |
| Address class | +06FB..+0755, 91 bytes / 28 instructions | e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347 |
| Load hook | +07BC..+0A73, 696 bytes / 233 instructions | 6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451 |
| Party index | +0756..+07BB, 102 bytes / 35 instructions | b307e1dd310b369eeb068ef1e0fd7b84667e59382144bec02e5295b974c3fcb9 |

## Behavior and dependencies

Entry is CS:0E0C after the far CALL: return IP/CS and one address word already
occupy SS:SP. C preserves frame writes, modified argument copies, registers,
arithmetic flags, source-order reads, and RETF 2. Multi-byte operands cross
offset boundaries contiguously; effective offsets/SP wrap at 16 bits and
physical bytes at 1 MiB.

Classes 0/2 load their far windows; class 3 zero-extends a script byte. Class 1
runs the hook first and retains its result only when the by-reference flag
remains nonzero. The complete hook covers direct/signed and indexed record
fields, conditional results, the record-class ladder, DS byte 5376,
window-1 word +067C, both party-index calls, and the plain fallback. Pointers
and frame fields are reread where the original reloads them.

Class 4 covers all nine documented mappings, including high-address argument
mutation by subtracting C04B. Other paths return the actual old result word
in supplied stack RAM. Hook address 6C0D also leaves its result local
unwritten with the flag set; out-of-table record-class values at 6DCF retain
the old hook local. These are RAM reads, not uninitialized C variables or
invented host defaults.

There are no unresolved external callee callbacks. Cyclic party lists remain
nonterminating, matching DOS and the merged callee. Code is immutable under
the contract. An altered internal callee return exposes exact RAM/CPU state
as TRANSFER; a higher-level executor must execute that target. C does not
guess the enclosing continuation. Undefined AF after XOR/shifts is normalized
to zero as in Unicorn, without claiming that bit as a DOS guarantee.

## Validation

The full original/C differential passed **4,077 cases**, comparing every
1 MiB RAM byte and all fourteen returned CPU words. It includes all 1,024
class-1 addresses, byte sweeps, conditional record results, finite party
lists and count wrap, stack boundaries, A20 wrapping, and DS/SS aliases.
**Nine altered internal returns** match exactly and stop as TRANSFER.
The suite visits 420 of the four bodies' 421 instructions. A separate
353-case replay confirms the sole unvisited address is +0EA4: the fallback
jump after rejecting class 4. The preceding fixed classifier returns only
0..4, so this guard is unreachable under the documented immutable-code
contract. C still preserves the fallback rather than assuming a new result.

Focused C tests pass with -Wall -Wextra -Wpedantic -Werror, including all 256
byte values, unwritten locals, argument mutation, record/window precedence,
indexed and conditional record cases, both party calls, and guards.
The complete project builds with the same strict warning flags; all
**200 CTest targets passed** after rebasing onto main 92546e9.

The separate DOSBox-X synthetic COM executes the pinned original classifier,
load hook, party-index callee, and whole load-word entry at their original
relative offsets. **56 native cases passed**, comparing all fourteen returned
CPU words, defined flags, 80 frame/argument bytes, and the record/window input
words. AF is masked only on the class-3 and class-4 XOR/shift paths where it
is architecturally undefined; defined CMP/ADD AF remains checked. The ignored
result file SHA-256 is
27e0ea0d867a482ada8a0f4be70857ea12b224f3b41135ba362ec849d6b7b136.
The header contains its DOS allocation segment, so its hash may change in
another emulator environment while comparisons continue to pass.

The published native note independently observes record byte 7 overriding
window word 10 at 6B18 after a controlled script edit. Both the C test and
fresh synthetic native oracle check that discrimination. Full RAM captures
referenced by that reached-state note are absent from this clone; their
locations have been requested from the PM. The new native oracle is synthetic,
not a new saved-game capture or a reached-state whole-RAM comparison.
Natural unedited-script reachability remains unmeasured.

Reproduce with owner media and local Capstone 5 / Unicorn 2:

    cmake -S . -B resources/build-u25-load-word -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
    cmake --build resources/build-u25-load-word
    ctest --test-dir resources/build-u25-load-word --output-on-failure
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u25_load_word_0e0c_audit.py
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u25_load_word_0e0c_native.py

Media, decoded instructions, builds, and captures stay under ignored
resources. The audit reports counts, hashes, and mismatch addresses only.

| Parity report field | Finding |
| --- | --- |
| target_function | Unit 0x25 +0E0C and its complete +07BC dependency |
| source_candidates | src/recovered/overlay_u25_load_word_0e0c.c; merged party-index callee |
| signals | Four pinned spans; complete call map; source-ordered RAM ABI; original-byte whole-RAM/register differential; published native branch evidence |
| score | Four checks passed: pinned CFG/ABI, focused C tests, 4,077 whole-RAM/register cases, 56 synthetic native cases; checklist, not a probability |
| verdict | match for defined terminating routine behavior under the documented RAM/CPU contract |
| review_notes | Immutable code; cyclic lists do not return; altered returns exposed as TRANSFER; AF not a DOS guarantee; no credit promotion |

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0025_0E0C --require-current-main
```
