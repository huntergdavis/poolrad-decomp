# U25 +0482: complete step probe composition

pr_u25_step_probe_0482_fullstate executes the complete 143-byte,
52-instruction caller through the real U3BD +047B nibble lookup and +031F
range leaf. Both public APIs share the existing caller body. The legacy
pr_u25_step_probe_0482 callback interface and its status values retain their
behavior; the new entry supplies every callee CPU/RAM effect without hooks.

## Selection and provenance

The full-history schema-6 successor to the requested schema-4 queue, against
main 00038c00, lists OVL_0025_0482 at rank 407, score 26. Higher ready hard
entries were already implemented, submitted, actively owned, or lacked a
raw-CPU callee interface. U33 +0810 belongs to Claude; U6C +4235 was submitted
as #886. U77 +27DF and U3C6 +0CB9 already have complete CPU implementations.
U68 +04DF's strict +0109 exposes a scalar map-update API, and U38 +0400's
resident +23DB exposes memory callbacks without CPU/stack effects. Those
two queue readiness caveats were reported to the VP while ds4 was offline.
The read-only operations ledger had no +0482 body event. All-ref history
found the merged callback model; the composition claim was sent to Claude.

A targeted deja query returned no matching sessions. Reused code/evidence:

- #768, commit add8e7aa: [existing caller](U25_STEP_PROBE_0482.md), its
  callback regression and original-byte checker. The caller remains one
  shared body. The old checker now links the added dependency objects.
- Merged #846: U3BD full-state nibble lookup composed by
  [U33 +012B](U33_CELL_LOOKUP_012B.md), including the loaded far-call/stub
  convention. The lookup and [range leaf](U3BD_IN_GRID_031F.md) are unchanged.
- #877: [U33 +0615](U33_GRID_PATTERN_0615.md) original-execution monitor,
  adapted for this caller's three bodies and one loaded stub. Generic CPU14,
  A20 and input-pin helpers retain their +01A0 audit attribution.

No acceptance, strict-manifest, denominator or credit update is included.
Per the VP's cross-review arrangement, Claude independently reviews this
lane's submissions; the author does not self-credit.

## Original identity

| Item | Pinned value |
| --- | --- |
| START.EXE | 65231 bytes; SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 bytes; SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U25 base / public entry | File 0x74D3; entry 6 at stub 002B:003E |
| Caller extent | [0482,0511), file [7955,79E4), 143 bytes / 52 instructions |
| Caller SHA-256 | bcfe56989d4733d260e0f85a7146d4cb56de3041cbf20b7de27851f0f513df2b |
| Callee site | +04C9 LCALL 03D5:002A; relocation at +04CC; continuation +04CE |
| Incoming sites | U1C +03F5, +1803, +1FF5, all fixup-backed far calls to 002B:003E |
| Return | +050E RETF 6 |
| U3BD +047B | [047B,057A), 255 bytes / 103 instructions; SHA-256 92d75878d963a7f5766b1ea49e2f6c1fb51287775d06a5b6ca485fee8bc43658 |
| U3BD +031F | [031F,0350), 49 bytes / 18 instructions; SHA-256 ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b |

The audit validates both overlay descriptors, all three incoming fixups,
the caller's sole outbound call/fixup and all three body hashes/counts.
Original instructions, extracted listings and generated fixtures remain
under ignored resources.

## ABI and ordered behavior

Entry SS:SP holds return IP/CS. Byte-word arguments at SP+4, +6 and +8 are
direction, y and x. Their high bytes remain observable on pushes. The
caller initializes count, stop and result locals, then reads the live far
window pointer at DS:49D2 and its word at +01CC.

If that word is zero, result becomes 2 and word 2 is stored through the
live DS:49D6 far pointer at +0582. This path never reaches the lookup stub.
Otherwise the caller probes at most twice. It calls the real nibble lookup
with its pushed x/y/direction words and full far frame. A nonzero result
sets stop; a zero result increments count/result and steps the caller's
argument bytes: direction 0 decrements y, 2 increments x, 4 increments y,
6 decrements x, and other values leave both unchanged. The callee's own
coordinate wrapping affects its pushed copies rather than these caller
argument words. Unrecognized selectors preserve the callee's stale local.

