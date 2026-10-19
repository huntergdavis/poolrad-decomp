# U7D +047A: raw indexed subtraction and normalization

The caller subtracts an unsigned byte amount from a signed-byte-indexed word
in DS:6DC2, borrowing through the radix table at DS:363A. It returns immediately
when words 1 through 4 are all zero. Otherwise it borrows from a higher column,
clears all seven words through actual SYSTEM FillChar when the scan reaches
index 5, or subtracts and composes the existing raw +03A3 normalizer (which
composes +02F2). All reads and writes remain live, including arguments, stack
locals, party links and callee return frames.

This is author evidence, pending independent verification. It changes no
classification, ledger, acceptance record or credit.

## Source identity and selection

- START.EXE SHA-256: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- GAME.OVR SHA-256: 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- Unit file base 0x23DCC; caller file [0x24246,0x2432F).
- Actual caller [047A,0563): **233 bytes, 92 instructions**, RETF 4 at 0560.
- Caller SHA-256: 471d0464b3c216b69f1c3188b7a8aa2cc69ca1f81109de5217e3f4dd574da8b7.
- +03A3 [03A3,03D5): 50 bytes, 19 instructions;
  3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad.
- +02F2 [02F2,03A3): 177 bytes, 69 instructions;
  8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9.
- SYSTEM 0AF8:[16A8,16BC): 20 bytes, 7 instructions;
  f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7.

The schema-4 queue lineage currently emits schema 8. The no-git selection
snapshot ranks this candidate 384, score 20, 2/2 ready, game_evidenced /
provisional. Its inline_in_strict hit is a false coverage inference from
span hashes and callsite references in check_overlay_u7d_03a3_natural_route.py;
there was no implementation. Its 235-byte / 93-instruction span includes two
trailing data bytes [0563,0565), SHA-256
44c808fd166dbb8961fb795587e5ee0082f8a2dc781ff06a3f02223dcc06a7da.
Those bytes are not translated or counted as caller instructions.

Shared-memory context query "poolrad U7D 047A 03A3 raw CPU update indexed
subtraction" recovered Claude session 281c8700-5acc-438b-b93d-c061e28437eb:
+03A3 already had a complete raw CPU API. This implementation reuses that API
and the existing +02F2 and SYSTEM leaf sources unchanged. The old Claude
reservation was reconciled and explicitly released by the VP (inbox i18536).
The native disk-input/capture pattern is adapted from
u1c_clearmonsters_125d_cpu_native.py; the original unit loader and party layout
come from u7d_update_03a3_native_audit.py. These are evidence infrastructure,
not additional game bodies claimed by this change.

## Contract and boundaries

pr_u7d_update_047a_cpu takes exact 1 MiB A20-off flat RAM, a CPU14 state, a caller
instruction budget, and an optional diagnostic. Entry IP is 047A; SP must be
0020..FFF8; reached return frames must not cross the 16-bit SS offset boundary.
CPU and diagnostic objects must not overlap RAM or one another. Original code
is immutable, interrupts/faults/MMIO are outside this profile. Arbitrary signed
index bytes are preserved, rather than clamped to the nominal seven words.

At entry SP+4 is the index word, SP+6 is the amount word. Only the low byte of
each is used; CBW sign-extends the index. RETF 4 consumes the return and two
argument words. The FillChar call at 04F0 checks its live opcode/offset and
loads its segment operand. The near call at 055A checks its live opcode and
relative target before composing +03A3. Callees execute on shared CPU/RAM,
without callback substitution or a host-side seven-element array snapshot.

RETURNED (0) preserves the live outer RETF destination; ARGUMENT (1) rejects
unsupported entry state; CODE (2) stops before a changed call; CALLEE (3)
retains a raw nonzero callee status; TRANSFER (4) retains an unexpected return
from an otherwise successful callee; LIMIT (5) stops before the next caller
instruction. Diagnostic callee 0 means FillChar, 1 means +03A3, -1 means
unreached. Its status starts at -1 and otherwise retains the last raw result.

