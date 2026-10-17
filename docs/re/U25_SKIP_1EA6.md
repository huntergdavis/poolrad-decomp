# U25 +1EA6: skip the next ECL statement

The complete unit 0x25 entry 22, [1EA6,1FB1), now composes the full-state
operand loader +0000 and its string callees. The routine fetches an opcode
through the script far pointer and PC, writes the opcode latch, and consumes
the original number of prefetched operands. Other opcodes increment the PC
by one. Every reached register, flag and RAM effect remains observable.

Implementation: [overlay_u25_skip_1ea6.c](../../src/recovered/overlay_u25_skip_1ea6.c).
Contract: [overlay_u25_skip_1ea6.h](../../include/poolrad/recovered/overlay_u25_skip_1ea6.h).
This change depends on the [loader](U25_LOADER_0000.md) and
[string cores](U25_STRING_CORES.md), delivered in PRs #863 and #862.
Independent integration review and native-route parity remain separate gates;
this report changes no strict-credit accounting.

## Identity and prior evidence

| Evidence | Pin |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206,105 bytes; SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Source extent | GAME file [9379,9484), unit [1EA6,1FB1); 267 bytes, 133 instructions |
| Body SHA-256 | a18580355e2e8766ca756e3e562e3fdde0ffe89dc65e9312dbe72f7074b0e2df |
| Entry identity | public entry 22; stub 002B:008E; six fixup-backed calls from U1C +0B52 |
| Loader near calls | 1EFD,1F27,1F5D,1F6E,1F7B,1F8C,1F99,1FA6, each preceded by aligned PUSH CS |
| Return | RETF at 1FB0, no argument cleanup |

