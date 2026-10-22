# U86 +028D: append grouped, prefixed string nodes

The complete routine appends an optional group-heading node and a
prefixed item-name node to the list at DS:4766/4768. The argument's low
seven bits select the item; bit seven selects one of two immutable
Pascal-string prefixes. The raw CPU14/RAM implementation composes the
existing SYSTEM GetMem, StrStore, StrLoad and StrCat APIs unchanged.

## Identity and call structure

The executable body is **[028D,044E), 449 bytes / 165 instructions**,
ending at RETF 2 at +044B. This agrees with the queue extent.

- GAME.OVR file span [0x272A5,0x27466); unit 0x86 starts at 0x27018.
- Body SHA-256:
  5f82bdbca3270a4d7dcf6a93f41fa14debd04f8f4a21d8860a9b4a64d2cd5038.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- Six prefix bytes at CS:[0287,028D), two length-two Pascal strings,
  SHA-256 c6ffaae3d56c86686f0b82d639479f1d874d650314dc72dbe0fde69a6b2700a7.
  Their bytes stay under ignored resources/ and are read live by the model.

| Caller sites | SYSTEM dependency |
| --- | --- |
| 02AE, 031F, 039C | +023F GetMem |
| 038B, 03DE, 03F3, 042A | +0B25 StrStore |
| 0403 | +0B0B StrLoad |
| 041C | +0B98 StrCat |

All nine segment words have loader fixups. Each reached LCALL's opcode
and target offset are checked, and its loaded segment is read live.
Five incoming PUSH CS / near CALL sites are asserted: +04EE in U86
+044E, and +060E/+064A/+067A/+070A in U86 +059C. This is a private helper,
not an entry-table slot. The audit hashes the original caller and media,
decodes the entire dependency chain, and rejects execution outside it.

## Raw state and ordered effects

Entry is a far-call frame with one word argument at SP+4; its low byte
is used. PUSH BP and SUB SP,0106h reserve a temporary Pascal string,
a far node pointer, and two bytes of bookkeeping. The list's node layout
is 41 string bytes, heading flag at +29h, and next offset/segment at
+2Ah/+2Ch. GetMem requests 46 bytes and rounds to 48.

For a nil list, allocate the first node, publish its pointer, and set
the previous group and count to zero. Otherwise walk to the terminal
node, starting the count at one and incrementing it for each preceding
node whose heading flag is zero. The byte count wraps. The previous
item is read from DS:476A + signed8(count) - 1; its group comes from
DS:31FB + 16*item. Allocate and link a new node after the existing tail.

Clear the new node's link. Compare the selected item's group byte at
DS:31FB + 16*(argument & 7Fh) with the previous group. If different,
StrStore copies up to 40 characters from DS:0A79 + 41*signed8(group),
marks the node as a heading, then allocates/links another node. The CBW
instructions sign-extend bytes; they are not unsigned array indices.

StrStore copies the selected prefix into the item node. StrLoad copies
that string to the SS temporary, StrCat appends the selected name from
DS:28E9 + 41*(argument & 7Fh), clamping the temporary length to 255.
StrStore then copies at most 40 characters back to the node. Clear its
heading flag and write the selected item byte to
DS:476A + signed8(count). No host array or list-length limits replace
these raw accesses; count wrap can make the history write alias the list
head itself.

Every read/write follows the original instruction order. In particular,
links are reread after storing a new pointer, and strings copy forward,
so data/heap/stack aliases retain their effects. Failure on the second
allocation leaves the already-published heading node and its text intact.

## API profile and stop states

The API requires exactly 1 MiB A20-off RAM, entry IP=028D, conservative
initial SP in [0140,FFFA], and CPU/diagnostic storage distinct from RAM.
All loaded code and the two prefixes must be immutable; writes must not
overlap them. Effective offsets wrap at 16 bits and physical addresses
at 1 MiB. LES pairs and RETF frames are physically contiguous. Every
reached dependency return frame must fit inside SS:0000..FFFF, including
after alias-induced frame changes. Interrupts, faults and self-modifying
code are outside this profile.

**Stack-top RETF wrap: unreachable inside this window (reviewer-added 2026-10-20, VP i19693).** With entry SP E in [0140,FFFA], SYSTEM GetMem `0AF8:023F` (called at `+02AE`, `+031F` and `+039C`) is entered at E−10Eh (`PUSH BP`, `SUB SP,106h`, the size word and the far return), i.e. in [0032,FEEC]. Its own far return executes at the entry SP, because the frame is balanced, so at most FFFA. None of these ranges reaches FFFE/FFFF. That is the only band where DOSBox-X wraps a RETF's CS word within SS while the merged SYSTEM models and Unicorn read it contiguous ([REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#retf-stack-top-wrap-a-recorded-class-deliberately-not-swept-2026-10-20)). The window, not a measurement, keeps the band out of reach, and a caller outside the window is outside the contract.

