# Saved slot-A load: basename and filename before the file probe

Two consecutive stops in one native DOS v1.3 saved slot-A load measure the
basename immediately after the unit `0x6F:+0x1974` capped copy and the
constructed filename immediately before the resident file probe at
`+0x19A8`. Both are address/hash/numeric observations. The run does not
execute or measure the file probe's return, either continuation, or an Up
key; it adds no routine count.

Before debugging, `deja "Poolrad slot A u6f 1983 basename DS 537A 198E
StrLoad native watch"` found no indexed match. The merged
[native `0x0425` return watch](SLOT_A_NATIVE_0425_RETURN_OUTPUT.md),
[string chain](U6F_PARTY_STRING_CHAIN.md), and
[C filename boundary](U6F_PARTY_FILENAME_BOUNDARY.md) supplied the frame
and caller leads. pC and PM confirmed the `+0x1983` destination ABI in
inbox i780/i781; the stops and hashes below were checked directly.

## Pinned inputs and caller identity

| Ignored input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The original 37-byte unit `0x6F:+0x1983..+0x19A7` span is at GAME file
`0x1F969..0x1F98D`, SHA-256
`980cf27a7e57ecae8ab4faceddcaa39190ada07c3464b26d08ab62e7e9178639`.
The loaded `2A8E:1983..19A7` span hashes
`974aecf11af654124cd2ca3b9153eb8135c6d76d55186181a718ff19ce9080bf`.
Only six far-call segment operand bytes differ, at unit
`+0x1991/+0x1992`, `+0x199C/+0x199D`, and `+0x19A6/+0x19A7`.
The decoded calls are SYSTEM StrLoad at `+0x198E`, then two SYSTEM StrCat
calls at `+0x1999` and `+0x19A3`. They use the filename destination
`SS:[BP-0x312]`, live first component `DS:537A`, basename
`SS:[BP-0x1F6]`, and compiled suffix `CS:15E9` in that order.

The original five-byte `+0x19A8..+0x19AC` selector call at GAME file
`0x1F98E..0x1F992` hashes
`84087c84e89649ccb1e5d5618377e5395beca7fc4080f5718865da6f0e0fdd6b`.
Its loaded span hashes
`8cb85b242ae034a616972aad3545d6f5fc95ee435692370a2512ade155f942bd`;
only the far-call segment bytes at `+0x19AB/+0x19AC` differ. The loaded
target is `1026:02EB`.

## Live stops and private hashes

At the visible slot prompt, the debugger was armed with only
`BP 2A8E:1983`, then slot A was selected. The guest stopped at
`CS:IP=2A8E:1983`, `DS=14A0`, `ES=1D16`, `DI=3BDD`,
`SS:SP=1D16:37B8`, `BP=3DCA`, `AX=0008`, and `FLAGS=7246`.
The ignored 655,360-byte RAM dump hashes
`78e17a06de55c781e6be02293d069bffe8ec3a2308fb8ce881fbb1a4c5d3348a`.

| `+0x1983` Pascal value | Length | Payload SHA-256 | Length plus payload SHA-256 |
| --- | ---: | --- | --- |
| Basename `SS:[BP-0x1F6] = 1D16:3BD4` | 8 | `03c11624866b756cd15a30609c7534e0b11e6891051d505106bf44435f24b64e` | `6c110ad7a5f343c8961fdb3e3762d2d8c7ebfba40902e1fd85015fcf640c3cdd` |
| First component `DS:537A = 14A0:537A` | 11 | `7eb58e7fad324c654b05255489704e14d8eb44da3d709646b48e7c7efc427eab` | `903c2ad4e1bb2d3871d2880ff2bb4961eafb7a8d95bcb9d609da2464e3a4cfde` |

The eight basename bytes and length byte match the prior native
`+0x1974` output privately, not just by hash. The `DS:537A` value also
matches the earlier RAM sample, now at the actual StrLoad caller boundary.
The return stack pointer advanced from `37B4` at `+0x1974` to `37B8` at
`+0x1983`, consistent with the intervening SYSTEM StrStore consuming the
retained far source pointer.

The debugger then armed `BP 2A8E:19A8` and resumed the same run. It
stopped before the resident selector with `CS:IP=2A8E:19A8`, `DS=14A0`,
`ES=1D16`, `DI=3AD0`, `SS:SP=1D16:37B4`, `BP=3DCA`, `AX=0004`, and
`FLAGS=7206`. This ignored 655,360-byte RAM dump hashes
`44fd648205d7555863dcd72d487e4d20d1fa1386d29edc8fe8cc908b24e75620`.
The filename at `SS:[BP-0x312] = 1D16:3AB8` has Pascal length **23**;
the 23 payload bytes hash
`e451e9faffe55f9c305ebe7b3b86f9c8333c473ee408471dc165001fd77b88c1`,
and the 24-byte length-plus-payload span hashes
`74c0568877934bb1fdedebf542d12210bd5fa43a70d557a32ef1630daa6ea843`.
The basename at `1D16:3BD4` still has length 8 and the same nine-byte
Pascal hash above.

PM independently extracted the ignored native filename and compared all
24 private bytes with the merged PR #250 C output. They are equal for this
pinned first-slot filename; see the [issue #3 hash-only result](https://github.com/huntergdavis/poolrad-decomp/issues/3#issuecomment-6130528333).
The selector IOResult, `DS:49CE` write, and branch to `+0x19B1` or
`+0x19EA` remain unmeasured here. This is an intermediate filename parity
result, not end-to-end saved one-Up parity.

## Reproduction and private-data boundary

With the pinned ignored inputs in an isolated checkout:

```sh
tools/oracle/prepare_run.sh slot-a-1983-basename-live-018
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-1983-basename-live-018 \
  -time-limit 1200
```

At the code-word prompt, send Enter; select `l` at the main menu. At the
slot prompt, enter the heavy debugger with Alt+Pause, set `BP 2A8E:1983`,
enter `RUN`, and select `a`. At the first stop, use
`EV CS IP DS ES DI SS SP BP AX FLAGS` and `MEMDUMPBIN 0000 0000 A0000`;
rename that private dump before resuming. Set `BP 2A8E:19A8`, enter `RUN`,
and take the second register/RAM sample. Read only the addressed Pascal
spans for lengths and hashes. The private dumps are under
`resources/oracle-runs/slot-a-1983-basename-live-018/` in this checkout;
media, save, output bytes, debugger stream, and screenshots stay ignored.
