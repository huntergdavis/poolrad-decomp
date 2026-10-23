# U68 +0294: raw-composition upgrade of the bounded roll gate

`pr_u68_roll_gate_0294_cpu` extends the existing
`overlay_u68_roll_gate_0294.c/.h` pair with direct raw CPU composition.
The [bounded model](U68_ROLL_GATE_0294.md) from merged commit
`fac003f33bebd7d052564aad42bbf115229827a8` and its hook API remain intact.
This is an upgrade of that model, not a new identity. No gameplay routing
or acceptance/accounting records are changed.

The raw entry walks the live far list, reads the U3BD field result,
performs the original node-type/value threshold comparisons, and invokes
the complete U9E/SYSTEM dice chain where reached. A hit invokes the raw
map updater twice. Registers, flags, real stack frames, RNG seed updates,
map writes and partial-stop state remain live throughout.

## Exact executable span

The hash-pinned GAME.OVR decode proves **U68 [0294,04DF)**, file
**[1695F,16BAA)**, exactly **587 bytes / 247 instructions**.
The last instruction is RETF at 04DE, occupying the final byte. The next
byte at 04DF is the next routine's PUSH BP. Every byte in the claimed
span decodes to an instruction; **no trailing Pascal strings or padding
are included**. All 247 starts were reached by the caller audit.

Caller SHA-256:
`8a2393e6e0632118ba5094510ee1d8aa5a0a3a2c84ef2eb1ad27a0f85844ef48`.

The audit verifies the single field call at 02D4, twelve dice-call sites,
two PUSH CS / near map calls, their fixup-backed far segment operands,
and the exact instruction extent. It does not infer an extent from a
queue row alone.

Original DOS v1.3 media, kept under ignored `resources/`:

- START.EXE:
  `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
- GAME.OVR:
  `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

## Dependencies and data boundary

| Executable body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
| U68 [0294,04DF) | 587 / 247 | `8a2393e6e0632118ba5094510ee1d8aa5a0a3a2c84ef2eb1ad27a0f85844ef48` |
| U68 [0109,0236) | 301 / 125 | `3f9a24eb07cda0c97f99a02fbd295fd7c4f55914e035d296b7e2b6ea7462d554` |
| U3BD [031F,0350) | 49 / 18 | `ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b` |
| U3BD [0350,047B) | 299 / 120 | `3b0a525351d0a12b3afeed4c9d9d39e28fdf3564166cdab0d16219fad1b60498` |
| U3BD [047B,057A) | 255 / 103 | `92d75878d963a7f5766b1ea49e2f6c1fb51287775d06a5b6ca485fee8bc43658` |
| U9E [0DE3,0E2E) | 75 / 29 | `7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba` |
| SYSTEM [11C0,11D6) | 22 / 10 | `e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9` |
| SYSTEM [120F,1245) | 54 / 22 | `f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242` |

These eight executable bodies total **1,642 bytes / 674 instructions**.
The loaded overlay dispatch stubs add two synthetic EA instructions
(10 bytes), tracked separately from the original-body totals.

SYSTEM also reads the **data word 8405h at 0AF8:1245**, immediately after
the seed helper's RET. Both oracles load and pin that two-byte operand;
it is excluded from executable counts. Its SHA-256 is
`d9221ae2959bbc366aaadbb08e200b236d45dbaa8bf463cd13da45a9115baa0d`.
Development fixtures that omitted this operand were superseded; the
reported final results below include it.

## Entry, guards and budgets

Enter at IP0294 with the caller's far-return frame already on SS:SP.
The API accepts exactly 1 MiB of A20-off ordinary RAM and entry
SP0040..FFFC. Code is immutable during execution. The stack interval
keeps reached frames from wrapping the 16-bit offset; physical addresses
still wrap at 1 MiB. Original overlay offsets and loaded EA stubs are
required. SYSTEM is fixed at 0AF8 by the reused dice contract.

The C body retains the decoded operand and branch order. Each far-call
boundary checks its opcode, offset and loaded stub before pushing the
far frame; each near map-call guard runs after the original PUSH CS.
The report identifies the last reached FIELD, DICE or MAP boundary,
with status -1 until that callee executes.

| Status | Meaning |
| --- | --- |
| OK | Outer RETF completed; popped return address and restored BP retained. |
| ARGUMENT | Invalid pointers, RAM size, entry IP or entry SP; CPU/RAM unchanged. |
| CODE | Reached call or stub guard rejected; prior effects retained. |
| CALLEE | Reached dependency stopped; exact dependency status retained. |
| TRANSFER | A successful dependency returned to an unexpected CS:IP; defensive and unmeasured here. |
| LIMIT | List-loop cap exhausted at target 02AB after the backward jump. |

