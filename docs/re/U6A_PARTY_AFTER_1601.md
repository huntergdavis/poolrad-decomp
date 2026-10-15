# Unit 0x6A +0x1601 active-party reorder

`pr_u6a_party_after_1601` translates the complete call-free routine
`+0x1601..+0x171F`. For a disjoint finite party containing active,
it moves active one place toward the tail. If active is already the
tail, it moves to the head. A singleton remains unchanged.

The bounded recall query
`timeout 30s deja "Poolrad u6A 14A9 1601 full C call-free"` ended 124
without results; the PM authorized continuing after timeout. Reused
repository evidence and code patterns are the [alias audit](FUNCTION_ALIAS_AUDIT.md),
the [verified/native party-pointer mapping](U6F_PARTY_INSERT_1446.md),
and the reviewed [adjacent +0x14A9 model and shared oracle](U6A_PARTY_BEFORE_14A9.md).
The two functions have separate source/header/test files and wrappers.
The shared oracle's alias-case names are made address-based to avoid
implying identical local layouts for the two originals; fixture data
and comparison behavior are unchanged.

## Exact original and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x6A | GAME file base `0x17831`, resident stub segment `0x0075` |
| Function | GAME file `0x18E32..0x18F50`; **287 bytes, 84 instructions, zero calls** |
| Function SHA-256 | `5d1b9b66f76f31872abcd27dc75a268a9b35263adc2c02fb9725a95e21ace285` |
| Exit | `RETF` at `+0x171F`; no arguments |

The 383-byte prologue upper bound includes 96 trailing bytes outside the
reached routine. All reached branches remain inside the exact 287-byte body.

Public entry 1 `+0x1E6F` has a descriptor-seeded static call path
`+0x1FAC -> +0x1C59`, `+0x1CCE -> +0x1780`, `+0x1813 -> +0x1601`.
PUSH CS precedes the final near call; the caller compares AL with `0x4F`
at `+0x180E`. This proves aligned callable ownership, not a saved-game route.

The routine directly manipulates the documented game party head DS
`[0x5D96/0x5D98]`, active pointer DS `[0x5D92/0x5D94]`, and member next
links `+0x104/+0x106`, established by the existing native u6F insertion
evidence. No new user-facing command name is inferred from the key byte.

## Complete ABI and ordered effects

The API enters after CALL with return IP/CS at SS:SP. It saves BP and
reserves eight bytes: current/predecessor at `[BP-4/-2]` and successor
at `[BP-8/-6]`. If head equals active it retains head as current;
otherwise it walks until current.next equals active.

It reloads active and saves active.next. If that saved pointer is zero,
active links the old head, head becomes active, and current links the
freshly reread successor locals. This last reread matters when earlier
global or record writes alias the locals. The singleton's temporary
self-link is then cleared.

If the successor is nonzero, active bypasses it by linking successor.next.
The code rereads head/active and, if equal, updates head to successor.
It then rereads current/active and updates current.next only when those
pointers differ. Finally successor.next becomes the freshly reread active.
None of these pointer comparisons or reloads is replaced by a cached
host-list operation; directed alias tests exercise the changed paths.

All eight local bytes and saved BP remain in A20-off shared 1 MiB RAM.
Effective offsets wrap at 16 bits; words, LES operands, and the four-byte
far-return frame cross offset boundaries contiguously. BX, CX, SI, DS,
and SS are preserved. Other registers and flags follow the reached path.
The tail-wrap path leaves arithmetic flags from the saved-successor OR;
other paths leave the final current/active CMP flags. Logical AF is
undefined and represented as zero. Tested native and Unicorn results
agree exactly without masking flags. Entry SP advances by four bytes.

Malformed/cyclic lists or a missing active member can prevent source
traversal from returning. The C model preserves this behavior and adds
no node cap or external hooks. Its parity claim covers terminating
source paths. The audit bounds original emulation and its outer command
and rejects nonreturning fixtures before invoking the candidate.
Invalid host RAM size, null RAM, or null CPU is rejected before mutation.

## Validation and parity review

The independent focused fixture checks **72 list permutations**, every
active position in one-to-eight-member parties with equal and alternating
segments, plus frame/return state, preserved registers, unchanged active
pointer, singleton behavior, tail wrap, and invalid host arguments.
Both party reorder CTest targets passed after a build with
`-Wall -Wextra -Wpedantic -Werror`.

The original-byte oracle passed **194 cases** for the entire 1 MiB RAM
and all 14 CPU words. All 84 instruction starts and both outcomes of all
nine conditional branches were reached. Cases include random parties,
offset/A20 wrapping, preserved flags, and directed stack/global aliases.
The shared fixture data remains identical to the adjacent routine's audit;
each target runs its own original bytes and candidate function.

The separate DOSBox-X normal-core/386 synthetic oracle passed **21 cases**,
every active position in parties of one through six members, comparing
all 14 CPU words with no flag mask, 32 frame/sentinel bytes, eight global
bytes and all six node links, including unused nodes. The ignored result
SHA-256 is `8b4caa0dfd3da0501adccf9b750bee302a7907a3b493791c0b45214bf7e92649`.
The header includes a DOS allocation segment, so another emulator setup
can yield a different result-file hash while retaining parity.

This native oracle executes a copied hash-locked leaf in a synthetic COM.
It does not establish saved-game call/return reachability. Original
instructions, COM, assembly, logs/results, and build files stay under
ignored `resources/`; public tests use synthetic state. CPU/RAM utilities
are reused from [unit 0x5E +0x2879](U5E_PARTY_VALUE_2879.md).

```sh
cmake -S . -B resources/build-u6a-1601 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u6a-1601 --target \
  poolrad_overlay_u6a_party_after_1601_tests poolrad_overlay_u6a_party_before_14a9_tests
ctest --test-dir resources/build-u6a-1601 \
  -R 'poolrad_overlay_u6a_party_(before_14a9|after_1601)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn timeout 60s \
  python3 tools/overlay/u6a_party_after_1601_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x6A:+0x1601..+0x171F` |
| `source_candidates` | `src/recovered/overlay_u6a_party_after_1601.c` |
| `signals` | Exact hash/CFG, party/active ownership and public caller chain, whole-RAM/CPU differential, independent permutations and native execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` for terminating source paths under the documented RAM/CPU contract |
| `review_notes` | Malformed traversal can fail to return; live route unmeasured; logical AF represented as zero |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.
