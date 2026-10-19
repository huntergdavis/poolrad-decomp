# U1C +125D CLEARMONSTERS: raw FillChar and FreeMem composition

The new separate raw API executes the complete caller and both runtime
callees over live CPU/RAM state. It clears the original DS block, snapshots
each node's next pointer in the original stack locals, releases the node
through the real heap manager, and reloads the locals into the live head.
The existing node-map/callback API remains unchanged.

## Identity and selection

| Item | Evidence |
| --- | --- |
| Caller | U1C [125D,12C6), no arguments, RETF at12C5 |
| Extent | 105 bytes /36 instruction starts; no trailing data |
| GAME.OVR file interval | [0x1FA3,0x200C) |
| Caller SHA-256 | 8112f2f70d8905153070bfe84d60f102d3a7e1e2aaa89e26c81fd79b10f4479d |
| GAME.OVR SHA-256 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| START.EXE SHA-256 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| FillChar | CALL1282 -> SYSTEM16A8; [16A8,16BC),20 bytes /7 instructions |
| FillChar SHA-256 | f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7 |
| FreeMem | CALL12AE -> SYSTEM0254 with03D3/0432/0491;233 bytes /106 instructions |

The fresh schema-8 queue, successor to the requested schema-4 queue, ranks
this row381, score20, with both runtime callees ready. The existing
main_model still substitutes a heap callback and node map. Higher
substantive ready callers already have raw composition; U1C+271F's
advertised86-byte extent includes data (14 actual code bytes). Ownership
was cleared before publication. Existing identity is game_verified /
verified; independent acceptance of this implementation is pending. No
classification, accounting or credit changes are included.

## ABI, ordering and stop boundaries

The prologue allocates six local bytes and increments DS:[49ED]. It clears
bytes4435 and84E7, writes8 to6F83, and calls FillChar(DS:67F4,28,0). The
cleared interval ends at680F, immediately before the head at6810/6812.
Both head words zero mean nil. Otherwise node+2A/+2C are copied to BP-4/-2,
then FreeMem(head,0x3F) rounds the size to64 bytes and performs the actual
free-list insertion/coalescing/HeapPtr contraction. After success, the
caller reloads the live stack locals into DS6810/6812 and repeats.

Writes, argument pushes and reads stay in original order. In particular,
FillChar can overwrite its own return frame, FreeMem can overwrite heap
and stack aliases, and the head is replaced only after a successful free.
Bad/duplicate frees preserve the clear, PC advance, saved link and any
partial heap writes. No failure rolls back state or invents allocation
ownership rules beyond the original manager.

The API requires exactly1MiB A20-off flat RAM, IP125D, initial SP0032..FFFC,
and distinct CPU/diagnostic/RAM storage. Effective offsets wrap at16 bits;
word, LES and far-return operands read physically contiguous bytes with
20-bit wrapping. All reached return frames must remain nonwrapping,
including alias-induced changes. Loaded code is immutable. Both live CALL
opcodes/offsets are checked before entry; each segment binds the original
SYSTEM body. Other instruction bytes are identity preconditions. MMIO,
IRQs, faults and self-modifying code are outside this profile.

- RETURNED executes the final RETF with live frame contents.
- CODE stops before the failing CALL, retaining earlier effects. The
  diagnostic still describes the last reached callee, if any.
- CALLEE retains FreeMem's exact nonzero status and partial CPU/RAM,
  including RunError204, walk LIMIT and changed near return.
- TRANSFER retains an unexpected successful FillChar or FreeMem far return.
- LIMIT stops at1287 after an exhausted JMP12C0 budget. Zero permits one
  successful free, including head replacement, before the next nil test.
- ARGUMENT changes no CPU/RAM. FreeMem's walk budget resets per call.

The diagnostic identifies FillChar as0, FreeMem as1, or unreached as-1,
with the raw callee status. Arithmetic flags before FillChar retain SUB's
CF and INC's other arithmetic flags, including on a refused call. CLD
clears DF. Logical AF follows the measured normal/386 zero convention;
AF after OR is architecturally undefined, not a universal x86 guarantee.
Native nil-head cases starting FLAGS0002 and00D7 both return0046 unmasked.