The list budget counts backward transfers 047B→02AB. The dice budget
limits SYSTEM calls **per dice invocation**; zero stops at U9E:0E0E
after the real prefix and argument push. Neither stop unwinds the stack.
The shared dice prefix leaves undefined XOR AF unchanged. Only at its
LIMIT boundary this new adapter clears AF to match the measured Unicorn
profile. Shared dependency implementations remain untouched. Undefined
shift and DIV/IDIV flags follow the pinned stored profile (see FLAG_SEMANTICS_CENSUS.md): shifts set AF, DIV/IDIV write the pinned values. Pinned DOSBox-X `DIVW`/`IDIVW`/`DIVB` write all six arithmetic flags (AF=SF=OF=0, ZF=(rem==0 && quo odd), CF=((rem&3) in 1..2), PF over rem:quo; `instructions.h:635-761`), measured natively in [U33_PARTY_ATTACH_12E1_CPU.md](U33_PARTY_ATTACH_12E1_CPU.md) (256 IDIV checkpoints, all FLAGS bits unmasked); the model writes those values. This is not
a claim across arbitrary CPUs. Native partial-stop flags are not measured.

## Author evidence

[`u68_roll_gate_0294_cpu_audit.py`](../../tools/overlay/u68_roll_gate_0294_cpu_audit.py)
executes original bytes and independently selects the return, guard or
budget stop. Each case compares **all 14 CPU words and all 1,048,576 RAM
bytes with no masks**.

- **1,662 original-byte cases:** 1,613 full returns, 34 call/stub guards,
  12 nested dice LIMIT stops, and 3 list LIMIT stops.
- **247/247 caller starts** and **76/78 syntactic caller conditional
  outcomes**. The two excluded outcomes are provably infeasible:
  0419→0434 is unsigned AL<0; 0436→0451 would require AL<51 after
  the preceding comparisons established AL>50.
- **661/674 original-body closure starts** and **136/156 conditional
  outcomes** were observed from this caller. No coverage is borrowed from
  another audit. Unvisited starts: U9E 0E05; U3BD field 0395, 03BB;
  U3BD nibble 0491, 0496, 0498, 049D, 049F, 04A3, 04AC, 04B6,
  04C0, 04CA. The JSON report lists every missing conditional edge.
- Fixtures include every byte node type, every byte value on type12,
  both field-result branches, multiple seeds at every roll site,
  guard failures at all fifteen call sites, loaded-stub failures,
  finite and cyclic lists, coordinate/map-id/selector boundaries,
  direction-table signs, entry-SP limits and A20 wrapping.

[`u68_roll_gate_0294_cpu_native.py`](../../tools/overlay/u68_roll_gate_0294_cpu_native.py)
runs **48 predetermined DOSBox-X normal-core/386 full-return cases**,
including **25 successes**. There is no candidate-C filtering of inputs.
Each compares **CPU14, a 256-byte stack window and the entire 65,536-byte
DS segment**, unmasked. Lists and maps live within DS in these native
fixtures; SYSTEM's seed writes and both map updates are captured.

Native RESULT.BIN SHA-256:
`fd5363a3b004e464692e217f4b56da392b95fab7f1b2b55b2ce9dc2ccfee7eec`.

The synthetic C test compares 36,864 combinations against the preserved
bounded model with an independent algebraic RNG fixture and explicit map
writes. It also checks rejection without mutation, guard frames, list
budget state, dice LIMIT state/AF and unchanged seed before SYSTEM runs.
ASan/UBSan passes **296,761 checks**. All **5 focused CTests** pass:
new raw caller, preserved bounded caller, raw map updater, U3BD lookup
composition, and U9E dice composition.

## Reproduce

```sh
python3 tools/overlay/u68_roll_gate_0294_cpu_audit.py \
  --unicorn --native --output resources/u68-roll-report.json
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target poolrad_u68_roll_gate_0294_cpu_tests \
  poolrad_overlay_u68_roll_gate_0294_tests
ctest --test-dir resources/build --output-on-failure -R 'u68_roll_gate_0294'
```

The audit needs Capstone/Unicorn; native capture additionally needs GNU
as/ld and DOSBox-X. It verifies the observed COM segment is 0814 before
accepting the absolute SYSTEM placement. Generated guest programs,
media, captures and receipts remain ignored.

## Reuse and adversarial check

Dunking Sheep query `U68 0294 roll gate raw CPU U3BD 0350 fullstate`
returned Claude sessions `12a7f8ad-6b6c-4d8e-b123-327925860532`,
`3e3ff494-f865-446a-8777-6a4baca8de94` and
`281c8700-5acc-438b-b93d-c061e28437eb`. Reuse consists of the preserved
bounded model, merged U3BD/U68/dice raw APIs, the adjacent U68 +04DF
flag helpers/native capture pattern, and the existing hash-pinned
lookup/dice extraction and algebraic test fixture. The new caller
composition is measured here. VP approval is i18846.

- **target_function:** U68 [0294,04DF), raw upgrade of the merged model.
- **source_candidates:** the new raw companion in the existing source pair.
- **signals:** exact executable extent/hash, all call sites/fixups, live
  operand order, independent original stops, CPU/RAM comparisons, native
  captures, exhaustive semantic table comparison and sanitizer checks.
- **score:** 9/10 author confidence within the stated profile; not credit.
- **verdict:** likely_match within that profile, pending independent review.
- **review_notes:** closure gaps above remain unmeasured. No native guard,
  budget-stop or full-1MiB capture, asynchronous interrupts, MMIO,
  self-modifying code, wrapping stack-offset frames, arbitrary stack/data
  alias coverage, or end-to-end gameplay claim. Oracle data/list/map/code
  and active stack are separate. Defensive unexpected-return paths remain
  unmeasured. No new identity is claimed.

The VP/reviewer owns independent acceptance and any credit.
