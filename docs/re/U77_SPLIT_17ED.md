# Unit 0x77 +17ED: composed sidecar split

Role: **AUTHOR — evidence only; no acceptance, identity promotion, strict
credit, or accounting change.** PM assignments i21384/i21412. The new
`pr_u77_split_17ed_cpu` composes the recovered GetMem, Move, string loader,
and resident display routines. The legacy `pr_u77_split_17ed` callback API
and its historical tests remain available.

## Source and behavior

The private DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
`START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The checker derives unit `0077` from the executable's overlay descriptors,
decodes from `17ED` through the first `RETF`, and only then measures the span:
**176 bytes / 63 instructions**, unit `[17ED,189D)`, file `[141351,141527)`.
The first return is `189A RETF 4`; the body SHA-256 is
`46c07cbe1120a95c2830f87199a1f5141a272b9dfba7df3f534e4168079e82e9`.
Every conditional/unconditional branch destination is an instruction start
inside the span. No trailing data is counted as executable code. The private
17-byte Pascal literal immediately precedes the entry at `17DC`.

The argument is a far pointer to a 63-byte node. Counts zero and one at
node `+39h` copy the private literal to the stack and display it. Larger
counts allocate 63 bytes, copy the node, put half the count in the new node,
clear its `+34h` byte, preserve its old next pointer, and link it after the
source, whose count becomes the remainder. Reads/writes retain source order.

| Caller site | Recovered callee | Far-frame cleanup |
| --- | --- | --- |
| `1820` | SYSTEM `023F` GetMem, including its core and default HeapError | `RETF 2` |
| `1839` | SYSTEM `1685` Move | `RETF 10` |
| `188D` | SYSTEM `0B0B` StrLoad | `RETF 4`, leaves destination pointer |
| `1892` | resident `00BA:1693` display, including its clear/printer/delay chain | `RETF 4`, consumes destination |

The four live segment relocation words are `1823`, `183C`, `1890`, and
`1895`. The new entry validates each reached LCALL opcode and target offset
before pushing its real far return frame, then calls the actual recovered C
callee. It accepts no semantic callee callbacks. `CODE` stops before the
call, `CALLEE` retains the nested status/state, and `TRANSFER` retains an
unexpected successful child return. Diagnostic indices 0/1/2/3 name
GetMem/Move/StrLoad/display. Original GetMem RUNERROR, custom HeapError,
and walk-budget stops propagate without a synthetic allocation result.

The header states the raw contract: 1 MiB A20-off RAM, attached EGA,
`SP` in `[018C,FFF8]`, ordinary CS/DS/SS windows, immutable loaded code and
bindings, nonwrapping reached frames, and the nested callees' operand and
device contracts. Flat runtime operands exclude the EGA aperture. Other
in-contract aliases remain live. The lower stack bound includes display's
`016A` minimum and the caller's 34 bytes. No arbitrary-PC resume, interrupt,
or fault closure is claimed.

## Evidence machinery

`tools/overlay/u77_split_17ed_audit.py` registers
`CHECKER_ROWS = ('OVL_0077_17ED',)`. `--compile-only` compiles and binds the
same concrete C closure as the differential, without loading game media.

The full differential set has 550 synthetic cases: all 256 counts in both
incoming DF states, code guards at all four calls, allocator failure/custom
HeapError/free-list/exact/split cases, stack bounds and A20 wrap, node overlap,
Move return-frame aliasing, and nested display budget/port/code stops. The
original caller and original callees execute under Unicorn. Each case
compares **CPU14, all 16 FLAGS bits, all 1,048,576 backing-RAM bytes**, the
independent Python EGA model's complete state, OUT records, and VRAM write
operands. There are no flag masks, RAM exclusions, or successful-callee
substitutes. Twelve host argument checks separately verify rejection,
state preservation, and diagnostic reset; they are not original executions.
The checker requires all 63 caller starts and both feasible `1805 JBE`
outcomes. Closure coverage is reported separately, including its gaps.

`tools/overlay/u77_split_17ed_native.py` loads the same original callee chain
into DOSBox-X under synthetic COM frames. It captures CPU14, a 1,024-byte
stack window, complete DS/font/string segments, and all four 64 KiB EGA
planes. The source node and allocation arena reside in the captured DS
window. All other backing RAM is explicitly listed as excluded; native
controller/latch state is not captured. Four declared far-JMP traps capture
caller CODE boundaries; successful callees are never stubbed. This is a
partial native snapshot, not a natural game-route frame or full native RAM
comparison. The exact emulator binary hash, version, source revision, and
local source changes are recorded. `POOLRAD_DOSBOX` selects the executable;
without it, the installed fallback is explicitly identified.

The full **550-case differential passed** on 2026-10-22 with Capstone
5.0.7 and Unicorn 2.1.4: CPU14, all 16 FLAGS bits, all 1,048,576 RAM bytes,
and all 262,164 EGA state bytes equal per case. There were **zero masks or
RAM exclusions**, 3,892 matching OUT records and 14,570 matching VRAM write
operands. These fixtures performed zero EGA reads; they do not add read-mode
coverage. All **63/63 caller starts and 2/2 feasible caller outcomes** were
reached. The separately reported closure coverage is **728/1,101 starts and
52/120 outcomes**; its missing starts/outcomes remain in the private JSON
receipt and no full-closure coverage claim is made. Twelve host guards passed.

Stops: 525 split returns, 5 message returns, 8 caller CODE stops (two at
each call), 11 propagated callee stops, and one TRANSFER after Move overwrote
its return frame. The callee stops include one GetMem RUNERROR, two custom
HeapError boundaries, one heap walk LIMIT, and seven nested display stops.
The wrapped-node and minimum-message-stack fixtures passed. The earlier
43-case quick run was a partial fixture set and is superseded by this run.

Native and handoff-build measurements are pending. The pinned emulator's
version command writes its valid banner to stderr and exits 1; the harness
now recognizes that informational response before executing the guest.

## Claim table

| Claim | Evidence | Limit |
| --- | --- | --- |
| Caller identity and extent | Hash-checked first-return span: 176 bytes, 63 instructions | Private DOS v1.3 media identity |
| Raw composition parity | 550-case pass: CPU14, all RAM, EGA, OUT and VRAM operands | No masks; closure coverage remains partial |
| Caller control-flow coverage | 63/63 starts and 2/2 conditional outcomes | Full closure coverage is not claimed |
| Host argument behavior | 12 rejection/preservation/diagnostic checks | No original execution for host guards |
| Native corroboration | Pending | Explicit stack/data/plane windows; other RAM and controller state excluded |
| Registered checker handoff | Pending | Actual compile/load/bind check, not parity acceptance |
| Naturally reached game route | Not established | All inputs are synthetic |
| Credit or identity promotion | None requested | Accounting and manifest remain unchanged |

## Reuse and adversarial claim check

Dunking Sheep recall located the prior sidecar work in Codex session
`01a1281b-85e3-7e23-bdf6-1ea8af4d69de` and GetMem/string context in Claude
session `281c8700-5acc-438b-b93d-c061e28437eb`. Reused source evidence is the
legacy 17ED model/checker, `system_getmem_023f_cpu`,
`system_string_move_leaves_cpu`, `system_string_leaves_cpu`, and
`resident_00ba_display_1693_cpu`. The U38 `3134` raw caller and its audit/native
harness supply the far-call, independent-device, and native-capture patterns.
Recall is a pointer to that work, not an execution result.

- `target_function`: `OVL_0077_17ED`, first-return span above.
- `source_candidates`: new `overlay_u77_split_17ed_cpu.c` (raw composition);
  legacy `overlay_u77_split_17ed.c` (bounded callback profile).
- `signals`: media/body identity, instruction and branch boundaries, four
  relocated call sites, 63-byte layout and count/link offsets, real stack
  cleanup, observable error paths, full differential state, and native windows.
- `score` / `verdict`: pending measured runs and author checker pass.
- `review_notes`: the legacy callback profile's string/report cleanup and
  substituted effects do not establish raw callee parity. Full caller
  coverage does not imply full closure coverage. Synthetic inputs do not
  prove a naturally reached game route. Native uncaptured RAM/controller
  state remains a limit; strict acceptance belongs to independent review.

Reproduction (every heavy invocation through `poolrad-heavy.sh`, only when
load1 is at most 12):

```sh
python tools/overlay/u77_split_17ed_audit.py --compile-only
python tools/overlay/u77_split_17ed_audit.py --unicorn --native --output resources/u77-17ed-evidence/results.json
python tools/overlay/check_checker_builds.py OVL_0077_17ED --require-current-main --output resources/u77-17ed-evidence/checker-build.json
```

Original media, literals, generated native guests, and raw snapshots remain
under ignored `resources/`.
