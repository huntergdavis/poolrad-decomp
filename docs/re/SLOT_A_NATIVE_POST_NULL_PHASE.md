# Saved slot-A post-null loop and mode-1 call

On the saved slot-A native load route, unit `0x6F` at `2A8E:1B7E`
copies the party head into an active pointer, visits six linked records,
restores the head after a null link, and takes the mode-1 path through a
far call before reaching `2A8E:1C08`. The six local calls to `+0482`
and the far call have unresolved internal effects in this replay. The
observed window-0 time words remain `(0,5,10)` through the final stop.
This is load-path evidence, not an Up trace, C parity claim, or new
routine count. Execution stopped before `+1C08`.

Before this run, `deja "Poolrad saved slot A unit 6F 1B7E null link exit
next control branch"` found no indexed session. The merged
[next-node watch](SLOT_A_NATIVE_NEXT_NODE_GATE.md) supplied the earlier
null exit, while the [resident selector notes](RESIDENT_DAX_SELECTOR_PUSH.md)
and [unit `0x3BD` entry-7 model](U3BD_WALLSET_ENTRY7.md) supplied
callee identities. This run independently checked its code spans,
live caller frames, stops, and RAM changes.

## Pinned inputs and code

The private run is ignored at
`resources/oracle-runs/slot-a-post-null-phase-live-034`.
Its DOSBox-X heavy binary is 47,259,176 bytes, SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |

Unit `0x6F` begins at `GAME.OVR` offset `0x1DFE6` and is loaded at
`2A8E:0000`. Bounds below are inclusive. Each instruction count is an
i8086 linear decode of that exact span; the direct native stops confirm
the control-flow boundaries. The two changed bytes in the far-call span
are its runtime segment relocation.

| Unit span | Bytes / instructions | Original SHA-256 | Loaded SHA-256 | Changed bytes |
| --- | ---: | --- | --- | ---: |
| `+1B7E..+1B8B` | 14 / 4 | `359f90a87c2895761f4c7fe1df43cd35e2561840798bc05c59e91b12e39fd1ea` | same | 0 |
| `+1B8C..+1BB2` | 39 / 13 | `225582d98e4ac420d4ef5a2a2e7fdd048f2c3ae96db929624f4656b09f9a79ad` | same | 0 |
| `+1BB3..+1BCC` | 26 / 7 | `4b8a4f57ae22f7dab89736c0d73c55a58e9ca9456ef968b07a3946cab2ba9d6e` | same | 0 |
| `+1BCD..+1BE2` | 22 / 6 | `18c8668f3d9b2b2cf982607150c28c11afa3c557e7905b99c91f9c72d7aafa8e` | `8ae00eddf3fc5f552b2924c5852bed1e455f055c48da88a9ac821a4ae31c8180` | 2 |
| `+1BE3..+1BE9` | 7 / 2 | `1736e61ea1b17cd01baf3edb954f4f85e51db908dd3feecf97afca6d14bc0563` | same | 0 |
| Whole `+1B7E..+1BE9` | 108 / 32 | `071850e147bffedc4eb0e9f83579867912ef549b8d98c5a12af4026f1d0d255b` | `30ea0c59fe9ebb97ee4a55b80126f663a76d92c4533349c0734c77f344b2acca` | 2 |

The local far-call body starts at `+0482`; its executable subspan
`+0482..+0673` is 498 bytes, original SHA-256
`be72ede6980e67ba2ef0d51e84c154a281ab9b27b9d00070b7701fcf4bd0a4a3`,
loaded SHA-256
`9fbcb4c9e21e44e82e01f754d2ad086c5b91ae333f57529629b2a834feadff72`
(28 relocation bytes differ). The endpoint is `RETF 2` at `+0671`;
the next 42 bytes `+0674..+069D` are trailing data with identical
original/loaded SHA-256
`8ccc8f762faf22b781b85f7460f4b03870e121f535ea8fbe98ee3d8de467017f`.
The prior ledger hashes the complete 540-byte span as
`a939d6fc0f5514a7efd9e9a08cbff48ef14ff41eddd13652284fd381489240aa`
original and
`30879c634aa9e33c87df7d3f3c5256731dd52aa615cb09a1f59151fc4dd5c9ca`
loaded. No internal behavior is assigned to this callee here.