max_list_visits must be positive and counts visits to +02D4, including
the terminal node. LIST_LIMIT stops before the next such instruction.
GetMem's max_heap_steps resets per allocation and counts taken free-list
walk edges; zero is allowed. CALLEE preserves raw dependency status and
state, including RunError 203, custom HeapError, allocator walk limit,
or an altered internal return. CODE stops before a malformed reached
LCALL. TRANSFER preserves a changed direct return. No stop rolls back
prior effects. ARGUMENT leaves CPU/RAM unchanged and initializes the
optional diagnostic to NO_CALLEE/-1.

Logical AF follows the measured normal/386 zero convention. MUL uses
the accepted U86 +07D6 undefined-flag profile; ADD overwrites those flags
before any observable stop or return in this caller. The fixed shifts
use the preceding CL=4, with values zero-extended from a byte.

## Evidence and reproduction

- **303 original-chain cases**, all CPU14 words and all 1,048,576 RAM
  bytes equal without masks: 265 returns, 23 RunError stops, one custom
  HeapError, two allocator walk limits, two caller-list limits, nine
  malformed call sites, and one changed direct return.
- Caller coverage **165/165 starts and 10/10 conditional outcomes**.
  Combined caller/runtime coverage **313/319 starts and 32/38 outcomes**.
  Fixed size 46 and explicit HeapError boundaries leave some allocator
  paths unexercised; this is not exhaustive runtime coverage.
- Inputs cover all 256 argument bytes; nil/existing lists; mixed heading
  flags; count boundaries 127/128/129/254/255/256/257; signed group bytes;
  short, long and truncated strings; exact/split/top allocation; first
  and second allocation failure; cyclic lists/heaps; malformed bindings;
  odd/boundary stacks; SS=DS; A20 wrap; heap/stack overlap; random RAM.
- **79 DOSBox-X normal-core/386 cases**: 69 returns and ten RunError 203
  exits. Every CPU14 word, 1,024 stack bytes, 65,536 DS bytes, 65,536 heap
  bytes, and 16,384 existing-list bytes match unmasked. Both prefixes,
  signed group/count cases and failure after a heading are included.
- Native result SHA-256:
  dcd9796211b18aebde3080078d616c5a9e6d63a108f93a95c69534a32d1cda5e.
- **3,673 synthetic C checks** pass with ASan/UBSan. Focused CTest: **1/1**.
- Private receipt resources/u86-028d/evidence.json SHA-256:
  c6e46a8fa46b462ace99c88493a93f3a949f2ad5efeeb9919a8ee80866e5ec27.

The native oracle executes original relocated caller/SYSTEM code with
IRQs masked. A far jump at RunError's entry records its incoming state;
no allocator or string result is substituted. These frames are synthetic,
not naturally captured gameplay reach. Original media, decoded bytes,
generated assembly/COM and captures stay under ignored resources/.

With owner-supplied media under resources/DOS/POOLRAD and Capstone/Unicorn:

    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u86_string_node_028d_audit.py --unicorn --native --output resources/u86-028d/evidence.json
    cmake -S . -B resources/u86-028d/build -DBUILD_TESTING=ON
    cmake --build resources/u86-028d/build --target poolrad_overlay_u86_string_node_028d_tests
    ctest --test-dir resources/u86-028d/build -R '^poolrad_overlay_u86_string_node_028d_tests$' --output-on-failure

## Reuse and parity assessment

Local recall was checked before implementation; no target-specific source
implementation was reused. The current queue identifies the runtime
readiness work in PRs #987/#988. The actual call sites and headers confirm
those contracts. GetMem from [PR #983](https://github.com/huntergdavis/poolrad-decomp/pull/983),
string leaves from [PR #987](https://github.com/huntergdavis/poolrad-decomp/pull/987),
and the existing StrStore full-state API are linked unchanged.
Memory/arithmetic primitives follow accepted U1C +18F8/U3C2 +0909;
MUL follows U86 +07D6. Original-chain stop tracking adapts U6A +0E37;
the native disk-fed capture adapts U1C +18F8 with an added existing-list
region and a larger allocation arena.

- target_function: OVL_0086_028D, executable [028D,044E).
- source_candidates: overlay_u86_string_node_028d.c with unchanged
  SYSTEM GetMem, StrStore, StrLoad and StrCat implementations.
- signals: exact hash/extent, nine call fixups, five incoming callers,
  stack/list/string fields, signed indices and ordered alias effects,
  full-state differential, native replay, independent semantic tests.
- score: five evidence categories (identity, structure, differential,
  native replay, contract tests); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: synthetic native reach, conservative stack/nonwrapping
  return profile, immutable prefix data, explicit allocator error stops.
  Acceptance, classifier, accounting and credit remain with the reviewer.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven in a worktree WITHOUT `resources/` (what CI has) with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0086_028D --require-current-main
```
