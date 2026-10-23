# Unit 0x6A +019C link high-bit clear

`pr_u6a_link_high_clear_019c` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x6A:+019C..+0213` and composes its one external call,
unit `0x86` public entry 6 (`+323D`, the merged
[record-table predicate](U86_RECORD_PREDICATE_323D.md)), reached through
the overlay stub `0096:003E`. The routine walks the far-linked node list
of a record and clears bit 7 of three node bytes wherever the predicate
accepts the node. It is the second half of the pair documented in
[U6A_RECORD_WEIGHT_0000.md](U6A_RECORD_WEIGHT_0000.md); the recall
attempt, branch search and reused work are recorded there.

## Loaded-state precondition

Added 2026-10-22 by par-e (credit authority), VP i21329, ruling i21377. This states a condition the credit always
assumed; the credit is unchanged.

This credit holds **only while every overlay unit the body far-calls is resident**: each stub below must hold the
loaded five-byte far `JMP` (`EA`) to the named entry. The credited C never reads these stubs. It models each `LCALL`
as a direct transfer to the loaded entry, so it cannot refuse when the unit is absent, and a caller must check the
stub before it runs the C. When the unit is not resident, the original's `LCALL` lands in the overlay manager, which
loads the unit with DOS reads. This C does not model that path. The differential in this document runs only the
resident case: it either places a loaded stub or patches each `LCALL` straight to the entry.

| Credit | Stub | Target | Call sites |
| --- | --- | --- | --- |
| `OVL_006A_019C` | `0096:003E` | unit `0x86` entry 6 `+323D` | `+01C3` |

Stubs were resolved from the original bytes: each `LCALL` segment is matched to its unit's `memory_segment`, and
the slot `0x20 + 5 * entry` gives the entry. SYSTEM (`0AF8`) calls are resident and need no stub.

Measured consequence (par-b, i21375). In hybrid run `hybrid-b5-v1-route2`, `OVL_006A_0214` (which composes
this body) was equal to the original on its first arrival. On its second it differed first at linear `0816E`
(PSP:002E, where DOS saves the caller's SP on each `INT 21h`): C `D8`, original `68`. The original had taken the
overlay-manager path. The hybrid now runs this C only when the stub is the loaded far `JMP`.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x6A | GAME file base `0x17831`, stub segment `0x0075`; `+019C` is an internal prologue, not a public entry |
| Function | GAME file `0x179CD..0x17A44`; **120 bytes, 42 instructions, three conditional branches, one far call** |
| Function SHA-256 | `d1c2b5b72d6abc696263f11879fa61be5a86c7ed7d4e0d040d9b2509cba34972` |
| Far call | `+01C3` `LCALL 0096:003E`, segment word fixup-backed; unit `0x86` entry 6 `+323D`, 77 bytes, SHA-256 `2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd` |
| Exit | `RETF 4` at `+0211`; exact end `+0214` exclusive |

Every branch target lies inside the 120-byte body. The audit decodes
each unit-0x6A routine from its own prologue and finds exactly three
aligned `PUSH CS; CALL +019C` sites, all game-evidenced:

| Owning routine | Call site | Argument evidence |
| --- | --- | --- |
| `+0214` | `+0240` | the same far pointer it has just passed to the strict [`+0159` record high-byte clearer](U6A_RECORD_CLEAR_HIGH_0159.md), then it follows that record's `+104/+106` link |
| `+0B5B` | `+0BC4` | pushes DS `[5D94]:[5D92]`, the active character record pointer used by the reviewed [U25](U25_PARTY_INDEX_0756.md), [U7F](U7F_STATE_FLAGS_0F7E.md) and [U38](U38_EFFECTS_INSTALL_31A2.md) evidence |
| `+0B5B` | `+0E25` | the same DS `[5D94]:[5D92]` pointer |

The node byte `+2E` indexes the `5582` table exactly as the predicate's
record byte does, and the node link words `+2A/+2C` and the record link
`+C8/+CA` are the list shape that `+0000` also walks. No gameplay meaning
is inferred for the three masked bytes; "high-bit clear" names the
operation.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS, `[SP+4]` record offset and `[SP+6]` record segment; `RETF 4`
leaves SP eight bytes above entry. The routine pushes BP and reserves six
local bytes: index `[BP-1]`, node far pointer `[BP-5/-3]`; `[BP-6]` is
never written.

It loads the record's `+C8/+CA` far pointer through `LES` on the stack
argument into the locals, then loops while `OR` of the two live pointer
words is nonzero: push the segment word, push the offset word (reread
after the first push), far call `+323D`, `OR AL,AL`; when nonzero, the
index local runs 1..3 and for each value the node pointer is reloaded,
byte `+3B+index` is read, masked with `7F` into DL, the pointer reloaded
again and the byte written back; then the node's `+2A/+2C` link replaces
the locals. The far call is modelled as the loaded-state transfer with
the caller's CS and `ip + 2C` pushed, the `+323D` model run on the same
RAM/CPU words and its RETF frame checked (`TRANSFER` is a guard that the
callee's own `[BP-1]` write cannot trigger). The exit path leaves AX and
DX zero (the null pointer words), BX/SI preserved, CX changed only when
the predicate ran (`CL=4`), ES:DI the last `LES` source, and the flags of
the final `OR AX,[BP-3]`: ZF and PF set, CF/OF/SF clear, undefined AF
represented as zero, which both tested engines also produce. Offsets wrap
at 16 bits; words, LES operands and the return frame are physically
contiguous; physical addresses wrap at 1 MiB. Null RAM/CPU or less than
1 MiB is rejected before mutation. A cyclic list never returns, in DOS
and in C; no host cap is added.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`) checks a
three-node list with true/false/true predicates (bytes `81 7F FF` to
`01 7F 7F`, false node untouched, neighbouring bytes untouched, exact
registers, flags `7246`, final index local 3), the empty list, an
offset-only head with a false node, and the host guards.

