# RES_03F1_02E8 identity: NO argument-borne game object (t100)

Base: `160dfbbc653422dd274549a63c166d7a604a719c` (`origin/main`).

**Factor 1 passes; factor 2 does not.** The measured 27 relocated overlay
calls and seven relocated resident calls all have an adjacent copy of a
constant CS Pascal literal into the passed stack string. No qualifying
caller + passed game object + defining document was found in this census.
This is an identity refusal, not a C parity claim or a credit grant.

## Body and inputs

The requested `[02E8,0364)` command decodes 124 bytes and reports no DS cells.
The complete body is **`03F1:[02E8,0367)`, 127 bytes, 61 instruction starts**:
`RETF 10h` starts at `0364` and occupies three bytes. Its SHA-256 is
`71dfae69a38bb9aadad319291d7218cd65523ba14162fb45e5e699ca2d6816d1`.
Every explicit memory operand in this body addresses its SS:BP frame;
there is no direct DS or ES game-object operand. The outgoing calls are
`02FC -> 0AF8:0B25`, `0312 -> 0137`, `032A -> 0000`, and
`035C -> 0709:0626`. The `0000` near call follows `PUSH CS` at `0329`.

Pinned inputs:

- START.EXE: `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
- GAME.OVR: `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`

## Two factors

**Factor 1: PASS.** Four overlay sites belong to three `game_verified`
owners: `001C_0001` at `002F` and `0061`, `001C_0856` at `08A2`, and
`001C_2775` at `2A3C`. The table below joins relocated sites to the committed
function ledger and classification. `001C:+3287` belongs to
`OVL_001C_31B4`, which is `game_evidenced`, not one of those verified owners.

**Factor 2: NO witness.** Each of the 34 measured sites has this adjacent
text setup: address an SS local destination, push it, load an immediate CS
source offset, push that source, call SYSTEM `0AF8:0B0B`, then call `02E8`.
[SYSTEM_STRING_LEAVES.md](SYSTEM_STRING_LEAVES.md) supplies the reviewed
string-load semantics. There is no intervening game-cell concatenation in
these setups. Copying a literal to the stack does not turn it into game state.

The verified callers' game-cell reads gate the call rather than supply its
text. `0001` and `0856` load the object through `DS:49D2` and compare its
`+1CC` field before constant argument setup. `2775` tests `DS:49FF` at
`2A14` (branch at `2A19`), then begins constant setup at `2A1B`. Its eight-byte
literal at `CS:272D` remains a literal; the other three verified sites pass
empty literals. None supplies the object/document witness required by the
argument-borne clause illustrated in
[RES_00BA_16EC_CLOSURE_IDENTITY.md](RES_00BA_16EC_CLOSURE_IDENTITY.md).

Do not overstate this as “all six scalar words are constants.” At
`0086:+0173`, one scalar low byte comes from `[BP-5E]`, selected as `0F` or
`16` by the caller's `[BP+0A]` branch at `0124..0130`; the other five low
bytes are immediate. That coordinate selection does not supply a documented
game object or change the text's CS-literal origin. At the other 33 sites
all six scalar low bytes are immediate. `MOV AL` does not normalize AH, so
this measurement does not call the pushed full words constants.

The seven resident sites likewise pass constant CS text. Four are in
`RPRO_032CA` (`RES_00BA_272A`), one in `RES_00BA_27D9`, and two in
`RES_00BA_2B88`. All have `unresolved` classification in the callable ledger
at this base. Existing identity prose for `27D9` does not change its literal
argument into a game object. The prior `272A` factor-2 refusal is recorded in
[NEAR_ONLY_GAME_SEGMENT_IDENTITY.md](NEAR_ONLY_GAME_SEGMENT_IDENTITY.md).

## Measured direct callers

Literal offsets are relative to the overlay CS, or resident CS `00BA`.
Only offsets, lengths and ownership are published; no game text is included.

| Overlay:site | Owner | Classification | CS literal | Length |
| --- | --- | --- | --- | ---: |
| `0017:022D` | `OVL_0017_0206` | `game_evidenced` | `004A` | 0 |
| `0017:0253` | `OVL_0017_0206` | `game_evidenced` | `004A` | 0 |
| `0019:013A` | `OVL_0019_00FD` | `game_evidenced` | `0000` | 0 |
| `001C:002F` | `OVL_001C_0001` | `game_verified` | `0000` | 0 |
| `001C:0061` | `OVL_001C_0001` | `game_verified` | `0000` | 0 |
| `001C:08A2` | `OVL_001C_0856` | `game_verified` | `0851` | 0 |
| `001C:2A3C` | `OVL_001C_2775` | `game_verified` | `272D` | 8 |
| `001C:3287` | `OVL_001C_31B4` | `game_evidenced` | `31B3` | 0 |
| `001E:0A98` | `OVL_001E_09D7` | `game_evidenced` | `09B5` | 0 |
| `001E:0ABE` | `OVL_001E_09D7` | `game_evidenced` | `09B5` | 0 |
| `001E:0B20` | `OVL_001E_09D7` | `game_evidenced` | `09B5` | 0 |
| `0020:0944` | `OVL_0020_0916` | `game_evidenced` | `083C` | 0 |
| `0020:0B3F` | `OVL_0020_0B14` | `game_evidenced` | `0B02` | 4 |
| `0020:1401` | `OVL_0020_12E8` | `game_evidenced` | `12A8` | 0 |
| `0020:15E5` | `OVL_0020_1527` | `game_evidenced` | `14CE` | 8 |
| `0023:0167` | `OVL_0023_0030` | `game_evidenced` | `001F` | 4 |
| `006A:1366` | `OVL_006A_0FBC` | `game_evidenced` | `0FB3` | 7 |
| `006C:01A9` | `OVL_006C_0161` | `game_evidenced` | `00E2` | 0 |
| `006C:3855` | `OVL_006C_382D` | `game_evidenced` | `37C8` | 0 |
| `0077:1218` | `OVL_0077_0EF8` | `game_evidenced` | `0E64` | 0 |
| `0077:2AA0` | `OVL_0077_299C` | `game_evidenced` | `2993` | 0 |
| `007F:0D14` | `OVL_007F_0CEB` | `game_evidenced` | `0CBA` | 0 |
| `0086:0173` | `OVL_0086_008A` | `game_evidenced` | `001C` | 0 |
| `03B6:022E` | `OVL_03B6_0207` | `game_or_other_library_unresolved` | `0206` | 0 |
| `03B6:0254` | `OVL_03B6_0207` | `game_or_other_library_unresolved` | `0206` | 0 |
| `03B6:027A` | `OVL_03B6_0207` | `game_or_other_library_unresolved` | `0206` | 0 |
| `03B9:053B` | `OVL_03B9_04F9` | `game_evidenced` | `04F8` | 0 |

| Resident image site | Owner | Classification | CS literal | Length |
| --- | --- | --- | --- | ---: |
| `032F1` | `RPRO_032CA` | `unresolved` | `2729` | 0 |
| `03317` | `RPRO_032CA` | `unresolved` | `2729` | 0 |
| `0333D` | `RPRO_032CA` | `unresolved` | `2729` | 0 |
| `03369` | `RPRO_032CA` | `unresolved` | `2729` | 0 |
| `033A7` | `RES_00BA_27D9` | `unresolved` | `27D4` | 0 |
| `03754` | `RES_00BA_2B88` | `unresolved` | `2B87` | 0 |
| `0377A` | `RES_00BA_2B88` | `unresolved` | `2B87` | 0 |

## Consequence for 03F1_0000 and 16EC

`RES_03F1_02E8` remains without a passing two-factor identity. Its near call
at `032A` therefore still prevents `RES_03F1_0000` from meeting the
ready-helper requirement that every caller have an accepted game identity.
Ready helpers cannot satisfy each other's caller condition. This task adds
no helper member and changes no identity, runtime, credit or replacement row.

`RES_00BA_16EC` has its own passing identity argument, but its in-domain
`03F1_0000` dependency remains unready. Thus this result does not unlock its
strict closure, strict credit, or the dependent 0038 handlers. A hypothetical
fully ready closure diagnostic is not evidence that the closure is ready.
At this base the t81 with/without measurement is unchanged: **431 candidates,
49 composable, 289 credited, 121 replaced**, with no ready-helper members.

## Reproduction and limits

Run from the repository root with the pinned media under ignored `resources/`:

```sh
export PYTHONPATH=/tmp/poolrad-capstone:tools/overlay
export PYTHONDONTWRITEBYTECODE=1
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/far_call_sites.py 03F1:02E8
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/resident_ds_operands.py 03F1:02E8:0364
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/resident_ds_operands.py 03F1:02E8:0367
/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res03f1_02e8_identity.py --output resources/t100-02e8-identity/identity.json
/home/hunter/workspace/poolrad-heavy.sh python3 -m unittest tools/overlay/test_res03f1_02e8_identity.py
```

The new census checks both media hashes, loader image pin, relocation-backed
calls and preceding string copies, complete adjacent argument setup, literal
bounds, owner joins, expected caller counts, verified caller set, and full
body decode. Five synthetic controls pass, including rejection of DS-source
and register-sourced text pointers and truncated literals. It reports the
variable scalar instead of silently labeling it constant.

This is static evidence for the enumerated direct sites, not an execution
trace, control-flow dominance proof, or universal claim about undiscovered
indirect calls. A future positive identity claim still needs a concrete
accepted caller, passed game object and defining document. The current NO
must not be promoted into an unrelated library-identity claim.

Prior context was recalled through Dunking Sheep memory before analysis.
The wrapper span and behavior reuse
[COLD_START_CODEWORD_BOX_SOURCE.md](COLD_START_CODEWORD_BOX_SOURCE.md);
the resident `27D9` path reuses
[RESIDENT_LOAD_ROUTE_27D9.md](RESIDENT_LOAD_ROUTE_27D9.md).
The new census independently re-decodes the pinned inputs and adds the
complete caller/object accounting above.