## Measured evidence

- **194 original-byte cases**:167 returns,10 RunError204,2 caller limits,
  2 heap limits,3 changed successful far returns,1 changed near return,
  9 CODE stops. CPU14 and all1,048,576 RAM bytes match without masks.
- Caller36/36 instruction starts and2/2 conditional outcomes; composed
  closure149/149 starts and35/36 outcomes. FreeMem's zero-size shortcut
  03D5->0431 cannot be reached from this caller's fixed0x3F argument; it
  remains in the denominator. REP iterations are not conditional outcomes.
- **128 DOSBox-X normal/386 cases**:120 returns and8 RunError204 stops.
  CPU14 plus1,024 stack,65,536 DS and65,536 heap bytes match unmasked.
  RESULT.BIN SHA-256:
  e495dbaa07bd6a5e09e6cc2e6a85f904556ab22e863639215208b7bb39ded9dc.
- **9,242 ASan/UBSan semantic assertions**; focused CTest1/1.

Cases include nil and one-word-nonnil heads; forward/reverse adjacent
lists; merge directions and top contraction; invalid/range/duplicate
pointers; cyclic lists and both budgets; preserved head on heap failure;
changed FillChar/FreeMem returns; stack/global aliases; offset-word and
A20 wrapping; odd/boundary SP; relocated SYSTEM; randomized flags/PC/RAM;
and call refusals across PC carry, sign, overflow and wrap boundaries.
Semantic checks assert all28 cleared bytes and adjacent sentinels, PC
wrapping, coalescing lists of1..64 nodes, precise partial frames, diagnostic
identity, and coexistence with the legacy header.

Native frames are synthetic and initialize all referenced node bytes in
the captured arenas. They establish returning/error paths, not natural
gameplay, native stack/A20-wrap, native limits, changed returns or refusal
parity. Native capture initially completed but the Python comparator hit
a duplicate ctypes Diagnostic identity when the audit was both a script
and an imported module. The wrapper now constructs the function's declared
diagnostic type. The same original capture was compared successfully;
no C, DOS input, original code or comparison masks changed. The initial
wrapper error log is retained privately. Final native INPUT is reproduced
byte-for-byte by the final harness.

## Reproduction, reuse and author assessment

With pinned media under resources/DOS/POOLRAD and oracle dependencies:

    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u1c_clearmonsters_125d_cpu_audit.py --unicorn --native --output resources/clearmonsters-evidence.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_u1c_clearmonsters_125d_cpu_tests
    ctest --test-dir resources/build -R '^poolrad_u1c_clearmonsters_125d_cpu_tests$' --output-on-failure

Pre-implementation shared-memory recall returned the existing bounded
[CLEARMONSTERS identity/ABI](U1C_CLEARMONSTERS_125D.md) and accepted runtime
work. This implementation composes unchanged
[FillChar](SYSTEM_RUNTIME_LEAVES.md) and [FreeMem](SYSTEM_FREEMEM_0254_CPU.md).
The caller's raw composition/oracle idioms adapt
[U3B0 +11F3](U3B0_DISPOSE_11F3_CPU.md) and the disk-fed capture adapts
[U1C +0A9F](U1C_RETURN_0A9F_CPU.md). No unpublished caller implementation
was found or inferred.

| Field | Assessment |
| --- | --- |
| target_function | Complete U1C [125D,12C6), composing FillChar16A8 and FreeMem0254/03D3/0432/0491 |
| source_candidates | overlay_u1c_clearmonsters_125d_cpu.c with unchanged raw SYSTEM dependencies |
| signals | Pinned identities, CALL/RETF/control flow, ordered clear/link/heap effects, unmasked CPU/RAM differential, native capture, semantic assertions |
| score | Five evidence categories: identity, structure, differential, native, contract checks; no probabilistic score |
| verdict | likely_match in the declared profile, pending independent verification |
| review_notes | Immutable code, nonwrapping reached frames, normal/386 AF convention, exact partial states, zero-size edge retained, synthetic native scope and wrapper correction above; no self-credit |
