# Saved slot-A first view: U3BD object chain through view one

A fresh saved-slot-A native run joins the reviewed U3BD `+0x0860`
placement, the later `+0x0844`/`+0x0860` dual-wall placements, and the
`+0x08E8..+0x097F` view-one scan. It captures ten ordered
`+0x02FB` tile handoffs and their `+0x0300` returns. One persistent C
offscreen destination, seeded **once** from the native object before the
first `+0x0860` call, matches the complete native 14,135-byte object
before and after **every** tile callback and at `+0x097F`. This proves
the bounded object value chain through that scan exit. It does not
produce the later EGA page or a complete first-view frame.

Before this debug run, bounded
`deja "Poolrad U3BD 0863 view one 097F same run offscreen object tile parity"`
timed out after 45 seconds with no indexed result. The work reuses the
accepted [dual-wall caller](SLOT_A_U3BD_DUAL_WALL_0824.md),
[view-one scan](SLOT_A_U3BD_VIEW1_SCAN.md),
[first art-effect join](SLOT_A_U3BD_0211_ART_JOIN.md), and
[group-zero tile model](SLOT_A_U40C_FIRST_TILE_PLANE.md). Their
contracts were checked against the new same-run stack frames and
original span hashes.

## Original bytes and native route

The ignored DOS v1.3 `GAME.OVR`, `START.EXE`, and `SAVGAMA.DAT`
SHA-256 values are respectively
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
and
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The isolated heavy DOSBox-X executable SHA-256 is
`c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`.
The recorded private launch script SHA-256 is
`570038924034eba9ab00f2cad6929b268735e15bc8395b60ebe7f1bc6211274c`;
the audit resolves its binary path and verifies the executable hash.

| Role | Original half-open file span | SHA-256 |
| --- | --- | --- |
| U3BD local placement `+0211..+031F` | `GAME.OVR [0x2EB08,0x2EC16)` | `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03` |
| U3BD dual-wall arm `+0824..+0865` | `GAME.OVR [0x2F11B,0x2F15C)` | `23a4689e3551c41ef9e029f4870be70705cc261c672d55599262ceffb15b9fab` |
| U3BD old-wall arm `+0865..+08BA` | `GAME.OVR [0x2F15C,0x2F1B1)` | `3af5d1fd4ecf9689eab13e72fc118b7e4517fd55e692632698b138554b7b1bf8` |
| U3BD view-one scan `+08E8..+097F` | `GAME.OVR [0x2F1DF,0x2F276)` | `ecab0ea500549a1daa4b74f6e852c4f28c3340c19a43c690dc9cef7dd898eb0a` |
| U40C entry-2 dispatcher | `GAME.OVR [0x32392,0x324E2)` | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` |
| Reached resident tile callee | `START.EXE [0x4F4D,0x53EA)` | `9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe` |

The ignored run is
`resources/oracle-runs/slot-a-u3bd-view1-object-join-p5-033/`.
Its closed debugger transcript SHA-256 is
`0e52d38d532a5a0eec6b58304497313ddc299db84279a34242902676a19fc109`.
The private 26-file, 640-KiB-per-stop RAM SHA manifest has SHA-256
`2d58f9e1a960dfeeffd969f939759fa8e7b1bbb60dfe695f0f4d40fa649dc219`.
The public audit verifies both hashes, every RAM image, all 14 CPU
fields at each stop, the original spans, and the saved input. Every
stop compared here belongs to this **one run**.

The first `2D14:+0860` frame has `DS=14A0`, `SS=1D16`, and
`DS:[4A20]=3DE4:0008`, placing the 14,135-byte offscreen object at
physical `0x3DE48`. The later `+08E8` scan entry follows the second
`+0863` return. Tile frames have `SS:SP=1D16:3E78`,
flags `(1,1)`, and the parent return words shown below.

| Native calls | Parent return | Ordered `(x,y,item)` | Immediate object bytes changed |
| --- | --- | --- | ---: |
| 1–2 | `+0863` | `(7,6,1)`, `(7,7,13)` | 0, 0 |
| 3–4 | `+0847` | `(8,6,1)`, `(8,7,13)` | 0, 9 |
| 5–6 | `+0863` | `(9,6,1)`, `(9,7,13)` | 0, 9 |
| 7–10 | `+0938` | `(6,5,1)`, `(6,6,18)`, `(6,7,26)`, `(6,8,28)` | 0, 14, 15, 22 |

## Persistent C object parity

The checker builds the merged `pr_u40c_entry2_mode1_tile_plane` under
strict C warnings. At each native pre-call stop, it checks the
persistent C object against that same run's full native pre-object,
then supplies the live stack tuple, group-zero remap, video state,
source object, and auxiliary mask. It compares the entire C object
with the immediate native post-object before proceeding. No expected
post-call byte is assigned to the C destination. Native `+0863`
returns, the second `+0860` entry, `+08E8` entry, and `+097F` exit
also equal the C object at their respective boundaries.

| Check | Result |
| --- | ---: |
| Native/C full object comparisons at each tile pre and post | 20/20 exact, 14,135 bytes each |
| Modeled tile plane-byte stores | 320 |
| Sum of immediate native/C changed object bytes | 69 |
| Native pre-object versus preceding C post-object | 0 differences at all ten calls |
| Final C/native `+097F` object SHA-256 | `15ef1f9670025967b98c18778f0e9cb0d5e6ca96b77b48e12e92a5ff57d9ffbc` |

The first object SHA-256 is
`39962babb659d2eb872c4548dcf7e592d4ee5c2c51672cba8964a16ed79a00e9`.
Zero net difference at inter-call boundaries does not exclude stores
of the same value or transient changes between stops. The C tile model
covers object data; CPU, stack, dirty metadata, later placement calls,
and page transfer remain separate boundaries. A synthetic CTest
composes the existing first placement, dual-wall, and view-one caller
models on one artificial object, checking all ten ordered effects and
retained bytes without game art. Neither full U3BD entry parity nor
new game-identity or strict full-C credit is claimed.

## Reproduce

With the ignored owner inputs and private run present:

```sh
python3 tools/replay/u3bd_first_view_object_chain_audit.py
cmake -S . -B resources/build-u3bd-first-view-object-chain \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u3bd-first-view-object-chain \
  --target poolrad_overlay_u3bd_first_view_object_chain_tests
ctest --test-dir resources/build-u3bd-first-view-object-chain \
  -R '^poolrad_overlay_u3bd_first_view_object_chain_tests$' \
  --output-on-failure
```

The focused strict CTest passed 1/1. The full strict build and CTest
suite passed 269/269. Original executables, save,
RAM images, transcript, screenshots, source art, and pixels remain
ignored under `resources/`.
