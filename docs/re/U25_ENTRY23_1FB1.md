# Unit 0x25 +0x1FB1: public entry 23, composed full state

`pr_u25_entry23_1fb1_composed` is the strict full-state translation of the
complete U25 public entry 23 (far stub `2B:009B`). It runs over caller-owned
A20-off 1 MiB RAM and a fourteen-word CPU frame. Its only external callee,
resident `BA:2199`, runs as a real far call into the merged
[raw-CPU `RES_00BA_2199` model](RESIDENT_SIDECAR_LOOKUP_2199_CPU.md) (PR #902).

The provisional hook entry `pr_u25_entry23_1fb1` from PR #756 is kept for
its existing callers. There, the lookup stays a fail-closed call-site
callback. **This revision asks the verifying lane to review the composed
entry for strict full-C credit. Game identity remains that lane's
decision**, as the provisional revision noted. The recall attempt for this
lane found nothing to reuse.

## Original boundary and ownership

Ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x25` starts at file `0x74D3`. The executable routine is exactly file
`[0x9484,0x9551)`, unit `+1FB1..+207D`, **205 bytes / 74 instructions**,
SHA-256 `8d6da69d20a93bea32da791cf65623426c7cd35910e9c9118158c564110882b3`.
Its only return is `+207B RETF 8`, and all jumps are internal.

The ledger CSV's 283-byte bound wrongly includes `+207E..+20CB`. Those 78
bytes are literal data owned by `+20CC`, whose `MOV DI` loads at
`+2117..+21A8` are their only references, and `+20CC`'s prologue follows
them. The text is not reproduced.

The only caller is the far call through `2B:009B` at U1C `+1F1A`. pG
cleared this interval relative to its own work, and pF had already cleared
it. No source, branch or sibling-clone file owned it at `862a498`.

`BA:2199` was decoded from START.EXE expanded with the repository's
`resident_loader_map.unpack`. It sits at image `0x2D39`, with a Pascal
prologue and a sole `RETF 0x000A` at `0x2DA5`. Its arguments are a cursor
far pointer, a key word (low byte) and a character far pointer, and it
returns AL=1 when found. Both call sites here push exactly these five
words, `MOV AL,key; PUSH AX` carrying AH through.

## Behavior

Arguments are two far byte pointers: minimum `[BP+6]` and maximum
`[BP+0xA]`. The frame is `SUB SP,0xA`: node `[BP-4]:[BP-2]`, cursor
`[BP-8]`, value `[BP-9]`.

1. Load the list head from `DS:[5D98]:[5D96]` and seed **both** outputs
   with that head node's byte `+0x11C`. This happens even when the head is
   null, so the byte is read from `0000:011C`.
2. While the node pointer is non-null:
   1. Take its byte `+0x11C`.
   2. Call the lookup with key `27h`. If found, double the value with
      `SHL`, truncated to a byte.
   3. Otherwise call it with key `2Ah`. If found, halve the value.
   4. Raise the maximum if the value is above it, and lower the minimum if
      it is below it.
   5. Follow the link at `+0x104/+0x106`.

Every pointer, byte and output is re-read where the original reads it, so
callee or alias writes (cursor, outputs, list) take effect in source order.
A cyclic list does not return, as in DOS. The hook entry has no host cap,
and the composed entry stops at a caller-chosen cap.

Flags are exact wherever the hook or the return observes them:

- **First lookup:** the `OR AX,[BP-2]` loop test.
- **Second lookup:** `OR AL,AL` after the first lookup.
- **Return:** the final `OR` that finds the null link.

The `XOR`, `SHL` and `SHR` flags are dead before the next `CMP`. `OR` uses
the reference emulator's cleared AF, which is architecturally undefined.

## Composed entry

The composed entry runs the same body. At each reached lookup it reads the
far call from RAM: opcode `9A`, offset `2199h` and a segment.

1. It pushes CS and the next IP, relative to the entry IP.
2. It runs `pr_res_00ba_2199_cpu` at that segment with
   `lookup_max_loopbacks`. That model's `RETF 0Ah` removes the return frame
   and the five argument words, so no wrapper cleanup is added.
3. The model must return to the call's next instruction.

Otherwise the entry returns one of these statuses:

- **`CODE`.** The reached site is not `LCALL xxxx:2199`. The entry stops
  before it with the five words pushed.
- **`CALLEE`.** The lookup model stopped. `*callee_status` holds its
  status, for example `LIMIT` on a cyclic sidecar list.
- **`TRANSFER`.** The lookup model returned elsewhere.

**Node-walk cap.** `max_loopbacks` caps the taken `JMP +2075 → +1FE0`
loop-backs, one per node visited. On the next one the entry stops at
`+1FE0` and returns `LIMIT`, with IP = entry IP + 2Fh, SP = BP − 0Ah and
every earlier effect kept. That is the original's state on reaching `+1FE0`
that time.

The final flags are the closing `OR AX,[BP-2]`'s. The composed entry needs
RAM of exactly 1 MiB.

**Read convention.** Both entries read `LES` pairs and the `RETF` frame
physically contiguous, as the Unicorn oracle does and as the other strict
models do. The provisional revision read their second words with the
16-bit offset wrapped. The two differ only for a frame or pointer pair
straddling the end of SS, and no earlier test pinned that edge.

## Verification

**Focused C test.** `test_overlay_u25_entry23_1fb1_composed.c` runs under
CTest with ASan/UBSan, and compiles with `-Wconversion -Werror`. It passes
30 checks, holding only the two call-site bytes and no game bytes:

- a three-node walk with halving and doubling;
- the doubling byte wrap and 27h tried before 2Ah;
- the null head reading `0000:011C` with no lookup;
- a two-node cycle stopping as `LIMIT` at `+1FE0`;
- a cyclic sidecar list stopping as `CALLEE` with the lookup model's
  `LIMIT` at `00BA:21B9`;
- `CODE` at a rewritten site;
- the argument guards.

The provisional `test_overlay_u25_entry23_1fb1.c` still passes unchanged.

**Unicorn differential.** `tools/overlay/u25_1fb1_composed_audit.py` places
the hash-checked expanded START.EXE image at load segment 0, so the
unrelocated `LCALL 00BA:2199` sites run the original resident bytes. It
loads unit 0x25 and compares the entire 1 MiB RAM and all 14 CPU words with
no mask. It passes **298 returning cases**:

- 240 random lists of one to eight nodes;
- every key pattern (27h, 2Ah, both orders, neither) for lists of one to
  five nodes;
- the null head;
- outputs aliasing a node's value byte or link, the value byte `[BP-9]`
  and the cursor cell;
- five segment-end stacks and SS across A20;
- a random background and preserved DF/IF.

For 278 of them it also checks min and max against the documented walk.
All 74 instruction starts and all 10 conditional outcomes are reached.

It also passes **9 cap stops**, each equal to the original stopped at the
same loop-head arrival. Four are node-walk `LIMIT` stops on cyclic lists,
at `+1FE0`. Five are lookup `CALLEE` stops, at `21B9`, where a max output
that rewrites a node link sends the walk into arbitrary memory.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **46
cases**, 42 of them with min and max checked. Each case compares all 14 CPU
words, 256 stack bytes and 12,880 data bytes, with no mask.

- The COM copies unit 0x25 and the 111 resident bytes to absolute
  paragraphs.
- The two fixup-backed far-call segment words are relocated loader-style to
  the resident paragraph `8C10`.
- Each case's DS head, nodes, sidecar lists and outputs stream from
  `WORLDS.BIN`, built by the same fixture.
- Null heads and link-rewriting worlds stay in Unicorn. The former read
  DOS-owned `0000:011C`, and the latter walk arbitrary memory.

The result SHA-256 is
`a3c19d680d94868b7b34e142840e4f495bdc389469116e696bbd8e400ac7d1d9` with COM
segment `0814`.

The earlier hook-based `check_overlay_u25_entry23_1fb1.py` is kept as it
was. Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u25_entry23_1fb1 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u25_1fb1_composed_audit.py --unicorn --native \
  --output resources/u25-1fb1-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x25` `+1FB1..+207D` (public entry 23), with `RES_00BA_2199` (raw CPU) composed at two sites |
| `source_candidates` | `src/recovered/overlay_u25_entry23_1fb1.c` (`pr_u25_entry23_1fb1_composed`) |
| `signals` | Exact hash/CFG, fixup-backed far calls, whole-RAM/CPU differential with the resident callee live and full instruction and branch-outcome coverage, matched cap stops in both loops, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Game identity has not been asserted. The node walk and the lookups loop forever on cyclic lists, which the host caps stop and the audit compares at the stop. Natural saved-game reach is unmeasured. |

No classifier or count ledger is changed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0025_1FB1 --require-current-main
```
