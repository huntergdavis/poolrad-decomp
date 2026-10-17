# U25 +1153 and +12FB complete string cores

The [C composition](../../src/recovered/overlay_u25_string_cores.c) recovers
the packed-text decoder and string-by-address reader with their actual
engine-memory and SYSTEM string-runtime effects. Both expose fourteen
16-bit CPU words and shared 1 MiB RAM through the
[API](../../include/poolrad/recovered/overlay_u25_string_cores.h).
There are no callbacks, separated host string slots, or result-only callees.

The earlier ecl_helpers functions remain useful bounded convenience
models. They omit or abstract effects that prevent their direct use as
complete-state callees. This implementation provides separate entry points
without changing those APIs.

## Source identity

Owner-supplied START.EXE: 65,231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Owner-supplied GAME.OVR: 206,105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

| Body, end exclusive | File start | Bytes / instructions | SHA-256 |
| --- | --- | --- | --- |
| U25 [1153,12FB) | OVR 0x8626 | 424 / 166 | 51ffb0a24648a5c249b00849c36964bfb21ebdc2bc2802269d11d4e47dad5fcd |
| U25 [12FB,154D) | OVR 0x87CE | 594 / 229 | 16bf5a9e5c1fe284cdaca16e5711975077d7f4b89369617db3a2dc0b77c6960e |
| U25 [06D3,06FB) character remap | OVR 0x7BA6 | 40 / 16 | 17523f3ee51e77b906204fc675d61e6d6d9a83dfed02333125a53ee014924715 |
| U25 [06FB,0756) address classifier | OVR 0x7BCE | 91 / 28 | e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347 |
| SYSTEM [0B0B,0B25) string load | EXE 0xB781 | 26 / 12 | 4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f |
| SYSTEM [0B25,0B49) string store | EXE 0xB79B | 36 / 16 | 009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118 |
| SYSTEM [0B98,0BC4) concatenate | EXE 0xB80E | 44 / 19 | c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64 |
| SYSTEM [0C3B,0C4D) character string | EXE 0xB8B1 | 18 / 8 | b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484 |

The +12FB executable body ends with RETF 4 at +154A. The next 32 bytes
[154D,156D) are the set constant used by +156D. Treating the entire interval
to +156D as instructions produces a misleading extent. Both targets are
internal U25 bodies. The audit verifies the U25 descriptor, all source
hashes, every near target, and every external-call relocation.

## Recovered behavior

+1153 takes buffer index at SS:[entry SP+4] and packed byte count at
SS:[entry SP+6], using their low bytes. It clears the length byte of
DS:(6FC8 + 256*index), initializes its phase and consumed-byte count, and
increments DS:49ED before each source byte. It obtains the script far
pointer from DS:49DE for each read and uses offset 6700 plus the PC.

The four six-bit phases preserve the exact partial-group behavior: a
complete three-byte group emits its fourth code after the loop. Zero
codes are skipped; codes 1..31 map by adding 40h. Each append executes
the actual local remap and load/character/concat/store routines with their
real stack frames, temporary Pascal strings and register effects.
The 255-character cap follows the original concatenation's byte carry
handling and store instructions. It is no longer a guessed host cap;
the PC continues advancing and later characters still perform runtime
calls after the result length reaches 255.

+12FB takes address at SS:[entry SP+4] and buffer index at
SS:[entry SP+6]. It classifies the address, clears the target length,
and reads through the original window pointers. Classes 0..2 stop only
on a zero word: a word with low byte zero and nonzero high byte appends
a zero character. Class 3 stops on a zero byte. The byte index wraps
after 255 and is not replaced with an unbounded host index. Consequently,
a 256-element nonzero window can loop indefinitely despite a full output
string. Class 4 leaves the slot empty. Address 6B00 copies the Pascal
string at the active-character far pointer DS:5D92, using the actual FF
store cap; this path does not impose a host name-length restriction.

SYSTEM helpers execute CLD, LES/LDS, length operations, and forward
byte-by-byte REP MOVSB effects. Reads remain ordered with writes, including
overlap between strings, tables and stack locals. There is no memcpy or
memmove replacement. Far returns restore the original frame and consume
the exact argument counts; unexpected callee returns expose TRANSFER.

## Contract and remaining limits

