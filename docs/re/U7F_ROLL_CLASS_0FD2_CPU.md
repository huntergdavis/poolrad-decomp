# U7F +0FD2: raw-CPU d20 class roll composing the U9E entry-8 dice chain

`pr_u7f_roll_class_0fd2_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x7F` routine `+0FD2`. It is the raw-CPU counterpart of the
hook-composed [`pr_u7f_roll_composed_0fd2`](U7F_ROLL_COMPOSED_0FD2.md), in
which a host hook supplies the stub, U9E `+0DE3`, SYSTEM and `RETF 4`
effects. Here the merged raw-RAM U9E entry-8 dice chain
(`pr_u9e_entry8_fullstate_compose_0de3`) runs behind the loaded stub
`00B0:0048` instead. The bounded prefix, call and return documents
([prefix](U7F_ROLL_PREFIX_0FD2.md), [call](U7F_ROLL_CALL_0FDE.md),
[return](U7F_ROLL_RETURN_0FE3.md)) stay as they are.

The VP assigned the routine to this lane on 2026-10-18 (i17699), after
Astra cleared it. Its only caller, U7F `+1010` (site `+10AC`, `PUSH CS;
CALL`), is Astra's next target, which will compose this full CPU14/RAM
contract. The classifier lists `+0FD2` as `game_evidenced`, grade B.
Identity and strict credit are the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; expanded image at load segment 0 places the SYSTEM generator at `0AF8:11C0` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U7F `+0FD2` | GAME file `0x25C5B..0x25C98`; **62 bytes, 27 instructions, four conditional branches**; plain `RETF`; SHA-256 `9205417006da7a4c928b547d116107e3f4d269d3d267d17b2f7cad2f3e63fe27` |
| Far call | `+0FDE` `LCALL 00B0:0048` (fixup-backed), the stub `EA 0DE3:U9E`; U9E `+0DE3..+0E51` SHA-256 `9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b` |

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP and
`[SP+2]` the return CS. It returns with plain `RETF`. After `PUSH BP; MOV
BP,SP; SUB SP,2`:

1. roll(1, 14h): `MOV AL,1; PUSH AX; MOV AL,14h; PUSH AX` and the stub
   call run U9E entry 8, one 20-sided die, with the host's
   `max_system_calls`. The chain reads the seed dword at `DS:4428` and
   writes it back.
2. r := AL is stored at `BP-2` and reloaded.
3. The class is 1 when 1 ≤ r ≤ 0Eh, 2 when 0Fh ≤ r ≤ 13h, and 3
   otherwise. It is stored at `BP-1`, and AL := class.
4. `MOV SP,BP; POP BP; RETF`.

The pushed argument words keep AH from the previous AX. Every operand is
reread from RAM in source order. A d20 is 1..20, so r = 0 cannot occur.
The two branch outcomes that only r = 0 reaches, `JB +0FEB` taken and then
`JB +0FF9` taken, are unreachable, and the audit asserts that neither is
ever taken. Class 3 therefore means r = 20.

**Stops.**

- `CODE`: the site is not `9A 48 00`, or its stub is not `EA` to `+0DE3`.
  The model stops before the call, with IP at the site.
- `CALLEE`: the chain stopped, with `*callee_status` holding its status.
  For example, `max_system_calls` = 0 gives the chain's `LIMIT` at
  `+0E0E`.
- `TRANSFER`: the chain returned to a CS:IP other than `+0FE3`. No callee
  writes its own return frame, so this is a guard.

**Flags.** The final flags are the last `CMP`'s: r against 0Eh for
class 1, against 0Fh for r = 0, and against 13h otherwise. At a `CODE`
stop they are `SUB SP,2`'s. Offsets wrap at 16 bits and physical addresses
at 1 MiB. `POP` and this routine's `RETF` frame are read physically
contiguous, and the chain keeps its own contract. Null RAM or CPU, or RAM
that is not exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u7f_roll_class_0fd2_cpu.c` runs under
CTest with ASan/UBSan, and this file compiles with `-Wconversion
-Wsign-conversion -Werror`. It uses no game bytes: only the site and the
loaded-stub `EA`, with the chain model running on synthetic RAM. For each
roll it checks all of the following:

- the class rule;
- the stored roll and class;
- AL;
- the last `CMP`'s flags;
- the pushed words and the return frame.

The chain's own arithmetic is covered by its merged tests. The test
passes 3,011 checks:

- 600 seeds reaching all three classes;
- both `CODE` stops, with `SUB`'s flags;
- the chain's `LIMIT` as `CALLEE`;
- a relocated entry;
- the argument guards.

**Unicorn differential.** `tools/overlay/u7f_roll_class_0fd2_cpu_audit.py`
places the expanded START.EXE image at load segment 0, so the SYSTEM
generator runs from its original bytes. It patches the stub to its loaded
form and loads units `0x9E` and `0x7F`. It compares the entire 1 MiB RAM
and all 14 CPU words with no mask. The site and stub bytes are read live
at the site's fetch. `max_system_calls` is mirrored by counting fetches of
U9E `+0E0E`.

| Stops | Cases |
| --- | --- |
| Returns | 308 |
| `CODE` | 2 |
| `CALLEE` (`max_system_calls` = 0) | 1 |

Coverage is 27/27 instruction starts and 6/6 reachable conditional
outcomes. The cases cover:

- 300 random seeds, stacks and flags;
- stacks near both segment ends;
- SS across A20;
- a random background;
- a relocated entry;
- preserved DF/IF;
- a rewritten site and a rewritten stub;
- `max_system_calls` = 0.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **160
cases** with no mask. They produce 113 class-1, 38 class-2 and 9 class-3
results.

- The COM copies unit `0x7F` to paragraph `8800` with the site's segment
  word relocated to `7F00`. It writes the loaded stub there and copies the
  U9E `+0DE3` span to `8000:0DE3`. It also copies the SYSTEM generator to
  absolute `0AF8:11C0` through a reserved hole in the COM layout.
- Each case compares all 14 CPU words, a 128-byte stack window and the
  seed dword.
- The result SHA-256 is
  `2e73755bd79968d4a8752c56e80e0a1187bc809c94329fdcf8c1ca531c160a55`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u7f_roll_class_0fd2_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u7f_roll_class_0fd2_cpu_audit.py --unicorn --native --output resources/u7f-0fd2-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U7F `+0FD2..+100F`, composing the U9E entry-8 dice chain through the loaded stub `00B0:0048` |
| `source_candidates` | `src/recovered/overlay_u7f_roll_class_0fd2_cpu.c` with the merged U9E entry-8 chain sources and `src/recovered/system_random_fullstate_11c0.c` / `system_seed_fullstate_120f.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with U9E and SYSTEM live and full instruction and reachable branch-outcome coverage, every reachable stop kind, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_evidenced`; identity is the verifying lane's call. The hook-composed wrapper stays as it is. The class meaning is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.
