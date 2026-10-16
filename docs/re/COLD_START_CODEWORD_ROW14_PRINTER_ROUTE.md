# U19 row-14 printer route and EGA store candidates

The fixed codeword row-14 source has a direct route from U19 `+02D8` through
resident `0709:0626` to four EGA store instructions in its glyph drawer.
This is a **static source route**. It does not identify the first
value-changing guest store or explain why the host screenshot at U19 `+02DD`
still differs from the source by 477 pixels. The first-value and display
presentation boundaries remain open.

Before this audit, bounded `deja "Poolrad U19 +02D8 row14 fixed printer
RES_0709:0626 EGA store 0417 0501"` timed out. The merged
[fixed-text source](COLD_START_CODEWORD_SOURCE.md) supplied the six print
calls and argument layout; the [font/drawer contract](FONT_8X8D_201.md)
supplied the EGA mode and plane behavior; the
[paired-frame boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md) supplied the
same-run mismatch and its later display-change interval. Their address
claims were independently checked against the ignored originals here.

## Original code route

Ignored `START.EXE` and `GAME.OVR` have SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
These span hashes refer to original, pre-relocation code:

| Span | Size | SHA-256 | Verified route |
| --- | ---: | --- | --- |
| U19 `+02C0..+02E2` | 34 | `144b7073fef51cd92e155eaf08dd5c679772c7788a989432d2bafc95356647ce` | low-byte column 1, row 14, color 10; fixed source `+00A9` |
| U19 `+02D8..+02DD` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` | far call `0709:0626` |
| `RES_0709:0626..06A7`, image `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` | printer `+0691` near call to drawer `+01DF` |
| drawer EGA path, image `[0x7417,0x7501)` | 234 | `3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4` | mode-1 path and planar stores |

U19 `+02D3` first calls runtime Pascal copy `0AF8:0B0B` from the fixed
length-36 source at `+00A9`. Its `+02D8` call passes that local string to
the resident printer. The printer's `+0691` call enters drawer `+01DF` once
per accepted character. The drawer's image `0x7417` compares the display
mode to one before entering the EGA branch. At the captured `+02DD` stop,
the display words `DS:[4A16,4A18,4A1A,4A1E]` are
`[A000,A000,0000,0001]`. This stop records the page/mode state; it is not a
per-instruction trace of the earlier printer call.

Within the hash-locked EGA path, the original store instructions are:

| Image address | Resident offset | Static operation |
| --- | --- | --- |
| `0x74A2` | `0709:0412` | glyph byte to first page segment |
| `0x74C8` | `0709:0438` | glyph byte to second page segment |
| `0x74D5` | `0709:0445` | zero byte to first page segment |
| `0x74E1` | `0709:0451` | zero byte to second page segment |

The path selects an EGA plane mask before these stores and advances one byte
per glyph row. Whether a glyph or zero store changes a plane depends on the
previous plane contents. The two captured page-segment words both equal
`A000`; their equality does not reveal the four EGA plane images or the
host's refresh timing.

## Measured stop and open boundary

The ignored U19 `+02DD` RAM capture is 655,360 bytes, SHA-256
`84191adc6aab0f6272fe0dc128b1cbff06668a01c9c40fd5fc1202d490b06699`.
Its 640×400 canonical RGB image hashes
`5044229ac07b046aaf144113a90e8fec64edcfef47f978d843319991603dabcb`.
Against the original row-14 fixed source and captured font, this screenshot
has **477 differing source pixels across 19 cells**. The paired-frame audit
shows the mismatch persists at `+02E2` and `+02FD`, then becomes zero at
`+0302`, during the line-input call interval. That interval does not prove
which guest instruction first changed EGA memory; a synchronized EGA plane
snapshot or bounded write watch is needed. This note gives no native,
full-frame, full-C, or identity credit.

Reproduce the read-only source audit with ignored inputs:

```sh
python3 tools/replay/cold_start_codeword_row14_printer_source_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --ram resources/oracle-runs/codeword-row14-p5-001/at-u19-02dd.ram \
  --png resources/oracle-runs/codeword-row14-p5-001/at-u19-02dd.png
```

Only addresses, hashes, and counts appear in public files. The original
code, font, RAM, and screenshots stay under ignored `resources/`. The
separate U17 first-credits border audit is outside this U19 codeword scope.