Required recall 'deja "1EA6"' found Claude session 997d7fe9-a03 (historical
pG ownership and loader dependency), 4d9851e1-f26 (267-byte entry/8-call
recovery), and Codex 01a1281b-85e (an older static-count review).
Reused findings were rechecked against the pinned media and the merged
[function ledger](FUNCTION_LEDGER.md#12-operand-count-dispatcher-function-2).
The earlier [party-index report](U25_PARTY_INDEX_0756.md#why-this-leaf-was-selected)
explicitly rejected strict credit for this caller while the loader's strings
remained unresolved. PRs #862/#863 supply those missing complete effects.
No older count-only implementation is substituted for the original calls.

The schema-3 selection snapshot ranked this target first (score 82,
game_verified, grade A, 133 instructions), using the current checkout with
the new loader and string cores. Queue readiness was checked against actual
callee contracts: the sole direct callee is +0000. A queue's modeled or
verified label alone does not establish full-state composability.

## Behavior and machine contract

The API pr_u25_skip_statement_1ea6 takes the same 14-word CPU and 1 MiB A20-off
RAM layout as the loader. Entry IP is 1EA6; far return IP/CS are at SS:SP.
The caller pushes BP, reads PC at DS:49ED, reads the script pointer at
DS:49DE, adds PC to its offset, fetches the byte at displacement 6700, and
writes then rereads DS:6F7F. These operations remain ordered under aliases.
AH retains the high byte of the fetched PC until subsequent original
instructions change it. The compare chain retains its original flag effects.

| Prefetched count | Opcodes (hex) |
| ---: | --- |
| 1 | 01 02 0A 0E 11 12 1D 20 2D 32 34 36 38 39 3C |
| 2 | 03 08 09 0C 0F 10 1F 22 |
| 3 | 04 05 06 07 0B 21 28 2A 2F 30 35 37 3B |
| 4 | 14 23 |
| 5 | 2E |
| 6 | 1E 2C |
| 8 | 27 |
| 14 | 29 |
| Fallback | All other byte values: increment PC, no loader call |

These are **prefetch counts**, including the original 0C/34/36 behavior;
they are not a replacement wire-format table. Count words retain AH. The
loader receives a real far return frame from PUSH CS; CALL near, consumes
its argument with RETF 2, and leaves the caller to pop BP and execute RETF.
String decoding/copying, stale operand bytes, table overlap and stack aliases
are inherited through the actual full-state C callees, without result hooks.

Initial SP is restricted to [0232,FFFC]: the loader entry is eight bytes
below the caller's entry, satisfying its 022A lower bound. No reserved frame
wraps; the final RETF may wrap SP from FFFC to 0000. CPU and RAM must be
distinct and synchronous. Code and loaded SYSTEM binding must remain
immutable, and data writes must not overwrite code. Effective offsets wrap
at 16 bits; word/LES bytes remain physically consecutive modulo 1 MiB.

Positive operand/string budgets use the loader's exact loop boundaries.
OPERAND_LIMIT or STRING_LIMIT returns the reached nested state before the
next loop-header instruction. An unexpected nested return yields TRANSFER;
a rejected nested entry yields CALLEE. These stops retain all prior effects
and never manufacture a successful DOS return. Invalid initial arguments
reject without changing CPU or RAM. Undefined flags use the dependency's
published convention; no new native undefined-flag claim is made here.

## Validation

The [audit](../../tools/overlay/u25_skip_1ea6_audit.py) pins all original caller,
loader, local-helper and SYSTEM bytes, executes them in Unicorn, and compares
them to compiled C. It checks all 14 CPU words and every byte of the 1 MiB RAM
image. Only loader-relocated SYSTEM segment operands are adjusted; no callee
results or runtime effects are stubbed.

The 640 cases cover every opcode byte, every operand tag byte, all eight
count groups, packed and address strings, all address classes, empty strings,
PC wrap, relocated code/SYSTEM segments, lower/upper stack bounds, seven
stack/data aliases, a script-source/latch alias, and 100 seeded mixed streams.
Measured outcomes: 635 returns, two operand-budget stops, two string-budget
stops and one unexpected nested return. CPU and full RAM match in all cases.

| Body | Reached instructions |
| --- | ---: |
| Caller +1EA6 | 133/133 |
| Loader +0000 | 118/119 |
| Word helper +06AF | 16/16 |
| Packed decoder +1153 | 166/166 |
| Address string +12FB | 229/229 |

Loader +0017 is its zero-count jump. This caller supplies only positive
counts; the independent loader audit covers that instruction. The audit
requires exactly this exclusion rather than silently accepting coverage gaps.

Focused synthetic C tests check mixed string side effects, all count groups,
fallback PC wrap, nested stops and atomic rejection. Strict warnings,
ASan/UBSan, the three focused CTests and the full recovered-library build
pass. Original media and generated disassembly remain in ignored resources.

Reproduce with installed Capstone and Unicorn:

~~~sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u25_skip_1ea6_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR
cmake -S . -B /tmp/poolrad-skip-build
cmake --build /tmp/poolrad-skip-build --target poolrad_overlay_u25_skip_1ea6_tests
ctest --test-dir /tmp/poolrad-skip-build -R poolrad_overlay_u25_skip_1ea6_tests --output-on-failure
~~~

## Adversarial parity review

- **target_function:** OVL_0025_1EA6, complete public entry 22.
- **source_candidates:** pr_u25_skip_statement_1ea6, composed with
  pr_u25_load_operands_0000 and its complete dependencies.
- **signals:** pinned extent and entry identity; all eight call sites and
  exact frames; ordered PC/latch/table access; complete compare-chain and
  defined-flag behavior; independent original-versus-C full-state comparisons.
- **score:** 5/5 listed evidence categories agree; this is not an exhaustive
  state-space proof or a completion percentage.
- **verdict:** match within the published machine/memory contract.
- **review_notes:** a table-only replacement would miss stack writes, loader
  side effects and final flags. Fallback bypasses the loader, rather than
  calling it with zero. Reordering the latch access or caching aliased RAM
  across calls changes behavior. Nested transfer/limit states must remain
  visible. Caller coverage is complete, while the unreachable loader
  zero-count instruction is explicitly excluded. Immutable code, bounded
  initial stack, undefined-flag conventions and absence of a native route
  comparison remain limits on the claim.
