# U3CE +014E: raw zero-return and first-external gate

pr_u3ce_gate_014e translates original instructions until the zero path
returns or a nonzero pointer requires resident 0464:2595. It exposes the
pending external call BEFORE its CALL instruction; no far-return words are
pushed, no heap routine is invoked and no successful free is fabricated.
This bounded model adds no full-function/strict game credit or native reach.

PM i3690 assigned this separate lane; i3790 acknowledged scope and additive
CMake registration. The branch is
codex/u3ce-014e-first-external-gate, based on approved U6F head
58fac09a12be7836c60a0366d9dc8fd1a9c5a69c. A bounded 12-second deja query
expired without findings. Reused source/ABI evidence is
[the preceding classification](U3CE_014E_SIDE_EFFECT_GATE.md),
[the entry-4 map](U3CE_ENTRY4_01C8.md),
[the existing bitmap-release model](RESIDENT_464_BITMAP_RELEASE.md), and
the source CFG and emulator methods in the prior U6F audits. Original bytes
and both source-body hashes were independently checked again.

## Source and ABI

Pinned START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Pinned GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Unit 3CE public entry 3, source stub 03E9:002F, resolves to +014E.
The executable extent [+014E,+01B0), file [0x31BBA,0x31C1C), is
98 bytes / 40 aligned starts, SHA-256
244d2bbd214ceb86d9d6daa80252da6af77fa0a92e4a287ac555aaca26b3c047.
The 24-byte literal tail before the next entry is excluded. Both external
call operands have overlay segment fixups; loaded segments are read from RAM.

Entry SS:SP holds far return IP/CS and one WORD at SP+4. The procedure
pushes BP, reserves two stack bytes, and reads only the low argument byte.
That slot indexes eight-byte records at DS:5E5A. Each far-pointer zero test
ORs both words; partially null pointers are nonzero. No object is dereferenced.

| Route | Status / boundary | Callback-side far argument |
| --- | --- | --- |
| First pair nonzero | CALLBACK before +017A | address DS:(5E5A + 8*slot) |
| First zero, second nonzero | CALLBACK before +01A5 | address DS:(5E5E + 8*slot) |
| Both pairs zero | RETURNED through +01AD RETF 2 | pending output unchanged |

For a callback, SS:[SP] is the pointer-pair offset and SS:[SP+2] is DS.
The stored object pointer is not the argument. Pending contains the loaded
callee, call IP and continuation IP (+017F or +01AA). The body writes only
its SS stack before these stops; stack/table aliases remain live and can
change subsequent guards or saved frame bytes. A zero return uses the
actual live far frame, including any aliases, and stops after RETF.

## Measured follow-on 1: independently supplied +017F state

The same API accepts IP +017F as a separate entry using the supplied
post-first-call BP/SP/RAM. This 49-byte/20-start continuation has SHA-256
fdbc2e759f77faaae556c294375ca453b738ed32cd4962186b1cbe52d6680e67.
It rereads the slot byte
and second pointer pair. It either stops before +01A5 or returns through
RETF 2. It does not infer that the first free ran or succeeded. Both the
argument and table may have changed before this independent entry.

## Original/C evidence and focused tests

**1,755 cases** compare every byte of 1 MiB RAM and all fourteen CPU words,
including unmasked FLAGS. They include **582 independent +017F cases**,
all 256 slot indexes crossed with guard states, seeded registers/pointers,
partially null pairs, stack/far-return boundaries, shared segments, A20
wrapping, and stack/table aliases. Results are 583 returns and 1,172 pending
calls: 581 stops at +017A and 591 at +01A5.

All 40/40 source starts are visited: 38 instructions are executed across
cases, and the two external CALL starts are stop sites, never retired.
The oracle stops before RETF, avoiding an unintended continuation. A directed
native DOSBox-X check found that Unicorn reads the CS word incorrectly when
entry SP is FFFE or FFFF. At those offsets the audit applies the independently
measured native rule: read IP at SS:SP and CS at SS:(uint16_t)(SP+2), with
each individual word physically contiguous. Other returns execute the original
RETF in Unicorn. Thus the sweep uses a native-established RETF correction,
not an independent Unicorn result at the two divergent offsets. See
[the six native wrap captures](U3CE_RETF_NATIVE_WRAP.md).
Callback arithmetic flags are defined by ADD DI. Zero-return logical AF is
architecturally undefined; C uses this Unicorn oracle's cleared-AF convention.
Unmasked equality there records emulator behavior, not undefined hardware flags.

Independent C tests check complete zero-return registers/flags/ABI, both
pointer-to-pointer callback frames, partial-pointer guards, FFFD/FFFE/FFFF
far-return stack offsets with distinct linear-CS canaries, unchanged host
pending output on return, a changed slot on independent +017F entry, and
atomic invalid-input failures. Strict C11 Werror and ASan/UBSan pass with
sandbox leak detection disabled. The unique registered focused CTest passes 1/1.

Parity report: target_function is this bounded +014E/+017F gate;
source_candidates contains overlay_u3ce_gate_014e.c; verdict is match for
the measured contract. Signals/score are the exact source/media pins,
fixup-backed calls, 1,755 corrected full-state cases, six synthetic native
wrap captures and the stated instruction coverage.
Review notes: heap effects, first-call results, native reach and source
ownership remain unresolved. Flat ordinary RAM, immutable modeled code and
separate host CPU/output storage are required. This is not full cleanup parity.

## Measured follow-on 2 and reproduction

[The resident first-heap gate](RES464_2595_FIRST_HEAP_GATE.md) supplies
148 original-byte probes before SYSTEM:0254. The existing resident wrapper
still requires memory/free callbacks; the new gate does not replace it.

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u3ce_gate_014e_audit.py
    cmake -S . -B resources/build-u6f-prefix -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
    cmake --build resources/build-u6f-prefix --target poolrad_overlay_u3ce_gate_014e_tests
    ctest --test-dir resources/build-u6f-prefix -R '^poolrad_overlay_u3ce_gate_014e_tests$'

Original media, disassembly, generated COM programs, native captures and
emulator RAM remain ignored under resources/. Committed C tests use synthetic
data. The native wrap programs supply synthetic frames; no natural game route
or display-driven run is established.
