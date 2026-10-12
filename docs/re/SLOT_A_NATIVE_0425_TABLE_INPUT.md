# Saved slot-A load: live table input at resident `0802:0425` call

This address/hash/numeric-only note captures the ten-byte DS table immediately
before one native DOS v1.3 slot-A load-path call to resident `0802:0425`.
It establishes a private host-fixture input for the pinned route. It does not
translate the callee, send an Up key, claim C-native parity, or change a
routine count.

Before tracing, `deja "Poolrad resident 0802 0425 DS 0D1A 0D23 unit6F
196F slot-A live input"` found no indexed match. The independent static
[party-string boundary PR #243](https://github.com/huntergdavis/poolrad-decomp/pull/243)
supplied the caller lead, and the checked-in
[party-counter watch](SLOT_A_NATIVE_PARTY_COUNTER_67C.md) supplied a loaded
unit `0x6F` segment lead. pC requested the single live table in inbox i709.
The file span, live stop, and table hash below were checked independently.

## Pinned ignored inputs and caller identity

| Private input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

Unit `0x6F` begins at GAME.OVR file `0x1DFE6`. Its original 15-byte caller
span `+1965..+1973`, file `0x1F94B..0x1F959`, hashes
`8caf30b0059e878ceaacb6f7fc0ec602ddd1838f9410c950805ad63d3c4e5126`.
The three consecutive far calls are at `+1965`, `+196A`, and `+196F`;
the last names original resident `0802:0425`. In the live RAM, the loaded
15-byte span at `2A8E:1965..1973` hashes
`e47115581e757e32ef6e46fbd56924366c0e8923917cc7ff18eeea66b97beac5`.
Only the six far-call segment operand bytes at `+1968/+1969`,
`+196D/+196E`, and `+1972/+1973` differ from the source. The guest stopped
at `2A8E:196F` before that last call; its relocated target was `1026:0425`.

## Live table and private fixture

The heavy debugger was armed with only `BP 2A8E:196F` at the visible load-slot
prompt, then slot A was selected. The breakpoint hit with `DS=14A0`,
`SS:SP=1D16:37B0`, `BP=3DCA`, and `AX=0008`. A private 655,360-byte RAM
dump at this pre-call stop hashes
`5f4da2821922f889a9df410846e111d7acd63259d2ef9cd32b4320600241323b`.

At that stop, the exact ten bytes `DS:0D1A..0D23` hash
`cf4e5025c04cb19b9596b886668127dababab7e9b18c05532b426d82bcb89959`.
All ten are nonzero and pairwise distinct. The same table hash appears in
three earlier ignored samples at unit `0x6F:+191E`, unit `0x6F:+19F7`, and
unit `0x1C:+3395`; their later timing alone could not prove the call-time
input. The resident body's static loop reads `DS:[0D19+i]` for `i=1..10`;
the pre-call stop supplies the corresponding live table identity.

The ten raw bytes were extracted to ignored
`resources/oracle-runs/slot-a-0425-table-live-016/private-ds-0d1a-table.bin`
in this isolated checkout. A host-supplied private fixture can use that path
and verify its length and hash above. Neither those bytes nor the RAM dump
are in this PR. This observation does not execute the C bridge or establish
the effects of the resident call.

## Reproduction boundary

With the pinned ignored inputs in an isolated checkout, prepare one private
run and attach the heavy debugger to a PTY:

```sh
tools/oracle/prepare_run.sh slot-a-0425-table-live-016
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-0425-table-live-016 \
  -time-limit 1200
```

Wait for the visible code-word prompt, send Enter, select `l`, and pause at
the load-slot prompt before selecting `a`. Arm `BP 2A8E:196F`, resume,
and select `a`. At the stop, use `EV CS IP DS SS SP BP AX` and
`MEMDUMPBIN 0000 0000 A0000`. Read only `DS:0D1A..0D23` from the private
RAM dump for the table length and SHA-256 above. Keep any extracted fixture,
media, save, screenshot, debugger stream, and RAM dump under ignored
`resources/`.
