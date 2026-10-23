# U68 +04DF: raw-CPU movement flag (strict, composed)

`pr_u68_movement_flag_04df_cpu` is a raw-CPU, full-state model of GAME.OVR
unit `0x68` `+04DF..+05AA`. The existing model
[`overlay_u68_movement_flag.h`](U68_MOVEMENT_FLAG_04DF.md) is bounded. It
keeps the roll at `00B0:0048` and both `+0109` calls as host hooks, and it
claims no strict credit. This model composes both callees in full state:

- the merged U9E entry 8 dice chain, `pr_u9e_entry8_fullstate_compose_0de3`,
  through the loaded stub, following the accepted U2F `+0131` convention;
- the raw `+0109`, `pr_u68_map_update_0109_cpu` (PR #1017).

It is offered as strict body credit for `OVL_0068_04DF` (queue class
`game_evidenced`). The VP approved it on 2026-10-18 (i17962).

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `0x68` `+04DF..+05AA`, GAME.OVR file offset `0x16BAA` |
| Size | 204 bytes, 81 instructions, plain `RETF` at `+05AA` |
| Calls | `LCALL 00B0:0048` at `+0513`; `PUSH CS; CALL +0109` at `+055C` and `+059A` |
| SHA-256 | `2d93ca5d0c9a3f1a5d609fc04bdc3a5ab66a33444d2dc4f2e9ee4aa08c2362b1` |

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

## Behaviour

Entry 1 calls the routine with `PUSH CS; CALL +04DF` at `+0ED7`. It takes
no arguments. After `PUSH BP; MOV BP,SP; SUB SP,0Ah` the frame holds:

| Frame slot | Contents |
| --- | --- |
| `[BP-0Ah]`, `[BP-8]` | a copy of the current member pointer (offset, segment) |
| `[BP-6]` | the success byte |
| `[BP-5]`, `[BP-3]` | the current member pointer (offset, segment) |
| `[BP-1]` | the returned byte |

1. **Roll loop** (`+04E5..+0543`). The success byte starts at 0 and the
   member at `DS:[5D96h]:[5D98h]`. While `offset OR segment` is nonzero
   and the success byte is 0:
   - The member is copied to `[BP-0Ah]`.
   - `MOV AL,1; PUSH AX; MOV AL,64h; PUSH AX` pushes two words whose AH
     is the high byte of that `OR`.
   - `LCALL 00B0:0048` runs entry 8, which rolls one d100 from the SYSTEM
     RANDOM seed and returns it in AL.
   - The roll succeeds if AL is at most the member's byte `+78h` (an
     unsigned `JA` skips) and its byte `+10Ch` is zero.
   - The next member is read from the link at `+104h/+106h`, even after a
     success, and `JMP 04F6` repeats the test.
2. **Clear** (`+0545`). `DS:[6DDFh] := 0` on every path.
3. **Marks** (`+0550..+059B`, on a success only). The first `+0109` call
   gets x = `DS:[6AADh]`, y = `DS:[6AAEh]` and facing = `DS:[6AAFh]`
   (each `MOV AL; PUSH AX`). Facing, x and y are reread after it returns,
   and the second `+0109` call gets:
   - x + t1[facing] and y + t2[facing], with the signed byte tables t1 at
     `DS:2736h` and t2 at `DS:273Fh` (`CBW; ADD AX,DX`);
   - selector (facing + 4) mod 8 (`XOR AH,AH; ADD AX,4; CWD; MOV CX,8;
     IDIV CX; XCHG DX,AX`).
4. **Return** (`+059E`). AL is set to the success byte through `[BP-1]`,
   then `MOV SP,BP; POP BP; RETF`.

**After a return:**

- AL is 0 or 1.
- Without a success, the flags are those of `CMP BYTE [BP-6],0` (ZF and
  PF set).
- After a success, the second `+0109` call either stored or returned
  early. AH, BX, CX, DX, DI, ES and the flags are whatever it leaves.
  `IDIV` writes the pinned IDIVW flags; they never survive `+0109`'s first `CMP`. Pinned DOSBox-X `DIVW`/`IDIVW`/`DIVB` write all six arithmetic flags (AF=SF=OF=0, ZF=(rem==0 && quo odd), CF=((rem&3) in 1..2), PF over rem:quo; `instructions.h:635-761`), measured natively in [U33_PARTY_ATTACH_12E1_CPU.md](U33_PARTY_ATTACH_12E1_CPU.md) (256 IDIV checkpoints, all FLAGS bits unmasked); the model writes those values.

