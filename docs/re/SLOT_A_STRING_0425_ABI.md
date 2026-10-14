# Saved-A resident +0425 CPU/stack/string ABI join

The new instruction adapter advances the reached saved-A call
`2A8E:196F -> resident 1026:0425` through its natural return to
`2A8E:1974`. It composes the merged PR #389 resident +0356 adapter and
SYSTEM Copy/substring/compare bodies, adding the original +0425 body,
SYSTEM C3B single-character constructor, and SYSTEM 182A ASCII uppercase.
It computes actual stack writes, live table reads, registers, and flags.
The existing value-only string transform is not used.

At +1974 the adapter blocks unchanged before the caller continuation
prefix. The typed report names the following original unit-6F
`+197E -> SYSTEM 0AF8:0B25` call and return +1983. This is a continuation
gate even though the helper instruction body is already translated:
the caller prefix and subsequent path are outside this slice. The
alternate resident +03FC Delete call remains blocked before its frame push.

## Recall and ownership

The new `codex/saved-string-0425-abi` branch started from main fb342db
separately from PR #389, then advanced through merged main 793f78b and rebased onto current main 46ea4a1.
The pre-implementation `deja` query for the resident +0425 CPU/RAM ABI
returned no matches (searched zero sessions); no prior-session result was
reused. Reused repository leads were independently checked:
[the resident inventory](RESIDENT_STRING_0425.md),
[the old native table input](SLOT_A_NATIVE_0425_TABLE_INPUT.md),
[the old native return](SLOT_A_NATIVE_0425_RETURN_OUTPUT.md), and
[the first view trace](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md).
The old captures did not supply all sixteen CPU fields at both ends of
one invocation. They are not combined into a fabricated full-state pair.
The merged [PR #389 instruction adapter](SLOT_A_STRING_0356_ABI.md) supplies
nested +0356 semantics and its original-byte checker supplies relocation
checks. No key-dispatch, arithmetic-lane, art, UI, shared CMake, or existing
value-only implementation is changed.

## Fresh same-invocation native evidence

A new isolated heavy DOSBox-X run, `slot-a-0425-abi-20261014`, used the
normal codeword/menu/load-A route. At the visible load chooser, breakpoints
were armed at caller +196F, loaded resident +0425/+0426, and caller +1974.
At each stop the debugger recorded
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`. No guest register or memory edits were made.
All game files, dumps, raw console output, and screenshots remain ignored
under this clone's `resources/`.

| Ignored evidence | Bytes | SHA-256 |
| --- | ---: | --- |
| Caller +196F RAM | 655360 | `76d331baf03e73951f46f88830691760a6ecb96d6af67a112416909bdda8b528` |
| Resident +0425 entry RAM | 655360 | `28cd43235f7a5e4cafdf2c309cffcf6e95c50b5eda59282e0b7384a63be36b79` |
| Resident +0426 RAM | 655360 | `c899cd755c9ebb268c3a4fa1b1e4f4e3c93122d275ee42c163f3a53baa9547ff` |
| Caller +1974 return RAM | 655360 | `a7705a78f6b014fa11406def3da8d1a255affdd573223f21d4645d50f6ddc35b` |
| Raw native transcript, four full EV rows | pinned | `728a2a6336abfebf39c8759737204b2ab5905cc6b194a7b210770e9609d7f082` |

The checker pins original START.EXE, GAME.OVR and SAVGAMA.DAT to the same
build/save hashes audited in PR #389, and validates every original/loaded
code span against every capture. The heavy debugger executable hashes to
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| CPU fields, hexadecimal | +196F | +0425 | +0426 | +1974 |
| --- | --- | --- | --- | --- |
| CS:IP | 2A8E:196F | 1026:0425 | 1026:0426 | 2A8E:1974 |
| SP | 37B0 | 37AC | 37AA | 37B4 |
| BP | 3DCA | 3DCA | 3DCA | 3DCA |
| AX | 0008 | 0008 | 0008 | 0008 |
| BX | 3490 | 3490 | 3490 | 349A |
| CX | 0000 | 0000 | 0000 | 0000 |
| DX | 14A0 | 14A0 | 14A0 | 14A0 |
| SI | 36AB | 36AB | 36AB | 36B3 |
| DI | 3ABF | 3ABF | 3ABF | 39BF |
| DS | 14A0 | 14A0 | 14A0 | 14A0 |
| ES / SS | 1D16 / 1D16 | 1D16 / 1D16 | 1D16 / 1D16 | 1D16 / 1D16 |
| FS / GS | 0000 / 0000 | 0000 / 0000 | 0000 / 0000 | 0000 / 0000 |
| FLAGS | 7246 | 7246 | 7246 | 7246 |

The far call is the first writer in the joined slice: it changes exactly
physical 2090C..2090F to the actual 2A8E:1974 return frame. The first
resident instruction, PUSH BP at +0425, changes exactly 2090A/2090B to
3DCA and decrements SP by two. These are native before/after observations,
not inferred effects. The full +196F-to-+1974 net delta is 93 RAM bytes.

At caller SP=37B0 the source far pointer is 1D16:3AB6 and output is
1D16:39B6. Resident BP becomes 37AA; SUB SP,0302 allocates the local frame.
Each table iteration reads DS:[0D19+i] at that instruction, constructs a
single-character Pascal string via C3B, PUSHes CS and makes the actual near
call to +0356. The latter's RETF consumes that paired frame. The adapter
preserves original ordering when table, source, destination, or local
buffers alias; it does not snapshot the table before executing the body.

## Original bytes and reached helpers

| Span | Size / instructions | SHA-256 |
| --- | ---: | --- |
| Resident 0802:0425..050E, START file 847C..8565 | 234 / 91 | `28c69009d8bd4218b1eb50754c3f32a6f36ef40d3927d630fc8481d10b99e5a3` |
| Same resident loaded at 1026:0425 | 234 | `7a6d1534f0a3b448a6eebc18108cc78743e14b842f3637acd7305e1cb8658bb5` |
| SYSTEM 0C3B, START file B8B1..B8C2 | 18 / 8 | `b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484` |
| SYSTEM 182A, START file C4A0..C4B2 | 19 / 8 | `738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c` |
| Caller +196F original | 5 | `fe81385603a3fc55df47de14941cf1bdbf72d0bd7c936edaecb4ceda0c23d52e` |
| Caller +196F loaded | 5 | `78cec5fcdde872ab5e4c4add6d5debd1f4cf3d3430f3e12b1a051c4ce061d147` |
| Next caller +1974..1982 original | 15 | `40340a7f4f937729d3786c40ce28c8cc26d796f9abd977727eaafe0a254b1253` |
| Next caller +1974..1982 loaded | 15 | `85ccb6667d3a08f5fe27f10b74ee586c9487648ced403928853c8d8c06d4aa6c` |

Exactly seven +0425 far-call segment words relocate 0AF8 to 131C; the local
near call stays within resident 1026. SYSTEM C3B/182A bodies are identical
between original bytes and loaded RAM. The nested +0356 and B25/B57/C10
bodies and relocations are independently pinned through the reused checker.

## Validation and limits

The differential executes the hash-linked original loaded instructions in
Unicorn 2.1.4 from the captured CPU/RAM. Each C instruction compares all
sixteen CPU fields and the entire 655360-byte RAM image. REP instructions
are atomic C host steps; the emulator finishes their iterations before
comparison. The natural route matches after **6968 C instruction steps**
and at all three post-entry native CPU/full-RAM checkpoints, including the
actual return. Native snapshots only supply assertions and never inject
registers, output RAM, string payloads, or journals into the C adapter.

Seven synthetic original-byte cases cover empty source, ASCII boundaries,
prefix truncation, source/output aliasing, binary table misses, a matched
needle reaching Delete, and a table alias with the mutable local. The last
case proves that actual live table reads, rather than an initial table
copy, affect the reached dependency. All seven cases passed: respectively
1532, 6981, 11614, 8974, 5612, 195, and 864 C instruction steps. Together
with the native route, **42740** instruction steps compare all sixteen CPU
fields and full RAM with original-byte Unicorn execution. Synthetic frames are separate from
native evidence. Strict C tests check immutable dependencies, invalid PCs,
failed far/near frame spans, incomplete far operands, table/indexed-local
bounds, unsupported DF-set string writes, and null arguments.

Unsupported spans block atomically at the current instruction. A span must
fit the host RAM image and one 64 KiB segment; multi-byte segment/A20 wrap
is outside this adapter. Individual address arithmetic retains 16-bit
wrapping. Logic AF is undefined and normalized to observed zero; defined
arithmetic flags are calculated. New code uses the measured segment
relocations and requires identity validation for other captures. Earlier
save/loader prerequisites remain outside this boundary. The strict suite and the complete 42740-step native/synthetic differential
also passed with the C adapter instrumented by AddressSanitizer and UBSan.
No playable one-Up,
full-game behavior, or game-function/count increase is claimed.

- `target_function`: measured unit-6F +196F call through native +1974 return,
  including nested resident +0356 and reached SYSTEM bodies.
- `source_candidates`: new slot_a_string_0425_abi.c and addressed tables,
  composing the merged slot_a_string_0356_abi.c.
- `signals`: original/loaded bytes and relocation; natural full-state pair;
  native first-writer observation; per-instruction CPU/full-RAM differential;
  immutable alternate-branch/host-boundary checks.
- `score`: five checked signal categories, not a probability.
- `verdict`: bounded `match` for the measured route and tested prefixes;
  `unclear` for complete runtime behavior, unsupported Delete, and spans.
- `review_notes`: all four native stops belong to one unedited invocation.
  They are not independent playthroughs. Byte identity supplies no full-C
  credit. The +1974 continuation and Delete dependencies stay explicit.

## Reproduction

Run `bash tests/replay/check_slot_a_string_0425_abi.sh` for strict synthetic
C checks without media or Python. For the full private audit, install
Unicorn 2.1.4 under ignored resources and run:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_string_0425_abi.sh \
  resources/oracle-runs/slot-a-0425-abi-20261014
```

The native root contains POOLRAD inputs, four ram-*.bin images, matching
cpu-*.json derivatives, and the pinned raw native.log. Optimized Python
assertions are rejected. Shared CMake registration remains with the
integration owner.
