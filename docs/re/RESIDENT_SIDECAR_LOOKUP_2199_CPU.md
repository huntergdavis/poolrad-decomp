# Resident RES_00BA_2199 sidecar lookup: raw-CPU full state

`pr_res_00ba_2199_cpu` is a raw-CPU, full-state model of the complete DOS
v1.3 `START.EXE` resident routine `RES_00BA_2199`. That routine is the 9-byte
sidecar lookup that the [callback model](RESIDENT_SIDECAR_LOOKUP.md) already
translates and that the checkpoint already counts as strict full C. This
companion **claims no new credit**.

It exists so that overlay routines calling `RES_00BA_2199` can compose it as
a CPU body. The callback model returns only AL and the cursor-cell writes.
It leaves the routine's AH, DX, ES:DI, flags, frame bytes and saved-BP
effects unmodeled. Any caller whose final state carries them could not be a
full-state body. The current candidate queue lists `RES_00BA_2199` as a
`resident_covered` callee of 26 uncredited overlay rows. U31 `+0675` is one
of them, and the VP approved this model so they can compose it (2026-10-17).
The recall attempt for this lane found nothing to reuse.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded image | `resident_loader_map.unpack`, the [hash-locked loader map](RESIDENT_LOADER_MAP.md) |
| Function | loaded `00BA:2199`, expanded `0x2D39..0x2DA7`; **111 bytes, 37 instructions, three conditional branches, no calls, no DS access** |
| Function SHA-256 | `721e86627eac3e72deed41d2a26283e7c95444d99fc4f762a9714b11504dfa9c` |
| Exit | `RETF 0Ah` at `2205`; every branch target lies inside the body |
| Callers | [Callable ledger](resident_callable_ledger.csv): exact Pascal prologue, 39 fixup-confirmed GAME.OVR callers |

## Complete ABI and ordered effects

The API enters at `CS:IP` = resident `00BA:2199` after the far CALL, with
`SS:[SP]` return IP and `[SP+2]` return CS. The Pascal arguments sit above
them:

- `[BP+6]/[BP+8]` is the cursor cell C, a far pointer.
- `[BP+0Ah]` is the key word; only its low byte is compared.
- `[BP+0Ch]/[BP+0Eh]` is the character object X, a far pointer.

`RETF 0Ah` removes the arguments, leaving SP fourteen bytes above entry.

1. `PUSH BP; MOV BP,SP; SUB SP,2`, then the match flag `[BP-2]` := 0.
2. AX and DX load X's `+7F`/`+81` words, and C := those two words.
3. **Loop head** `+20h` (`21B9`). AX := C.low `OR` C.high. The loop exits
   if that is zero (`JE`), or if `CMP BYTE [BP-2],0` finds the flag set
   (`JNE`).
4. Otherwise byte 0 of the node C points at is compared with `[BP+0Ah]`.
   On equality `[BP-2]` := 1. On a miss, AX and DX load the node's
   `+5`/`+7` link, and C := that link.
5. `JMP` back to the loop head. A match therefore takes one more loop-back
   before it exits.
6. At exit, `[BP-1]` := `[BP-2]` and AL := `[BP-1]`. Then
   `MOV SP,BP; POP BP; RETF 0Ah`.

At exit, AH keeps the high byte of the last `OR` result. That is 0 for an
empty or exhausted list, and the high byte of the match's offset `OR`
segment after a hit. DX holds the last head or link segment word loaded,
and ES:DI holds the cursor-cell pointer from the last `LES DI,[BP+6]`.

The final flags come from one of two instructions:

- **The `OR`** on the zero exit: ZF, PF and SF from AX; CF, OF and AF
  clear. Intel leaves AF undefined for `OR`; both oracles clear it.
- **The `CMP`** on the match exit.

`SUB`'s flags never survive. BX, CX, SI, DS and SS are unchanged.

Every operand is reread from RAM in source order: C, the node pointer, the
node byte, the key and the flag. A cursor cell that aliases the frame, the
arguments, the character head or a node therefore behaves as DOS does.

- With C at `SS:BP-2`, the head lands on the flag, the result copy and the
  saved BP, so `POP BP` returns the head's segment.
- With C on the argument cell itself, the walk rewrites its own pointer.

Offsets wrap at 16 bits and physical addresses at 1 MiB. Words, `LES`
pairs and the `RETF` frame are read physically contiguous. Unicorn reads
them that way, and so does DOSBox-X for the straddling node word below. A
segment-wrapping 8086 would differ only at a segment-end straddle. Null
RAM/CPU, or RAM that is not exactly 1 MiB, is rejected before mutation.

