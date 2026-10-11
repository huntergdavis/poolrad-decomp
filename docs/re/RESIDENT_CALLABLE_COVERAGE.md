# DOS v1.3 resident callable coverage and classification

This is the bounded resident follow-up to [issue #31](https://github.com/huntergdavis/poolrad-decomp/issues/31)
and the [merged loader map](RESIDENT_LOADER_MAP.md). The
[address-only ledger](resident_callable_ledger.csv) preserves all 86 `RES_`
IDs and 48 `RPRO_` IDs and adds 11 `RNEAR_` IDs. Every unresolved candidate
is explicitly identified in that ledger. These are **candidate starts**, not
an exhaustive function list or a whole-game denominator.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Reconciliation and classification

The merged audit found **86 overlay-called non-SYSTEM targets**: 84 exact
Pascal prologues and two `0xA40` starts immediately after `RETF`. It found
48 other raw prologue hits. This audit retains those IDs and source offsets.
A Capstone 5 candidate-seeded path walk closes over direct intra-segment near
calls, adding **11** directly targeted starts outside the exact-prologue scan.
Some of these use another prologue form; their absence from the raw scan is
not evidence of data.

| Candidate set | Game verified | Linked CRT inferred | SYSTEM/runtime | Proven data/false hit | Unresolved | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 86 overlay-called `RES_` | 6 | 2 | 0 | 0 | 78 | 86 |
| 48 other `RPRO_` | 1 | 1 | 0 | 0 | 46 | 48 |
| 11 direct-near `RNEAR_` | 0 | 6 | 0 | 0 | 5 | 11 |
| **Audited candidates** | **7** | **9** | **0** | **0** | **129** | **145** |

The separately known **48 `0xAF8` SYSTEM targets** remain outside this
non-SYSTEM candidate set. No raw prologue has been proved to be data or a
false hit. The CRT labels remain **high-confidence inferences from an API
neighborhood and call graph**, not exact matches to a stock Borland object.
The two previously classified CRT calls are `RES_0A40_029C` and
`RES_0A40_02FD`; the new inferred CRT initialization candidate is
`RPRO_0A400`. Six new near targets in its local video, file-setup, and
timing helper graph are also marked inferred CRT. The other five new near
targets (`RNEAR_09E5C`, `RNEAR_09E91`, `RNEAR_0A222`, `RNEAR_0A28E`,
`RNEAR_0A2FA`) stay unresolved. Unit membership alone is not promoted to a
verified library match.

### Program entry corrects the former `RPRO_000F6` uncertainty

The copied loader's final transfer uses its recorded target at loaded image
`0x0006`. A contiguous, relocation-backed sequence of **48 far calls** spans
`0x0006..0x00F5`, with initialization targets including `0xA40:0000` at
site `0x000B`, `0xA31:0000` at `0x0010`, and `0xBA:309E` at `0x0074`.
The sequence falls through at `0x00F6` to `RPRO_000F6` (first packed source
file byte `0x002F6`). Its body makes game and SYSTEM calls and ends with the
SYSTEM termination call at `0x014C`; the reached SYSTEM path terminates via
DOS at loaded image `0xB121`. The path walk stops there rather than decoding
following stub/data bytes. This makes `RPRO_000F6` a **verified static game
program entry**, correcting the merged report's tentative data/stub reading.
`RES_00BA_1509`, `RES_00BA_15F7`, `RES_00BA_2199`, `RES_00BA_23B7`,
`RES_00BA_23DB`, and `RES_00BA_2E03` are the other
verified game routines, grounded in the
[character sidecar trace](../formats/DOS_CHARACTER.md) and the reviewed
[append](RESIDENT_SIDECAR_APPEND.md), [lookup](RESIDENT_SIDECAR_LOOKUP.md),
[field predicate](RESIDENT_CHARACTER_FIELD_ZERO.md), and
[party counter](RESIDENT_PARTY_COUNT.md), and
[link predicate](RESIDENT_CHARACTER_LINK_CHECK.md) translations. The supported
resident game-routine lower bound is therefore **seven**.

The only other raw prologue without a listed direct caller is
`RPRO_0AA20`. Its short body is coherent, but this audit found no
relocation-backed or decoded direct call to it. A callback, initialization
path, or unused routine remains possible. It stays unresolved.

## Direct, indirect, and executable-region evidence

The non-SYSTEM candidate closure decodes **170 direct near-call sites** to
**79 distinct targets**, including the 11 new starts. Its paths contain
**498 of 547** loader relocation-backed far-call sites. The 48-call
initialization sequence accounts for another 48. The sole site outside both
is image `0x8A9C`, a far call to `0xA40:02FD` in the body adjacent to
`RES_0899_0104`. The entry at `0x8A94` skips that block to its return;
the block has a local back edge but no listed ingress. This is an explicit
coverage gap, not proof the block is unreachable at runtime.

The non-SYSTEM paths decode **12,834 instruction starts / 32,545 bytes**,
plus the separate **240-byte initialization call sequence**. All decoded
candidate bytes have copy provenance from the loader; none has fill
provenance. A separate SYSTEM-seeded pass starts from 48 overlay-called
`0xAF8` targets plus its initialization and exit entries; near-call closure
reaches **80 seeds**, **1,827 instruction starts / 4,003 bytes**, and
**69 direct near-call sites**. The two decoded byte sets do not overlap.
The generated JSON breaks these observations into 4 KiB image bins. These
figures measure *the decoder's audited paths*, not the fraction of actual
executable code. Unseeded code, data, overlay stubs, and indirect entries
remain indistinguishable in uncovered bytes, so no executable-coverage
percentage is asserted.

No indirect `CALL` or `JMP` instruction appears on the non-SYSTEM candidate
paths. The separate SYSTEM paths contain six indirect call sites at loaded
images `0xB273`, `0xB337`, `0xB343`, `0xB581`, `0xB77B`, and `0xB80C`.
Startup initializes a pointer at `0x4414` to `0xAF8:00A9` at images
`0xAFB7` and `0xAFBD`; sites `0xB337` and `0xB343` call through that
pointer. `0xAF8:00A9` is thus a **plausible target while the pointer retains
its startup value**. The other four indirect targets, possible later pointer
changes, return-based transfers, and INT 3Fh overlay dispatch are not resolved by
this walk. The [merged loader map](RESIDENT_LOADER_MAP.md) records the
separate 404 overlay stubs and handler evidence.

## Reproduce and limits

The tool imports the merged hash-locked unpacker and overlay fixup audit,
then asserts the entry sequence, far-call partition, near-call closure,
indirect sites, and candidate counts. It emits offsets, counts, hashes, and
classification labels only; decoded instructions and image bytes stay in
memory. Capstone 5.0.7 is used for both bounded path walks.

```sh
python3 -m pip install --target /tmp/poolrad-capstone capstone==5.0.7
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_callable_coverage.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --json /tmp/resident_callable_coverage.json \
  --csv /tmp/resident_callable_ledger.csv
cmp /tmp/resident_callable_ledger.csv docs/re/resident_callable_ledger.csv
```

`deja "resident callable coverage library classification START.EXE"` found
no prior exact match; broader `deja "resident"` recalled Claude's
START.EXE self-relocation lead and the earlier resident-inventory sessions.
The [merged PR #27 inventory](RESIDENT_FUNCTION_INVENTORY.md) supplied the
overlay-called IDs and SYSTEM partition. [Merged PR #30's loader map](RESIDENT_LOADER_MAP.md)
supplied byte provenance and the 134 initial candidates. Its tentative
`RPRO_000F6` assessment is corrected above. The existing Capstone
candidate-seeded method and overlay descriptor/fixup parser are reused;
Claude's overlay files are not edited.

There is **no defensible upper bound** for all resident game routines yet:
134 candidates remain unclassified, raw-prologue scans miss other encodings,
and indirect or unseeded code may add starts. This report makes no
whole-game percentage or exact denominator claim.
