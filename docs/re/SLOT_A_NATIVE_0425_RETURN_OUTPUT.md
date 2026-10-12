# Saved slot-A load: resident `0802:0425` return output

This note records one native DOS v1.3 stop immediately after the slot-A
load-path call to resident `0802:0425`, before the caller's next instruction at
unit `0x6F:+0x1974`. It measures the live Pascal output and retained far
pointer. It does not observe the following store, send an Up key, establish
the full C load path, or add a routine count.

Before debugging, `deja "Poolrad slot A resident 0802 0425 return u6f 1974
Pascal output destination ABI length SHA"` found no indexed match. The
[static party-string boundary](SLOT_A_PARTY_STRING_BOUNDARY.md) and the
[native call-time table watch](SLOT_A_NATIVE_0425_TABLE_INPUT.md) supplied
leads; the return stop, pointer, and hashes below were measured independently.

## Pinned inputs and caller boundary

| Ignored input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The original unit `0x6F:+0x1974..+0x1982` caller span is 15 bytes at
`GAME.OVR` file `0x1F95A..0x1F968`, SHA-256
`40340a7f4f937729d3786c40ce28c8cc26d796f9abd977727eaafe0a254b1253`.
The corresponding loaded span at `2A8E:1974..1982` hashes
`85ccb6667d3a08f5fe27f10b74ee586c9487648ced403928853c8d8c06d4aa6c`.
Only the far-call segment operand bytes at unit `+0x1981/+0x1982` differ.
The source decodes as `lea di,[bp-0x1F6]`, `push ss`, `push di`,
`mov ax,0x28`, `push ax`, then a far call to SYSTEM `0x0AF8:0x0B25`.
The preceding resident call is at `+0x196F`; its loaded target is
`1026:0425` on this run.

## One post-call stop

The sole breakpoint `BP 2A8E:1974` hit after selecting saved slot A. Guest
registers were `CS:IP=2A8E:1974`, `DS=14A0`, `SS:SP=1D16:37B4`,
`BP=3DCA`, `ES=1D16`, `DI=39BF`, `AX=0008`, and `FLAGS=7246`.
The private 655,360-byte RAM dump hashes
`a05b315da2205b048ca8e4ef9fa41a2c672325b81cda8851eb4556044f5091d3`.
Its ten bytes at `DS:0D1A..0D23` still hash
`cf4e5025c04cb19b9596b886668127dababab7e9b18c05532b426d82bcb89959`,
matching the earlier pre-call stop.

| Live return-side item | Numeric or hash result |
| --- | --- |
| Far output at `SS:[BP-0x414]` | `1D16:39B6` |
| Pascal length | `8` |
| Eight payload bytes, SHA-256 | `03c11624866b756cd15a30609c7534e0b11e6891051d505106bf44435f24b64e` |
| Length byte plus payload, 9 bytes, SHA-256 | `6c110ad7a5f343c8961fdb3e3762d2d8c7ebfba40902e1fd85015fcf640c3cdd` |
| Far pointer at `SS:[SP]` | `1D16:39B6`, equal to the output address |

The source instructions at `+0x1974..+0x197E` set up destination
`SS:[BP-0x1F6]` and cap `0x28` for SYSTEM `0x0B25`, with the returned
source pointer still on the stack. This stop is **before** that copy. The
pointer value and instruction sequence identify how the next call receives
the output; they do not measure the copy's result.

PM independently compared the ignored native buffer with the pinned C
`0x0356` then `0x0425` composition. Length, both hashes above, and the
private bytes match; see the [issue #3 hash-only comparison](https://github.com/huntergdavis/poolrad-decomp/issues/3#issuecomment-6130207477).
This establishes equality for this one intermediate output, not the later
file/record effects or the end-to-end one-Up route.

## Reproduction and private-data boundary

In an isolated checkout with the pinned ignored inputs:

```sh
tools/oracle/prepare_run.sh slot-a-0425-return-live-017
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-0425-return-live-017 \
  -time-limit 1200
```

At the visible code-word prompt, send Enter; choose `l` at the main menu.
At the slot prompt, enter the heavy debugger with Alt+Pause, set
`BP 2A8E:1974`, enter `RUN`, and choose `a`. At the stop, use
`EV CS IP DS ES DI SS SP BP AX FLAGS` and `MEMDUMPBIN 0000 0000 A0000`.
Read the little-endian far pointer at `SS:SP` and the length-prefixed output
at `SS:[BP-0x414]` from the private dump. The raw output, save, executable
bytes, debugger stream, screenshots, and RAM dump remain ignored under
`resources/`.