## Direct native route and effects

At `+1B7E`, `DS:[5D92:5D94]` was `4529:0008` and party head
`DS:[5D96:5D98]` was `449A:0008`. By the `+1B8C` stop, the active
pointer was `449A:0008`. The intervening RAM diff is exactly physical
`0x1A794..0x1A795`, the segment store at `+1B88`; the offset store
at `+1B85` rewrote the same value. `+1B93` took the nonnull path.

Each loop calls `+0482` through `PUSH CS; CALL near`, with a preceding
`MOV AL,1; PUSH AX`. At the first callee entry, `SS:SP=1D16:37B2`:
the return IP at `SS:[37B2]` was `1B9C`, return CS at `[37B4]` was
`2A8E`, and argument AX at `[37B6]` was `4401`. Only its low byte is
established as 1; AH is inherited. The six entry/return pairs and
link-step stops yielded these active pointers:

| Call | `+0482` entry and `+1B9C` return active | Next `+1B8C` active | Link-step changed RAM addresses |
| ---: | --- | --- | --- |
| 0 | `449A:0008` | `44BB:0008` | `0x1A794` |
| 1 | `44BB:0008` | `44D6:0008` | `0x1A794` |
| 2 | `44D6:0008` | `44F5:0008` | `0x1A794` |
| 3 | `44F5:0008` | `4513:0008` | `0x1A794..0x1A795` |
| 4 | `4513:0008` | `4529:0008` | `0x1A794` |
| 5 | `4529:0008` | `0000:0000` | `0x1A792,0x1A794..0x1A795` |

At each return, `+1B9C` loads the active far pointer, `+1BA0/+1BA5`
read its link words at record `+0x104/+0x106`, and `+1BAA/+1BAD`
store the next active pointer. These exact RAM diffs are the link-step
effects. Each callee had other aggregate RAM changes; they are not
assigned to any nested instruction. The seventh `+1B8C` gate saw
null, and `+1B93 JE +1BB3` was taken. The `+1B8C` null and `+1BB3`
images are identical. At `+1BC1`, the active pointer was restored to
`449A:0008`, changing only physical `0x1A792,0x1A794..0x1A795`.

At `+1BC1`, the actual window-1 far pointer in `DS:[49D6]` was
`3A16:0000`; its byte `+0x624` was 2 and was copied to `DS:[5376]`.
The `+1BCD` stop had `DS:[49FA]=1`; `+1BD2 JAE +1BE3` was untaken.
The actual window-0 pointer in `DS:[49D2]` was `3996:0000`, and its
byte `+0x18A` was 20. Immediately before the far call, `AX=0014`.
The call at `+1BDE` has original operand `03D5:0043`; the loaded
operand was `0BF9:0043`. At that live stub the stack contained return
`2A8E:1BE3` and word argument `0014`. The stub resolved to
`2D14:10C7`, the [unit `0x3BD` entry-7](U3BD_WALLSET_ENTRY7.md)
boundary. This replay stopped at the stub and at `+1BE3`, without
tracing that far callee internally. The `+1BDD` pre-push to `+1BE3`
return interval changed 7,772 RAM bytes; the far-stub entry to return
interval changed 7,769. Neither aggregate identifies a game-state writer.

At `+1BE3`, `DS:[49FA]` was still 1; `+1BE8 JBE +1C08` was taken.
The `+1BE3` and `+1C08` RAM images are identical. At all recorded
stops, actual window-0 words `+0x18E/+0x190/+0x192` were `(0,5,10)`.
`DS:[49CE]` changed from 103 to 0 during the first `+0482` call,
and `DS:[5376]` was 2 at the following stops; their exact nested
writers were not watched.

