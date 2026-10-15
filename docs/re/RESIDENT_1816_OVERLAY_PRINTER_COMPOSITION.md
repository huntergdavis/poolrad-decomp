# Eight overlay callers through RES_00BA:1816 to first printer entry

This composition joins the merged eight-site overlay caller model with the
merged `RES_00BA:1816` first-printer gate. Its final state is the first
instruction of `RES_0709:0626`, before the printer body executes. It claims
no strict full-C or game-function credit.

## Pinned sources and route

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `resources/DOS/POOLRAD/START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `resources/DOS/POOLRAD/GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

The overlay prefix inventory and its per-site SHA-256 values are pinned in
[`RESIDENT_1816_OVERLAY_CALLER_ROUTES.md`](RESIDENT_1816_OVERLAY_CALLER_ROUTES.md).
The resident prefix `[00BA:1816,00BA:1858)` has SHA-256
`7b68709c7c03334a3b277bf7e2b100849407e2bf09c1f17b1e34f03e578c7d99`.
The C composition verifies callback events in order: node segment, node
offset, column, row, flag, then far call. It pushes the overlay return
address, executes the C 1816 gate, and stops at `0709:0626` with return
address `00BA:1858`. The eight static call sites are:

| Unit | Offset |
| --- | ---: |
| `05E` | `+0372` |
| `06C` | `+2D4F` |
| `077` | `+0089` |
| `077` | `+0DE0` |
| `077` | `+11CC` |
| `077` | `+2AB6` |
| `07F` | `+1B50` |
| `086` | `+33C3` |

## Differential result

`tests/recovered/check_resident_1816_overlay_printer_composition.py`
executes the original overlay prefix and original resident prefix together
in Unicorn. It compares their printer-entry state against the C composition
for each of the eight sites and each of the three color branches: **24/24
cases pass**. Every case compares all 14 CPU fields (architecturally
meaningful flag bits), every byte of 1 MiB RAM, the seven-word printer-entry
stack, and the six ordered C callback events. The C sources compile with
`-std=c11 -Wall -Wextra -Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_1816_overlay_printer_composition.py
```

The composed state is a call boundary. The effects and return from the
`0709:0626` printer, the flag-dependent suffix of `1816`, and any actual
overlay caller return are unresolved dependencies. The existing EGA page
model establishes one page result, but not the printer's full state effects.
An eight-site end-to-end return claim must stop here until those effects
are independently verified. Static call sites alone do not establish live
reachability of every overlay path.

Bounded `deja "RES_00BA:1816 eight overlay caller printer gate composition"`
timed out before implementation; no prior-session assertion was reused.
The merged caller-route and first-printer-gate evidence were used as leads,
then the exact original prefixes and complete machine state were compared
again in this joined test. Private media and RAM stay in ignored `resources/`.
