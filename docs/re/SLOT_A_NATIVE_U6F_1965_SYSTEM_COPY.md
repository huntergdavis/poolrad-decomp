# Saved slot-A `+1965` SYSTEM copy: first writer and return

Two fresh native DOS v1.3 saved-slot-A replays stopped at consecutive
`2A8E:1965` calls. The first call changes a local Pascal string length
and one payload byte. The second call is the reached continuation from
the merged [`+1937` pre-call setup](SLOT_A_NATIVE_U6F_1937_PRECALL.md):
it executes the same two stores, but those bytes already equal the source,
so the helper makes no net RAM value change. Both return to `2A8E:196A`.
This is a native call-boundary result, not a composed C one-Up replay or
a new game-specific routine count. The callee is a SYSTEM runtime helper.

Before tracing, `deja "poolrad slot A unit 6F +1965 far call
131C:0B0B native replay callee return writer"` returned no match. The
shorter `deja 1965` recalled the prior `+1965` copy boundary and the
merged `+1937` handoff. I reused those leads and the independent
[string-chain audit](U6F_PARTY_STRING_CHAIN.md), then checked the original
and loaded bytes, both live caller frames, callee stops, and return RAM
in fresh private runs. The later [post-loop SYSTEM call](SLOT_A_NATIVE_POSTLOOP_SECOND_CALL.md)
was a semantic cross-check, not a substitute for these stops.

## Pinned inputs and instruction boundary

