# U38 +2B20 and U9E +0E2E raw dice composition

## Result and provenance

This extends the existing U38 +2B20 and U9E +0E2E source/header pairs
with raw CPU/RAM entries. Their legacy callback API bodies remain
byte-for-byte identical. The new caller composes the new latch, which
composes the unchanged raw U9E +0DE3 / SYSTEM dice API.

U38 reads the active character and its linked pointer. A null link or
linked byte +2E other than 55h preserves DS:6818. On a match, it rolls
1d6 through the latch, clears AH, increments AX, and stores AL at DS:6818
(normally 2..7). The latch stores the live count byte to DS:681C, rereads
its arguments in original order, and returns the dice result.

Shared memory was queried before implementation with
`dunkingsheep memory context "poolrad U38 2B20 U9E 0E2E raw roll latch ownership"`.
Codex session `01a1386e-bea7-7883-8ddd-06fd2be8db9b` and
[legacy caller evidence](U38_LINK_ROLL_2B20.md) supplied call mapping and
behavior hypotheses rechecked against original instructions.
The historical `codex/fullc-u38-2b20` branch is already merged, as confirmed
by VP approval `i18687`. These are extensions of those existing bodies,
not new identity or count claims.

The raw dice contracts are reused from
[U9E composition](U9E_ENTRY8_FULLSTATE_COMPOSITION_0DE3.md) and
[the U38 +2DE4 caller](U38_ROLL_CLEAR_2DE4_CPU.md). Replay structure adapts
the merged +2DE4 and +2AE6 audit/native tools. The latest queue schema 8
(successor to schema 4), generated with `--no-git` at integration main `6fe1e713`,
ranks +2B20 **386, score 20, composable** with +0E2E `strict`.
That label alone does not supply a raw ABI: the previously merged latch
still delegated the full dice CPU effects to a callback. The new entries
close that boundary. Higher rows already exposing raw bodies were skipped;
the U9E +0000 indirect-dispatch boundary remains separate.

VP `i18687` approved this raw-composition extension, unchanged shared
dice API, and the explicit LIMIT-only AF profile below. Identity remains
**game_evidenced / provisional**. The execution tests ran at `7c91f45a`; integration main `6fe1e713`
changes only queue/acceptance files. All compiled/imported execution
dependencies remain identical. No ledger, classification, acceptance,
denominator, or credit changes are included.

## Original closure

| Artifact / body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
| START.EXE, complete | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR, complete | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| 0AF8:11C0-11D6 (end exclusive) | 22 / 10 | `e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9` |
| 0AF8:120F-1245 (end exclusive) | 54 / 22 | `f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242` |
| 5000:2B20-2B63 (end exclusive) | 67 / 25 | `9a322acac07936548b9d1257854e48fc256e29bf2dec0f4c58dfab9ca639366b` |
| 6000:0DE3-0E2E (end exclusive) | 75 / 29 | `7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba` |
| 6000:0E2E-0E52 (end exclusive) | 36 / 16 | `08dc2c3413d0fcfa12d26b589873f619761741e3f38c235477e89ed3a7350443` |
| SYSTEM +1245 multiplier data | 2 bytes | `d9221ae2959bbc366aaadbb08e200b236d45dbaa8bf463cd13da45a9115baa0d` |

The caller FAR CALL at +2B52 uses offset 004Dh and segment fixup +2B55.
The harness supplies a loader binding EA 0E2E to the loaded U9E segment.
This five-byte far-jump binding substitutes only loader relocation, not
the latch or dice implementation, and is outside the body coverage counts.
The latch PUSH CS at +0E42 and near CALL +0E43 use relative operand FF9Dh
to enter +0DE3, returning at +0E46. Dice calls SYSTEM 0AF8:11C0 at +0E0E.
SYSTEM +11C0 near-calls +120F, whose MUL reads the original two-byte
multiplier 8405h from CS:1245. That constant is data, not an extra function.

The five complete instruction bodies contain **254 bytes / 102 instructions**.
U38 returns via RETF 0Ah at +2B60, the latch via RETF 4 at +0E4F,
dice via RETF 4 at +0E2B, SYSTEM random via RETF 2 at +11D3,
and SYSTEM seed via near RET at +1244.