**Host cap.** `max_loopbacks` caps the taken `JMP 21F7 → 21B9` loop-backs
that the model follows. On the next one it stops at the loop head and
returns `LIMIT`. CS is then unchanged, IP is the entry IP plus 20h, SP is
BP − 2, and every earlier effect is kept. That is the original's state on
reaching the loop head that time. A list of n nodes needs at most n + 1
loop-backs. A cyclic list without the key, or a cursor cell that feeds the
walk its own writes, can make the original loop forever.

## Validation and parity review

The focused C11 test runs under CTest with ASan/UBSan, and compiles with
`-Wall -Wextra -Wpedantic -Wconversion -Werror`. It passes 1,838 checks:

- an empty list, a third-of-four hit and a miss, with exact AX, DX, ES:DI,
  flags, frame bytes and return;
- a cap one loop-back short, stopping after the match;
- a two-node cycle stopped after 1,000 loop-backs;
- the cursor cell on `SS:BP-2`, with AX `3201h` and BP restored to the
  head segment;
- the argument guards;
- **200 random worlds against the credited callback model**
  `pr_rec_res_00ba_2199`, with the same AL, the same cursor cell, and RAM
  identical outside the raw model's own four frame bytes.

The Unicorn original-byte differential places the whole expanded image at
load segment `0800h`. It compares the entire 1 MiB RAM and all 14 CPU
words with no flag mask. It passes **331 returning cases**:

- 240 random worlds of zero to eight nodes, with hits, misses, and cycles
  that contain the key;
- every hit position and a miss for lists of zero to six nodes, with the
  key's high byte varied;
- 49 cursor-aliasing worlds: on the head, the frame, the argument cell and
  a node's link;
- 20 stacks at both segment ends, including the frame alias;
- six nodes whose `+5` link word straddles EA `FFFF`, so `+7` wraps to
  `0001`;
- six A20 worlds and SS across A20;
- a random background and preserved DF/IF.

For 281 of them the test also checks AL and the cursor cell against the
documented lookup. It also passes **26 host-cap stops**, each equal to the
original stopped at the same loop-head arrival. These are five endless
cycles, all twelve argument-cell worlds and nine node-link worlds. All 37
instruction starts and all 6 conditional outcomes are reached in returning
runs.

The DOSBox-X normal-core/386 synthetic oracle passes **36 cases**, 22 of
them hits. Each case compares all 14 CPU words, 32 stack bytes and 1,072
world bytes at absolute segments, with no mask. The worlds cover:

- an empty list;
- hits at every position of a five-node list, with key high bytes varied;
- misses;
- a two-node cycle that contains the key;
- the cursor cell on the character's head;
- the cursor cell on `SS:BP-2`;
- a node at `6000:FFFA` whose `+5` link straddles physical `6FFFF/70000`.

That last walk ends because DOSBox-X reads the word contiguously. A CPU
wrapping inside the segment would take the high byte `5Ah` from
`6000:0000` and walk on. The COM holds the routine's original 111 bytes at
offset `2199h` of its own segment, with IRQs masked. The result SHA-256 is
`2aff6e83d607e4d161baa6820569c48a1831c2ce00a130f11e3991368c6c5e2f` with
COM segment `0814`. Original bytes and generated images stay under
ignored `resources/`.

```sh
cmake -S . -B resources/build-res2199 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all'
cmake --build resources/build-res2199 --target poolrad_resident_00ba_sidecar_lookup_2199_cpu_tests
ctest --test-dir resources/build-res2199 -R poolrad_resident_00ba_sidecar_lookup_2199_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1800s \
  python3 tools/overlay/resident_2199_cpu_audit.py --unicorn --native \
  --output resources/res2199-cpu-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Resident `RES_00BA_2199`, expanded `0x2D39..0x2DA7`, a leaf |
| `source_candidates` | `src/recovered/resident_00ba_sidecar_lookup_2199_cpu.c`; the callback model `resident_sidecar_lookup.c` is unchanged |
| `signals` | Exact hash/CFG, whole-RAM/CPU differential with full instruction and branch-outcome coverage, matched host-cap stops, callback-model cross-check, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Already strict-credited through the callback model, so no new count. Frame- and argument-aliasing worlds can loop forever (host `LIMIT`, compared at the stop). Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed. The reviewer lane owns
how this model is recorded, for example as raw-CPU readiness of
`RES_00BA_2199` for overlay composition.
