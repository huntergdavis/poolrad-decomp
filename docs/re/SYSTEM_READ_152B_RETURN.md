# Saved-A read post-DOS return and independent later IOResult

Two separate C executions are supported: actual post-DOS SYSTEM131C:152B
through real RETF0E to loader2A8E:C49, and the later actual IOResult
131C:04A2 through real RETF to resident1026:0334. Both inputs and return
frames come from one newly measured original saved-A run. No C state is
replaced across DOS or the gap between these fragments.

PM i3980 authorized this independent-branch follow-on after
[Close cleanup7c763bd](https://github.com/huntergdavis/poolrad-decomp/blob/7c763bdb2b2b7ecfce8d7d95fb7f8ecd1773b1d8/docs/re/SYSTEM_CLOSE_14A8_CLEANUP.md).
Base is fresh main217e809448be096fcc48b2413714856f5b165284, with six new
files only and no unmerged source dependency. The
[read-prefix6de2dc8](https://github.com/huntergdavis/poolrad-decomp/blob/6de2dc8e72721c0aebe49131df31369ae3a4de59/docs/re/SYSTEM_READ_14FB_PREFIX.md)
and [loader359f97a](https://github.com/huntergdavis/poolrad-decomp/blob/359f97ac2574eda606a8efaeb5fd29f44b5b0b1e/docs/re/SLOT_A_LOADER_BLOCKREAD_ABI.md)
supply cited address/ABI leads. IOResult CPU semantics reuse the separately
reviewed [Close/IOResult52aee089 source](https://github.com/huntergdavis/poolrad-decomp/blob/52aee0899389528ca9bf6f80d5d6d5da96f534f0/src/replay/slot_a_close_ioresult_abi.c).
A bounded prior-session read-return/IOResult query expired after35s;
no recalled result was reused.

## Native-first capture and disjoint scope

The read tail itself contains no IOResult call: it returns directly to
C49. The new original run captured actual read-entry14FB, pre-DOS1529,
actual post-DOS152B, every read-return instruction, and C49. Original DOS
returned285 bytes in AX/CF0, changing145 conventional RAM bytes:126 net
changes within the285-byte buffer,19 elsewhere. These effects establish
the independently seeded post-DOS input and receive no C credit or delta
journal. Bytes returned and net changed bytes are different measurements.

After C49, original native code alone traversed Close and the loader's
sidecar context to reachedCAA. The first later selector IOResult call
1026:032F then supplied a new independently seeded C input at04A2.
CAA/call/entry/every leaf instruction/0334 were captured in the same run.
No intervening Close, sidecar, Assign, Reset, filename, allocator or
printer body is implemented or credited by this adapter.

All22 CPU/full655360-byte RAM states and the raw log were frozen before C
was written. Native CPU/RAM was not edited. The isolated emulator stopped
before implementation. Original media, derived content, screenshots,
native logs, RAM, binaries and build outputs remain ignored resources/.

| Pin | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| SYSTEM152B..1562,START C1A1..C1D8 | 56 | 43bdec36c4caf86049d8462bbebf5bc214c9659a6a18f3bb899edb0b268a1a9b |
| IOResult04A2..04A8,START B118..B11E | 7 | d70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b |
| Raw native log | private | 0717931a6f6562b0dc7fc4ab6829f4a077f7bbd7877dd72560725d10072e6e69 |
| Actual post-DOS152B full RAM | 655360 | b0bc6f2870f918551cf7e60f7e108930d667e506558435a568a123a4e5b508c3 |
| Actual loaderC49 full RAM | 655360 | f17c306e3e6463aacebf026d372c9637ae5112186fa1a85c74aec70520c0bd73 |
| Later IOResult-entry full RAM | 655360 | bd41dd87a14134fa94ec9440d279a3e5729bd29ad24d9586fe96f6c717247d47 |

Every RAM image is individually pinned; CPU JSON is compared with raw EV
rows. Both original SYSTEM bodies and loaded copies are checked in all22
snapshots, as are the original/loaded C44 read call, CAA selector call and
resident032F IOResult call. SYSTEM AF8->131C and resident802->1026 are the
observed +0824 relocations.

## Reached read-return ABI

Read-entry frame SS33DA contains nine words:

~~~
C49 2A8E 3624 1D16 011D 0008 449A 3626 1D16
~~~

At actual152B: SS1D16 SP33D4 BP33D8 DS449A ES1D16 AX011D BX0006 CX011D
DX0008 SI33C6 DI3626 FLAGS7202. SS33D8 contains saved BP37A6; the same
real outer far-return frame remains intact before/after DOS. POP CX and
POP DS restore CX0064 and DS14A0. Carry-clear execution zeroes DX, divides
the returned byte count by guest record size1, and loads actual-count
pointer1D16:3624 from SS:[BP+6]. The pointer-presence OR sets DX3F36;
the whole AX011D word is stored at that guest pointer.

POP BP restores37A6. Real RETF0E consumes18 bytes from SP33DA (four-byte
return plus14-byte arguments), reaching2A8E:C49 with SP33EC, AX011D,
CX0064 DS14A0 ES1D16 DI3624 DX3F36 FLAGS7206. The typed boundary stops
before Close setup. Only the actual-count word can change on this native
C fragment; all RAM is compared after every instruction.

Independent carry-set inputs store full AX at DS:4426, reload the result
pointer, and zero its actual-count word when nonnull. A null success
pointer compares actual count with requested count and stores CX0064 as
the error word on mismatch. Those controlled paths are not observed DOS
failure results. A zero divisor or quotient exceedingFFFF blocks atomically
before any divide CPU/RAM effect; original divide exception0 is separately
observed for those controlled inputs, without claiming a C fault handler.

## Separately reached IOResult

The later actual selector032F call enters IOResult04A2 with SS1D16 SP3334
BP33E2 DS14A0 AX3E01 FLAGS7202 and frame words[0334,1026]. The read C
execution is already stopped at C49; a fresh C state starts here.

XOR AX clears the whole register, XCHG exchanges the full DS:4426 word
with AX, and real RETF reaches1026:0334 with SP3338. Native DS:4426 is zero,
so AX becomes0000, FLAGS7246, and the complete RAM remains unchanged.
The typed caller boundary stops before its DS:49CE store. One original
far-CALL-only full-state frame comparison proves032F->04A2 provenance and
is excluded from all C comparison counts.

## Undefined DIV flags and verification classes

All13 read-return native post-instruction checkpoints and all three
IOResult checkpoints match every16-bit CPU field, every FLAGS bit and
all655360 RAM bytes. Native pre-DIV FLAGS7246 becomes7242; independent
Unicorn2.1.4 original-byte execution retains7246. This real discrepancy
persists through LES and MOV DX,ES, then the original OR overwrites status
flags and the states agree again.

The captured DOSBox-X2026.01.02 build reports commit59915c1. Its local
instructions.h DIVW policy clears AF/SF/OF, sets ZF when remainder is zero
and quotient odd, sets CF when remainder modulo4 is1 or2, and sets PF from
even parity of the whole16-bit quotient XOR remainder. C explicitly
implements that captured policy. These DIV status flags are architecturally
undefined; this is not a portable x86 flag guarantee. The reached quotient
011D/remainder0 explains7242. Controlled DIV flags beyond that reached case
have no additional native captures.

Neither emulator's flags are changed or normalized. The comparison masks
only status bits08D5 between DIV and the next original status-flag write,
and labels those checks separately. Native checkpoint comparisons always
use unmasked flags. Exact results:

- 16 native full CPU/RAM checkpoints, across two independent fragments.
- 376 independent Unicorn/C full-state instruction comparisons.
- 69 separate Unicorn/C defined-flag/full-RAM comparisons after DIV;
  66 expose a raw undefined-flag difference.
- One original-CALL-only frame comparison and two observed original divide
  exceptions verify provenance/fault stops and are excluded from C counts.
- 145 native DOS RAM changes, earlier runtime effects and the native gap
  between read return and later IOResult are excluded from C.

Focused strict C11 compilation uses Wall/Wextra/Wpedantic/Wconversion/Werror.
Focused tests and the entire differential audit also pass ASan/UBSan.
Controlled cases use fresh initial copies only: byte counts0/1/285/FFFF,
record sizes1/2/128/FFFF, carry-set errors/full AX words, null-result success
equal/mismatch, alternate/end-of-segment pointers, result stores aliasing
saved BP or return IP, IOResult words0/1/0100/8000/FFFF with flag/DF variants,
XCHG aliasing its real return IP or CS, RETF0E/RETF stack wrap and divide
zero/overflow. Atomic bounds/NULL/typed stops are tested. Each case is
bounded at100 instructions/10s; process120s plus5s kill allowance.

target_function: originalAF8:152B read return and separately reached
originalAF8:04A2 IOResult leaf.
source_candidates: src/replay/system_read_152b_return.c only.
signals: same-run actual DOS return and real frames; original/loaded code
pins;16 unmasked native full-state checkpoints; actual count/error exchange
writes; real return argument consumption; separate original-byte controls,
write/read aliases and observed divide exceptions.
score: 4/5 ordinal bounded evidence strength, not a probability.
verdict: match for these two independently seeded native fragments.
review_notes: DIV oracle differences are disclosed; masked comparisons
are excluded from full-state totals. Controlled read failures have no
native failure capture. IOResult is genuinely reached later through an
unmodeled native gap, not called by the read tail. No DOS handle/file result,
in-flight replacement, continuous read/Close/sidecar join, full runtime,
full-C function identity/count or playable credit follows. Caller0334 and
loaderC49 remain typed stops. Sidecar/allocator/printer C and shared
integration/PR410/0425 guards remain untouched.

~~~
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
bash tests/replay/check_system_read_152b_return.sh \
resources/oracle-runs/system-read-152b-return-20261015
~~~
