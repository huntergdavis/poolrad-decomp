# U20 +0355: raw-CPU party award composing the SYSTEM LongInt divide

`pr_u20_party_award_0355_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x20` routine `+0355`. The VP named it as the leverage body
for the SYSTEM LongInt divide on 2026-10-17 (i17614). In the census-2218
queue its only callee is that divide (`runtime:2678`), with no inline
coverage, model, branch or main file. Every division is a real far call into
the raw-CPU [`pr_system_longdiv_0a76`](SYSTEM_LONGDIV_0A76.md). The
classifier lists the routine as `game_evidenced`. The function name
describes the code; identity and strict credit are the verifying lane's
decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U20 `+0355` | GAME file `0x58D3..0x5A46`; **372 bytes, 126 instructions, 17 conditional branches**; `RETF 4`; SHA-256 `72267bd65af83ce8e1d52a7483e19724b2d7937f192db2fc87f912b9d7732f48` |
| Calls | six `LCALL 0AF8:0A76` at `+03BB`, `+03E8`, `+0415`, `+0441`, `+046F` and `+048F`; every segment word is fixup-backed |
| Caller | U20 `+04C9`, sites `+061D` and `+07C3`, through `PUSH CS; CALL` |

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP,
`[SP+2]` the return CS and `[SP+4]/[SP+6]` a LongInt X, low word first. It
returns with `RETF 4`. After `PUSH BP; MOV BP,SP; SUB SP,8`, it walks
p := the far pointer `DS:[5D96]` while p <> nil, with p := p^[104h]. A
member with p^[10Dh] <> 0 and p^[10Ch] <> 1 receives v, starting from
v := X, by its byte c = p^[2Fh]:

| c | v |
| --- | --- |
| 0, 2, 5, 6 | X div 10 + X when p^[12h], p^[10h], p^[11h] or p^[13h] respectively exceeds 0Fh; otherwise X |
| 8, 10–14 (signed), 16 | X div 2 |
| 9, 15 | X div 3 |
| other | X |

Then the LongInt p^[0ACh] += v, by `ADD`/`ADC` to memory. Division
truncates and is signed. Every operand is reread from RAM in source order.

**Stops.** The model returns one of four stop statuses:

- `CODE`: a call site holds other bytes. IP is at the site, with BX:CX the
  divisor already loaded.
- `LIMIT`: a cyclic party list loops forever in DOS. `max_loopbacks` caps
  the taken `JMP +0368`, and the next one stops at the loop head, entry
  IP + 13h, with every earlier effect kept.
- `CALLEE` and `TRANSFER`: guards that cannot trigger. Every divisor is
  10, 2 or 3, and the divide writes only its own stack slots below its
  frame.

**Flags.** The final flags are `OR AX,[BP-2]`'s on the nil link. Three
flag writes are computed because a stop exposes them:

- the memory `ADC`, for a `LIMIT` stop;
- `XOR BX,BX`, for a `CODE` stop;
- `SUB SP,8`.

Other dead writes are folded.

Offsets wrap at 16 bits and physical addresses at 1 MiB. `LES` pairs, `POP`
and the `RETF` frame are read physically contiguous. Null RAM or CPU, or RAM
that is not exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u20_party_award_0355_cpu.c` runs under
CTest with ASan/UBSan and compiles with `-Wconversion -Werror`. Its only
game bytes are the six five-byte far-call sites. An independent reference
of the award rules, using C's truncating division, predicts every member's
LongInt field, and all RAM outside the stack window must match it. It
passes 11,002 checks:

- every class code 0 to 20 and 13 others;
- stat bytes 15 and 16;
- eleven X values, including 0, −1, −10, 2^31 − 1 and −2^31;
- 2,000 random parties of zero to six members;
- a `LIMIT` stop and a `CODE` stop;
- the argument guards.

**Unicorn differential.** `tools/overlay/u20_party_award_0355_audit.py`
places the expanded START.EXE image at load segment 0, so the original
divide runs from all six unrelocated far calls. It loads unit `0x20` and
compares the entire 1 MiB RAM and all 14 CPU words with no mask. **227
cases** pass: 223 returns, 2 `CODE` and 2 `LIMIT`. Coverage is 126/126
instruction starts and 34/34 conditional outcomes. The cases cover:

- every class code 0 to 20 and four signed edges, with stats 15 and 16;
- fifteen X edge values;
- 150 random parties;
- empty and cyclic parties and small caps;
- stacks at both segment ends, SS and the party across A20, and a random
  background;
- two rewritten sites.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **120
cases** with no mask. Each compares all 14 CPU words, an 80-byte stack
window and four members' LongInt fields. The COM copies unit `0x20` to
paragraph `8800` and the divide to `9000:0A76`. It relocates the six
fixup-backed segment words to `9000`, loader-style. The result SHA-256 is
`189da364ea27acf4f7d1135450dd2d0f1264aec5a832ec00eac75a7dfc710204` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u20_party_award_0355_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u20_party_award_0355_audit.py --unicorn --native --output resources/u20-0355-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U20 `+0355..+04C8`, composing SYSTEM `0AF8:0A76` six times |
| `source_candidates` | `src/recovered/overlay_u20_party_award_0355_cpu.c` with `src/recovered/system_longdiv_0a76_cpu.c` |
| `signals` | Exact hash and CFG, fixup-backed far calls, whole-RAM/CPU differential with the divide live and full instruction and branch-outcome coverage, `CODE` and `LIMIT` stops, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `game_evidenced`; identity is the verifying lane's call. The record fields' game meaning is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.
