# DOS v1.3 resident loader map and called-code evidence

This is the START.EXE follow-up to [issue #24](https://github.com/huntergdavis/poolrad-decomp/issues/24)
and [issue #28](https://github.com/huntergdavis/poolrad-decomp/issues/28).
The [address-only ledger](resident_loader_ledger.csv) has stable IDs for the
86 overlay-called non-SYSTEM resident targets and the 48 other loaded-image
prologue hits. `tools/overlay/resident_loader_map.py` reproduces it from the
ignored owner binaries. File offsets identify the **first source byte** used
for each loaded address; they do not imply an entire routine is contiguous in
the packed file.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## What the START.EXE entry does

The MZ header has a 512-byte header, zero DOS relocation entries, and entry
`0xF72:0x0012` at file `0xF932`. Its instructions, checked on the exact hash,
explain the otherwise discontinuous packed-file mappings:

| Entry instruction offsets (file) | Observed operation |
| --- | --- |
| `0xF932..0xF957` | Compute a destination from the load segment, copy `0x5AF` loader bytes backward with `STD; REP MOVSB`, then far-transfer into the copy. |
| `0xF958..0xF9BC` | Scan trailing padding, then expand backward. Controls `0xB0/0xB1` fill a run; `0xB2/0xB3` copy a run; the low bit ends expansion. |
| `0xF9BE..0xF9FC` | Walk the loader's own relocation stream, adding the load segment to segment words. |
| `0xF9FE..0xFA1E` | Set the runtime stack and far-transfer to the expanded program entry. |

The source walk starts at loaded-image offset `0xF71E` and ends at `0x150`.
Its 440 copy and 449 fill runs write `0x151..0x10BEF` (68,255 bytes).
The relocation stream occupies file `0xFA4D..0xFECF`: 561 unique sites in
the first of 16 groups, ending exactly at EOF. Of those sites, 547 have a
nearby far-call opcode. The tool follows the copy/fill provenance in memory;
it writes no expanded image to Git or to its JSON/CSV output.

For a resident call target, the **loaded** position is
`16*segment + offset`. The first packed source file offset is
`0x200 + origin[loaded_position]`, where `origin` is the expansion's per-byte
source map. Thus the shifts reported in [PR #27's inventory](RESIDENT_FUNCTION_INVENTORY.md)
are source-to-loaded deltas caused by the backward expansion, not relocation
bases to apply to a whole segment.

| Called segment | First-byte source shift(s) | Targets | Loader result |
| --- | --- | ---: | --- |
| `0xBA` | `−292` (24), `−316` (13), `−330` (5) | 42 | Confirms all three old groups |
| `0x3F1` | `−446` | 3 | Confirms old mapping |
| `0x464`, `0x709` | `−455` | 26 | Confirms old mapping |
| `0x802` | `−457` | 7 | Confirms old mapping |
| `0x899` | `−457` (1), `−481` (3) | 4 | Resolves the split without a guessed instruction alignment |
| `0x8AE` | `−489` | 2 | Selects this shift over the accidental `−948` prologue alignment |
| `0xA40` | `−1287` | 2 | Newly maps both previously unsupported targets |

All 84 prior source offsets equal the loader-derived positions. The
`0x899` and `0x8AE` choices therefore have direct expansion provenance in
this executable. Both `0xA40` targets also have unique source positions:
`RES_0A40_029C` loads at image `0xA69C` from file `0xA395`, and
`RES_0A40_02FD` loads at image `0xA6FD` from file `0xA3F6`. Neither begins
with the Pascal prologue. Each begins immediately after a `RETF`, at loaded
`0xA69B` and `0xA6FC` respectively, and has a fixup-confirmed incoming
overlay call. The revised comparison with PR #27 is **86/86 source-mapped,
84 exact-prologue starts, and two nonprologue starts**; it is not a new claim
that 86 Pascal prologues exist.

## Game versus linked-library classification

The ledger retains one verified game-specific target, `RES_00BA_1509`, from
the character sidecar trace. It marks the two `0xA40` calls as
`linked_crt_inferred` with high confidence, rather than as proven stock
library object code. Instruction neighborhoods support the inference:

| Loaded address | Instruction evidence |
| --- | --- |
| `0xA40:0x029C` | Counted busy-wait loop repeatedly reading BIOS data-area address `0x40:0x0000`; far-return with one word argument. |
| `0xA40:0x02C9` | Adjacent speaker-frequency routine writes timer and speaker I/O ports. |
| `0xA40:0x02F6` | Adjacent speaker-off routine clears the enable bits. |
| `0xA40:0x02FD` | Polls buffered-key state, otherwise invokes BIOS keyboard `INT 16h` with `AH=1`; far-returns a Boolean. |
| `0xA40:0x030F` | Adjacent keyboard-read routine invokes BIOS keyboard `INT 16h` with `AH=0`. |

These functions occur in the same order and with the same roles as the CRT
unit's Delay, Sound, NoSound, KeyPressed, and ReadKey family in the
[Borland Turbo Pascal 7.0 Language Guide, table 13.4](https://turbopascal.nl/docs/Turbo_Pascal_Version_7.0_Language_Guide_1992.pdf).
That later manual establishes the library API family, not an exact object
match to this 1989 executable. Of the 86 overlay-called targets, the current
split is **one verified game-specific, two inferred linked CRT, and 83
unresolved**. The 48 separately identified `0xAF8` targets remain Turbo
Pascal SYSTEM/runtime per the earlier instruction evidence.

## The 48 other prologue hits

The expanded image has 132 `55 89 E5` hits. Exactly 84 coincide with the
overlay-called resident targets; 48 do not. The loader relocation stream
supports 18 of those 48 through a far-call site. An optional Capstone 5 path
walk, seeded at all 132 prologue candidates and the two called nonprologue
starts, decodes 158 direct near-call sites and finds 31 of the 48 as targets.
Three have both forms of evidence, so **46 of 48** have at least one listed
direct caller. This is candidate-seeded reachability, not proof that every
seed is a function or that indirect dispatch is exhausted.

Two hits have neither listed call form:

- `RPRO_000F6` (source file `0x002F6`) lies below the expansion start in the
  untouched prefix. Its candidate-seeded decode fails; it may be a byte
  pattern in data or a stub, and is not promoted to a function.
- `RPRO_0AA20` (source file `0x0A716`) decodes as a short prologue-bearing
  path that calls `0xAF8:0x00E2` and returns. No direct caller was found by
  these two audits; an indirect or initialization entry remains possible.

The CSV records the 48 candidate IDs, their first source offsets, and counts
of relocated far and decoded near call sites. Blank near-call counts mean the
optional pass was not run; this committed ledger was generated **with** it.
Raw prologue hits are not added to a full game denominator.

## Related INT 3Fh stub evidence

The resident install at image `0xAAF8` sets `AX=0x253F`, `DX=0x02E6`,
and `DS=CS`, placing the overlay handler at `0xAA2:0x02E6` (image `0xAD06`,
source file `0xA9FC`). That handler reads the word following a stub's
`INT 3Fh` at image `0xAD1A`, rewinds the saved IP at `0xAD1D`, and later
passes the word through `BX` at `0xAD40`. A separate runtime installation
sets a handler at `0xAF8:0x00D7`.

The tool compares all 404 live stub words with their raw GAME.OVR entry
words: **399 equal**, while exactly five raw words exceed the live stub word
by `0x800`. These are the five entries in Claude's
[overlay ledger PR #29](https://github.com/huntergdavis/poolrad-decomp/pull/29).
The callable offset is already normalized in each resident stub before the
handler reads it. This address/instruction finding was
[posted to issue #26](https://github.com/huntergdavis/poolrad-decomp/issues/26#issuecomment-6104586775)
for Claude's cross-check. The reason for the raw-table `+0x800` remains
unresolved; no claim about its linker encoding is made.

## Reproduce and limits

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --json /tmp/poolrad-resident-loader-map.json

# Optional: install the audited decoder outside the repository.
python3 -m pip install --target /tmp/poolrad-capstone capstone==5.0.7
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --near --csv /tmp/resident_loader_ledger.csv
cmp /tmp/resident_loader_ledger.csv docs/re/resident_loader_ledger.csv
```

The core audit uses only Python's standard library and existing repository
parsers. `--near` requires Capstone 5 and uses the same candidate-seeded
boundary approach as `validate_near_calls.py`. Both modes are hash-locked and
assert the measured counts; neither exports executable bytes or text.

Prior work reused after `deja "resident"` and `deja "segment mapping"`:
Claude's preliminary START.EXE self-relocation observation led to the entry
trace, and [PR #27's resident inventory](RESIDENT_FUNCTION_INVENTORY.md)
supplied the 86 target IDs and empirical source offsets. The existing
overlay descriptor/fixup parser and PM's Capstone near-call method were
reused without changing lane A files. The remaining 83 game/library
classifications, indirect callers, candidate seed validity, and full
executable coverage keep the whole-game denominator unresolved. No
completion percentage is reported.
