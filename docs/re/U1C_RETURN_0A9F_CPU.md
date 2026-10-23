# U1C +0A9F RETURN: complete raw FreeMem composition

The separate raw API replaces the allocator boundary with the complete
SYSTEM FreeMem chain. RETURN now preserves the original heap mutations,
CPU state, stack writes, failures and live DS temporary reloads. The
existing callback API and semantic array model remain unchanged.

## Identity and selection

| Item | Evidence |
| --- | --- |
| Caller | U1C ECL RETURN, complete [0A9F,0AED), RETF at0AEC |
| Extent | 78 bytes /25 instruction starts; no trailing data |
| GAME.OVR file interval | [0x17E5,0x1833) |
| Caller SHA-256 | a0b08ebfbd9bab9e5a735600cbc6120d0e56d67f77f585d2d07705db74d1681c |
| GAME.OVR SHA-256 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| START.EXE SHA-256 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| Callee | CALL0AD8 to SYSTEM0AF8:0254, with near03D3/0432/0491 |
| Callee extent | 233 bytes /106 instructions, independently hash-pinned by the audit |

The refreshed schema-8 queue (successor to the requested schema-4 queue)
ranks this row361, score24, one of one runtime callees ready. Its existing
main_model is the provisional callback implementation, which masks AF and
does not execute FreeMem. Higher substantive ready bodies already have raw
composition; U1C+271F's advertised86-byte extent contains only14 code bytes.
Ownership was cleared before publication. The existing identity is
game_verified / verified; acceptance of this raw implementation is still
for the independent reviewer. No ledger, classification or credit changes
are included.

## Behavior and profile

The no-argument far routine increments DS:[49ED], then tests both words of
the head DS:[6F70/6F72]. A nil head returns immediately. Otherwise the node's
word0 restores the saved PC, words2/4 are copied to DS:[6F74/6F76], and the
current head is passed to FreeMem with size6, rounded to8 bytes. Only after
a successful return does the caller reload those live DS temporaries into
the head. They cannot be cached across the heap operation: a directed
heap/global alias overwrites them and is exercised by both original and
native oracles.

Each original read/write remains ordered, including the second LES of the
head after restoring PC, parameter pushes, actual far-call frame and
post-call temporary reloads. Bad pointers can fail after PC/temporary
writes. Duplicate frees change size words before RunError204. No stop
rolls back those effects or invents stronger heap ownership checks.

The API takes exactly1MiB A20-off flat RAM and CPU at IP0A9F with initial
SP002C..FFFC. CPU/diagnostic storage is separate from RAM and each other.
Effective offsets wrap at16 bits; word/LES/return operands are physically
contiguous with20-bit wrapping. All reached return frames must stay
nonwrapping, including after alias-induced changes. Loaded code is
immutable. The live CALL opcode/offset is checked before entry; its segment
binds SYSTEM. Other code bytes are identity preconditions. MMIO, interrupts,
faults and self-modifying code are outside this profile.

RETURNED executes RETF and preserves its live result. CODE stops before
CALL0AD8 after earlier writes/pushes. CALLEE retains FreeMem's exact
RunError204, walk LIMIT or changed near-return status and CPU/RAM. TRANSFER
retains an unexpected successful FreeMem far return. ARGUMENT changes no
CPU/RAM. The single heap-walk budget applies to taken edges back to040F.

AF is architecturally undefined after OR. This API explicitly uses the
**measured DOSBox-X normal/386 AF=0 convention**, rather than claiming an
ISA guarantee. Native nil-head cases start with FLAGS0002 and00D7 (AF0 and
AF1 respectively); both return FLAGS0046, including AF0, with no mask. The
nil path's last flag writer is the caller's OR. The old callback model's
AF exception remains unchanged. All14 CPU words, including full FLAGS,
are compared in both new oracles.

## Measured evidence

- **204 original-byte cases**:188 returns,10 RunError204,2 heap limits,
  one changed near return, one changed far return and2 CODE stops.
  All14 CPU words plus all1,048,576 RAM bytes match unmasked.
- Caller25/25 instruction starts and2/2 conditional outcomes; composed
  closure131/131 starts and35/36 outcomes. The sole missing outcome is
  FreeMem03D5->0431, the zero-size shortcut. This caller always pushes6;
  that unreachable edge remains in the reported denominator.
- **128 DOSBox-X normal/386 cases**:123 returns and5 RunError204 stops.
  CPU14 plus1,024 stack,65,536 DS and65,536 heap bytes match unmasked.
  RESULT.BIN SHA-256:
  cd20d3bc8a2e846e9faa126894761a653bfd6efa632221cd784f9472eb937a3a.
- **1,690 ASan/UBSan semantic assertions**, focused CTest1/1.

Cases cover nil/one-word-nonnil heads, PC wrapping and restoration, arbitrary
next pointers, allocator insertion/coalescing/top contraction, invalid and
duplicate pointers, malformed-list budgets, exact error frames, changed
near/far returns, code refusal, global/stack/node aliases, live overwritten
temporaries, boundary/odd SP, wrapped node fields, A20 wrapping, relocated
SYSTEM, randomized flags and background RAM. The semantic test also includes
both headers to guard coexistence of legacy and raw APIs.

Native fixtures initialize every referenced node in captured DS/heap
memory and include the temporary/heap alias. They are synthetic; they do
not establish natural gameplay reach, native stack/A20-wrap behavior,
native limit/transfer/code-refusal parity, or hardware beyond the declared
normal/386 flat-memory profile. Final native INPUT is reproduced byte for
byte by the final harness. An original-only branch case was added after
the passing native run; native cases and C behavior were unchanged.

## Reproduction, reuse and assessment

With pinned media under resources/DOS/POOLRAD and the oracle dependencies:

    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u1c_return_0a9f_cpu_audit.py --unicorn --native --output resources/return-evidence.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_u1c_return_0a9f_cpu_tests
    ctest --test-dir resources/build -R '^poolrad_u1c_return_0a9f_cpu_tests$' --output-on-failure

Pre-implementation shared-memory recall found the prior provisional RETURN
work and the accepted raw FreeMem work. The published
[RETURN model](U1C_RETURN_FULLSTATE_0A9F.md) supplies the original caller
identity and ABI; the unchanged [FreeMem implementation](SYSTEM_FREEMEM_0254_CPU.md)
supplies actual heap effects. The full-memory and disk-fed native harnesses
adapt [U3B0 +11F3](U3B0_DISPOSE_11F3_CPU.md), which in turn uses FreeMem's
pinned original-byte oracle. A recalled older raw plan had no surviving
implementation; none is inferred or reused.

| Field | Assessment |
| --- | --- |
| target_function | U1C complete [0A9F,0AED), composing SYSTEM0254/03D3/0432/0491 |
| source_candidates | overlay_u1c_return_0a9f_cpu.c and unchanged system_freemem_0254_cpu.c |
| signals | Pinned identity, CALL/RETF/branch ABI, ordered PC/temp/heap effects, unmasked CPU/RAM differential, native capture, semantic assertions |
| score | Five evidence categories: identity, structure, differential, native, contract checks; no probabilistic score |
| verdict | likely_match within the declared profile, pending independent verification |
| review_notes | Explicit normal/386 AF convention, immutable code/nonwrapping reached frames, exact partial stops, synthetic native scope, zero-size edge retained in denominator; no self-credit |

## Checker registration (2026-10-29)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_001C_0A9F --require-current-main
```
