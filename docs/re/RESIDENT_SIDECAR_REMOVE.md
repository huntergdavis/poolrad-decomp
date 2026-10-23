# Full C translation: resident sidecar-list removal

`pr_rec_res_00ba_1509` translates the complete external-effect path of
`RES_00BA_1509` from DOS v1.3 `START.EXE`. The entry is loaded
`0x00BA:0x1509` (image `0x20A9`), with first packed source byte at file
`0x2185`. The [loader map](RESIDENT_LOADER_MAP.md) establishes that mapping;
the [resident inventory](RESIDENT_FUNCTION_INVENTORY.md) records ten
fixup-confirmed overlay callers. The [function ledger](FUNCTION_LEDGER.md)
identifies unit `0x25` entry 21 as one caller during party-record traversal.

| Ignored input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## DOS contract and path evidence

The routine has 83 reachable instruction starts at loaded image
`0x20A9..0x2194`, with one shared `RETF 8` exit. Its first far argument at
`[BP+6,+8]` is the **node to remove**; the second at `[BP+0xA,+0xC]` is the
character-record base. The character's sidecar head is a far pointer at
`+0xC8/+0xCA`, consistent with the [DOS character load trace](../formats/DOS_CHARACTER.md).
Each 63-byte sidecar node's next far pointer is at `+0x2A/+0x2C`.
Far-pointer null tests OR both words; equality checks segment then offset.
Consequently a pointer with only one zero word is non-null.

A Capstone 5.0.7 path walk over the hash-checked expanded image found these
11 branch edges (instruction offset to target): `20B5→20D1`, `20CE→2191`,
`20ED→210E`, `20F2→210E`, `210C→213E`, `2114→213E`, `2124→212B`,
`2129→213E`, `213C→210E`, `2144→215F`, and `215D→2191`. Its five far-call
sites are `20C4`, `20C9`, `2153`, `2158`, and `218C`; the sole return is
`2194`. These offsets are loaded-image offsets, not file positions.

| Loaded instruction span | Independently transcribed condition/effect |
| --- | --- |
| `0x20AF..0x20CE` | A null target invokes the first error message path and exits before reading the character. |
| `0x20D1..0x210C` | Read the character's head. If it equals target, copy target's next pointer into the head before entering the shared success block. |
| `0x210E..0x213C` | Otherwise walk `current->next` until it equals target or the chain ends. A non-match reads `current->next` a second time before advancing. |
| `0x213E..0x215D` | Empty head or exhausted chain invokes the second error message path and exits without link writes or free. |
| `0x215F..0x2181` | On a match, copy `target->next` into `current->next`, then zero `target->next`. The head case writes target's link redundantly before clearing it; the translation preserves this order. |
| `0x2182..0x2194` | Call SYSTEM free with the original target far pointer and size `0x003F`, then return. |

The two error paths each copy a private Pascal literal into a local buffer
through `0xAF8:0x0B0B` (call sites `0x20C4`, `0x2153`), then pass that buffer
and low-byte parameter `0x0E` to the resident UI routine `0x709:0x0BF5`
(sites `0x20C9`, `0x2158`). The copied literal source offsets are `0x14CE`
and `0x14E2`; this repository contains their addresses only, not their
text. The UI return value is unused. The success call at `0x218C` targets
`0xAF8:0x0254`; its body reaches the SYSTEM heap free-list insertion at
image `0xB353`, confirming the release direction. Its terminal heap-error
path is represented by a release-callback failure status.

The C API models each 16-bit far-memory word access through caller-supplied
callbacks, including offset wraparound, both words of each pointer, and the
observed write order. Separate callbacks model literal copy, presentation,
and the 63-byte heap release. No static game string or real character bytes
are compiled in. The C return value is a **host outcome label**, not a
recovered DOS return register; the DOS routine simply returns through
`RETF 8` on its normal paths. Failed host memory accesses and the bounded
cycle guard are explicit adapter outcomes for states the original does not
safely handle. On valid acyclic lists with successful callbacks, all decoded
branches and external effects above are represented. This is one full
resident routine translation under the project's callback boundary, not an
extracted decision core.

## Synthetic verification

The new test independently fixes expected branch and callback traces for:
null target, empty head, head removal, interior and tail removal, missing
target, a cycle caught by the host guard, offset wraparound, one-word-zero
non-null pointers, unreadable host memory, and terminal release failure. It
also checks the unchanged head on non-head removal, cleared target link,
the original pointer and size passed to free, and error callback order.

```sh
cmake -S . -B /tmp/poolrad-issue-35-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-35-build
ctest --test-dir /tmp/poolrad-issue-35-build --output-on-failure
```

The strict local build passed all seven CTest targets, including
`poolrad_resident_remove_tests`. The hash-locked loader audit can be rerun
with ignored owner binaries to validate the address mapping:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --json /tmp/poolrad-resident-loader-map.json
```

Before implementation, `deja "RES_00BA_1509"` recalled the inventory ID and
Claude's earlier mention of the target; `deja "DOS character sidecar pointer
traversal"` had no exact hit. The merged [character trace](../formats/DOS_CHARACTER.md),
[resident inventory](RESIDENT_FUNCTION_INVENTORY.md),
[loader map](RESIDENT_LOADER_MAP.md), and
[callable coverage report](RESIDENT_CALLABLE_COVERAGE.md) supplied the field,
mapping, caller, and game-classification evidence reused here. No Claude
`ecl_helpers` source, header, or test file was edited.

The callback interface does not translate the called UI implementation or
SYSTEM heap manager, and synthetic tests cannot prove DOS execution parity
for invalid/cyclic pointers. Those dependencies and host-only failures are
explicit, while every branch and externally visible effect of this resident
routine on valid lists is included.

## Reproduce (original-byte checker)

`RES_00BA_1509` is checked against the original bytes by the registered checker `tools/overlay/res00ba_rec_models_audit.py`,
through the reusable io adapter `tools/overlay/res00ba_io_adapter.py`. What it compares is described in
[RES_00BA_REC_MODELS_AUDIT.md](RES_00BA_REC_MODELS_AUDIT.md).

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay \
  python3 tools/overlay/res00ba_rec_models_audit.py --output resources/res00ba-rec-audit/report.json
```