The budget counts only the 92 caller instruction starts, including repeats.
It does **not** cap delegated +03A3/+02F2 execution. Those existing APIs retain
their terminating-input precondition: malformed cyclic party lists or aliases
that prevent their loop from progressing can fail to return. Caller loops
with unhelpful radices can be stopped precisely. Undefined logical/shift AF
uses the zero profile: intermediate boundaries are compared with Unicorn;
native normal/386 evidence compares final full flags without masking. This
is not a claim about every historical CPU's undefined flags.

## Measured evidence

| Check | Result |
| --- | --- |
| Original-byte Unicorn, CPU14 + all 1,048,576 RAM bytes, unmasked | 366 cases |
| Stops | 197 returns, 162 caller limits, 2 successful-callee transfers, 1 nested-callee transfer (raw +03A3 status 3), 4 code refusals |
| Caller coverage | 92/92 instruction starts; 19/20 conditional outcomes |
| Complete caller/callee closure | 187/187 starts; 31/32 conditional outcomes |
| DOSBox-X normal/386 synthetic replay | 128 returns; CPU14 + 1,024 stack + 65,536 DS + 65,536 heap bytes, unmasked |
| ASan + UBSan semantic checks | 12,336 passed |
| Focused CTest | 1/1 passed |

The sole absent branch outcome is 0509 -> 0543 (signed JL). In each scan,
local BP-1 starts at signed-byte(index+1). It increments only while its signed
value is below 5 and the indexed word is zero. Thus it cannot wrap from 127
to -128 and cannot finish below its starting signed value. The argument is
not written between initialization at 04B7 and the reload at 04FB; all writes
in that interval target the distinct BP-1 local. BP-3 then receives the same
starting value. Consequently the JL cannot be taken in the immutable,
no-interrupt profile. The outcome remains in the coverage denominator and
the translation retains the original conditional.

Fixtures cover gate order, direct/equal/zero subtraction, repeated borrowing,
clear/retry, signed and high-byte arguments, party-counter wrap, saturation,
zero radices with a caller limit, 160 instruction-prefix stops, SS=DS, A20
stack alias, minimum/maximum accepted SP, relocated FillChar, bad call
opcodes/targets, and live return-frame aliases. Prefix cases compare transient
flags and partial stack/RAM effects. Semantic tests independently compare
five decimal digits against saturating arithmetic across 1,120 combinations,
including the original early-return exception when only column 0 is nonzero.

Native RESULT.BIN SHA-256:
1adf8e01d67d453a6e960dfd08e620e45c6ba51203e2a8bd8fc484f6c1aea6a7.
Native evidence is synthetic; it does not establish gameplay reachability,
whole-RAM native parity, native limit/refusal/transfer behavior, or native A20
and wrapping-frame behavior. Original-byte tests provide the stated broader
full-RAM boundary checks. All private media/captures remain under resources/.

## Reproduction and author assessment

With the pinned private media and Capstone/Unicorn available:

    PYTHONPATH=resources/pythondeps:tools/overlay python3 tools/overlay/u7d_update_047a_cpu_audit.py --unicorn --output resources/original.json
    PYTHONPATH=resources/pythondeps:tools/overlay python3 tools/overlay/u7d_update_047a_cpu_audit.py --native --output resources/native.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target test_overlay_u7d_update_047a_cpu
    ctest --test-dir resources/build -R '^overlay_u7d_update_047a_cpu$' --output-on-failure

- target_function: OVL_007D_047A, complete actual code extent [047A,0563).
- source_candidates: overlay_u7d_update_047a_cpu.c with unchanged raw +03A3,
  +02F2, and SYSTEM FillChar sources.
- signals: pinned bytes and RETF ABI; exact live call bindings; full CPU/RAM
  differential; all 187 closure starts; independent DOSBox-X capture;
  independent arithmetic and error-boundary semantic assertions.
- score: 5/5 author evidence categories present (identity, ABI, original
  differential, native replay, semantic checks), not an acceptance score.
- verdict: likely_match within the stated execution profile.
- review_notes: provisional game identity; false inline queue hit and trailing
  data; unbounded delegated termination precondition; one analytically
  unreachable branch retained in denominator; synthetic native scope;
  independent acceptance and any credit remain with the reviewer/VP.
