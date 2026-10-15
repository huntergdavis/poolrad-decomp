# RES_0709:0626 runtime-copy return and first control gate

This bounded C composition starts at the first `0AF8:0B25` call made by
`RES_0709:0626`. It executes the actual 36-byte runtime copy body, compares
the return state at `0709:0641`, then executes the printer's column and row
guards. In-range input stops at `0709:064F`, before glyph side effects.
Out-of-range input executes the printer epilogue and stops at the caller's
`00BA:1858`. No strict full-C or game-function credit is claimed.

## Pinned original spans

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `resources/DOS/POOLRAD/START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `resources/DOS/POOLRAD/GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Original span | Source mapping | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| SYSTEM `0AF8:0B25..0B49` | START file `[0x0B79B,0x0B7BF)` | 36 | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |
| printer guards `0709:0641..064F` | expanded `[0x076D1,0x076DF)` | 14 | `9dc9d7a58ae7e84af2cb712b67ffd086bcb1920d31d453b86c7178d36f258bfa` |
| printer epilogue `0709:06A1..06A7` | expanded `[0x07731,0x07737)` | 6 | `c7918d1a892076e6a8394f8cae0f325efe62679cef918facbb20000424f87559` |

The runtime body clears DF, loads the source and destination far pointers
and maximum length from the call stack, reads the source Pascal length,
stores `min(length,limit)`, copies that many characters forward with
`REP MOVSB`, restores DS, and returns with `RETF 10`. The C transition
models each register and byte write in that order, including overlapping
source/destination behavior. The printer then compares the low bytes of
column `[BP+0E]` and row `[BP+0C]` with `0x27`. A rejected value reaches
the early `RETF 10`; both accepted values reach `064F`.

## Differential result and limits

`tests/recovered/check_resident_0709_0626_postcopy_control.py` executes
the pinned original overlay prefixes, `1816`, printer, and runtime copy
bytes in Unicorn. It compares C and original state at printer entry,
runtime-copy entry, runtime-copy return, and the final supported stop.
The first 24 cases retain the eight-site × three-color composed frames,
with synthetic source strings of lengths 0, 1, and 14. Three additional
cases exercise an out-of-range column, a shortened copy cap, and a 255-byte
source. **27/27 cases pass:** 24 stop at `064F`, and three take the early
printer return to `00BA:1858`. Each boundary compares all 14 CPU fields
and every byte of 1 MiB RAM. AF after the runtime's `XOR CH,CH` is
architecturally undefined and is excluded from flag comparison; other
meaningful flag bits are checked. C compiles under `-std=c11 -Wall -Wextra
-Wpedantic -Werror`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_0626_postcopy_control.py
```

The accepted path after `064F` still prepares glyph calls and changes EGA
memory; it is outside this model. The early guard return is proven only
for the tested original-byte frames, including one explicit column
intervention. A same-run full printer return after a valid glyph path
remains queued once its effects can be measured. The separate `0709:0768`
gate is untouched.

Before implementation, bounded `deja "Poolrad 0AF8:0B25 Pascal runtime
string copy exact bytes effects"` timed out; no prior-session assertion
was reused. Merged `RESIDENT_STRING_0356.md` and
`RESIDENT_0709_0626_FIRST_COPY_GATE.md` supplied source-address leads,
which were checked again against the ignored original inputs. Original
media and full RAM stay under ignored `resources/`; only addresses,
counts, and hashes are published.
