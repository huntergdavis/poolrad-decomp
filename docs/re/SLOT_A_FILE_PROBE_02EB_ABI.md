# Saved-A +19A8 file-probe CPU/RAM ABI and unsupported runtime gates

The separate addressed adapter executes the natural unit-6F +19A8 far CALL,
resident 0802:02EB prologue, actual SYSTEM B25 copy and argument setup. It
reaches loaded 1026:0310 after **34 C instructions** and blocks unchanged
before SYSTEM Assign AF8:13E2, return +0315. Assign, Reset, Close and
IOResult remain unsupported effects. The value/callback selector model is
not called, and no native results, replay journals or file effects are
injected into C.

This distinct branch starts at merged main
`a0299a663a042cf5b64801372c7ed1a535bd3756`, after PR #428. Before debugging
or implementation, bounded `deja "Poolrad saved-A 19A8 0802 02EB file probe
native return 19AD DOS ABI"` expired without reusable output. Reused
repository leads are the [resident selector model](RESIDENT_DAX_SELECTOR_02EB.md),
the [earlier native return](SLOT_A_NATIVE_19AD_SELECTOR_RETURN.md), and
the [B98 caller ABI](SYSTEM_B98_NATIVE_ABI.md). All addresses, bytes,
frames and register effects below were rechecked in a fresh natural run.
Bounded span/flag primitives follow the merged 0356/CBC adapters; B25
composition uses the existing addressed prefix API rather than host strings.
The review branch is restacked onto fresh main
`b749eb90962c448d61663cfafe3dc413e191f6da`; all tested replay dependencies
are byte-identical across that restack.

## Pinned code and fresh native capture

| Ignored input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SAVGAMA.DAT | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original resident +02EB..0355, START file 08342..083AC | 107 | `141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474` |
| Loaded 1026:02EB..0355 | 107 | `a1bd874454a477dc56eb74bee8be51f5e77b17c782081440306e0ffb5213d2e2` |
| Original unit-6F +19A8..19B0 | 9 | `34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb` |
| Loaded 2A8E:19A8..19B0 | 9 | `09eb6c96435627d600410ae10b43548304ec9f27e62070481443ab6bc639c6fe` |
| Raw twenty-one-row native transcript | — | `1a2a7c30d75c61af2c53ca05d957861eb37297ba0c7284c6e7c0f2d9ff6110dd` |

The selector has 43 original instruction boundaries and five SYSTEM far
calls. Exactly five segment words relocate AF8 to loaded 131C. The caller
call's segment word relocates 0802 to 1026. Full original inputs, B25 code,
loaded selector/caller bodies, and runtime entry fingerprints are checked
against every captured RAM image. The following native stops all belong
to one ordinary saved-A load, with no guest register or RAM edits:

`19A8, 02EB, 02EC, 02F2, 0300, 0305, 0310, 13E2, 0315, 031F, 1410,
0324, 032A, 1491, 032F, 04A2, 0334, 0337, 0348, 0353, 19AD`.

Caller stops use CS=2A8E, resident stops CS=1026, and the four runtime
entry stops CS=131C. All sixteen CPU fields and all 655360 RAM bytes are
pinned by the raw transcript, per-stop JSON and per-stop RAM hashes.
The ignored capture root is `slot-a-probe-02eb-abi-20261014`.

## Live frames and effects

At +19A8, SS:SP=1D16:37B4, BP=3DCA, AX=0004, BX=37AC, CX=0000,
DX=DS=14A0, SI=15EE, DI=3AD0, ES=SS, FS=GS=0000, FLAGS=7206.
The retained filename pointer is 1D16:3AB8, with Pascal length 23.
The far CALL pushes 2A8E:19AD at SS:37B0, changing exactly physical
20910 and 20911. PUSH BP and SUB SP establish BP=37AE, SP=3704;
the 170-byte local region contains the name at SS:3784 and private file
record at SS:3704.

At resident +0300 the actual B25 arguments are capacity=40,
destination=1D16:3784, source=1D16:3AB8. B25 returns at +0305 with
SP=3704, AX=0017, BX=36F6, CX=0000, DX=14A0, SI=3AD0,
DI=379C, FLAGS=7246. C calculates the copy and flags in original order.
The complete +19A8-to-+0310 prefix changes 42 RAM bytes, including
stack and the 24-byte local Pascal string.

| Runtime boundary | Native entry SS:SP | Real return | Arguments |
| --- | --- | --- | --- |
| +0310 -> SYSTEM 13E2 Assign | 1D16:36F8 | 1026:0315 | name 1D16:3784; record 1D16:3704 |
| +031F -> SYSTEM 1410 Reset | 1D16:36FA | 1026:0324 | size 0080; record 1D16:3704 |
| +032A -> SYSTEM 1491 Close | 1D16:36FC | 1026:032F | record 1D16:3704 |
| +032F -> SYSTEM 04A2 IOResult | 1D16:3700 | 1026:0334 | no explicit arguments |

Each unsupported far CALL changes four RAM bytes in its measured frame.
Original-byte emulator execution of each CALL alone matches native entry
CPU/full RAM. These four comparisons supply frame evidence only; the C
adapter blocks before every one of those frame pushes.

