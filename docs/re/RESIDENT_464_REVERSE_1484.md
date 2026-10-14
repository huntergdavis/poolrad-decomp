# RES_0464:1484 complete byte reversal

`pr_r464_reverse_1484` translates the complete call-free byte-reversal
leaf. `pr_r464_reverse_1484_frame` adds its final stack, 16-bit CPU and
FLAGS effects. This is a separate procedure outside the excluded +1951
and +2632..+2A07 work. No native game reachability is claimed for this
leaf: a bounded startup/menu probe did not stop at its breakpoint.

## Provenance and callgraph

Required MCP deja recall `Poolrad RES_0464 1484 1509 1596 native bit reversal`
returned no matching sessions. Reused the reviewed
[loader mapping](RESIDENT_LOADER_MAP.md),
[resident inventory](RESIDENT_FUNCTION_INVENTORY.md), and
[full-state container](../../include/poolrad/recovered/slot_a_directory_fullstate.h).
The new audit independently decodes the original leaf and both near callers.

| Artifact/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Loaded image `[0x5AC4,0x5B49)`, source file `[0x5AFD,0x5B82)` | 133 | `4f836924c0aff281cbe052049608847347095686294ada955e435357e552fc88` |

There are 54 contiguous instructions, no calls or branches, and one
`RETF 2` at +1506. The next procedure begins at +1509. Source-file
provenance is contiguous throughout the hash-locked span. The resident
inventory identifies the fixup-confirmed GAME.OVR call at unit
`0x3CE:+0592` to `RES_0464:15C7`. That routine calls +1484 at +17AB and
+183D, with `PUSH AX; PUSH CS; CALL` establishing the far frame. These
are the video-state-1 primary and auxiliary bitmap reversal paths.

The procedure reads only the low argument byte at `[BP+6]`, reverses all
eight bits, and returns AL; the high argument byte is ignored. The frame
adapter requires an accessible nonwrapping stack, `SP in [4,FFFA]`, and
independently verified code at the supplied CS. It preserves BP and all
registers except AX, CX, SP, CS, IP and arithmetic FLAGS. AX is the
zero-extended result, CX is 7, and SP advances by six. The pushed BP and
two result locals remain in RAM. The last ADD defines CF/AF/OF as zero
and SF/ZF/PF from the result. Intermediate writes and interrupt timing
are outside the adapter contract; faults reject atomically as a host bound.

## Parity review

| Field | Finding |
| --- | --- |
| target_function | `RES_0464:1484..1508` |
| source_candidates | `src/recovered/resident_464_reverse_1484.c` |
| signals | Exact span hash, contiguous loader provenance, full instruction extent, caller frames, bit permutation, local writes, return cleanup and FLAGS |
| score | 256/256 independent original-x86 CPU/RAM comparisons; 65,536/65,536 argument-WORD value tests |
| verdict | `match` within the stated final-state/nonwrapping-frame contract |
| review_notes | No native reached frame for this leaf; no source-library attribution or whole-engine parity |

Tests compare all 16 fields in the existing CPU container and the complete
1 MiB synthetic RAM image with Unicorn executing the original bytes.
The CPU contract covers 16-bit general/segment registers, IP and FLAGS;
FPU, hidden CPU state, hardware devices and instruction timing are excluded.
Game use is callgraph-confirmed, but the resident classifier still says
`game_or_other_library_unresolved`. No game-specific/full-C accounting
increment is proposed without independent ownership review.

## Reproduction

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -shared -fPIC -Iinclude \
  src/recovered/resident_464_reverse_1484.c \
  src/recovered/resident_464_bitmask_frame.c \
  src/recovered/resident_464_bitmask.c -o /tmp/poolrad-resident-leaves.so
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_reverse_1484_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE --ovr resources/DOS/POOLRAD/GAME.OVR
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 \
  tests/recovered/check_resident_464_reverse_1484_x86.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --library /tmp/poolrad-resident-leaves.so
```

Capstone and Unicorn are local optional audit dependencies. Original and
derived DOS bytes remain only in ignored resources or in audit process memory.
