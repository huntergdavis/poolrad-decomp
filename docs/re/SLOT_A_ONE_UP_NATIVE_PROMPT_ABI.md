# Slot-A one-Up native prompt ABI watch

One DOS v1.3 replay of exactly one Up measured the input prompt return in
unit `0x68` entry 2. The restored display changed from `15,4 N 10:50` to
`14,4 N 10:51`. This is a guest observation, not C-native parity or a new
game-routine count.

Before debugging, `deja "poolrad slot A one Up unit 0x68 entry2 prompt 0x3C5:0x2A BP-4 key AL heavy DOSBox-X"`
and `deja "poolrad native prompt key extended flag SS BP-4 0x09D0"`
returned no earlier prompt trace. A broader DOSBox-X slot-A query surfaced
the [earlier minute watch](SLOT_A_ONE_UP_NATIVE_WRITE_WATCH.md); its private
heavy-debug build and hash-pinned replay procedure were reused. The
[Up-preflight note](U68_UP_PREFLIGHT_068D.md) supplied the call-site lead.
The loaded location, frame, and return below were measured again.
The merged [entry-2 Up slice](U68_ENTRY2_UP_SLICE.md) exposes both prompt
outputs as required host inputs; this trace independently measures their
DOS values without asserting C replay parity.

## Pinned inputs and call site

| Ignored input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| slot-A world save | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| private heavy-debug DOSBox-X executable | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The bounded original unit `0x68` entry-2 body is `+0x0989..+0x0D6C`,
GAME.OVR file `0x17054..0x17437`: 996 bytes, SHA-256
`5f98635ca6d7837bfb2af18c1bbe8178db3e5e8ca13fc20817cdc639d254627e`.
Its five-byte far call at `+0x09D0`, file `0x1709B..0x1709F`, has SHA-256
`369af068023cb73db52eb83f2d63d9a97804ff2e707e69f66c165bcd1575c5d3`.
The next instruction at `+0x09D5`, file `0x170A0..0x170A2`, stores `AL`
to local `[BP-3]`; its three bytes hash to
`be7e3c08b70995be17c8c4a7c48e719b5756bc9b2bdad5c36d3cb07320ea26ba`.
The following comparison at `+0x09D8` reads the extended-key flag at
`SS:[BP-4]`.

In this run, one 32-byte original entry-2 prefix matched guest physical
`0x2EF89`, giving loaded segment `2E60`. The full loaded 996-byte body
hash was `2ee00dbe84729ffddf53c29733f4f1016e196145fdf66e8fe0a9ca7ed23338bb`.
It differed from the original at 35 two-byte far-call segment operands,
70 bytes total. The live call at `2E60:09D0` used relocated far target
`0BE9:002A`; its five-byte hash was
`eb000fa494b2b87f70c19fbdf4b57e1a3508686d7c80752873fa14cf34507de9`.
Its return address was `2E60:09D5`.

## Exactly one Up and the next prompt

Two code breakpoints were armed at `2E60:09D0` and `2E60:09D5` after the
`15,4 N 10:50` screen appeared. The first prompt call was already waiting,
so its pre-call breakpoint did **not** fire. Exactly one Up key was sent.
The debugger stopped before the `+0x09D5` store with
`CS:IP=2E60:09D5`, `DS=14A0`, `SS=1D16`, `SP=3EB4`, `BP=3EF0`, and
`AX=0048`: returned `AL=0x48`. The output byte `SS:[BP-4]`, at
`1D16:3EEC`, was `1`; `[BP-3]` was still zero before the store. The
residual stack held return pair `09D5:2E60`, output pointer `1D16:3EEC`,
numeric argument words `(0,1,15,10,13)`, and prompt pointer
`14A0:04B6`.

After resuming this same Up, the display showed `14,4 N 10:51`. Before
any second key, the next prompt stopped *before* the call at
`2E60:09D0`, with `SS=1D16`, `SP=3E9E`, and `BP=3EF0`. The guest then
stopped at the relocated far-entry stub `0BE9:002A`, with `SP=3E9A`.
Its live entry stack was:

| Guest stack address | Numeric value | Role |
| --- | ---: | --- |
| `SS:3E9A`, `SS:3E9C` | `09D5`, `2E60` | far return |
| `SS:3E9E`, `SS:3EA0` | `3EEC`, `1D16` | output flag far pointer |
| `SS:3EA2..3EAA` | `0,1,15,10,13` | five numeric arguments |
| `SS:3EAC`, `SS:3EAE` | `04B6`, `14A0` | prompt far pointer |

All nine argument words at that next pre-call stop matched the residual
words captured at the prior Up return. These are two distinct prompt
invocations; the next pre-call frame is not presented as a pre-call stop
for the Up itself. Before this next call, the flag byte at `SS:[BP-4]`
was `0x82`, demonstrating that it must be treated as an output of the
prompt call, not assumed to be initially zero. No second key was sent.

## Reproduction boundary

Use the [private heavy-debug replay procedure](SLOT_A_ONE_UP_NATIVE_WRITE_WATCH.md#reproduction-commands)
from a terminal so DOSBox-X exposes its debugger. Prepare a fresh ignored
run, use its documented `AUTOTYPE -w 160 -p 8 enter l , a` sequence without
Up, and confirm `15,4 N 10:50` first. The emulator clamps the requested
160-second wait to 30 seconds; the pace remains eight seconds per key.
Identify the loaded entry-2 body by its original prefix and far-call
segment fixups before arming `BP 2E60:09D0` and `BP 2E60:09D5` for this
run. Send exactly one Up, inspect the return, then resume to the next
prompt without another key. At that pre-call stop, arm `BP 0BE9:002A` and
resume once to capture the relocated callee-entry frame. Use
`EV CS IP DS SS SP BP AX` and `MEMDUMPBIN SS 3E9A 16` at that entry;
the latter captures its 11 stack words for this run. Guest memory dumps,
save, screenshot, emulator trace, and game bytes remain under ignored
`resources/` and are not part of this note.