The original-byte oracle passes **507 returning cases** for this body
with the same fixture families as `+0000`, each comparing the entire
1 MiB RAM and all 14 CPU words with no flag mask; all 42 instruction
starts and all 6 conditional outcomes are reached. One directed
self-linked node fixture fails the original's 100,000-instruction budget
and is excluded; C is not invoked on it. The DOSBox-X normal-core/386
synthetic oracle passes **200 cases** for this body in the shared run
described in the `+0000` note (result SHA-256
`19e6e9fb4325b2a995aac25e2ea34e07994e144d3f614a9a9ee50705e859a231`,
COM segment `0814`), comparing all 14 CPU words, 128 frame bytes, the
288-byte record and the 256-byte node area with no mask. The reproduce
commands are in the `+0000` note; the focused test target is
`poolrad_overlay_u6a_link_high_clear_019c_tests`.

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x6A` `+019C..+0213` with its `+323D` dependency composed |
| `source_candidates` | `src/recovered/overlay_u6a_link_high_clear_019c.c` |
| `signals` | Exact hash/CFG, three aligned game-evidenced callers including the active-record pointer, whole-RAM/CPU differential with full instruction and branch coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` for terminating source paths under the documented RAM/CPU contract |
| `review_notes` | Cyclic lists excluded; loaded-state stub only; natural saved-game reach unmeasured; `TRANSFER` not exercised |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.

## Reproduce

Added 2026-10-22 by par-e (VP i21177). Until then this document described its checks but
gave no command to rerun them. The evidence kind is recorded in
[credit_evidence_kind.csv](credit_evidence_kind.csv) (labels: [CREDIT_EVIDENCE_KIND.md](CREDIT_EVIDENCE_KIND.md)).

Original-byte differential. The script runs the credited C as a direct target and compares
it with the original DOS bytes under Unicorn. It needs the private media under ignored
`resources/`.

```sh
python3 tools/overlay/u6a_record_weight_0000_audit.py --unicorn
```

- `tools/overlay/u6a_record_weight_0000_audit.py` covers target `019c` (`pr_u6a_link_high_clear_019c`). Measured on `ee07bd68`: +019c: 507 original-byte cases passed with full RAM and CPU equal; 42/42 instruction starts; 6/6 conditional outcomes. One directed cyclic fixture exceeded the original's return budget, so the C was not invoked for it.

Synthetic C tests. These are hand-derived checks of the C, with no DOS bytes, so they are not
an original-byte differential:

```sh
cmake -S . -B build
cmake --build build --target poolrad_overlay_u6a_link_high_clear_019c_tests
ctest --test-dir build -R '^poolrad_overlay_u6a_link_high_clear_019c_tests$' --output-on-failure
```

| Credit | Exercised by |
| --- | --- |
| `OVL_006A_019C` | the whole `poolrad_overlay_u6a_link_high_clear_019c_tests` executable |

Measured on `ee07bd68`: `poolrad_overlay_u6a_link_high_clear_019c_tests` passed (1/1 under ctest).
