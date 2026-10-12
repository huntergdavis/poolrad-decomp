# Shared resident block reader on LOAD MONSTER caller lanes

Two static LOAD MONSTER caller lanes lead to `RES_0802:0716`. The complete
reader body already has a [path-complete bounded C
model](RESIDENT_ECL_DAX_BLOCK.md) from issue #74. This audit pins the
`RES_0464:0111` caller contract and the first file-record operation hidden
by that model's `open_archive` callback. It does not add C, establish a live
saved-game LOAD MONSTER invocation, or claim full-C or game-identity credit.

## Reused work and original inputs

Before decoding, deja recall for `Poolrad RES_0802_0716 DAX block loader LOAD
MONSTER 0x464 0x0111 0x6F entry 6` found no exact session; `0802:0716`
recalled the earlier overlay call trace. The merged [reader model and
audit](RESIDENT_ECL_DAX_BLOCK.md), [file-open helper](RESIDENT_DAX_FILE_OPEN.md),
[nested open helper](RESIDENT_DAX_NESTED_OPEN.md), [unit 0x6F entry-6
model](U6F_ENTRY6_0EC4.md), its [static map](U6F_ENTRY6_0EC4_MAP.md),
and [unit 0x3CE entry-4
map](U3CE_ENTRY4_01C8.md) supplied leads. The spans, direct calls,
reader ID accesses, and shared constant equality below were rechecked
against the hash-matched ignored originals.

| Ignored input | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

The hash-locked START unpacker gives contiguous copied-byte provenance for
each code span. Endpoints in this table are inclusive.

| Procedure | Loaded image | START file | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0464:0111` | `0x04751..0x048E4` | `0x0478A..0x0491D` | 404 | `ac5dab193845bac0fc67613a82dc60b7148a930e9d1d9f319129e32a7f24271a` |
| `RES_0802:025D` | `0x0827D..0x0830A` | `0x082B4..0x08341` | 142 | `eeea6613012d3db99c7ad85788fdd3ff5b9888d7cc6701e9ce7a612195210dc6` |
| `RES_0802:02EB` | `0x0830B..0x08375` | `0x08342..0x083AC` | 107 | `141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474` |
| `RES_0802:05CD` | `0x085ED..0x08706` | `0x08624..0x0873D` | 282 | `d8067fb72bfede3fb56aa146e941d7bc9524cb23715f8f63e60f4918c65449b5` |
| `RES_0802:0716` | `0x08736..0x08969` | `0x0876D..0x089A0` | 564 | `d14db7945a8e717bc536422426c5df4d35913bdd4c5f4fa10f8a51f5103b34d2` |

## Caller values and outputs

The merged unit `0x3CE` entry-4 model's saved-input probe selects its
**path C** with host callbacks; it is not a native replay. That path has
two calls to `RES_0464:0111`, at unit offsets `+045B` and `+04C4`. The
first passes its p3 byte unchanged, and the second adds `0x80` to the
zero-extended p3 before pushing a WORD. For the probed p3 of zero these
are low-byte block IDs `0x00` and `0x80`. The wrapper copies the caller's
Pascal name with cap 255 at loaded `0x04767`, conditionally releases a
non-nil destination through `RES_0464:2595` at `0x0477E`, then calls
`RES_0802:0716` at `0x047A7` with:

| Reader argument | Wrapper source |
| --- | --- |
| far name `[BP+0x14]` | wrapper's capped local `[BP-0x100]` |
| far extension `[BP+0x10]` | `CS:+010C` Pascal constant |
| block-ID WORD `[BP+0x0E]` | low byte of wrapper `[BP+0x0E]` |
| far output size `[BP+0x0A]` | wrapper local `[BP-0x113]` |
| far output pointer `[BP+0x06]` | wrapper local `[BP-0x119]` |

The wrapper tests the returned size at `0x047AC`; zero goes to its
`RETF 0x0E` without its later bitmap assembly. On nonzero size it copies
selected block fields, calls resident `0x0464:248E` at `0x0485F`,
uses another local helper at `0x048C6`, frees the block
at `0x048D5`, then returns. The prior destination release may already
have occurred on either outcome. These wrapper heap and object effects
are **outside** `pr_rec_res_0802_0716`.

The other lane, unit `0x6F` entry 6, calls the same resident reader
directly at `+0F27`, `+10C2`, and `+11AA`. Each call pushes only the
low byte of its incoming ID, the local size word `[BP-2]`, and the local
far block pointer `[BP-6]`. Across all 242 decoded reader instructions,
the block-ID argument `[BP+0x0E]` is accessed only by two **byte**
comparisons, loaded `0x08875` and `0x088A7`; its high byte does not
select a block. The first zero size enters the unit 0x6F error path;
the second and third zero sizes skip their optional chains.

The wrapper's `CS:+010C` extension and unit `0x6F`'s `CS:+0EA0`
extension are byte-for-byte equal length-four Pascal constants. Their
five-byte SHA-256 is
`a374cb557933d9962ea3cc729cdda2459992f5b370d3bd9be9c42407eef0951e`.
No string content is published. The unit 0x6F name assembly also reads
runtime DS byte `[0x5376]`; the saved-input probe did not measure it.

## First file-record operation and C boundary

On either call lane, the first nested file-operation chain in the
resident reader is unconditional until the selector returns:

| Caller site | Callee | Role |
| --- | --- | --- |
| reader loaded `0x08795` | `RES_0802:05CD` (`0x085ED`) | configured file-open helper |
| helper loaded `0x08679` | `RES_0802:025D` (`0x0827D`) | nested open/retry helper |
| nested loaded `0x082AF` | `RES_0802:02EB` (`0x0830B`) | candidate selector |
| selector loaded `0x08330` | runtime `0xAF8:13E2` | first file-record call, Assign-shaped |
| selector loaded `0x0833F` | runtime `0xAF8:1410` | Reset-shaped call that enters DOS open |

The reviewed helper notes give the file-operation roles and later
IOResult/retry paths. The existing reader's `open_archive` callback
instead supplies a borrowed archive view. It does not reconstruct the
configured path from `DS:0x547C`, `[0x5376]`, `[0x547D]`, and `[0x557C]`,
or reproduce the two runtime file records, retry/input behavior, and
exceptional DOS I/O effects. Its successful archive decode was compared
with 29/29 private ECL DAX blocks in issue #74; that comparison does
not verify a LOAD MONSTER filename, ID, heap allocation, or DOS replay.

On valid calls, the C reader reports host statuses and zero/null outputs
for non-OK results.
The DOS callers react to output size and pointer, with different error
paths in the two lanes. The merged unit-0x6F model exposes a reader hook
whose signature matches the C resident reader through a Pascal-string
and object-table adapter; that adapter is not yet implemented. A direct
C hookup needs status, ownership, far-memory, and wrapper-effect
contracts resolved before
claiming parity. No new C routine is added here because the reader body
already has a bounded model and the remaining boundary is runtime and
caller specific.

## Reproduction

With Capstone 5 and the ignored DOS inputs locally:

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/load_monster_resident_block_route.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR
```

The script asserts input hashes, all five span hashes and source runs,
both path-C calls, all three unit-0x6F calls, the nested I/O call chain,
the two byte-only ID reads, and constant equality. It emits only
addresses, counts, and hashes. No media or decoded content enters Git.