## Snapshot chain and private verification

All snapshots are 655,360 bytes and remain ignored. Each row is a
direct debugger stop, apart from the unlabeled branch at `+1BE8`
whose outcome is bounded by the `+1BE3` and `+1C08` stops. The
private `ev-numeric.tsv` contains 36 register rows from these stops.
It is not published because the associated RAM and media stay private.

| Stop / private RAM suffix | SHA-256 |
| --- | --- |
| `+1B7E` / `1b7e` | `7b916e730b267f610cfae626bf72c29022d660fb4a8632ecd76e521b4294dd45` |
| first `+1B8C` / `1b8c-first` | `66486ad9eb3272c161d155825994a70c9ff9109d79e04a8ed89ebbadff7255fe` |
| first `+0482` / `0482-first` | `9deed65a4337d6501c17f21e13edf088c02ead6b0215196c4de623523607841c` |
| first `+1B9C` / `1b9c-first` | `5376fa153cc9d873c4e9d5d440f4c91f21ffead786a0deee812a209fa3748f0e` |
| `+1B8C` calls 1..5 / `1b8c-1..5` | `adc8ce8bacb464939a483b2e5c3c162434d425f9f35e7ba3d732089baa72e149`, `791bfb1c6ba229786fb823ffa12cf8f58cfe32ec8cfba3dbc17de9753cf93e0f`, `1421883d5dbcde8f7ec49cd40c60659036cf6eadd750fdd5b62220ff64f6ae0c`, `eff56bb0a308174ae9b7fff8cd04948488fecc376c7a269ac43022c96e6e96de`, `7265b24ef81ff475f1193d0a3d56513c498735e0e4a1a1992d8a0bc7b79713f1` |
| `+1B9C` calls 1..5 / `1b9c-1..5` | `18f6a6769474fe2badd1718574b07549c4f2a62b30632c3646f3ccad7995a987`, `8b66e74dd1d84c245470902e4435489d3a0a2005fe6047d843b19b847627dbe9`, `200d2b47539418839a490e79a3502d12370ebbfa164faa053169173e5c1642ae`, `878b433ea652c5e81f0abcd357118b43c1d0466a39235cd5affc3e9a89667f1f`, `6e29efbad68ce5463f98e374419ce12ab252c793f3e0cc5d44539a3bba859e9a` |
| null `+1B8C` and `+1BB3` / `1b8c-null`, `1bb3` | `dc470d5c531a8975e35dd3384848805e6b1332227e75a900c817b54a6f81b3a5` |
| `+1BC1`, `+1BCD`, `+1BDD` / corresponding suffix | `6555459ce4ab64182d99f33018ea9c11a48decd5bf30021898476e46aca3261f` |
| far stub / `far-stub` | `f9ad7cf3fb16e5d3735f1a35364a66468b11e26ff8a2dffb739b12752dc72d2c` |
| `+1BE3`, `+1C08` / corresponding suffix | `f3396a1eccf91c7ea73ab0b9c09ba9fdfa2b38e9bd0b159c06e39e45a256c679` |

To reproduce, prepare an ignored run from the pinned slot-A save with
`tools/oracle/prepare_run.sh`, then launch the pinned heavy DOSBox-X via
`xvfb-run -a -s '-screen 0 1280x1024x24'
tools/oracle/launch.sh <run-directory> -time-limit 1200`. At the
startup/load prompt, enter the debugger with Alt+Pause. Arm breakpoints
`BP 2A8E:1B7E`, `BP 2A8E:1B8C`, `BP 2A8E:1B93`,
`BP 2A8E:0482`, `BP 2A8E:1B9C`, `BP 2A8E:1BB3`,
`BP 2A8E:1BC1`, `BP 2A8E:1BCD`, `BP 2A8E:1BD2`,
`BP 2A8E:1BD4`, `BP 2A8E:1BDD`, `BP 0BF9:0043`,
`BP 2A8E:1BE3`, and `BP 2A8E:1C08`; resume and select slot A
with Enter, `l`, comma, `a`. At each stop, record
`EV CS IP DS ES DI SS SP BP AX BX CX DX FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`, moving `MEMDUMP.BIN` to a distinct
ignored `ram-<stop>.bin` before resuming. Stop at `+1C08`.
The breakpoint at the loaded far stub is valid after the unit load;
the original operand remains `03D5:0043` in the file.

