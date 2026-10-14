# Unit 0x6A +0x14A9 active-party reorder

`pr_u6a_party_before_14a9` translates the complete call-free routine
`+0x14A9..+0x1600`. In a disjoint finite party containing the active
member, it moves that member one place toward the head. If already the
head, it moves to the tail. A singleton remains a singleton.

The required query
`timeout 30s deja "Poolrad u6A 14A9 1601 full C call-free"` ended 124
without results. The PM authorized proceeding after bounded recall.
Reused evidence and patterns are the [alias-audit queue](FUNCTION_ALIAS_AUDIT.md),
the [verified/native party and active-pointer mapping](U6F_PARTY_INSERT_1446.md),
and the [full-state and synthetic native audit approach](U6A_RECORD_REMAINING_025C.md).
This target remains separate from the `+0x0824` native-gate work and the
queued `+0x1601` translation.

## Exact source and role

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x6A | GAME file base `0x17831`, resident stub segment `0x0075` |
| Function | GAME file `0x18CDA..0x18E31`; **344 bytes, 104 instructions, zero calls** |
| Function SHA-256 | `76faaa709b22d0d563ae00745d122c9bd9d192a4f31bec85c60e69b1514486c7` |
| Exit | `RETF` at `+0x1600`; no stack arguments |

Every branch stays within the exact body. Public entry 1 `+0x1E6F`
has an instruction-boundary CFG call path
`+0x1FAC -> +0x1C59`, `+0x1CCE -> +0x1780`,
`+0x1809 -> +0x14A9`. The final call is preceded by PUSH CS and
selected by an AL comparison with `0x47` at `+0x1804`.
This is a static path; it does not establish a saved-game key route.

The body directly reads the documented game party head DS
`[0x5D96/0x5D98]`, the active-character pointer DS `[0x5D92/0x5D94]`,
and each member's next pointer at `+0x104/+0x106`. These globals and
links are the same ones measured in the native u6F insertion evidence.
This establishes a concrete party-list role beyond the generic shape
of a linked-list utility. No new gameplay command name is inferred.

## ABI and source-order effects

The API enters after CALL with IP/CS at SS:SP. It saves BP and reserves
12 bytes for current, previous, and successor far pointers.

If the original head equals active, it walks current to the tail. Otherwise,
it finds the member whose next pointer equals active, retaining that
member's predecessor. It then rereads active and stores active.next in
the successor locals. Crucially, it **recompares head and active after
those writes**; stack/global aliases can change the selected path.

When head still equals active, it sets head to the nonzero successor,
links the tail to active, and clears both words of active.next using AX=0.
DX remains the loaded active segment. For a singleton, the temporary
self-link is subsequently cleared.

Otherwise, current bypasses active by linking to successor, active links
to current, and either head or previous.next is set to active. Every
source LES reload and separate word read/write is retained; the model
does not replace the operation with a host-side node swap.

All 12 local bytes and saved BP remain in shared A20-off 1 MiB RAM.
Effective offsets wrap at 16 bits; word operands, LES pointers, and the
four-byte RETF frame cross offset boundaries contiguously. BX, CX, SI,
DS, and SS are preserved. AX, DX, DI, ES, restored BP, SP, CS/IP, and
returned flags follow the reached source path. Entry SP advances by four.
AF after logical operations is architecturally undefined and represented
as zero; the tested Unicorn and DOSBox-X executions agree exactly,
without masking flags.

The routine has two traversal loops. A cyclic list, a missing active
member, or some malformed aliases can prevent DOS from returning.
The C model preserves those traversals and adds no loop cap or partial
core boundary. Its parity claim covers terminating source paths. The
audit bounds original emulation and its command externally; it rejects
a fixture that fails to reach RETF before calling the C candidate.
Null or incorrectly sized host RAM and null CPU are rejected before mutation.

## Validation and parity review

The focused test checks **72 independent list permutations**: every active
position in parties of one through eight members, with both equal and
alternating record segments. It verifies the resulting list order,
unchanged active pointer, preserved registers, frame/return state, and
invalid host inputs. Both this target and the existing `+0x025C` target
passed after a build with `-Wall -Wextra -Wpedantic -Werror`.

The original-byte audit passed **194 cases**, comparing the full 1 MiB
RAM and all 14 CPU words. It reached all 104 instruction starts and both
outcomes of all 11 conditional branches. Besides normal/random parties,
it includes offset/A20 wrapping, preserved DF, and directed aliases in
which local pointer writes change the head or active globals and thus
change a later source branch. These are not approximated by the normal
list permutation rule.

The separate DOSBox-X normal-core/386 synthetic oracle passed **21 cases**,
every active position in parties of one through six members. It compares
all 14 CPU words with no flag mask, 32 stack/frame/sentinel bytes, all
eight head/active global bytes, and all six node links (including unused
nodes). Its ignored result SHA-256 is
`de433edc3bb4830b853b0333c6ddc66190047ee7de83b7a3fe5ef98296c3d227`.
The header contains a DOS allocation segment, so another emulator
environment can produce a different file hash while retaining parity.

This oracle copies the hash-locked original function into a synthetic COM;
it is not a reached saved-game call/return frame. Actual native route
reachability remains unmeasured. All copied original instructions,
assembly, COM, native log/output, and build products stay under ignored
`resources/`. The common oracle reuses the repository CPU/RAM utilities
from [unit 0x5E +0x2879](U5E_PARTY_VALUE_2879.md).

```sh
cmake -S . -B resources/build-u6a-14a9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u6a-14a9 --target \
  poolrad_overlay_u6a_party_before_14a9_tests poolrad_overlay_u6a_record_remaining_025c_tests
ctest --test-dir resources/build-u6a-14a9 \
  -R 'poolrad_overlay_u6a_(party_before_14a9|record_remaining_025c)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn timeout 60s \
  python3 tools/overlay/u6a_party_before_14a9_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x6A:+0x14A9..+0x1600` |
| `source_candidates` | `src/recovered/overlay_u6a_party_before_14a9.c` |
| `signals` | Pinned extent and complete CFG, game party/active globals and caller chain, whole-RAM/CPU differential, independent permutation and native oracles |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` for terminating source paths under the documented RAM/CPU contract |
| `review_notes` | Cyclic/missing-active lists can fail to return in both source and model; live route unmeasured; logical AF represented as zero |

No classifier, identity ledger, or count is edited. Strict credit and
classification remain the PM's review decision.