All operands are reread where the original reads them. CPU state is copied
by named fields at the callee boundary. The real RETF6 performs the callee
cleanup once; the wrapper adds no synthetic cleanup. The final outer RETF6
reads the actual frame, including window writes that replace its words.
The new entry uses physically contiguous return bytes with A20 wrapping.
The legacy entry retains its existing return behavior.

The new API requires exactly 1 MiB synchronous A20-off RAM, distinct stable
CPU/RAM objects, IP=0482 and SP in [0080,FFFA]. A conservative 128-byte
stack reservation rejects caller-code overlap atomically. All reached
original bodies and loaded stubs must remain immutable; other data/code
aliases are outside the profile. Logical AF follows the dependencies'
zero profile. No interrupts or concurrent mutation are modeled.

At the reached call, the original 9A opcode and stub offset are validated,
then the loaded EA stub and target +047B are checked. Stub and loaded-body
segments may relocate. Malformed metadata returns CODE at +04C9 with prior
caller effects preserved. CALLEE and TRANSFER preserve the reached callee
state; no scalar fallback is substituted. The 2,083 original cases all
complete outer returns, so no changed nested-return coverage is claimed.

## Fresh validation

The new audit compares every byte of 1 MiB RAM and all 14 CPU words,
including all FLAGS bits, without masks. Actual original instructions run
through the loaded far-jump stub and both callees. A20 has shared backing
at 0 and 1 MiB. Reached code/stub bytes are checked for immutability.

**2,083 cases pass**, all with actual outer returns. Instruction coverage
is **52/52 caller + 103/103 nibble + 18/18 range = 173/173**. All **16/16
caller conditional outcomes** are covered. No instruction-budget case is
silently excluded: failure to return is a failing audit.

The matrix includes 63 window/direction/map-pattern combinations, four
second-probe stops, 1,152 coordinate/direction/map cases, all 256 direction
bytes, six stack endpoints, 152 window/stack aliases, 64 pointer/stack
aliases, 76 map/stack aliases, A20 and relocated code/stub cases, incoming
FLAGS and 300 deterministic random cases. Whole-RAM comparison catches
callee frame writes and argument-copy mutations as well as the result.

The existing callback differential also passes **157 cases**, with 52/52
caller instructions and 16/16 outcomes. Its historical FLAGS mask remains
explicit in that checker; the new composed audit has no mask. Strict C11
warnings, ASan/UBSan, the recovered-library build, and both focused CTests
pass. New regression assertions cover actual lookup stop/step behavior,
changed outer return from a window alias, reached invalid-stub failure,
and atomic argument/code-overlap guards.

| Review field | Assessment |
| --- | --- |
| target_function | OVL_0025_0482, complete [0482,0511) |
| source_candidates | Shared caller core and fixed U3BD +047B/+031F composition |
| signals | Pinned original bodies/descriptors/fixups, real call frames, whole CPU/RAM comparison, complete instruction/outcome coverage, legacy compatibility |
| score | 2,083 exact composed cases; 173/173 chain instructions; 16/16 caller outcomes; 157 legacy callback cases |
| verdict | Author evidence submitted for independent acceptance |
| review_notes | Synthetic parity does not establish natural saved-game reachability; arbitrary alias space is not exhaustive; immutable-code and stack profile applies |

## Reproduction

~~~sh
python3 tools/overlay/u25_step_probe_0482_composed_audit.py \
  --exe /path/to/ignored/resources/START.EXE \
  --ovr /path/to/ignored/resources/GAME.OVR
cmake -S . -B /tmp/poolrad-build
cmake --build /tmp/poolrad-build --target poolrad_overlay_u25_step_probe_0482_tests
ctest --test-dir /tmp/poolrad-build --output-on-failure -R step_probe_0482
~~~

Capstone and Unicorn are required for the original audit. Media remain
owner-provided and ignored.
