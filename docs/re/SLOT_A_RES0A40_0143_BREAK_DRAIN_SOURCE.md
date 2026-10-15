# Loaded RES_0A40:0143 conditional break and keyboard-buffer drain source

The source-only `1264:030F` keyboard routine near-calls loaded
`1264:0143` at `1264:032D`. This target is physical
`0x12783`, original image `0A40:0143` physical `0xA543`, and START
file `0x0A23C`. The bounded body ends at `INT 23h` at `1264:0169`;
the next prologue begins at `1264:016B`. This is source extent, not
a native visit or a claim that the interrupt returns or terminates.

`deja "Poolrad 1264:0143 conditional break keyboard buffer drain static source extent"`
found no indexed session. pC cleared static source and extent ownership
in PM inbox i5814, which also supplied the 40-byte candidate extent;
the original and loaded bytes and each instruction were checked here.

| Item | Address / count | SHA-256 |
| --- | --- | --- |
| `START.EXE` | 65231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Loaded RAM | 655360 bytes | `a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f` |
| START body | `0x0A23C..0x0A263`, 40 bytes | `13a55513f0a200a1881c7a6390aa09771c86074b92c70ca5511ff9794e79b9ce` |
| Loaded body | `0x12783..0x127AA`, 40 bytes | `13a55513f0a200a1881c7a6390aa09771c86074b92c70ca5511ff9794e79b9ce` |
| First decision prefix | START `0x0A23C..0x0A243`, loaded `0x12783..0x1278A`, 8 bytes | `33829488593660a57d88cc10534e3e439fd5ce6c75742aedfec89696001df063` |
| `030F` caller near CALL | START `0x0A426..0x0A428`, loaded `0x1296D..0x1296F`, 3 bytes | `5a71529d8f099022e64f13de73e379c00ccebccc5939da4f1ced9e93fec85b13` |

The 40-byte body matches loaded RAM exactly, has one START occurrence,
and decodes to 16 instructions: three branches, three near calls,
three interrupts, one early RET, and no terminal return after
`INT 23h`. The initial compare tests `DS:8502` against zero. Zero
falls through to RET. Nonzero branches to a clear of `DS:8502`, then
sets AH=1 and reaches `INT 16h`. The `JE` after that interrupt depends
on supplied BIOS flags. Its fallthrough sets AH=0, reaches a second
`INT 16h`, and jumps back to the AH=1 poll. The taken branch sets
AL to `0x5E`, calls loaded `1264:047E`, sets AL to `0x43`, calls that
helper again, calls loaded `1264:0477`, then reaches `INT 23h`.

The first `INT 16h`, its return flags/AX, the second `INT 16h`, the
two called helpers, and `INT 23h` are external boundaries. The source
shows a conditional break/keyboard-buffer-drain path, but this audit
does not assert the keyboard queue, visible text, interrupt effect,
or caller return. Original and loaded physical addresses differ by
`0x08240`; original and START file addresses differ by `0x0307`.

Reproduce with
`python3 tools/replay/slot_a_res0a40_0143_break_drain_source_audit.py`.
No native reach, C parity, or full-C count is claimed. Original media
and RAM stay ignored under `resources/`.
