# Saved-A +1965 native call/copy/return join

`pr_slot_a_saved_copy_1965_step` advances one instruction from the already
reached native caller `2A8E:1965`, through SYSTEM `131C:0B0B..0B24`, and
back to `2A8E:196A`. It models the far-call return frame, all 12 helper
instructions, register effects, ordered Pascal-string stores, and `RETF 4`
cleanup. It uses the existing CPU/RAM state type, in new replay files only.
The existing party-string chain already models string values; it does not
provide a standalone native CPU/stack bridge for this SYSTEM call.

At the next call, `2A8E:196A`, the stepper returns typed
**BLOCKED_0356_ABI** with CPU/RAM unchanged. Its report identifies original
unit `0x6F:+196A -> resident 0802:0356`, returning to `+196F`.
The existing resident string-value translation remains available; the full
native CPU/stack effect at that call is the next integration dependency.

## Ownership and recall

The preceding PR #378 head was recorded as
`6cf8f6231a9ea81ea49da00d25e909529fc8566b`. This independent branch
`codex/saved-copy-1965` is based on main `1d00750` and edits no key,
resident string, art, UI, or shared CMake implementation.
Before implementation, bounded recall
`timeout 10s deja "poolrad saved A 1965 SYSTEM 0B0B string copy C native CPU ABI"`
expired; no prior-session result was reused. Reused and checked repository
evidence: [native first/second copy captures](SLOT_A_NATIVE_U6F_1965_SYSTEM_COPY.md),
[party-prefix ABI](SLOT_A_PARTY_ENTRY.md), the existing party-string-chain
source, and the existing native checker
`tests/recovered/check_slot_a_u6f_1965_native.py`.

## Original bytes and native entry

| Source span | Bytes / instructions | SHA-256 |
| --- | --- | --- |
| Original caller `GAME.OVR[1F94B..1F94F]`, `+1965` | 5 / 1 | `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae` |
| Loaded caller `2A8E:1965..1969` | 5 / 1 | `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b` |
| SYSTEM `START.EXE[B781..B79A]`, original `0AF8:0B0B` | 26 / 12 | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| Loaded SYSTEM `131C:0B0B..0B24` | 26 / 12 | same as original |
| Next original call `+196A -> 0802:0356` | 5 / 1 | `965606c688f124e8166f41f356e580d9d2d0897521f1a7db054b55733834822d` |
| Next loaded call `2A8E:196A -> 1026:0356` | 5 / 1 | `f2fd40cc31e4aa01a804aac27d0a12ab79d65949e939e63564877d0448cc6dee` |

The host audit pins START.EXE (65231 bytes) to
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR (206105 bytes) to
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and SAVGAMA.DAT (13137 bytes) to
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

The native caller stack at `SS:SP=1D16:37A4` contains source
`2A8E:15E7`, followed by destination `1D16:3BB6`. The far call writes
return `2A8E:196A` at `SS:37A0..37A3`. The helper clears DF, saves DS
in DX, loads the two far pointers, copies the Pascal length and then its
payload forward, restores DS, and returns with `SP=37A8`. Thus it consumes
the source pointer and retains the destination pointer for the next call.
These are live arguments; no private literal is embedded in C or tests.

The first native call changes the destination's length and payload. The
second call stores the same values already present. The adapter executes
both stores in both cases; matching zero net change is not treated as
evidence that an instruction was skipped.

| Private endpoint / transcript | SHA-256 |
| --- | --- |
| First `+1965` RAM | `d019b40c52b233a773b23ef255e7927ed10f4f4da41c8e94f30cb4755a6d0121` |
| First `+196A` RAM | `67d1ff270888916e4f05ca07946d16e2c335775a5d5709fe44eff1a44da1c2a8` |
| First numeric CPU transcript | `4b24e13e3470057d2a93f4539b6570e35b09217ed11b9aada9f1abdb1e8f6269` |
| Second `+1965` RAM | `a2f88f2c6b6c326fd4bd556a1f549ec87c3886276ced73e2a1ca30077ba883b2` |
| Second `+196A` RAM | `e878e2d519ce6a722aa6ed2236262bbc02ac29005dbec04816af2e6a664ad77d` |
| Second numeric CPU transcript | `df2651e012b3363b905ee2afc7601d7a1b615d0dceefa20becef804c110ee472` |

