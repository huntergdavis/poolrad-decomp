# Unit 0x25 +0x0C77: bounded raw-RAM ECL store word

The new pr_u25_store_word_0c77 closes the class-4 stores and composes the
class-1 window write with the store hook's local effects in one source-ordered
A20-off RAM/CPU model. The original pr_rec_ecl_store_word core and separate
state-object hook API are unchanged. **This remains a bounded model, not a
whole-entry strict-C translation:** four far-call sites reach two unresolved
callees. No accounting manifest, identity set, battle card, or dashboard is
edited; no credit or playable-route completion is claimed.

## Provenance and reused evidence

The bounded deja query "poolrad u25 0C77 store word class4 0A74 raw RAM post
store hook" timed out after 15 seconds, with no completed findings reused.
Reused: [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md),
[ECL_STORE_HOOK_0A74.md](ECL_STORE_HOOK_0A74.md),
[ECL_STORE_WORD_84E8.md](ECL_STORE_WORD_84E8.md), and
[U25_STORE_HOOK_6B20_NATIVE_RECORD.md](U25_STORE_HOOK_6B20_NATIVE_RECORD.md).
The raw CPU/RAM conventions and classifier adaptation follow the prior
load-word lane and merged party-index ABI. The PR is independently based
on current main and does not require the load-word PR to be merged.

Owner GAME.OVR is 206,105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
START.EXE build SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Unit 0x25 begins at GAME file offset 29,907.

| Span | Extent | SHA-256 |
| --- | --- | --- |
| Store word | +0C77..+0E0B, 405 bytes / 141 instructions | 3a7c7ed533244cf2eef7ab02bc9ac923f335b52099cf3e6f452fbe38f4e366c5 |
| Store hook | +0A74..+0C76, 515 bytes / 163 instructions | 258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938 |
| Classifier | +06FB..+0755, 91 bytes / 28 instructions | e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347 |

## Implemented local behavior

Entry is CS:0C77 after the far CALL, with return IP/CS, value, and address
words at SS:SP. Complete local execution returns through RETF 4. Window
stores, register/flag effects, frame writes, argument copies, effective
offset wrapping, contiguous multi-byte operands, and A20 wrapping follow
the original order.

Class 0 writes the window first, then rechecks the live address for 49FD/49FE
before setting 84E8. Class 1 writes the window before pushing the two hook
arguments; its local hook covers record ranges and fields, value adjustment,
conditional record/flag writes, and quiet cases. Record pointers are reloaded
after earlier writes where the source repeats LES. A window target that
aliases the active-record pointer therefore changes the subsequent record
destination; a first record-byte store can change the pointer used by the
second. Class 2 writes its word window. Class 3 writes only DL while
preserving DH.

Class 4 implements word stores at 03DE/B8/B9, the read-only/no-op addresses,
all high-address cases, and argument mutation by subtracting BF68. At C04B
the 84E6 flag store precedes reading the value byte; at C04C it follows the
data write. C04D repeatedly subtracts four in the actual value argument,
updates the local loop flag, and stores twice the final remainder. C059 and
C05F write 6F68/6F69 and set 84E5. Unknown high cases still retain the original
address-argument mutation and compare flags.

The loop is deliberately not replaced by a host modulo expression. When
DS=SS and the C04D data byte aliases the local loop flag, each iteration can
clear that flag again; the original and C may never return. The bounded
probe observes the original still looping after 10,000 instructions and
the C call still running for one second after startup. This is a bounded
observation plus the source loop analysis, not a proof about all inputs.

## Explicit dependency boundary

DEPENDENCY returns before the far CALL, with CPU IP at the call site and
the exact argument words already pushed. No far-return frame is invented.

| Stop IP | Original target | Prepared words, first pushed first |
| --- | --- | --- |
| +0C03 | resident 0802:094A | 0300 OR low byte of value |
| +0C26 | unit 3BD entry 6, source stub 03D5:003E | value AND 007F, selector 1 |
| +0C49 | same | value AND 007F, selector 2 |
| +0C6C | same | value AND 007F, selector 3 |

The last three calls occur only for unsigned value greater than 0080.
Their hook value argument is also masked in stack RAM. These target
coordinates describe the original file; loaded segment operands can be
relocated. A host executor must execute the actual instruction in supplied
immutable RAM and its continuation. Reentering the function at +0C77 would
repeat the window store and is not a resume protocol. TRANSFER similarly
exposes an altered internal return without guessing its continuation.

## Validation and limits

**4,452 original-byte/C cases passed**, comparing every byte of 1 MiB RAM
and all fourteen CPU words at the same return or stop: **4,396 completed
calls, 43 dependency stops, and 13 altered internal returns**. Cases include
all 1,024 class-1 addresses, byte sweeps, word boundaries, the subtract-four
loop, stack boundaries, A20 wrapping, and DS/SS/pointer aliases.

The trace visits 330 of 332 instruction sites. This includes the four far
CALL sites stopped before execution. Unvisited +0C08 is the continuation
after the unresolved resident call; +0D11 is the impossible class-outside-0..4
fallback guard. Neither is silently counted as completed callee behavior.

Focused C tests pass with strict warnings. They check class-4 argument
mutation and all byte values, the published native 6B20 value-7 store,
window/record alias order, repeated pointer reloads, local record conditions,
dependency argument words, and guards. The complete project builds with
-Wall -Wextra -Wpedantic -Werror; all **200 CTest targets passed** on base
513e93f plus this bounded store-word change.

The existing native note observes window BEEF becoming 7 before record
byte A5 becomes 7 on a controlled DIVIDE instruction. The new C test and
original-byte differential reproduce those effects, including alias-sensitive
order. There is no new store-word DOSBox-X capture or reached-state whole-RAM
comparison here. Natural unedited-script reachability stays unmeasured.
Code is immutable; asynchronous hardware interrupts/traps are outside this
routine model. Undefined AND/XOR/shift AF follows Unicorn's normalization.

Reproduce with owner media and local Capstone 5 / Unicorn 2:

    cmake -S . -B resources/build-u25-store-word -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
    cmake --build resources/build-u25-store-word
    ctest --test-dir resources/build-u25-store-word --output-on-failure
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u25_store_word_0c77_audit.py
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u25_store_word_0c77_audit.py --nontermination-check

Media, decoded instructions, builds and local reports remain under ignored
resources; public outputs contain addresses, counts and hashes only.

| Parity report field | Finding |
| --- | --- |
| target_function | Unit 25 +0C77 with +06FB and local +0A74 paths up to explicit far-call dependencies |
| source_candidates | src/recovered/overlay_u25_store_word_0c77.c |
| signals | Pinned spans/call map; source-ordered aliasing; 4,452 full-RAM/CPU cases; independent C assertions; published native store order |
| score | Three checks passed: pinned CFG/ABI, focused C tests, original/C differential; bounded nonreturn observation also agrees |
| verdict | unclear for the whole entry because external callees remain; local return/stop states match within the documented scope |
| review_notes | No callback substitutions; immutable code; possible nontermination; no new native capture, strict credit, or gameplay claim |
