# Saved slot-A resident dispatch: reached call, unsampled branch

The saved DOS v1.3 one-Up coordinate watch proves that the call from unit
`0x68:+0742` to `RES_08AE:00DC` returned. It does **not** measure the
dispatch's input state or select one of its local callees. This note audits
the immediate gate and its direct resident state writers so a native replay
can sample the smallest remaining boundary. No C translation or function
identity/count follows from this audit.

## Prior research and inputs

Before this audit, direct deja recall for `Poolrad slot A one Up resident 8AE
00DC 5372 5374 2628 live dispatch branch` found no matching session. Shorter
recall `slot A one Up` surfaced the earlier [native coordinate
watch](SLOT_A_ONE_UP_NATIVE_COORDINATE_WATCH.md) and Up preflight work;
`8AE:00DC` surfaced the previous [dispatch audit](RESIDENT_8AE_00DC.md).
This work reuses their address leads, the [coordinate-writer
decode](U68_COORDINATE_WRITER_073B.md), and the [adjacent-leaf
audit](RESIDENT_8AE_LEAVES.md). The START mapping and gate were checked
again against the ignored originals.

| Ignored input | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

The existing loader map pins `RES_08AE:00DC` to loaded image
`0x08BBC..0x08C5A`, START file `0x08BD3..0x08C71`, 159 bytes, SHA-256
`aae1673000ebd85e67c66df978d2b986de8e4d52cdd7994ade46f1ac5d50bb2a`.
The original unit `0x68:+073B..+07F3` has SHA-256
`342da33f91df7b31ba2aa2819f3e2fc6508f9d0b6634f7f22df616788d675fc5`.

## What the one-Up stop establishes

The native watch stopped at unit `0x68:+0752` during the saved one-Up.
The hash-locked coordinate-writer prefix pushes DS word `[0x2628]` at
`+073E`, calls `RES_08AE:00DC` at `+0742`, and then reaches the x store at
`+0752` without an intervening branch. Therefore that particular dispatch
call returned. The watch did not stop inside the dispatch, so neither the
pushed WORD nor its DS state was captured.

At resident `+00DF` (loaded `0x08BBF`), `cmp byte [0x5372],2` selects an
immediate return through `+0178` when equal. Otherwise `+00E9` (loaded
`0x08BC9`) reads word `[0x5374]`; a positive value calls `+137C` before
the argument branch. The WORD argument at `[BP+6]` then selects the paths
in the [dispatch audit](RESIDENT_8AE_00DC.md). Later reads at `+0137`
and `+0156` may see callee effects. The observed return is compatible with
multiple paths, including the immediate mode-2 return. It does not prove a
call to `+17AE`, `+1742`, `+181A`, or any other leaf.

## Direct resident state writers

`tools/overlay/resident_dispatch_state_audit.py` reuses the hash-checked
START unpacker and the candidate-seeded resident near-call path walk. It
decoded 12,834 instruction starts from 134 seeds, then selected explicit
absolute-memory destinations in write instructions. It reports loaded
addresses and source file offsets only. The three spans below have
contiguous copied-byte provenance; the first procedure ends at its
`RETF` at `0x089C2`. The intervening `0x089C3..0x089E8` bytes are outside
that procedure and are excluded from the code hash.

| Procedure code | Loaded span | START file span | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0899:0000` | `0x08990..0x089C2` | `0x089C7..0x089F9` | 51 | `f4d6f25f38c01ad8730fc424ad38214c037804afe497ff125794d8e008465d7d` |
| `RES_0899:0059` | `0x089E9..0x08A93` | `0x08A08..0x08AB2` | 171 | `4a361032123f0bdc7fa2ff158dc2d8f690716ee716462f66bda9740f32e3362f` |
| `RES_08AE:01A5` | `0x08C85..0x08CB1` | `0x08C9C..0x08CC8` | 45 | `7307263a3cf75d350b4ac4c02226dc74790e437194fb31922620ef24074e0c2f` |

| DS destination | Loaded direct store sites | Bounded interpretation |
| --- | --- | --- |
| byte `[0x5372]` | `0x0457B`, `0x04586`, `0x0458D`, `0x08996`, `0x08A14`, `0x08A2E` | The first three store 0, 1, or 2 under local comparisons. `RES_0899:0000` copies `[0x5373]` at `0x08996`, then calls `RES_08AE:00DC` at `0x0899D` with `[0x2612]`; this is a different caller/argument from the saved coordinate writer. `RES_0899:0059` stores 2 at `0x08A14` after its own dispatch call; `0x08A2E` restores `[0x5373]`. |
| byte `[0x5373]` | `0x08A08`, `0x08C61` | Both are direct backup-byte writes. |
| word `[0x5374]` | `0x08CAB` | `RES_08AE:01A5` stores its WORD argument after its mode/active checks. |
| words `[0x2612]`, `[0x2628]` | none in this scan | Their live values and any indirect or unseeded writers remain unknown. |

This is a bounded static write-site inventory. Candidate-seeded decoding
does not cover every executable byte, and indirect stores or aliasing can
change any watched address. The setters' execution order during slot A is
unmeasured; their presence does not establish the live mode.

## Reproduction and next measurement

With the ignored DOS inputs and Capstone 5 available locally:

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 tools/overlay/resident_dispatch_state_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR
```

The next native sample is at the reached `RES_08AE:00DC` entry before
`+00DF`: record DS, byte `[0x5372]`, word `[0x5374]`, and `[BP+6]` after
the prologue, then record the branch/return or first local call. If a
callee runs, sample its later mode/active rereads before attributing a
leaf effect. The [prior native watch](SLOT_A_ONE_UP_NATIVE_COORDINATE_WATCH.md)
gives the saved one-Up setup; this audit has not repeated that replay.