## API and observable stops

`pr_u38_link_roll_2b20_cpu` and `pr_u9e_roll_latch_0e2e_cpu` accept
CPU14 (AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS), exactly 1 MiB
A20-off RAM, a maximum SYSTEM-call count, and optional two-field diagnostic.
U38 requires IP2B20, SP0040..FFFC, and ten unused argument bytes.
Direct latch entry requires IP0E2E, SP0020..FFFC, and range/count words
whose low bytes are used. Both preserve the original high-byte behavior
of argument pushes.

Code must be original, immutable, correctly mapped, and separate from
mutable stack/data. SYSTEM is fixed at 0AF8 with the original multiplier.
Host CPU, diagnostic, and RAM storage are distinct. Every reached far-return
frame must fit within SS:0000..FFFF, including after alias-induced BP
changes. Effective offsets wrap at 16 bits; words are physically contiguous
with 20-bit wrap. No MMIO, asynchronous mutation, IRQ, fault, or SMC model
is included.

Invalid host arguments reject atomically, including the diagnostic.
A valid entry initializes `latch_status` and `dice_status` to -1.
The caller guards its FAR CALL and loaded EA stub before pushing the far
frame; the latch guards its near CALL before pushing the return IP.
CODE preserves all earlier effects. CALLEE preserves the nested status
and live CPU/RAM stop, with the explicit AF exception below. TRANSFER
preserves an unexpected returned CS:IP. There is no rollback.

The host budget is checked by the existing dice composition immediately
before each SYSTEM slice. Zero budget permits zero-count completion;
a nonzero count stops at +0E0E after the prefix. Later limits preserve
the completed rolls and loopback state. This is a bounded composition;
no unbounded completion or generic resume interface is claimed.

### Explicit LIMIT-only AF profile

The initial full-state oracle exposed C FLAGS0056 versus Unicorn0046 at
a zero-budget stop. The shared prefix retains undefined AF at XOR AH,AH;
the shared loopback retains INC's preceding AF. Recall with
`dunkingsheep memory context "U9E 0DE3 prefix zero budget AF XOR flags 0E0E"`
returned prior descriptions of that convention, including Codex session
`01a12b3e-db20-70e1-95f9-d3fe44b8d309`. The prefix/loop source confirms it.

Only when the dependency returns LIMIT does the new raw latch clear AF.
The last FLAGS writer at that stop is XOR AH,AH at +0E0B; this selects
the measured Unicorn AF-clear profile. The shared dice API is unchanged.
All comparisons remain unmasked. This is not a universal x86 AF guarantee
and no native host-budget-stop claim is made. First-call limits and the
15-call limit before loop index 16 exercise this rule explicitly.

## Author measurements

| Check | Measured result |
| --- | --- |
| Original-byte Unicorn | **610 cases:** 556 caller, 54 direct latch; CPU14 + all 1,048,576 RAM bytes and diagnostics, unmasked |
| Returns and stops | **585 returns, 10 budget stops, 8 call/stub guard refusals, 7 changed-return stops** |
| Coverage | **102/102 instruction starts, 11/12 conditional outcomes**; all caller/latch starts and all four caller outcomes |
| Native DOSBox-X normal/386 | **160 returns:** 96 caller, 64 direct latch; CPU14 +132,096 RAM bytes, unmasked |
| Strict C11 + ASan/UBSan | **13,667 assertions passed** |
| Focused CTest | **4/4 passed**: new raw chain, legacy caller, legacy latch, shared dice composition |

Only U9E +0DFE -> +0E1F remains unobserved. No coverage is borrowed from
earlier runs and no general alias-unreachability claim is made. The caller
fixtures produce all six actual die values 1..6 before the final increment.

The seven changed-return stops include seed writes altering SYSTEM's near
return (dice status6), SYSTEM's far return (dice status7), dice's return to
the latch (raw TRANSFER), and the latch's return to U38 (raw TRANSFER).
Other dependency argument/guard rejection statuses remain defensive and
unobserved within this immutable-code profile. Apart from the declared
LIMIT exception, dependency FLAGS conventions are preserved, including
undefined AF conventions at unmeasured dependency stops. The tests do not
claim universal hardware values for undefined flags.