**Addressing.** Every operand is reread in source order. LES pairs, word
operands, `POP` and the `RETF` frame are read physically contiguous.
Offsets wrap at 16 bits and physical addresses at 1 MiB. The party list
can be cyclic in RAM. The code must sit at its original offsets
(IP = `04DFh`), with `+0109` at its own.

## Composition and stops

| Status | Meaning |
| --- | --- |
| `OK` | returned through `RETF` |
| `ARGUMENT` | null pointers, RAM other than 1 MiB, or IP not `04DFh`; nothing changed |
| `CODE` | the `+0513` bytes are not `9A 48 00`, or the loaded stub is not `EA E3 0D`; stops at `+0513` after the two pushes |
| `CALLEE` | the dice chain or a `+0109` call stopped; `*callee_status` holds its status |
| `TRANSFER` | a callee's `RETF` popped a CS:IP other than the call's next instruction (a guard that is never exercised) |
| `LIMIT` | `max_loopbacks` loop-backs were taken; the next one stops at `+04F6` |

**The roll.** The `LCALL` operand bytes are checked at the site before the
pushes, and the stub segment is read from the site. The stub must be
`EA E3 0D seg`. After CS and the return IP `0518h` are pushed, the merged
`pr_u9e_entry8_fullstate_compose_0de3` runs from `seg:0DE3` with
`max_system_calls` 256.

**The marks.** Each `+0109` call pushes CS and `site + 3`, then runs
`pr_u68_map_update_0109_cpu`.

## Validation

**Focused C test.** `test_overlay_u68_movement_flag_04df_cpu.c` runs under
CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. It passes **20** checks. Each roll and
seed is predicted with the shared synthetic `dice_chain_fixture.h`. The
cases are:

- A sure success (byte `+78h` = FFh) uses one roll, makes both marks,
  returns AL = 1 and leaves `[6DDFh]` = 0. It marks (3, 4) with facing 2
  (`AND F3h; OR 04h`) and the neighbour (4, 4) with selector 6
  (`AND 3Fh; OR 40h`).
- At the `JA` boundary, a byte equal to the predicted roll succeeds and one
  below it fails.
- A zero byte followed by a busy member uses two rolls and makes no marks.
- An empty party uses no roll, and the flags come from the final `CMP`.
- A self-linked member with cap 0 stops with `LIMIT` at `+04F6`.
- A patched `LCALL` stops with `CODE` at `+0513`, with both words pushed.
- The guards.

**Unicorn differential.** `tools/overlay/u68_movement_flag_04df_cpu_audit.py`
builds one 1 MiB image in the layout of the merged U33 dice-chain audits. It
holds:

- unit `0x68` at its original offsets;
- the original U9E `+0DE3..+0E51` span;
- the original SYSTEM RANDOM bytes at `0AF8:11C0`;
- the stub at `00B0:0048`.

The audit compares the whole RAM and all 14 CPU words with **no mask**. The
original decides each stop, and a fetch outside the legal spans is an
error.

- **Cases: 159** (156 return, 1 `CODE`, 2 `LIMIT`).
- **Coverage.** 81/81 instruction starts and 10/10 conditional outcomes.
- **Inputs.**
  - Party lists of 0 to 6 members, some of them cyclic. Byte `+78h` is
    0, 5, 50, 99, 100, FFh or random, and each member has a busy byte.
  - In-range and out-of-range x and y, every class of facing, and t1/t2
    bytes of 00h, 01h and FFh.
  - Map pointers whose `+300h` wraps.
  - SS across A20, DF set, several flag seeds, a random SP and a random
    background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **32
cases**, 23 of them with a success, with no mask. Each compares all 14 CPU
words, the stack window and the whole data segment. The SYSTEM bytes sit at
absolute `0AF8:11C0`. The result SHA-256 is
`d71e96f991dafd0671d6cb8b502bd2e89f93696d3047da66455f4e8386d8b416`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u68_movement_flag_04df_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u68_movement_flag_04df_cpu_audit.py --unicorn --native --output resources/u68-04df/report.json
```

`deja "Poolrad U68 +04DF movement flag raw CPU full-state compose dice +0109"`
found no prior session. The decoding was checked against the bounded
model's span table. The roll convention reuses the accepted U2F `+0131` and
U33 dice-chain audits.

No classifier, queue or count ledger is changed. Crediting `OVL_0068_04DF`
is the verifying lane's call.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0068_04DF --require-current-main
```
