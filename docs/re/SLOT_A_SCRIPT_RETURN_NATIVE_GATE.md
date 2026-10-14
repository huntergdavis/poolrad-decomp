# Saved slot-A Up: direct search-script return and zero-gate loop

The same unmodified saved-slot-A one-Up run used for the
[post-EXIT caller trace](SLOT_A_POST_EXIT_NATIVE_RETURN.md) continued from
`3582:34E7`. The search-script call entered `3582:3171`, passed an in-run
dispatcher stop at `3582:026C`, returned directly to `3582:34EA`, then
reached caller `+34F5` and the outer-loop target `+3395`. These are separate
native stops in one run, not a projection from later state.

Before tracing this dependency, `deja "Poolrad slot A +34E7 +3171 9975
script runner return gates"` failed because its local index lock was on a
read-only filesystem. Reused and rechecked the
[runner ABI and writers](U1C_ECL_RUN_3171.md),
[gameplay-loop map](U1C_LOOP_3387.md),
[first EXIT boundary](SLOT_A_FIRST_EXIT_9974.md), and
[earlier native AL10 route](SLOT_A_ONE_UP_NATIVE_AL10_BRANCH.md). That earlier
native route stopped at runner `+31B0`, but did not capture `+34EA` directly.

## Input and code identity

All inputs and 655,360-byte RAM dumps stay under ignored `resources/`.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMJ.DAT` | 13,137 | `4b64213a9d19d082af6a075b7aa54407c5cfe3883f6ade1f3862283c130d6ca9` |

Unit `0x1C` begins at `GAME.OVR` offset 3398. The 66-byte
`+3171..+31B2` runner SHA-256 is
`4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489`.
The 33-byte caller tail `+34E7..+3507` SHA-256 is
`317fd3546f947679efeb34c4c490c9002e836323aabf40b5c35425700b40807e`.
Both original spans and the loaded spans at all six native stops are
byte-identical.

## Direct stop sequence

| Stop at `3582:` | RAM SHA-256 | ECL PC | Previous/current opcode | RAM bytes changed from previous stop |
| --- | --- | ---: | --- | ---: |
| `34E7` | `04ade382f947462c95505389a6b729ee9d0bf3a4836b807c29316264c28375c2` | `9975` | `2F/00` | input |
| `3171` | `6e1b669c855b74826bc6e4af1dbd3c91190ef649dab3ad9b1e55ba02a17c7a47` | `9975` | `2F/00` | 1 |
| `026C` | `2d4195a044bdcebff8c776cfb2990978d06062a70df8a537673e8d232a106ae9` | `9B40` | `00/08` | 9,550 |
| `34EA` | `07be274372542cdb93d9454bd7013f0f04c78b7f2d3f53238cfbe60a34057a11` | `9B68` | `1A/00` | 42 |
| `34F5` | `07be274372542cdb93d9454bd7013f0f04c78b7f2d3f53238cfbe60a34057a11` | `9B68` | `1A/00` | 0 |
| `3395` | `07be274372542cdb93d9454bd7013f0f04c78b7f2d3f53238cfbe60a34057a11` | `9B68` | `1A/00` | 0 |

At `+3171`, the only change from the pre-call `+34E7` RAM is physical
`0x21050` `D8 → EA`, the near call's pushed return-IP low byte. The frame at
`SS:3EF0..3EF5` is return `3582:34EA` and argument `0x9975`. From runner
entry to direct `+34EA` return, 9,556 full-RAM bytes changed; transient
loaded, stack, and display regions are included in that count. The returned
ECL PC is `0x9B68`. DS `[442E]`, `[442F]`, and `[49FF]` were all zero at every
stop. Party `(14,4,6)`, window-0 clock `(1,5,10)`, comparison words `(15,4)`,
and stage bytes `[84E0,84E1]=(0,1)` remained stable.

The direct `+34EA → +34F5` transition changes **no RAM**. With `[442F]=0`,
the optional near `+31B4` call was skipped. The direct `+34F5 → +3395`
transition also changes **no RAM**. With `[49FF]=0`, the caller took the
outer-loop jump at `+34FC`, rather than its `+3507` return path. The debugger
observed `+3395` before the next key call; no second key was sent.

## Verification and boundary

`bash tests/replay/check_slot_a_script_return_native.sh` takes the three
ignored inputs above, then RAM dumps at `+34E7`, `+3171`, `+026C`, `+34EA`,
`+34F5`, and `+3395` in that order. Its audit pins every SHA-256, checks the
original and loaded code spans, the runner frame, full-RAM delta counts,
ECL PC/opcode trail, selected party/window state, and full-RAM equality of
the two zero-write caller gate transitions.

This is direct native reachability and state attribution for one search-script
call and its immediate caller gates. The merged `pr_rec_ecl_run` remains a
callback model; this note does not compose the script handlers into a full C
one-Up replay or add a full-C game-routine count. The next outer-loop key
call at `+3395` remains outside this slice.
