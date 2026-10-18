# U7D +0000: raw-CPU party list timer up to its U9E +0026 call (partial)

`pr_u7d_list_timer_0000_cpu` is a raw-CPU, full-state model of GAME.OVR unit
`0x7D` `+0000..+02F1` (754 bytes, 248 instructions). It covers everything up
to the routine's one unmodeled call. **This is a partial model, not strict
credit.**

- The only unready callee is U9E `+0026` (`LCALL 00B0:002A` at `+01FE`).
- That routine is itself blocked by U9E `+0000` (inline-covered) and SYSTEM
  FreeMem, so it is not modeled here.
- The model stops at that call with `PENDING`, its five argument words
  pushed.
- Every path before the call is modeled exactly.

The VP assigned it on 2026-10-18 (i17934, "a partial model with honest
evidence"); it unblocks U7D `+03D5` once U9E `+0026` exists.

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `0x7D` `+0000..+02F1`, 754 bytes, 248 instructions, `RETF 4` at `+02EF` |
| SHA-256 | `89f3ee9874752acb304c19bc51e0359391d2b19259524c3d51001b51bf4119b1` |
| Calls | `LCALL 0AF8:16A8` (FillChar, composed) at `+0019`; `LCALL 00B0:002A` (U9E `+0026`, the stop) at `+01FE` |

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.

## Behaviour

The routine is entered after a far call. `[SP+4]` holds the byte u and
`[SP+6]` the byte n; `RETF 4`. After `PUSH BP; MOV BP,SP; SUB SP,1Ch`:

1. **Flags.**
   - If `DS:[49F3h]` is not 2, it runs `FillChar(DS:471Eh, 48h, 1)`.
   - Otherwise it scans the flags `DS:[471Dh+i]` from i = 1 while i is at
     most the word at `[DS:49D6h]+67Ch`. If none is set, it returns.
2. **Amount.** It starts at n and is multiplied (`MUL`, low word) by
   `DS:[363Ah + 2(k−2)]` for k = u down to 2 (a signed byte loop).
3. **Passes.** While the amount is above 0:
   - step := min(amount, 10);
   - it walks the party list (head `DS:5D96h`, links `+104h/+106h`) with i
     from 1;
   - for each member whose flag is set, it clears the flag, finds the tail of
     the member's list (head `+7Fh/+81h`, links `+5/+7`, the 9-byte nodes
     U9E `+0E52` appends), and visits nodes up to the tail:
     - a word `+1` of 0 is skipped;
     - one above the step is reduced by it and sets the flag again;
     - one at or below the step reaches `+01FE` (U9E `+0026`, the stop).
   - It then visits the nodes left after the tail, setting the flag for any
     with word `+1` above 0.
   - The amount then drops by 10, or to 0.
4. **Return.** `MOV SP,BP; POP BP; RETF 4`.

**Statuses:**

| Status | Meaning |
| --- | --- |
| `OK` | the routine returned |
| `PENDING` | stopped at `+01FE` with the member, the node's byte `+0` (over AH) and the node pushed |
| `CODE` | a reached `LCALL` has other bytes |
| `CALLEE` / `TRANSFER` | FillChar failed or returned elsewhere; unexercised guards |
| `LIMIT` | `max_loopbacks` taken backward jumps, a host cap (cyclic lists never end) |

Every operand is reread from RAM in source order. Flags follow every
flag-setting instruction; `MUL`'s undefined flags are overwritten by `DEC`
before any stop.

## Validation

**Focused C test.** `test_overlay_u7d_list_timer_0000_cpu.c` runs under
CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. It passes **18** hand-predicted
checks:

- a three-pass decrement;
- a `PENDING` stop and its frame;
- nothing flagged;
- `LIMIT` on a cyclic member list;
- `CODE` at FillChar;
- the FillChar path;
- the guards.

**Unicorn differential.** `tools/overlay/u7d_list_timer_0000_cpu_audit.py`
runs the original unit bytes, with FillChar from the expanded START.EXE
image, under Unicorn. It compares the whole 1 MiB RAM and all 14 CPU words
with **no mask**.

- **Cases: 216.** 133 return, 65 `PENDING`, 16 `LIMIT`, 2 `CODE`.
- **Coverage.** All 230 instruction starts before the call and 51/51
  conditional outcomes. Two exclusions:
  - The `+01FE` site itself.
  - The `+0251` exit for a nil list head. Only U9E `+0026` (after the call)
    or an alias of the routine's own frame can empty the head. Every case
    keeps the frame apart from the data.
- **Inputs.**
  - Same-segment and separate node lists.
  - A flag/link alias: the tail's link sits on its member's flag byte, so
    the routine's own flag writes turn the tail into a cycle. This reaches
    the "done with nodes left" path.
  - SS across A20, DF set on entry, and a random background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **48
returning cases** (24 through FillChar) with no mask. Each compares all 14
CPU words, the stack window and the whole data segment. Native members
always have at least one node: the routine reads `head->link` before its
nil test, and with an empty list that would read the live DOS vector table
at `0000:0005`. The result SHA-256 is
`d0f61e9a6bea3c4feb6b65e931033379a6e5d74c9e395a4868b8d3e2129646c4`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u7d_list_timer_0000_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u7d_list_timer_0000_cpu_audit.py --unicorn --native --output resources/u7d-0000/report.json
```

No classifier, queue or count ledger is changed. This partial model needs
U9E `+0026` before it can compose to a strict body; that decision is the
verifying lane's.
