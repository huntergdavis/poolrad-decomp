# Saved slot-A Up: controlled native AL10 script branch

This is an address/hash/numeric-only native DOS observation. A fresh saved-slot-A
run was stopped before the reached ECL RANDOM call. Its four-byte RNG seed was
set to the value independently measured on the earlier genuine single-Up run;
execution then continued in the native guest. This **controlled seed-restored
continuation is not an unmodified replay**. It establishes the subsequently
reached script branch for that seed. It adds no C, routine count, or C-native
parity claim.

`deja "poolrad slot A DOSBox-X AUTOTYPE startup playable view debugger"`
searched the local index before this replay and returned no match. Reused
checked-in evidence: [native RANDOM seed/store](SLOT_A_ONE_UP_NATIVE_RANDOM_SEED.md),
[party input](SLOT_A_PARTY_INPUT.md), [class-1 ECL load hook](ECL_LOAD_HOOK_6DCA.md),
[script runner](U1C_ECL_RUN_3171.md), [gameplay loop](U1C_LOOP_3387.md), and
[RANDOM handler](U1C_RANDOM_0234.md). The earlier genuine run measured seed
3140872353 at `3582:026C`, returned `AL=10`, and stored 10 at window 2 `+021E`.

## Pinned private inputs and startup gate

| Ignored input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

Six stale emulator processes in this isolated checkout were stopped before the
fresh run. This may have affected startup timing; causality was not established.
The private DOSBox-X `AUTOTYPE -w` delay is capped at 30 seconds, and `-p 8`
waits eight seconds per key; earlier blind key sequences sometimes missed the
menu or load prompt. The fresh run used **visible screen gates**: launch without
AUTOTYPE, wait for title, send Enter, wait for main menu, send `l`, wait for
`LOAD WHICH GAME`, send `a`, then verify the playable `(15,4,N)` view at 10:50
before arming a breakpoint or sending Up. The RGB HUD crop at pixel rectangle
`(580,540,780,575)` on the 1280×1024 private capture had SHA-256
`bf1e6e8f2804c8317f61029670ef899f5a2c79e77df306d464b8b2551c4fe1a2`
over `PIL.Image.crop(...).convert("RGB").tobytes()`. The same crop hash appears
in the earlier successful run. Private images and saves remain ignored.

## One Up and exact intervention

At the verified playable screen, the debugger armed `BP 3582:026C`, resumed,
and sent **exactly one Up**. At the pre-call stop: `CS:IP=3582:026C`,
`DS=14A0`, `SS:SP=1D16:3EDC`, `BP=3EE2`, `AX=14` (the RNG range), script PC
`DS:[49ED]=0x9B40`, and party bytes `DS:6AAD..6AAF=(14,4,6)`. Window 0 far
pointer `DS:[49D2]=3996:0000` had time words `+018E/+0190/+0192=(1,5,10)`;
window 1 `DS:[49D6]=3A16:0000` had `+0594=0`; window 2
`DS:[49DA]=3A96:0000` had `+021E=16`.

The fresh run's `DS:4428` seed was 32944464. The debugger command
`SM 14A0:4428 D:BB35E8A1` restored **only** the earlier genuine run's
four-byte seed, 3140872353, before the RNG call. Comparison of both complete
640 KiB RAM dumps showed changes only at physical `0x18E28..0x18E2B`
(`DS:4428..442B`). No result register, party byte, script state, or window word
was set by the debugger. Before/after RAM SHA-256 values were respectively
`ff29bedcec345c2953433dc13300da097a43940e2ed6ce54eb54c1b5276c1284`
and `3ee6a79b9a53a7401d2679fd803635ef2df144a0d3f17f5c9b29cbd376fd108c`.

After native execution reached `3582:028E`, `DS:4428=3238563622` and window 2
`+021E=10`; RAM SHA-256 was
`d33e3eaf1968a4f656f6df1304a4348eae620bbf0a2655bbec39178efb95a62b`.
The store confirms the native result path. `AX` at this later stop is not the
RNG return register; the earlier genuine run measured `AL=10` at the immediate
return and conversion stops.

## Reached script sequence

The following stops are all at dispatcher `3582:2F2D`. The PC is `DS:[49ED]`;
the previous/current opcode bytes are `DS:[6F7E]/[6F7F]`. RAM hashes cover the
complete 640 KiB guest memory, stored only in ignored `resources/`.