The caller supplies the already-loaded SYSTEM code segment. Code, loaded
bindings and RAM are synchronous and immutable except for original data
writes; data writes must not overlap code. The API requires RAM distinct
from CPU, exactly 1 MiB of RAM, the correct entry IP, a positive budget and
entry SP in [0218,FFF8]. The lower bound reserves the deepest complete
frame without offset wrap; it is conservative for +12FB. Effective offsets
wrap at 16 bits and physical addresses at 20 bits. Individual word/LES
bytes are physically consecutive.

Each reached loop header at +1172, +1327, +13D3, +1453 or +14D2 consumes
one budget unit. LIMIT stops before that original instruction, exposing
the full reached CPU/RAM state. It is a host boundary rather than an
invented DOS return. The normal routines return through RETF 4.

Logical and shift AF is set to zero by model convention. Multi-bit shift
OF is retained; later comparisons/arithmetic overwrite it before the
documented loop/return boundaries. Matching Unicorn is not native proof
for undefined flags. Native saved-route reachability, A20 crossings,
wrapped stack profiles, loader execution, asynchronous RAM changes and
self-modifying code remain outside the measured evidence.

## Validation

The [differential audit](../../tools/overlay/u25_string_cores_audit.py)
runs the complete original caller and dependencies in Unicorn on synthetic
RAM. It relocates only far-call segment operands, executes every dependency
body, and compares all fourteen CPU words and all 1,048,576 RAM bytes at
return, the exact host-budget boundary, or an unexpected callee return.
No runtime callee is stubbed.

The final run passed **177 +1153 cases and 187 +12FB cases**, including
five exact budget stops and two corrupted-callee-return stops. It exercised
166/166 +1153 instructions, 229/229 +12FB instructions, and all 44 local
helper instructions. The SYSTEM helpers exercised 54/55 instructions,
giving 493/494 instructions across the complete composition. Every case
matched all fourteen CPU words and every RAM byte. The matrix includes
100 seeded random cases per target and six output/stack-overlap fixtures.

Focused CTest, strict-warning compilation, AddressSanitizer,
UndefinedBehaviorSanitizer and the entire recovered-library build passed.
SYSTEM +0B3B is the stricter-cap selection instruction: every composed call
pushes cap FF, so that instruction is unreachable through these callers.
Its transcription is retained; it is not counted as exercised.

The [focused C test](../../tests/recovered/test_overlay_u25_string_cores.c)
checks synthetic six-bit decoding, zero-code skipping, the 255-byte cap
with continuing PC advancement, PC wrap, empty-path DF preservation versus
runtime CLD, low-zero/nonzero-word handling, name copy, invalid address,
nonterminating scan/index wrap and atomic API rejection.

## Parity review and reuse

target_function: OVL_0025_1153 and OVL_0025_12FB.
source_candidates: pr_u25_unpack_text_1153 and pr_u25_string_by_address_12fb.
signals: exact hashes and extents; descriptor/call-fixup resolution; ordered
memory/register/flag/frame transcription; whole-state original-byte execution.
score: four evidence classes. verdict: match within the documented contract.
review_notes: the older host-state APIs are not promoted by association.
No strict-credit or queue-accounting change is claimed before independent
review. A caller can now compose these APIs directly under their explicit
memory/budget/loaded-code contract; OVL_0025_0000 itself remains a separate
body to translate.

The required pre-implementation deja query "1153 12FB" found Claude session
997d7fe9-a03 and Codex session 01a14af7-41a, corroborating the partial-core
status and ownership/bounds investigation. FUNCTION_LEDGER.md and
ecl_helpers.h supplied prior semantic hypotheses, independently checked
against the complete original bodies. SYSTEM file mappings/hashes reuse
resident_inventory.py, RESIDENT_STRING_0425.md and U6F_PARTY_STRING_CHAIN.md.
CPU arithmetic and physical-memory conventions are adapted from the merged
overlay_u3c2_entry4_057b.c. Their prior parity does not establish this result.

Reproduction with owner-supplied media and installed Capstone/Unicorn:

~~~sh
python3 tools/overlay/u25_string_cores_audit.py \
  --exe /private/START.EXE --ovr /private/GAME.OVR
cmake -S . -B /tmp/poolrad-strings-build -DBUILD_TESTING=ON
cmake --build /tmp/poolrad-strings-build --target poolrad_overlay_u25_string_cores_tests
ctest --test-dir /tmp/poolrad-strings-build -R poolrad_overlay_u25_string_cores_tests --output-on-failure
~~~

Original media, disassembly and private logs remain under ignored resources/.
