# Full C translation: resident sidecar-list append


`pr_rec_res_00ba_15f7` translates the complete valid-list path of DOS v1.3
`START.EXE` resident routine `RES_00BA_15F7`. Its loaded entry is
`0x00BA:0x15F7`, expanded-image `0x2197`, and first packed source byte
`0x2273`. The [hash-locked loader map](RESIDENT_LOADER_MAP.md) establishes
copied-source provenance at the entry and at its `RETF 8` endpoint `0x2230`.
The older [resident inventory](RESIDENT_FUNCTION_INVENTORY.md) uses a
different packed-image coordinate (`0x2073`); this note uses expanded-image
addresses for instruction paths.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Selection and game role

The [callable ledger](resident_callable_ledger.csv) lists `RES_00BA_15F7`
as an exact Pascal prologue with two fixup-confirmed overlay callers. They
are unit `0x38:+0x08D3` (GAME.OVR file `0x0F52E`) and unit
`0x77:+0x17B3` (file `0x227ED`). The former constructs a 63-byte record,
passes it to `0x15F7`, then frees the temporary at `+0x08E2`. The latter
calls `0x15F7` and then calls the already translated `RES_00BA_1509` at
`+0x17C6`. The [character load trace](../formats/DOS_CHARACTER.md)
independently establishes 63-byte `.ITM` sidecar units and an in-memory far
pointer at character `+0xC8`. Merged [PR #37](RESIDENT_SIDECAR_REMOVE.md)
establishes the same head and node `+0x2A` link in the inverse removal
routine. Together, the game-specific character fields, caller sequence, and
inverse routine support classifying this append routine as game code rather
than a generic heap/list library function.

Two shorter alternatives remained weaker: `RES_00BA_148B` has 24 reachable
instructions and clears a byte in a 21-position indexed table through a
global far pointer at `0x5D92`, but that table's role is unverified.
`RES_00BA_23B7` has 13 reachable instructions and tests character byte
`+0x10E`, whose meaning is also unverified. Neither is promoted to a full
game translation by proximity or small size alone.

## Instruction paths and external effects

A Capstone 5.0.7 path walk over the hash-checked expanded image finds
**53 reachable instruction starts**, four branch edges (`21E5→21FC`,
`21FA→222D`, `2207→221C`, `221A→21FC`), two far calls (`21A1`, `21BA`),
and one `RETF 8` at `2230`. The first far argument at `[BP+6,+8]` is the
source sidecar; the second at `[BP+0xA,+0xC]` is the character-record base.
The body has no direct near call, indirect call, or unseen exit on the
decoded path. The host function's status is an adapter outcome, not a DOS
return register.

| Loaded instruction span | Observed condition and effect |
| --- | --- |
| `0x219D..0x21BF` | Push size `0x003F` to SYSTEM `0xAF8:0x023F`; use returned DX:AX far pointer. Push source, new pointer, and size `0x003F` to SYSTEM `0xAF8:0x1685` in that order. |
| `0x21BF..0x21CC` | Write zero to both words of new node's `+0x2A/+0x2C` next pointer, replacing the copied link. |
| `0x21CC..0x21FA` | Read character head at `+0xC8/+0xCA`. If both words are zero, store the new pointer there and return. |
| `0x21FC..0x221A` | Otherwise read both words of `current->next`. If non-null, read both again to advance, then repeat. There is no DOS cycle limit. |
| `0x221C..0x2230` | Store the new pointer to the tail's next field, then return. |

At loaded image `0xB1BF`, SYSTEM `0xAF8:0x023F` reads its word size
argument, calls the heap core at `0xB288`, and branches to the error
path at `0xB1CE` on carry. That path jumps to the runtime error
dispatcher at `0xB062`; its later heap and termination effects are outside
this translation. The C allocation callback represents that call; false or
an all-zero pointer stops with a host
`ALLOCATE_ABORT` label. At `0xC605..0xC625`, SYSTEM `0xAF8:0x1685`
loads source from `[SP+0xA]`, destination from `[SP+6]`, and count from
`[SP+4]`, copies the bytes, and returns. The C copy callback represents
that dependency. A callback failure is a host `COPY_FAILURE` outcome;
the DOS body contains no copy-failure branch.

Far-memory reads and writes remain individual 16-bit operations, including
offset wraparound and the second link read on each nonterminal iteration.
The host's `max_nodes` guard stops corrupt cyclic lists after the prior
allocation, copy, and next-link clear, preserving those preceding effects.
Valid acyclic lists with successful callbacks take every decoded DOS path.
The C implementation does not translate the SYSTEM allocator or Move body,
nor does it claim parity for invalid pointers or failed host memory access.

## Verification and prior work

Synthetic tests check the full 63-byte copy except the four cleared link
bytes, source preservation, empty-head insertion with 16-bit offset wrap,
one- and two-node tail insertion, a one-word-zero non-null pointer, repeated
link reads, cycle guard, and allocation/copy/read/write failures. Expected
callback traces are fixed separately from the C control flow. Build and run:

```sh
cmake -S . -B /tmp/poolrad-issue-38-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-38-build
ctest --test-dir /tmp/poolrad-issue-38-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --near --json /tmp/poolrad-issue-38-loader.json
```

The strict local build passed all eight CTest targets. The address audit
reads the ignored owner binaries and emits only addresses, counts, hashes,
and evidence labels; no original media or decoded text enters Git.

Before implementation, `deja "resident character party segment 0xBA small
game routine"` and `deja "resident callable ledger game specific target
candidate"` recalled broad prior classification work. `deja
"RES_00BA_15F7 sidecar allocate append"` found no exact prior analysis.
The merged [callable coverage](RESIDENT_CALLABLE_COVERAGE.md),
[loader map](RESIDENT_LOADER_MAP.md), [character trace](../formats/DOS_CHARACTER.md),
and [PR #37's removal note](RESIDENT_SIDECAR_REMOVE.md) supplied the
classification, mapping, field, and inverse-operation evidence reused here.
No Claude ECL helper file was edited.

## Reproduce (original-byte checker)

`RES_00BA_15F7` is checked against the original bytes by the registered checker `tools/overlay/res00ba_rec_models_audit.py`,
through the reusable io adapter `tools/overlay/res00ba_io_adapter.py`. What it compares is described in
[RES_00BA_REC_MODELS_AUDIT.md](RES_00BA_REC_MODELS_AUDIT.md).

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay \
  python3 tools/overlay/res00ba_rec_models_audit.py --output resources/res00ba-rec-audit/report.json
```
