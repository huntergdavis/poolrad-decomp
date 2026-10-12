# Unit 0x1c +0x3171: callsite and stop-flag audit for the saved-slot-A Up loop

This is an **address/hash/numeric note**. It adds no C and claims no count. The
routine at unit `0x1c` `+0x3171` is already the merged lane-A model
`pr_rec_ecl_run` (**FUNCTION_LEDGER.md function 19**, `ecl_helpers.h`,
issue #75), with its dispatcher `+0x2F2D` as **function 20**
(`pr_rec_ecl_dispatch`). Neither is re-modelled or re-counted here. What this
note adds is an independent byte re-derivation of the boundary, the complete
list of call sites, and a static scan for the writers of the three flags the
gameplay loop tests after each run
([U1C_LOOP_3387.md](U1C_LOOP_3387.md), PR #169).

`deja "Poolrad unit 0x1C +0x3171 script runner"` found no earlier trace beyond
the ledger entries cited. Reused and re-checked: FUNCTION_LEDGER.md functions
19/20 (run function paths), ECL_CONTRACTS.md section 3 (opcode to handler
offsets), GAMEPLAY_PATH.md section 3 (the loop), SLOT_A_UP_ECL_GATE.md (the
byte-checked saved run-entry path), U1C_LOOP_3387.md (the flags the loop tests).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Boundary and ABI (independent re-derivation)

| Item | Value |
| --- | --- |
| routine | unit `0x1c` (file `3,398`) `+0x3171..+0x31B2` = file `0x3EB7..0x3EF8`, **66 bytes, 22 instructions**, `retf 2` at `+0x31B0`. SHA-256 `4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489` |
| padding | `+0x31B3` is a single `0x00` byte before the next prologue `+0x31B4`; the 67-byte ledger bound (SHA-256 `128080601b881c471e934ba59702c9ac3d9b482238a81e5b161e130b708e2ef4`) includes it. It is not body |
| frame | `push bp; mov bp, sp`, no locals; one word argument `[bp+6]` (the script PC); `retf 2` |
| effect | `[0x49ED] := arg`; `[0x442E] := 0`; while `[0x442E] = 0` and `[0x49FF] = 0`: `[0x6F7E] := [0x6F7F]`; `[0x6F7F] := es:[[0x49DE] + [0x49ED] + 0x6700]` (a byte); near call `+0x2F2D`; at exit `[0x442E] := 0` |
| call | exactly one: near `+0x2F2D` at `+0x31A5` |

## Call sites (every `push cs; call rel16` in unit 0x1c whose target is `+0x3171`)

| Caller offset | Inside routine | Argument source |
| --- | --- | --- |
| `+0x2DE2`, `+0x2DFF` | `+0x2DD3` (61 bytes) | not decoded here |
| `+0x31F2`, `+0x32BE`, `+0x32CD` | `+0x31B4` (324 bytes) | not decoded here |
| `+0x3367` | `+0x32F8` (the gameplay loop), initial entry | `[0x49EB]` |
| `+0x3410` | the loop's search branch (`+0x33EC..`) | `[0x49E5]` |
| `+0x3458` | the loop at `+0x344C` | `[0x49E3]` |
| `+0x34E7` | the loop's movement bookkeeping | `[0x49E5]` |

The loop's two sites reached after an Up key are `+0x3458` (run entry
`[0x49E3]`) and `+0x34E7` (search entry `[0x49E5]`). By
SLOT_A_UP_ECL_GATE.md the saved entry words are `0x9914` and `0x9975`.

## What the loop tests after a run: writers of `[0x442E]`, `[0x442F]`, `[0x49FF]`

**Method and limit (this is an inference, not a trace).** A byte scan of every
unit in `GAME.OVR` for **direct-address byte stores** to these three data-segment
bytes: `mov byte [addr], imm8` (`C6 06 lo hi imm`), `mov [addr], al`
(`A2 lo hi`), and `inc`/`dec byte [addr]` (`FE 06/0E lo hi`). It does **not**
see stores through a register or pointer, word stores that cover two of the
bytes, runtime fill/move calls into the range, or stores reached only through
a callee. A hit list from this scan is therefore a lower bound on writers.

| Byte | Value | Unit and offset | Routine containing it |
| --- | ---: | --- | --- |
| `[0x442E]` | 1 | `0x1c +0x009B` | `+0x0001` (opcode 0x00 EXIT handler) |
| `[0x442E]` | 1 | `0x1c +0x0C5A` | `+0x0BD1` (opcode 0x20 NEWECL handler) |
| `[0x442E]` | 0 | `0x1c +0x317A`, `+0x31AA` | the run function itself |
| `[0x442F]` | 1 | `0x1c +0x0C5F` | `+0x0BD1` (NEWECL handler; the only setter) |
| `[0x442F]` | 0 | `0x1c +0x32B4` | `+0x31B4` |
| `[0x442F]` | 0 | `0x1c +0x331C` | `+0x32F8` (the loop's initialisation) |
| `[0x49FF]` | 1 | `0x1c +0x29E2` | `+0x2775` (opcode 0x2E DAMAGE handler) |
| `[0x49FF]` | 1 | `0x20 +0x04D4` | not mapped here |
| `[0x49FF]` | 0 | `0x1c +0x29FA` | `+0x2775` |
| `[0x49FF]` | 0 | `0x1c +0x34FF` | `+0x32F8` (the loop's exit) |
| `[0x49FF]` | 0 | `0x20 +0x052E`, `+0x05AF`; `0x36 +0x0429` | not mapped here |

Handler offsets come from ECL_CONTRACTS.md section 3 (EXIT `+0x0001`, NEWECL
`+0x0BD1`, DAMAGE `+0x2775`).

## Consequence for the saved slot-A run-entry path (inference)

SLOT_A_UP_ECL_GATE.md byte-checks the saved run entry `0x9914` as SAVE (0x09),
COMPARE (0x03), IF (0x16..0x1B family, handler `+0x0B52`), GOTO (0x01), AND
(0x2F, handler `+0x0DEF`) and EXIT (0x00). None of those handlers is `+0x0BD1` or
`+0x2775`. So, **if the scan above is complete for these bytes and the handlers
on the path make no indirect or callee-reached store to them**, then after the
run `[0x442F]` and `[0x49FF]` keep their pre-run values (EXIT sets `[0x442E]`,
which the run function clears on exit), and the gameplay loop goes on to its
movement bookkeeping. This is a static reading, not a captured guest trace.

## Open dependencies (not closed here)

* The handler bodies on the saved path: SAVE, COMPARE, IF and GOTO are covered
  by lane A's merged models (bounded); **AND `+0x0DEF`** (111 bytes) has
  its source operand `0xC04F` unresolved (SLOT_A_UP_ECL_GATE.md) and is the next
  audit; EXIT's call-stack release and frame effects are as in function 22's
  neighbours.
* The search entry `0x9975` run at `+0x34E7` reaches SAVE and then PICTURE
  (opcode 0x0E, handler `+0x0856`), which is unread.
* Indirect stores and callee chains inside the handlers (the limit of the scan).

No hook in this tree may report success for these handlers; a host wiring the
loop to `pr_rec_ecl_run` must let `UNMODELLED`/`HANDLER` statuses stop the run.
