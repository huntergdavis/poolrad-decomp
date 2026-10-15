# U6F +0482: prefix before the first unresolved call

The bounded pr_u6f_prefix_0482 reproduces caller +0482..+04D7 and four
closed helper invocations, then returns an explicit callback request BEFORE
the far call at +04D8. It does not execute that call, push its return frame,
or resume its continuation. It represents a prefix, with no full-function,
strict/game-count or native-reach claim.

PM fallback i3576 requested this prefix and a separate U3CE entry gate.
i3599 coordinated the stop and unique CMake additions; i3610 accepted scope.
Branch codex/u6f-0482-prefix-gate starts at approved leaf head
25f32f0b09fc68a4eb41e81464f2f20aac4e61df. Any push requires exact PM review.
The former display run is not repeated. A target-specific 12-second deja
query expired without findings.

## Pinned source and closed dependencies

Original media remain owner-supplied files under ignored resources/.

| Input | SHA-256 |
| --- | --- |
| START.EXE | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |

Unit 6F's original base is 0x1DFE6. The **86 executed caller bytes** are
file [0x1E468,0x1E4BE), unit [+0482,+04D8): 32 aligned instruction starts,
straight-line control, SHA-256
cdeb91ac5a9d6d47b4accf8dfd0fb357cdb6edfb4833d36c9c32710115e76ebb.
Including the unexecuted five-byte call gives 91 bytes, SHA-256
50e6f7712a661af42cc1cd4a0fee30b1e6aded0a63762789497a966d6d392acf.
All five original call operands and segment fixups are checked.

| Caller site | Closed original body | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| +048C | 0802:094A selector save/replace; RETF 2 | 19 | 9cb0afed6f73df887b61f47778db5d55bd9063b59c8f6067ab0c38ec3350d205 |
| +04A6 | SYSTEM 0AF8:0B0B Pascal copy; RETF 4 | 26 | 4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f |
| +04C2 | SYSTEM 0AF8:0C3B one-byte string; RETF 2 | 18 | b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484 |
| +04C7 | SYSTEM 0AF8:0B98 capped append; RETF 4 | 44 | c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64 |

These helpers have no calls. C includes their raw CPU, stack, string and
flag effects; it reads loaded segment operands from caller RAM. Existing
leads reused are [the selector note](RESIDENT_DAX_SELECTOR_PUSH.md),
[the runtime-string boundary](U6F_PARTY_FILENAME_BOUNDARY.md),
[the char-string ABI](SLOT_A_STRING_0425_ABI.md), and
[the unpacker](../../tools/overlay/resident_loader_map.py).
Their exact instruction bodies were independently checked again.

The complete caller's executable CFG is [+0482,+0674), 498 bytes / 183
starts, ending +0671 RETF 2, SHA-256
be72ede6980e67ba2ef0d51e84c154a281ab9b27b9d00070b7701fcf4bd0a4a3.
The old selector note uses a 540-byte next-prologue extent; 42 trailing
bytes are outside this executable CFG. This prefix does not reach +0546
or +056A, which are two calls to the same +032F leaf.

## Entry, effects and callback ABI

Entry SS:SP holds return IP/CS followed by one pushed WORD. This prefix
never reads that incoming argument. It pushes BP, allocates 0x22A bytes,
and passes a WORD whose low byte is 3 to the selector, retaining incoming
AH. The selector saves DS:5376 to DS:5377, then reloads its live argument
and stores DS:5376. Stack aliases can change those inputs and saved control data.

The caller loads the descriptor far pointer DS:5D92 into SS:[BP-29],
copies the opaque Pascal literal CS:0476 into SS:[BP-229], reloads the
descriptor and indexes DS:08C5 by its byte at +C0. The selected byte forms
a one-character Pascal string at SS:[BP-129], appended to the first string
with the original 255-byte cap. No game string is embedded. The descriptor
is reloaded again before its byte at +BD is read.

On PR_U6F_0482_CALLBACK, CPU IP is 04D8 in caller CS. The live stack is:

