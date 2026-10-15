# RES_0709:0626 printer first-copy gate

This bounded C transition starts at the exact first-printer entry state
established by the eight-site `1816` composition. It executes the printer's
first 27 bytes and stops at its first external call, `0AF8:0B25`. The
runtime copy body and printer return remain unresolved. No strict full-C
or game-function credit is claimed.

## Pinned boundary

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `resources/DOS/POOLRAD/START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `resources/DOS/POOLRAD/GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

The loader maps `0709:0626..0641` to expanded image `[0x076B6,0x076D1)`.
Its 27 bytes have SHA-256
`372493c3602f9a20a394587515421dd79b7fdb8932a2c728bd4ec390d8647a4b`.
Capstone decodes no local branch before the `CALL FAR 0AF8:0B25` at `063C`.
The complete printer body `0626..06A7` is 129 bytes with SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`.
Only addresses, counts, and hashes are published.

The prefix saves BP, allocates `0x104` local bytes, loads the source far
pointer from `[BP+6]`, and pushes source segment/offset, destination
`SS:BP-0x100`, and maximum length `0x00FF`. The far call pushes return
`0709:0641`, leaving seven words at the copy entry: return IP, return CS,
maximum length, destination offset, destination segment, source offset,
source segment. The C model includes the 16-bit `SUB SP,0x0104` flags and
real-mode address wrap. It does not invent the callee's copy effects.

## Original/C parity

`tests/recovered/check_resident_0709_printer_copy_gate.py` starts from the
same eight original overlay call prefixes and three 1816 color branches
used in the merged 24-case composition. It executes those original bytes
and the pinned printer prefix in Unicorn, then checks the C composition
at printer entry and this C transition at copy entry. **24/24 baseline
cases pass**, comparing all 14 CPU fields (architecturally meaningful
flags), every byte of 1 MiB RAM, ordered caller events, and the copy-entry
stack. Three additional caller-derived stack seeds exercise carry, zero,
and overflow results of `SUB SP,0x0104`; **27/27 total pass**. The C sources
compile with `-std=c11 -Wall -Wextra -Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_printer_copy_gate.py
```

The call boundary stops before `0AF8:0B25` executes. The printer's first
control branch at `0645`, later glyph calls, return to `1816`, optional
suffix, and overlay returns remain open. The separate `0709:0768` printer
helper is queued as a distinct follow-on.

Before implementation, bounded `deja "RES_0709:0626 printer prefix first
runtime string copy 0AF8:0B25"` timed out. No past-session assertion was
reused. The merged 1816 composition was used as an entry-frame lead, and
the original prefixes and full state were checked again here. Private
media and RAM remain under ignored `resources/`.
