# Function alias audit and shortest callable leaf leads

This bounded follow-up consumes the 898-start queue produced by
[PR #377](https://github.com/huntergdavis/poolrad-decomp/pull/377), producer
commit `b30847c2b549ae8b251c979a2ccbd92aeffdc9f3`, with queue SHA-256
`5df46c8f516f0670f2974eeae9d2cab3504c39a10404f197f63ae3c9cca74a77`.
It starts from main `1e1b13adb1206f41053612f48f9d8a4548787b1e` and imports no
pending queue implementation. The queue JSON is a pinned local input.

## Alias and credit milestone

An independent resolver reconstructs original overlay addresses from inventory
prologues and raw entry words, resolves `+0x800` words, computes every resident
overlay-stub address, and joins resident/SYSTEM aliases by loaded image address.
It checks each queue ID, original address and alias against those source
coordinates, rather than trusting the queue's alias list as its own proof.
Source JSON/CSV artifact hashes must match the producer queue's provenance.

The owner-media audit on 2026-10-14 checked **2,200 IDs and aliases** across
**898 distinct originals**. It found **zero duplicate logical originals** and
**zero duplicate strict credits**. Strict full-C remains **66**, verified game
identities remain **128**, and **681** ownership labels remain unknown. The
whole-game denominator stays unknown. This adds no translation or identity
credit and does not compute a completion percentage.

The duplicate-credit invariant also rejects two differently named rows that
resolve to one original through an entry alias. Separate tests reject a
resident-looking duplicate of an overlay stub and a strict credit attached
to unknown ownership. Resident and overlay ownership must agree with their
source classification evidence.

Two equal-byte families are review leads, not logical-identity merges:

| Bounded span | Distinct starts | SHA-256 | Classification | Strict credits |
| ---: | ---: | --- | --- | ---: |
| 5 bytes | 34 | `fc1e97b8fe4158fdf9a46de4d3a1b4a04602fb787f6793778bc6d2b7a758d780` | helper candidate | 0 |
| 7 bytes | 4 | `3d5d31028d8927cd81dffdf3a86f216fa4eb84d0874dc3daada06bd31188514d` | helper candidate | 0 |

The seven-byte starts are `OVL_0038_0647`, `OVL_0038_1355`,
`OVL_0038_135C`, and `OVL_0038_319B`. The five-byte family has one start
in each of 34 units; all member addresses are in the local report. Matching
bounded bytes do not prove shared dispatch identity, source ownership or an
alias. Distinct original code addresses therefore stay distinct. No strict-C
credit is hidden in either family.

## Ranked handoff for pF/pG

[function_leaf_candidates.csv](function_leaf_candidates.csv) contains seven
uncredited overlay candidates, ordered by the measured span from prologue to
the last reached instruction. All seven are **game_evidenced / ownership
unknown**, not newly verified game-specific functions. Each has entry or
validated near-call evidence, documented engine-global evidence, and a bounded
control-flow walk with a return and no reachable call, interrupt, indirect
branch, external branch or port-I/O dependency.

| Rank | Original address | Span / reachable bytes | Instruction starts | Far-return cleanup bytes |
| ---: | --- | ---: | ---: | ---: |
| 1 | `0x6A:+0x0824` | 155 / 155 | 57 | 0 |
| 2 | `0x6A:+0x025C` | 166 / 166 | 64 | 4 |
| 3 | `0x77:+0x27DF` | 223 / 223 | 71 | 2 |
| 4 | `0x6A:+0x1601` | 287 / 287 | 84 | 0 |
| 5 | `0x6A:+0x14A9` | 344 / 344 | 104 | 0 |
| 6 | `0x33:+0x08A4` | 755 / 755 | 338 | 0 |
| 7 | `0x86:+0x3413` | 925 / 925 | 286 | 0 |

All seven have one reached far return. The CSV records each body hash and
original candidate bound. Several bounds extend past the reached return;
the ranking does not count that trailing region as part of the reached body.
This CFG check assumes the candidate start and bound; it is not a behavioral
translation, proof of loop termination, native execution, or ownership proof.
The resident ranking is not extended to the 126 ownership-unresolved resident
starts. Of the ten verified resident originals, nine already have strict
credit and the remaining program entry has calls.

The claim snapshot is deliberately explicit. On initial base main `1e1b13a`, source,
header and evidence searches found an existing bounded model for
`0x7D:+0x02F2` in [U7D_ENTRY1.md](U7D_ENTRY1.md); it is excluded from the
new-recovery queue. Open issues **3, 24, 123** and the initially open PRs were
checked for published claims. The final refresh against main `1d00750` found
open PRs **377, 380**. [PR #380](https://github.com/huntergdavis/poolrad-decomp/pull/380)
now claims `0x25:+0x0756`, whose 102-byte body hash exactly matches this audit;
it is excluded without awarding identity or strict-C credit. The 90-byte
`0x7F:+0x05E0` leaf is also excluded as `claim_unknown`: unpublished pF
ownership was requested from the PM, and live pane readback was unavailable.
Their dispositions are in [function_leaf_claims.csv](function_leaf_claims.csv).
The final ignored open-PR snapshot SHA-256 is
`c7107bd78b699aebc43b866ea5a44ec24683ecbbf8f4b04acd00dd87297278fc`;
the open-issue snapshot SHA-256 is
`8be37de25889561684864809349c77bb7ee65551f38f3309b8f43fc20e6f0ad0`.
`unclaimed_in_snapshot` describes this published snapshot, not unpublished
worker activity. Unpublished pF/pG claims were requested from the PM; assignment
requires that current claim check. Missing or non-unclaimed dispositions are
excluded by the tool rather than guessed.

**Next audit target:** establish the exact caller/ABI and game-state role of
`0x6A:+0x0824`, then `0x6A:+0x025C`, before requesting strict-C credit.
Their body hashes are
`c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7`
and `949abe77fa2823648d99cd3b76412560837646604ed9526f5f575dd8d3118b14`.
These are advisory recovery leads for PM assignment, not claims on pF/pG's work.

## Reproduce

Use the ignored queue products generated by producer commit `b30847c` with
Capstone 5.0.7 and the owner DOS build. START.EXE remains pinned by the input
provenance to SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
the audit directly reads GAME.OVR and checks SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 tools/overlay/test_function_alias_audit.py
mkdir -p resources/function-alias-audit
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/function_alias_audit.py \
  --queue resources/function-census-audit/queue.json \
  --queue-sha256 5df46c8f516f0670f2974eeae9d2cab3504c39a10404f197f63ae3c9cca74a77 \
  --inventory resources/function-census-audit/inventory.json \
  --resident resources/function-census-audit/resident.json \
  --classification resources/function-census-audit/classification.csv \
  --claims docs/re/function_leaf_claims.csv \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --json resources/function-alias-audit/report.json
```

Eleven new alias/CFG unittest methods and ten existing classifier methods
passed (21 combined). Repeated audit JSON and all public CSV rows are compared
with the measured output after refreshing claim dispositions.
The final seven-leaf report SHA-256 is
`b7f67981ac7008d4b321207ca560f9e5aa7e4aae2e8b4f924c30de1ee08434fb`.

Only addresses, hashes, classifications and counts leave the media audit.
Decoded instructions and original bytes are not exported. Equal-byte groups
and claim snapshots stay in ignored local audit products; the public CSV
contains address/hash/count metadata only. Prior lookup
`deja "poolrad duplicate logical function identities aliases shortest unclaimed callable leaves"`
timed out after 12 seconds (exit 124); no session findings were reused.
Reused evidence is the pinned producer queue, the canonical overlay
ledger/classifier, the resident callable audit and the linked source notes.