| Stop | PC | Previous/current | RAM SHA-256 |
| ---: | --- | --- | --- |
| 0 | `9B40` | `08/03` | `26ea01908c6c4b9e512687ff5d9a2dfc3876d9834c27480ae7c1805b9c7ab5e8` |
| 1 | `9B46` | `03/16` | `9c3a223bd89941cabe4bc2410f29dc21cfced283a13a3eb8a0ee6c2cb21a9bb7` |
| 2 | `9B50` | `04/03` | `9e7cb4f44ee120a836bbd2c5d1ce6c5c4f4c7f4ed0f7c2378b744e87d5ea7672` |
| 3 | `9B56` | `03/16` | `c5cf5066193216df0cd480dad0de485aed18bbff448b8545c10fd40e0503d926` |
| 4 | `9B60` | `04/03` | `b931382d4e68b25233ec64d23412f12e8b3ca49f09e74697707f7a98d529adff` |
| 5 | `9B66` | `03/1A` | `a208851898a27a6d1034d5c795166e6c10c1a557fddcbddf49f84872161738c7` |
| 6 | `9B67` | `1A/00` | `bae33746a473a4b2c28aa40097095f67615cf1552641b0a6002bf02bbe4500a8` |

At every dispatcher stop the six compare-flag bytes `DS:6F78..6F7D` were
`(0,1,1,0,1,0)`, party bytes were `(14,4,6)`, window 0 time words were
`(1,5,10)`, window 1 `+0594` was 0, and window 2 `+021E` was 10. The observed
successor PCs show the `0x16` IF at `9B46` taking the route to `9B50`, the
`0x16` IF at `9B56` taking the route to `9B60`, and the `0x1A` IF at `9B66`
proceeding to `9B67` EXIT. Throughout these stops, `DS:[442E]=0`,
`DS:[442F]=0`, `DS:[49FF]=0`, `DS:[5E24]=1`, and `DS:[5E25]=17`.

EXIT entry `3582:0001` was reached at PC `9B67` (RAM SHA-256
`7d9303e803eaf54ecf8d30db5aa339a2107546543c1678debf1eee5189196091`).
The runner reached `3582:31AA` with PC `9B68` and stop flag `[442E]=1`
(`12a715b7ee9ca93844ed2edd0127a10353e4df38f0f389d245d985f32264557a`),
then `3582:31B0` with the flag cleared (`0`) and `SS:SP=1D16:3EF0`
(`d43b75633114880a24480a73a64bc623bbadb736729d39d78d2612e94f2c2d12`).
The latter stack held return `CS:IP=3582:34EA` and argument `0x9975`,
consistent with the checked-in near call at `+34E7`. A breakpoint had been
armed at `+34EC`, so **direct execution at `+34EA` was not captured**.

After native continuation without another game key, a later pause at
`2116:056F` retained party `(14,4,6)`, time `(1,5,10)`, window 1 `+0594=0`,
window 2 `+021E=10`, script PC `9B68`, and stop flag 0. Its RAM SHA-256 was
`4dec8b32869e6688d5363245b2a5a0ee1aa4276cf902638ab01bf7208a9dffc7`.
This is a settled snapshot, not an immediate caller stop.

## Private reproduction

From the pinned ignored inputs, in an isolated checkout:

```sh
tools/oracle/prepare_run.sh slot-a-ready-live-010
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a tools/oracle/launch.sh resources/oracle-runs/slot-a-ready-live-010 \
  -time-limit 1200
```

Gate each key on the visible title, main menu, load prompt, and playable view
as above. Arm `BP 3582:026C`, resume, send one Up, and privately dump guest RAM
before and after the single `SM` command above. Arm `BP 3582:028E`,
`BP 3582:2F2D`, `BP 3582:0001`, `BP 3582:31AA`, and `BP 3582:31B0`; continue
through the stops and record `EV AX BX CX DX SI DI CS IP DS SS SP BP` plus
private `MEMDUMPBIN 0000 0000 A0000` samples. To directly observe the
stack-indicated caller target in a repeat run, also arm `BP 3582:34EA`.
Different unmodified seeds can select another branch: a separate fresh run
reached `+028E` with seed 1683588514 and window 2 `+021E=13`; its later script
route was not captured here.