Every RAM image is 655360 bytes. The existing native checker pins all
intermediate RAM hashes too. The first run has five captured stops and
the second seven; each starts by loading its actual `+1965` CPU/RAM.
The adapter then executes **13 instructions per invocation** and matches
**all 16 CPU fields and all 655360 RAM bytes at ten later checkpoints**
across the two runs. No output CPU or string bytes are borrowed from the
expected return snapshots. The final typed blocker preserves both.

## Bounded behavior and next dependency

The adapter accepts only the measured relocated caller/helper PCs. RAM
spans must fit the host image and their 64 KiB segment. An unsupported
span leaves CPU/RAM unchanged at that instruction; REP validates both
entire spans before its first write. Overlapping forward copies preserve
MOVSB's ordered reads/writes. Zero-count REP touches neither pointer's
memory. Individual pointer increments retain 16-bit wrap semantics, but
multi-byte spans crossing a segment or the host RAM end remain blocked.
The host is responsible for matching the original/loaded code identities.

XOR CH,CH leaves AF undefined in x86. This adapter normalizes it to the
observed zero; other affected arithmetic flags and CLD are modeled.
Re-entering the string instructions with DF set blocks as outside this
reached slice. These are explicit limits, not a general x86 emulator.

At the `+196A` blocker the live stack retains the destination/needle far
pointer, then the selected name-slot source, then the next output pointer.
Resident `0802:0356` consumes the two input pointers with `RETF 8`, retaining
its output for the later `+196F -> 0802:0425` call. The current adapter
neither executes the resident call nor substitutes its value-only C result
for unmodeled register, local-frame, or RAM effects.

The upstream `+18E5` native-close dependency from PR #378 remains unresolved.
This adapter requires an actually reached `+1965` snapshot; it cannot turn
a raw save into that live caller frame. There is no playable saved one-Up
claim or added strict game-routine/full-C identity. SYSTEM is runtime code.

## Adversarial review and reproduction

- `target_function`: reached unit-6F `+1965` far call and the complete
  26-byte SYSTEM copy body, through return to `+196A`.
- `source_candidates`: new replay-only `slot_a_saved_copy_1965.c`.
- `signals`: complete original/loaded span identity; exact argument and
  retained-pointer ABI; all CPU fields and RAM bytes at ten post-entry
  checkpoints; both value-changing and same-value store cases.
- `score`: four checked signal categories, not a probability.
- `verdict`: `match` for the two measured invocations; `unclear` for
  unrestricted runtime inputs, segment-wrap spans, and unnormalized AF.
- `review_notes`: both native literals have length one. Synthetic checks
  exercise a longer string, preserved AH, forward overlap, zero REP,
  failed source/destination/stack bounds, and outside-slice DF. Register
  agreement is checked independently of RAM agreement. No later callee
  behavior is inferred from the preserved output pointer.

Run `bash tests/replay/check_slot_a_saved_copy_1965.sh` for focused strict
C11 synthetic checks. Pass the two ignored native run roots in order
(`slot-a-native-u6f-1965-first-047`, `slot-a-native-u6f-1965-046`) to reuse
the complete source/hash/ABI audit and compare the C instruction sequence
against both captured CPU/RAM traces. All files, transcripts, game text,
and builds remain under ignored `resources/`. Shared CMake registration
is left to the integration owner.

The C native test also decodes the next original and loaded far-call
operands and checks the report's callee and five-byte return PC, including
the measured segment relocation. It never executes that instruction.