The following private check validates hash-pinned inputs, original and
loaded code, snapshot chain, pointer diffs, and unchanged time words
without printing game bytes:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

r = Path('resources/oracle-runs/slot-a-post-null-phase-live-034')
read = lambda name: (r / name).read_bytes()
h = lambda b: sha256(b).hexdigest()
w = lambda b, p: int.from_bytes(b[p:p+2], 'little')
a = lambda s, o: s * 16 + o
inputs = {
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for name, expected in inputs.items():
    b = read('POOLRAD/' + name)
    assert (len(b), h(b)) == expected, name
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), h(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
snap_hashes = {
    '1b7e': '7b916e730b267f610cfae626bf72c29022d660fb4a8632ecd76e521b4294dd45',
    '1b8c-first': '66486ad9eb3272c161d155825994a70c9ff9109d79e04a8ed89ebbadff7255fe',
    '0482-first': '9deed65a4337d6501c17f21e13edf088c02ead6b0215196c4de623523607841c',
    '1b9c-first': '5376fa153cc9d873c4e9d5d440f4c91f21ffead786a0deee812a209fa3748f0e',
    '1b8c-1': 'adc8ce8bacb464939a483b2e5c3c162434d425f9f35e7ba3d732089baa72e149',
    '1b9c-1': '18f6a6769474fe2badd1718574b07549c4f2a62b30632c3646f3ccad7995a987',
    '1b8c-2': '791bfb1c6ba229786fb823ffa12cf8f58cfe32ec8cfba3dbc17de9753cf93e0f',
    '1b9c-2': '8b66e74dd1d84c245470902e4435489d3a0a2005fe6047d843b19b847627dbe9',
    '1b8c-3': '1421883d5dbcde8f7ec49cd40c60659036cf6eadd750fdd5b62220ff64f6ae0c',
    '1b9c-3': '200d2b47539418839a490e79a3502d12370ebbfa164faa053169173e5c1642ae',
    '1b8c-4': 'eff56bb0a308174ae9b7fff8cd04948488fecc376c7a269ac43022c96e6e96de',
    '1b9c-4': '878b433ea652c5e81f0abcd357118b43c1d0466a39235cd5affc3e9a89667f1f',
    '1b8c-5': '7265b24ef81ff475f1193d0a3d56513c498735e0e4a1a1992d8a0bc7b79713f1',
    '1b9c-5': '6e29efbad68ce5463f98e374419ce12ab252c793f3e0cc5d44539a3bba859e9a',
    '1b8c-null': 'dc470d5c531a8975e35dd3384848805e6b1332227e75a900c817b54a6f81b3a5',
    '1bb3': 'dc470d5c531a8975e35dd3384848805e6b1332227e75a900c817b54a6f81b3a5',
    '1bc1': '6555459ce4ab64182d99f33018ea9c11a48decd5bf30021898476e46aca3261f',
    '1bcd': '6555459ce4ab64182d99f33018ea9c11a48decd5bf30021898476e46aca3261f',
    '1bdd': '6555459ce4ab64182d99f33018ea9c11a48decd5bf30021898476e46aca3261f',
    'far-stub': 'f9ad7cf3fb16e5d3735f1a35364a66468b11e26ff8a2dffb739b12752dc72d2c',
    '1be3': 'f3396a1eccf91c7ea73ab0b9c09ba9fdfa2b38e9bd0b159c06e39e45a256c679',
    '1c08': 'f3396a1eccf91c7ea73ab0b9c09ba9fdfa2b38e9bd0b159c06e39e45a256c679',
}
ram = {}
for name, digest in snap_hashes.items():
    b = read('ram-' + name + '.bin')
    assert len(b) == 655360 and h(b) == digest, name
    assert tuple(w(b, a(0x3996, off)) for off in (0x18E, 0x190, 0x192)) == (0, 5, 10), name
    ram[name] = b
rows = [line.split() for line in read('ev-numeric.tsv').decode().splitlines()]
assert rows[0] == 'label CS IP DS ES DI SS SP BP AX BX CX DX FLAGS'.split()
assert len(rows) == 37 and all(len(row) == 14 for row in rows)
expected_stops = [
    ('entry', 0x2A8E, 0x1B7E), ('active_copy', 0x2A8E, 0x1B8C),
    ('nonnull_branch', 0x2A8E, 0x1B93),
    ('callee_entry_0', 0x2A8E, 0x0482), ('callee_return_0', 0x2A8E, 0x1B9C),
]
for i in range(1, 6):
    expected_stops += [(f'loop_{i}', 0x2A8E, 0x1B8C), (f'branch_{i}', 0x2A8E, 0x1B93),
                       (f'call_{i}', 0x2A8E, 0x0482), (f'return_{i}', 0x2A8E, 0x1B9C)]
expected_stops += [
    ('loop_null', 0x2A8E, 0x1B8C), ('branch_null', 0x2A8E, 0x1B93),
    ('post_loop', 0x2A8E, 0x1BB3), ('head_restored', 0x2A8E, 0x1BC1),
    ('mode_compare', 0x2A8E, 0x1BCD), ('threshold_branch', 0x2A8E, 0x1BD2),
    ('mode1_path', 0x2A8E, 0x1BD4), ('far_call_input', 0x2A8E, 0x1BDD),
    ('far_stub', 0x0BF9, 0x0043), ('far_return', 0x2A8E, 0x1BE3),
    ('mode1_target', 0x2A8E, 0x1C08),
]
assert [(row[0], int(row[1], 16), int(row[2], 16)) for row in rows[1:]] == expected_stops
assert [w(ram['0482-first'], a(0x1D16, p)) for p in (0x37B2, 0x37B4, 0x37B6)] == [0x1B9C, 0x2A8E, 0x4401]
assert [w(ram['far-stub'], a(0x1D16, p)) for p in (0x37B2, 0x37B4, 0x37B6)] == [0x1BE3, 0x2A8E, 0x0014]
game = read('POOLRAD/GAME.OVR')
code = ram['1b7e']
spans = [
    (0x1B7E, 0x1B8C, '359f90a87c2895761f4c7fe1df43cd35e2561840798bc05c59e91b12e39fd1ea', None, 0),
    (0x1B8C, 0x1BB3, '225582d98e4ac420d4ef5a2a2e7fdd048f2c3ae96db929624f4656b09f9a79ad', None, 0),
    (0x1BB3, 0x1BCD, '4b8a4f57ae22f7dab89736c0d73c55a58e9ca9456ef968b07a3946cab2ba9d6e', None, 0),
    (0x1BCD, 0x1BE3, '18c8668f3d9b2b2cf982607150c28c11afa3c557e7905b99c91f9c72d7aafa8e', '8ae00eddf3fc5f552b2924c5852bed1e455f055c48da88a9ac821a4ae31c8180', 2),
    (0x1BE3, 0x1BEA, '1736e61ea1b17cd01baf3edb954f4f85e51db908dd3feecf97afca6d14bc0563', None, 0),
    (0x1B7E, 0x1BEA, '071850e147bffedc4eb0e9f83579867912ef549b8d98c5a12af4026f1d0d255b', '30ea0c59fe9ebb97ee4a55b80126f663a76d92c4533349c0734c77f344b2acca', 2),
]
for lo, hi, original, runtime, changed in spans:
    src = game[0x1DFE6 + lo:0x1DFE6 + hi]
    dst = code[a(0x2A8E, lo):a(0x2A8E, hi)]
    assert (h(src), h(dst), sum(x != y for x, y in zip(src, dst))) == (original, runtime or original, changed)
for lo, hi, original, runtime, changed in [
    (0x0482, 0x0674, 'be72ede6980e67ba2ef0d51e84c154a281ab9b27b9d00070b7701fcf4bd0a4a3', '9fbcb4c9e21e44e82e01f754d2ad086c5b91ae333f57529629b2a834feadff72', 28),
    (0x0674, 0x069E, '8ccc8f762faf22b781b85f7460f4b03870e121f535ea8fbe98ee3d8de467017f', None, 0),
    (0x0482, 0x069E, 'a939d6fc0f5514a7efd9e9a08cbff48ef14ff41eddd13652284fd381489240aa', '30879c634aa9e33c87df7d3f3c5256731dd52aa615cb09a1f59151fc4dd5c9ca', 28),
]:
    src = game[0x1DFE6 + lo:0x1DFE6 + hi]
    dst = code[a(0x2A8E, lo):a(0x2A8E, hi)]
    assert (h(src), h(dst), sum(x != y for x, y in zip(src, dst))) == (original, runtime or original, changed)
active = (0x449A, 0x44BB, 0x44D6, 0x44F5, 0x4513, 0x4529, 0)
assert (w(ram['1b7e'], a(0x14A0, 0x5D94)), w(ram['1b7e'], a(0x14A0, 0x5D92))) == (0x4529, 8)
assert (w(ram['1b7e'], a(0x14A0, 0x5D98)), w(ram['1b7e'], a(0x14A0, 0x5D96))) == (0x449A, 8)
assert [p for p, (x, y) in enumerate(zip(ram['1b7e'], ram['1b8c-first'])) if x != y] == [0x1A794, 0x1A795]
for i, seg in enumerate(active[:-1]):
    before = ram['1b8c-first' if i == 0 else f'1b8c-{i}']
    after = ram['1b9c-first' if i == 0 else f'1b9c-{i}']
    nxt = ram['1b8c-null' if i == 5 else f'1b8c-{i+1}']
    for b in (before, after):
        assert (w(b, a(0x14A0, 0x5D94)), w(b, a(0x14A0, 0x5D92))) == (seg, 8)
    assert (w(nxt, a(0x14A0, 0x5D94)), w(nxt, a(0x14A0, 0x5D92))) == (active[i+1], 8 if active[i+1] else 0)
    changes = [p for p, (x, y) in enumerate(zip(after, nxt)) if x != y]
    assert changes == ([0x1A792, 0x1A794, 0x1A795] if i == 5 else [0x1A794, 0x1A795] if i == 3 else [0x1A794]), i
assert [p for p, (x, y) in enumerate(zip(ram['1b8c-null'], ram['1bc1'])) if x != y] == [0x1A792, 0x1A794, 0x1A795]
assert w(ram['1bc1'], a(0x14A0, 0x5D94)) == 0x449A
assert w(ram['1bc1'], a(0x14A0, 0x5D92)) == 8
assert w(ram['1b7e'], a(0x14A0, 0x49CE)) == 103
assert w(ram['1b9c-first'], a(0x14A0, 0x49CE)) == 0
assert ram['1bcd'][a(0x14A0, 0x49FA)] == 1
assert ram['1bcd'][a(0x3A16, 0x624)] == 2
assert ram['1bdd'][a(0x3996, 0x18A)] == 20
assert sum(x != y for x, y in zip(ram['1bdd'], ram['1be3'])) == 7772
assert sum(x != y for x, y in zip(ram['far-stub'], ram['1be3'])) == 7769
print('slot-A post-null private assertions passed')
PY
```
