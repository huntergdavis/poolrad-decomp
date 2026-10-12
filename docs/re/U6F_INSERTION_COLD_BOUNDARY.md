# Unit 0x6F +0x1446 first-insertion cold dispatch boundary

The `+0x1446..+0x157E` insertion routine reaches a far call at
`+0x1574` through stub `00AC:0025`. The stub's descriptor selects unit
`0x9B:+0x0000`, whose complete C translation is merged. On the saved
**first** insertion, the call also reaches resident `INT 3Fh` and its
loader before the u9B entry. A direct call from the existing u6F
SS-memory model into the u9B C function does not reproduce that cold
dispatch's RAM writes or establish all input registers.

Before this audit, `deja "Poolrad u6F 1446 157E full SS frame insertion
u9B 0000 dispatch parity"` found no exact session. The shorter
`deja "u6f 1446"` recovered the earlier party-build research. This
check reuses the [SS-frame model](U6F_PARTY_INSERT_1446_SS.md), the
[corrected stub map](U6F_U9B_TRANSFER_1574.md), the [runtime
dispatch audit](U9B_RUNTIME_DISPATCH_0000.md), and the reviewed
[u9B full-state translation](U9B_ENTRY_ZERO_COMPOSITION.md). It
independently rechecks their original executable hashes, call operand,
stub and handler spans, and all nine pinned private RAM captures.

| Original span | Bounded evidence |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| u6F `+0x1446..+0x157E` | 313 bytes, 99 instructions, GAME file `0x1F42C..0x1F564`; SHA-256 `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` |
| Handler `0xAA2:+0x02E6..+0x0337` | 82 bytes; SHA-256 `6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c` |
| Loader subpath `0xAA2:+0x0338..+0x03E8` | 177 bytes; SHA-256 `58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991` |

## Measured first-insertion handoff

The saved pre-call frame has `SS=1D16`, `SP=379A`, argument words
`0008:449A` at `SS:379A/379C`, and caller `CS=2A8E`. The far call
pushes return `2A8E:1579` at `SS:3798/3796`. The `INT 3Fh` frame
then saves stub `00AC:0027` and is rewound to `0025`. The handler
loads unit 0x9B, patches the stub to a far jump, returns through the
runtime dispatch, and reaches `2CCD:0000`. The nine hash-locked native
captures verify this route and the u9B return to `+0x1579`.

The following counts compare consecutive **640 KiB RAM snapshots**;
they are changed byte counts, not instruction or unique-write counts.

| Native checkpoint | Changed RAM bytes | Contiguous address runs |
| --- | ---: | ---: |
| Pre `+0x1574` to handler `+0x02E6` | 10 | 1 |
| Handler entry to pre-loader `+0x0314` | 23 | 2 |
| Pre-loader `+0x0314` to pre-indirect `+0x032A` | 1,188 | 22 |
| Pre-indirect to u9B `+0x0000` | 3 | 2 |
| **Pre `+0x1574` to u9B `+0x0000`** | **1,214** | **20** |

The net changed ranges include transient SS stack
`0x208B8..0x208F9`, resident/stub bookkeeping around
`0x0816E..0x18DF7`, and loaded overlay memory
`0x2CCD0..0x2D132`. These are physical addresses in the saved
640 KiB image. The accompanying audit prints all 20 exact runs,
with no byte values. The differing stage totals do not add to the
net count because later writes can restore or replace earlier values.

## C parity boundary

`pr_u6f_party_insert_1446_ss` models pre-transfer 1 MiB RAM effects
and stops at `+0x1574`; its API has no full CPU register result.
`pr_u9b_entry_zero_0000` models full RAM and register effects from
an already dispatched `+0x0000` entry. The pinned saved u9B capture
contains RAM but no complete entry register record, and this local
capture set has no complete `+0x1446` entry CPU checkpoint. The
measured cold handler/loader effects above are also outside both C
APIs. Thus this work does **not** claim full u6F translation, saved
first-insertion whole-route parity, strict C credit, or a new game
identity.

The next dependency is a full-state model of the reached resident
`0xAA2:+0x02E6` handler and `+0x0338` cold-loader path, including its
stack/register effects and overlay-load writes. A native capture of
all registers and RAM at u6F `+0x1446` entry and u9B `+0x0000`
entry would fix the remaining saved-route input state. Those inputs
would allow a single original-byte full-RAM/register comparison across
the composed routine.

Reproduce locally with ignored DOS binaries and private capture files:

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/u6f_insertion_cold_boundary_audit.py \
  --native-dir resources/oracle-runs/u9b-entry-zero-runtime-002
```

The audit emits only hashes, counts, and addresses. No original code,
RAM, saves, or game media are committed.