The original oracle runs every original body and the loaded jump binding.
It compares CPU14, every RAM byte, and both diagnostic statuses without
masks. Caller cases exhaust the linked kind byte and vary RNG seeds;
direct latch cases include counts 0/1/2/3/16/255 and ranges 0/1/6/127/255.
Additional cases cover null and partial pointers, offset/A20 wrap,
odd/boundary SP, relocated code/stub, DS/SS aliases, character/stack aliases,
call/stub corruption, budget stops, and seed writes into return frames.

The semantic suite uses the independent recurrence
`seed = seed * 08088405h + 1 (mod 2^32)`, exhausts all 256 direct counts
with four ranges, and checks caller gates, argument high bytes, zero-count
budget behavior, and atomic invalid arguments. A direct-latch SS=DS,
SP6818 fixture puts the range at DS:681C; storing count3 changes the live
range from6 to3 before the dice call. This confirms the reread matters.

The native harness executes original code at caller5000, U9E6000, and
SYSTEM0AF8 via a loader jump at 7000:004D; only the caller's declared
segment fixup is patched. It captures CPU14, 1,024 SS bytes, all 65,536 DS
bytes, and all 65,536 heap bytes. There is no latch/dice/SYSTEM stub.
IRQs are disabled during each isolated entry. Frames are synthetic and
do not establish gameplay reach, host-budget/refusal/transfer stops,
A20/wrapped-frame behavior, or memory outside the captured windows.

Native RESULT.BIN SHA-256:
`1c61e3a1bd984490f9d6546730239c93f093bf8c35a910361169da935f2c0faa`.

Original media, generated assembly/COM, inputs, captures, private logs,
ownership/history records, and the hash receipt remain ignored under
`resources/`. The receipt pins public files, unchanged dependencies,
private artifacts, tools, and the tested/integration base.

## Reproduction

With owner-supplied media under `resources/DOS/POOLRAD/`,
Capstone/Unicorn, a C compiler, GNU as/ld, and DOSBox-X:

```sh
PYTHONPATH=resources/pythondeps:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/u38_link_roll_2b20_cpu_audit.py \
  --unicorn --native --output resources/oracles.json
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target test_u38_link_roll_2b20_cpu \
  poolrad_overlay_u38_link_roll_2b20_tests \
  poolrad_overlay_u9e_entry9_roll_latch_0e2e_tests \
  poolrad_overlay_u9e_entry8_fullstate_compose_0de3_tests -j4
ctest --test-dir resources/build \
  -R '^(u38_link_roll_2b20_cpu|poolrad_overlay_u38_link_roll_2b20_tests|poolrad_overlay_u9e_entry9_roll_latch_0e2e_tests|poolrad_overlay_u9e_entry8_fullstate_compose_0de3_tests)$' \
  --output-on-failure
```

The strict sanitizer run compiles the new semantic test with the ten C
sources enumerated by the audit's `SOURCES`, using C11, Wall/Wextra/Wpedantic/
Werror, ASan/UBSan, and frame pointers.

## Adversarial assessment

- **target_function:** OVL_0038_2B20 and supporting OVL_009E_0E2E raw extensions.
- **source_candidates:** the two extended existing sources and unchanged
  eight-source dice/SYSTEM dependency.
- **signals:** exact extents and hashes, entry slots and relocation, complete
  branch and call structure, low-byte/high-byte argument behavior, live aliases,
  seed/stack writes, CPU14/full-RAM differential, actual native chain.
- **score:** five author evidence categories supported (identity, structure,
  original CPU/RAM, native replay, semantic/stop tests); no acceptance credit
  or probability of correctness.
- **verdict:** **likely_match**, within the documented profile.
- **review_notes:** bounded SYSTEM calls, explicit LIMIT-only undefined AF
  choice, finite measured branch coverage, bounded native capture, and
  provisional identity remain material limits. Independent verification and
  credit belong to the reviewer/VP. No self-credit.