Assign entry-to-return changes 49 RAM bytes. Reset entry-to-return changes
30, including lower DOS memory; Close changes 26, also including DOS
memory. IOResult entry-to-return changes zero RAM bytes in this successful
run, while changing CPU. Reset's return has DX=3734; Close's has
AX=3E01, BX=0006, FLAGS=7202; IOResult returns AX=0000, FLAGS=7246.
Those measured values do not define generic runtime implementations.

At resident +0334 the full AX word is stored to DS:49CE. Both before and
after RAM contain zero, so the executed store has zero net changed bytes.
Synthetic nonzero AX tests establish the full-word writer independently.
The nonempty-name path reaches +0348, selects AL=1 and executes RETF 4.
At natural +19AD, SS:SP=1D16:37B8, BP=3DCA, AX=0001, BX=0006,
CX=0000, DX=3734, SI=379C, DI=3704, FLAGS=7206; DS=14A0,
ES=SS=1D16 and FS=GS=0000. The complete native +19A8-to-+19AD
net RAM change is 81 bytes. This is native observation across unknown
runtime effects, not continuous C parity across the probe.

| Private RAM stop | SHA-256 |
| --- | --- |
| Natural caller +19A8 | `9f246341a8e2a1ad5b6492b0669d6cdea2e599f698e4f448a76f9639971310e5` |
| Resident entry +02EB | `c97808d3ef3d6c4f837954f2fcb09c32305c3e802bd2689eb20c1f6bdbe6ca3f` |
| Unsupported Assign call +0310 | `f4c5a354693735b9e00beec62abb659331cf5ede2ec582c6997bb56313befa07` |
| Natural return +19AD | `e8356e1107adcd130ea03a79033ca764cebd67f91375485efe2632c1bddbae77` |

## Bounded C parity and remaining gates

The continuous **34-instruction prefix** matches original-byte Unicorn
execution after every instruction and **six post-entry native C stops**.
Separately seeded native fragments after Assign, Reset and IOResult match
5, 3 and 11 instructions with 1, 1 and 4 native checkpoints respectively.
The after-Close input is already at the unsupported IOResult gate and
executes zero C instructions. Together these give **12 native C checkpoints
across independent fragments**, not a stitched end-to-end replay. The
twenty-one captured rows partition into five fragment inputs, twelve
post-input C checkpoints and four emulator-only far-call entry comparisons.

Eight synthetic prefix cases cover empty, exact-capacity, over-capacity and
maximum-length strings; identical source/destination; both overlap
directions; and physically identical pointers using different segments.
Six synthetic return fragments cover nonempty success, empty-name failure,
and IOResult words 0001, 0100, 8000 and FFFF. Original alias order,
truncation, all CPU fields, defined flags, AH preservation and every RAM
byte agree. The natural fragments and fourteen synthetic cases total
**390 C instruction comparisons**. Strict C and the complete differential
suite also pass ASan/UBSan.

Strict checks cover failed operand/call/return/copy spans, the full-word
global writer, stale reports, unknown PC, null arguments and RAM size.
All unsupported states preserve CPU/RAM at the current instruction.
Host spans must fit one 64 KiB segment and an image <= 1 MiB; individual
pointer arithmetic wraps to sixteen bits. Multi-byte segment/A20 wrap and
unsupported DF-set string steps remain blocked. Original/loaded code
identity is a caller prerequisite validated by the audit.

The first C stop is +0310 -> Assign AF8:13E2, return +0315. Subsequent
independent stops are +031F -> Reset 1410, +032A -> Close 1491, and
+032F -> IOResult 04A2. After an independently supplied native IOResult
return state, C stops unchanged at +19AD before executing any caller code.
The +19AD return/next-observable-state task is queued as a second distinct
branch; this PR supplies no caller continuation. Existing B98 and older
adapter gates stay unchanged, and no general dispatcher is added.

The merged 0425 timeout/progress guard and immutable PR #410 document are
untouched. Tests have a 1000-instruction/30-second per-fragment bound and
a 180-second process bound with five-second kill grace. Original media,
RAM images, debugger stream and screenshots remain ignored. No ordinary
saved-A end-to-end C parity, playable-route or identity/full-C count credit
is claimed; linked-library versus game ownership remains unresolved.

- `target_function`: reached +19A8 probe prefix, native entry/return ABI,
  and independently seeded resident fragments through RETF 4.
- `source_candidates`: slot_a_file_probe_02eb_abi.c and the existing
  instruction-level B25 API; the callback selector is context only.
- `signals`: original/loaded code identity; same-run natural full state;
  actual runtime frames; instruction/flag/writer parity; bounds/alias cases.
- `score`: five checked signal categories, not a probability.
- `verdict`: bounded `match` for the verified C fragments; `unclear` for
  unknown runtime effects, library/game identity and complete C probe parity.
- `review_notes`: C stops before unsupported far-call pushes. No snapshot
  bridges or injected runtime results connect the independent fragments.

Run `bash tests/replay/check_slot_a_file_probe_02eb_abi.sh` for media-free
strict checks. Full private reproduction:

```sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
  bash tests/replay/check_slot_a_file_probe_02eb_abi.sh \
  resources/oracle-runs/slot-a-probe-02eb-abi-20261014
```

Unicorn 2.1.4 and non-optimized Python assertions are required. The six new
files need no shared CMake changes; registration remains with integration.
