# Native slot-A first selector return at unit 0x6F:+0x19AD

One native DOS v1.3 replay of the hash-pinned saved slot A reached the first
far call from unit `0x6F:+0x19A8` to resident `0x0802:0x02EB`. The call returned
`AX=0x0001`; the caller continued at `+0x19B1`, the nonzero-AL branch. The
same run measured `DS:0x49CE=0x0000` both immediately before and immediately
after the call. The unchanged word is an observed value, not evidence that no
write occurred. The resident's [bounded model](RESIDENT_DAX_SELECTOR_02EB.md)
documents its word store and DOS callbacks; this replay does not isolate each
callback or infer later loop outcomes.

Before this audit, `deja "Poolrad slot A resident 0802 02EB return 19AD
IOResult DS 49CE branch 19B1 19EA"` found no indexed match. The
[first-slot filename boundary](U6F_PARTY_FILENAME_BOUNDARY.md) supplied the
caller and resident span leads; both were rechecked against the ignored
original images and loaded memory. The prior
[native filename capture](SLOT_A_NATIVE_1983_FILENAME.md) supplied the saved
route and filename-length lead. This run independently stopped before and
after the resident call.

## Hash-locked inputs and code

| Span or private input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Saved slot-A `SAVGAMA.DAT` | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original caller `+0x19A8..+0x19B0`, GAME file `0x1F98E..0x1F996` | 9 bytes, 3 instructions | `34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb` |
| Loaded caller `2A8E:19A8..19B0` in post-call RAM | 9 bytes, 3 instructions | `09eb6c96435627d600410ae10b43548304ec9f27e62070481443ab6bc639c6fe` |
| Resident `0x0802:0x02EB`, START file `0x08342..0x083AC` | 107 bytes | `141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474` |
| Private heavy DOSBox-X executable | — | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The original caller decodes as far call to `0x0802:0x02EB`, then `OR AL,AL`
and `JE +0x19EA`. At runtime the far call targets `0x1026:0x02EB`; only its
segment word at relative bytes `+3,+4` differs between original and loaded
caller spans. Thus the `+0x19B1` breakpoint is the fall-through path after
testing returned AL, while `+0x19EA` is the zero-AL branch.

## One observed first iteration

The heavy debugger was paused at the visible load-slot prompt before choosing
`a`. Only `BP 2A8E:19A8` was armed at first. The following stops all belong to
that same saved-slot run; register values are hexadecimal.

| Stop before instruction | `DS`; `SS:SP`; `BP` | `AX`; `FLAGS` | `DS:49CE` |
| --- | --- | --- | --- |
| `2A8E:19A8`, before far call | `14A0`; `1D16:37B4`; `3DCA` | `0004`; `7206` | `0000` |
| `2A8E:19AD`, after return and before `OR AL,AL` | `14A0`; `1D16:37B8`; `3DCA` | `0001`; `7206` | `0000` |
| `2A8E:19B1`, after `OR AL,AL; JE +19EA` | `14A0`; `1D16:37B8`; `3DCA` | `0001`; `7202` | not separately dumped |

At the first stop the pre-call RAM dump contained `DS:49CE=0`. At the second
stop the post-call RAM dump contained `DS:49CE=0`. Breakpoints were then armed
at both `+0x19B1` and `+0x19EA`; the first stop was `+0x19B1`, so this
iteration took the nonzero-AL continuation. The pre-call and post-call
655,360-byte private RAM dumps have SHA-256
`097d2d4dd595e16a9adf3669b57c9a43365af7d77f755d48eef04f3a98b29553`
and `7bea92e0747ca007f4d7851e365194fe7a6ff56cf04cc01e4d658eaf219a9efa`,
respectively. No later iteration, file-record effect, Up input, or C-native
end-to-end parity is established by these stops. No strict-C routine count is
claimed.

## Private reproduction and numeric assertions

Run in an isolated checkout with the ignored pinned DOS media, owner save,
and private heavy debugger:

```sh
tools/oracle/prepare_run.sh slot-a-19ad-selector-live-020
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-19ad-selector-live-020 \
  -time-limit 1200
```

At the code-word prompt enter Return, choose `l`, then pause at the visible
load-slot prompt with Alt+Pause. In the debugger set `BP 2A8E:19A8`, enter
`RUN`, and choose `a`. At the stop, use `EV CS IP DS SS SP BP AX FLAGS`
and `MEMDUMPBIN 0000 0000 A0000`; move the private `MEMDUMP.BIN` to
`pre-selector-19a8-ram.bin` in the ignored run directory. Set
`BP 2A8E:19AD` and `RUN`, repeat the register and dump commands, and move
that dump to `post-selector-19ad-ram.bin`. Then arm `BP 2A8E:19B1` and
`BP 2A8E:19EA`, `RUN`, and record the first stop with the same `EV` command.
The loaded image relocates the call target, so verify the breakpoint
addresses on the debugger code view.

With those private files in the ignored run directory, the numeric-only
assertions are:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

run = Path('resources/oracle-runs/slot-a-19ad-selector-live-020')
game = (run / 'POOLRAD/GAME.OVR').read_bytes()
start = (run / 'POOLRAD/START.EXE').read_bytes()
save = (run / 'POOLRAD/SAVGAMA.DAT').read_bytes()
pre = (run / 'pre-selector-19a8-ram.bin').read_bytes()
post = (run / 'post-selector-19ad-ram.bin').read_bytes()
digest = lambda data: sha256(data).hexdigest()
assert digest(game) == '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
assert digest(start) == 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
assert digest(save) == 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'
assert len(pre) == len(post) == 655360
assert digest(pre) == '097d2d4dd595e16a9adf3669b57c9a43365af7d77f755d48eef04f3a98b29553'
assert digest(post) == '7bea92e0747ca007f4d7851e365194fe7a6ff56cf04cc01e4d658eaf219a9efa'
original = game[0x1F98E:0x1F997]
loaded = post[(0x2A8E << 4) + 0x19A8:(0x2A8E << 4) + 0x19B1]
assert digest(original) == '34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb'
assert digest(loaded) == '09eb6c96435627d600410ae10b43548304ec9f27e62070481443ab6bc639c6fe'
assert [i for i, (a, b) in enumerate(zip(original, loaded)) if a != b] == [3, 4]
assert digest(start[0x8342:0x83AD]) == '141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474'
word = lambda ram: int.from_bytes(ram[(0x14A0 << 4) + 0x49CE:(0x14A0 << 4) + 0x49D0], 'little')
assert (word(pre), word(post)) == (0, 0)
print('same-run DS:49CE pre/post:', word(pre), word(post))
print('private RAM SHA-256:', digest(pre), digest(post))
PY
```

Original images, saves, RAM dumps, screenshots, and debugger stream remain
under ignored `resources/`; none is included in this note or Git.