Both ignored runs use the same source tree: `START.EXE` 65,231 bytes,
SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` 206,105 bytes,
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
slot-A `SAVGAMA.DAT` 13,137 bytes,
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
and the private heavy DOSBox-X executable 47,259,176 bytes,
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The private runs are `resources/oracle-runs/slot-a-native-u6f-1965-first-047`
and `resources/oracle-runs/slot-a-native-u6f-1965-046`.

| Span | Size / decoded instructions | SHA-256 |
| --- | ---: | --- |
| Original caller `GAME.OVR[0x1F94B..0x1F94F]`, unit `0x6F:+1965..+1969` | 5 / 1 | `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae` |
| Loaded caller `2A8E:1965..1969` | 5 / 1 | `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b` |
| Original SYSTEM helper `START.EXE[0x0B781..0x0B79A]` | 26 / 12 | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| Loaded helper `131C:0B0B..0B24` | 26 / 12 | same as original |

The caller's far target is relocated from original `0AF8:0B0B` to live
`131C:0B0B`; only its segment operand changes. The helper executes
`CLD`, saves the caller's DS, loads the destination and source far
pointers from the stack, reads the Pascal length, stores that length at
`+0B19`, copies the payload with `REP MOVSB` at `+0B1E`, restores DS,
and ends with `RETF 4` at `+0B22`. No conditional branch or other call
occurs within these 12 instructions. The original and loaded 26-byte
bodies are byte-identical in both live runs.

At either caller stop, SS=`1D16`, SP=`37A4`, and the top four words are
`15E7, 2A8E, 3BB6, 1D16`: source `2A8E:15E7` followed by destination
`1D16:3BB6`. The far call pushes return `2A8E:196A` at SS:`37A0` and
`37A2`. `RETF 4` returns with SP=`37A8`, consuming the source pointer
and retaining the destination pointer at the stack top. Both returns
restore DS=`14A0`; AX=`0001`, BX=`37A0`, CX=`0000`, DX=`14A0`,
SI=`15E9`, DI=`3BB8`, and FLAGS=`7246` are the measured output registers.

## Live writes

The first `+1965` hit has AX=`0029`; it precedes the merged `+1937`
continuation. The second hit has AX=`0052` and matches that continuation's
complete pre-call CPU tuple. In both calls, source length is 1 and the
first callee store is `STOSB` at `131C:0B19`, with AL=`1` and
ES:DI=`1D16:3BB6` (physical `0x20D16`). The payload store is the
one-iteration `REP MOVSB` at `131C:0B1E`, targeting physical `0x20D17`.

| Call | Before `+0B19` | After `+0B19` | After `+0B1E` | At `2A8E:196A` |
| --- | --- | --- | --- | --- |
| First | destination length 97 | length 1; exactly `0x20D16` changed | exactly `0x20D17` changed | source/destination two-byte Pascal objects equal; six net RAM bytes changed from caller stop |
| Second, reached from `+1937` | destination length 1 and two-byte object already equals source | same value stored; zero changed bytes | same value copied; zero changed bytes | only four far-call return-frame bytes differ from caller stop |

The first call's six net changed bytes are `0x20900..0x20903` from the
far call and `0x20D16..0x20D17` from the helper. On the second call,
only `0x20900..0x20903` change. The debugger's pre/post stops around
each store and full 655,360-byte RAM comparisons establish the first
**value-changing** writer at `131C:0B19` for the first call. Both store
instructions still execute on the second call; no value-changing callee
writer exists there. Payload values and private text are omitted.

| Run / stop | 655,360-byte RAM SHA-256 |
| --- | --- |
| First `+1965` | `d019b40c52b233a773b23ef255e7927ed10f4f4da41c8e94f30cb4755a6d0121` |
| First `+0B19` | `76f78b872811b48514083e031d24b87b791ee4afa9a0967c3a42dbcf20e5c9d6` |
| First `+0B1A` | `9b856250978bac20226acdaeca3bf3ae5eee0efe444397bb415f13e6f9053baf` |
| First `+0B20` and `+196A` | `67d1ff270888916e4f05ca07946d16e2c335775a5d5709fe44eff1a44da1c2a8` |
| Second `+1965` | `a2f88f2c6b6c326fd4bd556a1f549ec87c3886276ced73e2a1ca30077ba883b2` |
| Second `+0B0B`, `+0B19`, `+0B1A`, `+0B1E`, `+0B20`, `+196A` | `e878e2d519ce6a722aa6ed2236262bbc02ac29005dbec04816af2e6a664ad77d` |

The ignored five-row first-call EV transcript hashes
`4b24e13e3470057d2a93f4539b6570e35b09217ed11b9aada9f1abdb1e8f6269`;
the seven-row second-call transcript hashes
`df2651e012b3363b905ee2afc7601d7a1b615d0dceefa20becef804c110ee472`.
Each row has all 16 guest CPU fields. The source-only checker validates
these hashes, original/loaded code and input hashes, pointer frame,
full-RAM hashes, exact changed-address sets, and return state:

```sh
python3 tests/recovered/check_slot_a_u6f_1965_native.py \
  resources/oracle-runs/slot-a-native-u6f-1965-first-047 \
  resources/oracle-runs/slot-a-native-u6f-1965-046
```

## Reproduction boundary

Create two fresh ignored runs with `tools/oracle/prepare_run.sh`, and
launch each with the hash-pinned heavy DOSBox-X on `PATH` through
`tools/oracle/launch.sh` under Xvfb. At the visible load-slot prompt,
pause with Alt+Pause, arm `BP 2A8E:1965`, resume, and select slot `a`.
The first hit is the control call; the second hit is the `+1937`
continuation. At the chosen caller hit, arm `BP 131C:0B19`,
`BP 131C:0B1A`, `BP 131C:0B20`, and `BP 2A8E:196A`;
the second run also used `BP 131C:0B0B` and `BP 131C:0B1E`.
At each stop, record
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`, moving each private `MEMDUMP.BIN`
to its stop-labeled `ram-*.bin` before continuing. The checker above
uses those ignored snapshots and the EV transcripts.

No save, executable, debugger capture, RAM image, screenshot, or game
text is tracked in this PR. This call-boundary result does not execute
the subsequent resident `0x0356` call at `+196A` or establish a full
C-native saved one-Up path.
