# SYSTEM runtime leaves: FillChar, Move, set IN and UpCase (raw CPU)

`system_runtime_leaves_cpu.c` holds raw-CPU, full-state models of four
Turbo Pascal SYSTEM runtime leaves at loaded segment `0AF8`. The VP
approved them on 2026-10-17 as runtime models with **no strict or
identity credit**. The precedent is the
[SYSTEM string store](SYSTEM_STRSTORE_0B25.md). They exist so overlay
bodies can compose these calls as real far calls.

In the census-2115 candidate queue on main `64aae19`, each leaf is the only
unready callee of some bodies:

| Leaf | Bodies it alone blocks |
| --- | --- |
| FillChar (`runtime:5800`) | U1C `+266A`; U7F `+0508`, `+1010` and U3C6 `+03A0` also have callees modeled elsewhere |
| Move (`runtime:2623`) | U31 `+128D`, U3C2 `+002E` |
| set IN (`runtime:3520`) | U5E `+1BF8`, whose other callees are strict or modeled |
| UpCase (`runtime:6186`) | U7F `+0B93`, together with the string store |

The VP assigned U31 `+128D`, U1C `+266A`, U3C2 `+002E`, U7F `+0B93` and
U3B0 `+0B03` to this lane once these leaves land. Astra confirmed they own
none of them. The timeboxed `deja` recall failed once on an index update
and then reached its 90-second cap, so nothing is reused. The FillChar
bytes match the hash already cited in
[COLD_START_CREDITS_FILL_16B7_SOURCE_GATE.md](COLD_START_CREDITS_FILL_16B7_SOURCE_GATE.md).

## Exact originals

START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`, expanded
with `resident_loader_map.unpack`. Each body occurs exactly once in the file.

| Leaf | Loaded | File | Size | SHA-256 |
| --- | --- | --- | --- | --- |
| FillChar | `0AF8:16A8..16BB` | `0xC31E` | 20 bytes, 7 instructions, `RETF 8` | `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7` |
| Move | `0AF8:0A3F..0A56` | `0xB6B5` | 24 bytes, 9 instructions, `RETF 0Ah` | `5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa` |
| set IN | `0AF8:0DC0..0DE0` | `0xBA36` | 33 bytes, 13 instructions, `RETF 6` | `753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c` |
| UpCase | `0AF8:182A..183C` | `0xC4A0` | 19 bytes, 8 instructions, two branches, `RETF 2` | `738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c` |

## ABIs and ordered effects

Each leaf is entered after a far call with `SS:[SP]` the return IP and
`[SP+2]` the return CS. It sets BX := SP and reads its arguments through
`SS:[BX+disp]`.

**FillChar(var X; Count: Word; Value: Byte).** `[SP+4]` is the value byte,
`[SP+6]` the count and `[SP+8]/[SP+0Ah]` X. `LES DI,X`; CX := count;
AL := value; `CLD`; `REP STOSB` writes AL count times. DI wraps inside ES.
`RETF 8` reads its frame after the fill, so a fill over the frame changes
where it returns. On return CX = 0, DI is past the fill and ES is X's
segment.

**Move(var Source, Dest; Count: Word).** `[SP+4]` is the count,
`[SP+6]/[SP+8]` Dest and `[SP+0Ah]/[SP+0Ch]` Source. DX := DS;
`LDS SI,Source`; `LES DI,Dest`; CX := count; `CLD`; `REP MOVSB`;
DS := DX; `RETF 0Ah`. This SYSTEM's Move copies forward one byte at a
time, even when the regions overlap. A destination just above the source
therefore reads back bytes it has just written. On return CX = 0, SI and DI
are past the copy, DX holds the entry DS and ES is Dest's segment.

**set IN (e in S).** `[SP+4]/[SP+6]` is a pointer to a 32-byte `set of
byte`, and `[SP+8]` is the element e. `LES DI,S`; BX := e >> 3;
CL := e AND 7, keeping CH; AL := 1 SHL CL; `TEST ES:[BX+DI],AL`;
`RETF 6`. The `[BX+DI]` offset wraps at 16 bits.

**UpCase(Ch).** `[SP+4]` is the character. If 61h <= AL <= 7Ah, AL :=
AL - 20h. `RETF 2`.

**Flags.** FillChar and Move change only DF, which `CLD` clears. The set
test's final flags are `TEST`'s: CF = OF = 0 and ZF, SF and PF from the
masked byte. AF is architecturally undefined there; both oracles clear it,
and so does the model. The earlier `XOR`, `SHR`, `AND` and `SHL` writes
are folded. UpCase leaves the flags of the last `CMP` or of the `SUB`. AH
and every unnamed register keep their entry values.

**Addressing.** Offsets wrap at 16 bits and physical addresses at 1 MiB.
The `LES`/`LDS` pairs and the `RETF` frames are read physically contiguous,
as Unicorn reads them. Null RAM or CPU, or RAM that is not exactly 1 MiB,
returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_system_runtime_leaves_cpu.c` runs under CTest
with ASan/UBSan and compiles with `-Wconversion -Werror`. It uses synthetic
data only and compares the whole 1 MiB buffer with an expected copy. It
passes 2,924 checks:

- fills of 0 to FFFFh bytes that wrap DI, and a fill over the leaf's own
  frame;
- moves with overlaps from -7 to +7 bytes, the propagating +1 case, and SI
  and DI wrap;
- every element against a fixed set, including a set whose `[BX+DI]`
  wraps;
- every character for UpCase with independently computed flags;
- the argument guards for all four leaves.

**Unicorn differential.** `tools/overlay/system_runtime_leaves_audit.py`
places the expanded START.EXE image at load segment 0. It runs each
original at `0AF8:<entry>` and stops at the first fetch after its `RETF`,
wherever that goes. It compares the entire 1 MiB RAM and all 14 CPU words
with no mask:

| Leaf | Cases | Instruction starts | Conditional outcomes |
| --- | --- | --- | --- |
| FillChar | 63 | 7/7 | none |
| Move | 56 | 9/9 | none |
| set IN | 261 | 13/13 | none |
| UpCase | 517 | 8/8 | 4/4 |

The cases cover counts up to FFFFh, overlaps in both directions, fills and
moves over the leaf's own frame, all 256 elements and characters, segment
wraps, stacks at both segment ends, SS and data across A20, varied FLAGS
including DF, and a random 1 MiB background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **416
cases** with no mask: 48 FillChar, 48 Move, 64 set IN and 256 UpCase. Each
case compares all 14 CPU words, a 64-byte stack window and a 1 KiB data
window. The COM copies the four original leaves to `9000:<entry>`. The
result SHA-256 is
`6ea58b2183669a01586803e29a50eec348b8c02631a3e128d69762678b41bef5` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_runtime_leaves_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_runtime_leaves_audit.py --unicorn --native --output resources/leaves-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | SYSTEM FillChar `0AF8:16A8`, Move `0AF8:0A3F`, set IN `0AF8:0DC0` and UpCase `0AF8:182A` |
| `source_candidates` | `src/recovered/system_runtime_leaves_cpu.c` |
| `signals` | Exact hashes and CFGs, whole-RAM/CPU differential with full instruction and branch-outcome coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Runtime library code: no strict or identity credit. Next are U31 `+128D` and U1C `+266A`, then U3C2 `+002E`, U7F `+0B93` and U3B0 `+0B03`. |

No classifier, queue or count ledger is changed.