| Callback-side item | U3CE entry-4 argument |
| --- | --- |
| WORD SS:[SP] | low byte 11; current AH retained (normally zero after the earlier clear) |
| WORD SS:[SP+2] | low byte from descriptor +BD |
| far pointer SS:[SP+4] | retained destination string, normally SS:[BP-229] |

The pending request identifies loaded target CS/IP, call IP 04D8 and
continuation 04DD. Source target 03E9:0034 resolves to U3CE entry 4 +01C8,
whose [bounded model](U3CE_ENTRY4_01C8.md) retains external file/heap/
palette/key effects. This is the first unresolved call. U3CE entry 3 +014E
is a distinct later cleanup call at +065F; see
[its separate gate](U3CE_014E_SIDE_EFFECT_GATE.md).

Arguments, locals, bytes, pointers and return frames use source-order live
accesses. A changed closed-helper return yields PR_U6F_0482_CONTROL_STOP
at that CS:IP without continuation or pending-output mutation. Shared DS/SS
can make the selector read a newly pushed BP byte, then modify saved BP
and return IP. Invalid inputs/entry IP leave RAM, CPU and pending unchanged.

## Measured parity and verification

The former published head 58fac09 reported 681 configurations with 673
callbacks and eight changed-return stops. Seven of those stops were false
results of the shared C/Unicorn RETF wrap defect. The corrected default sweep
passes **681 configurations**, with **680 unresolved-call stops and one
alias-altered helper-return stop**. Every 1 MiB RAM byte and all fourteen CPU
words match, including unmasked FLAGS. The earlier claim at divergent return
offsets is superseded by [the native repair evidence](U6F_0482_RETF_NATIVE_WRAP.md).

Cases cover all 256 literal lengths and descriptor indexes, 128 seeded
register/data cases, live selector/local aliases, word/far-stack boundaries,
shared code/data/stack segments, relocated helpers and A20 wrapping. The
full sweep reached 32/32 caller starts and 47/47 helper starts, including
both append-cap outcomes. No unresolved callee was executed. The oracle
stops before every closed RETF. At helper SP FFFE/FFFF it applies the
independently measured native two-word stack-pop rule; other offsets execute
the original RETF separately. This disclosed hybrid oracle prevents the
Unicorn CS-pop defect from validating the same C defect. Altered returns
remain explicit control stops. Memory far operands retain contiguous reads.

Callback arithmetic flags are defined by the append's DI arithmetic; string
helpers clear DF. Logical AF on some early stops is architecturally undefined.
C uses this Unicorn oracle's cleared-AF convention there; equality is an
emulator observation, not a claim about undefined native hardware flags.

The independent registered CTest checks complete normal callback registers,
frame/string/selector effects, empty append, length-255 clamp, alias-altered
BP/return, integrated FFFD/FFFE/FFFF helper returns, contiguous memory far
arguments crossing FFFF, and atomic invalid inputs. Strict warnings and ASan/UBSan pass
(sandbox leak detection disabled), as does focused CTest 1/1. The recovered
library also compiles with strict warnings.

## Checker and limits

Parity report: target_function is U6F:+0482 prefix through pre-call +04D8;
source_candidates contains overlay_u6f_prefix_0482.c; verdict is match for the
measured bounded contract. Signals are two media pins, exact prefix/four
helper hashes, aligned calls/fixups, callback ABI, full-state cases and
instruction coverage. The score is those measured counts, not a probability. Review notes follow.

Negative evidence matters: the complete caller has 14 far calls, two leaf
calls and ten distinct targets. No callback result or full-function return
is established. Natural reach, game/library ownership, heap/file effects
and graphics hardware remain unmeasured. Flat RAM, pinned immutable code
and separate host CPU/output storage are required; self-modifying code and
mapped-device behavior lie outside the contract.

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6f_prefix_0482_audit.py
    cmake -S . -B resources/build-u6f-prefix -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
    cmake --build resources/build-u6f-prefix --target poolrad_overlay_u6f_prefix_0482_tests
    ctest --test-dir resources/build-u6f-prefix -R '^poolrad_overlay_u6f_prefix_0482_tests$'

All original bytes, strings, disassembly and captures remain ignored under
resources/; public tests use synthetic data only.
