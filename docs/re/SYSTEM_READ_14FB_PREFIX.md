# Saved-A SYSTEM 14FB read-entry prefix

This isolated adapter executes the reached loaded SYSTEM 131C:14FB entry
through the instruction before INT 21h at 1529. It models the real near
CALL/RET through signature helper14B6 and constructs the DOS request from
guest arguments and file-record fields. It stops before DOS, without an
injected return, callback, journal, or replacement of an in-flight C state.
The original segment is AF8; the observed loaded relocation is +0824.

## Provenance and boundary

PM assignment i3885 requested this prefix after the approved loader argument
slice at [359f97a](https://github.com/huntergdavis/poolrad-decomp/blob/359f97ac2574eda606a8efaeb5fd29f44b5b0b1e/docs/re/SLOT_A_LOADER_BLOCKREAD_ABI.md).
That slice's actual native read entry and DOS input supplied the initial
address/ABI lead. This branch starts at main217e809 and does not link its
unmerged implementation. The near signature-helper semantics were also
checked against the separate [Close evidence](https://github.com/huntergdavis/poolrad-decomp/blob/52aee0899389528ca9bf6f80d5d6d5da96f534f0/docs/re/SLOT_A_CLOSE_IORESULT_ABI.md).
A bounded recall query for SYSTEM14FB/BlockRead/14B6/1529 expired after35s;
no recalled result was reused.

A fresh original DOSBox-X heavy-debugger run loaded saved A naturally,
stopped first at loaderC26, then at the reached14FB call. Before writing C,
it captured every instruction boundary through pre-DOS1529:22 CPU/full
conventional-RAM states. No native CPU/RAM was edited. The DOS instruction
was never executed in this capture. Captures, original media, screenshots,
logs and binaries remain private under ignored resources/.

| Pin | Value |
| --- | --- |
| START.EXE,65231bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR,206105bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT,13137bytes | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Raw native log | f3bed6079585d31b69d1e93f79071ee568ab3dfe2dd1973a8c49c5094d8d3234 |
| SYSTEM14FB..152A,48bytes,START C171..C1A0 | 4e7ca8b1abee0e0b48d592245da1470730d0baa56178fd9e3abb9f78cd037a8c |
| SYSTEM14B6..14C4,15bytes,START C12C..C13A | 24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363 |
| Initial full conventional RAM,655360bytes | b1c583e6bf4d8a00b050dd855b63bea1735e55acf99d955032fa1eace3432bc6 |
| Final full conventional RAM,655360bytes | 9f08570bdbd720d86a44125036c02d8157204d1995cce1a1a162a13011b9e0ad |

All22 CPU JSON files are checked against the raw debugger EV rows; every
RAM image is individually SHA256-pinned. Both executed original-byte spans
are compared against loaded RAM at every checkpoint. The complete media
files are pinned even though the adapter executes only this prefix.

## Reached ABI

Native entry has SS1D16 SP33DA BP37A6, DS14A0 ES449A, AX011D BX3378 CX0000
DX3656 SI33C6 DI3624 FLAGS7246. Its nine stack words are:

~~~
C49 2A8E 3624 1D16 011D 0008 449A 3626 1D16
~~~

The prefix saves BP, sets BP33D8, loads file-record pointer1D16:3626, then
pushes near return1510 for the signature helper. The helper compares the
record's word+2 with D7B3, branches to its real near RET at14C4 on equality,
and restores SP33D8. The requested-record count comes from SS:[BP+A].
A zero count stops at1535 before the post-read/result tail.

For a nonzero count, the prefix saves DS and CX, multiplies requested count
by the record-size word at ES:[DI+4], puts the low product in CX, loads the
buffer from SS:[BP+C], builds AH from BL, and loads BX from the guest
record handle. It does not force the native handle6, count285, record size1,
buffer, or runtime output.

Native pre-DOS1529 has AX3F1D BX0006 CX011D DS449A DX0008 ES1D16,
SS1D16 SP33D4 BP33D8 SI33C6 DI3626 FLAGS7202. The typed report gives the
original AF8:1529 DOS boundary, return152B and actual AX/BX/CX/DS/DX.
Repeated blocked calls preserve every CPU field and RAM byte.

## Undefined MUL flags: separate evidence classes

The native21-instruction path matches all16 CPU fields and all655360 RAM
bytes at every post-instruction checkpoint, including every FLAGS bit.
The native pre-MUL flags are7206 and post-MUL flags7202. An independent
Unicorn2.1.4 execution of the unchanged original bytes produces7206 after
the same MUL and retains it through pre-DOS1529. This is a real oracle
difference, not a successful full-state Unicorn match.

The captured DOSBox-X2026.01.02 build reports commit59915c1. Its local
instructions.h MULW policy computes PF from the full32-bit product,
sets ZF from the low word for its captured CPU, retains SF/AF, and sets
CF/OF from a nonzero high word. C explicitly implements this native
policy. For011D*1 the full product has odd parity while its low byte1D
has even parity, explaining the PF difference. These flags, other than
CF/OF, are architecturally undefined after MUL; this is a bounded native
policy, not a claim that every x86 emulator chooses those flags.

The audit never normalizes or mutates either execution's flags. Before
MUL it compares the complete Unicorn state; after MUL it separately
compares all RAM and all CPU fields with only SF/ZF/AF/PF masked in the
comparison. Native checkpoint comparisons always use unmasked flags.

## Verification and review

Focused strict C11 compilation uses Wall/Wextra/Wpedantic/Wconversion/Werror.
The focused tests and complete private differential audit also pass under
ASan/UBSan. Results:

- 21 native original-byte/C full CPU/RAM checkpoints, from22 pinned states.
- 557 independent Unicorn/C full-state comparisons.
- 155 separate Unicorn/C defined-flag/full-RAM comparisons after MUL;
  120 of these expose raw undefined-flag differences.
- No DOS service, native DOS-return state, runtime-return injection, or
  original CALL-only frame comparison is included in those counts.

Controlled cases start from fresh copies of native inputs: count0/1/285/
8000/FFFF; record-size0/1/2/128/FFFF; invalid signatures aroundD7B3;
initial AX/BX/flags and DF; alternate buffer/handle; file-record aliasing
the stack before and after signature validation; effective-offset wrap;
independent MUL overflow/zero inputs; and the independent error branch.
The audit bounds each case at100 instructions/10s and the process at120s.
Focused tests check atomic segment/RAM failures, stack wrap, the real
near frame and immutable DOS/error/zero gates.

target_function: original AF8:14FB read-entry prefix, loaded131C.
source_candidates: src/replay/system_read_14fb_prefix.c only.
signals: exact original/loaded byte identity; naturally reached native
frame; real near signature CALL/RET; ordered guest loads/pushes/product;
all21 native full-state checkpoints; controlled aliases and failure stops.
score: 5/5 for the bounded prefix, ordinal evidence score, not probability.
verdict: match for this bounded native prefix.
review_notes: MUL undefined flags differ between oracles and are disclosed;
controlled undefined flags have no additional native captures. Invalid
signature stops at14BE before the error store; error continuation1551 and
zero-count tail1535 remain blocked. Adjacent write-entry1502 is unsupported.
Post-DOS152B, post-read result stores/division/return and complete runtime
semantics remain unimplemented here. This grants no continuous-loader,
full-C function, identity-count or playable credit.

The next distinct queued task is the reached loader's runtime1491 Close
prefix to its own external stop. Sidecar, allocator and printer gaps stay
separate. Shared integration, immutable PR410 maps,0425 timeout/progress
guards and other lanes' printer/key files are unchanged.

~~~
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
bash tests/replay/check_system_read_14fb_prefix.sh \
resources/oracle-runs/system-read-14fb-prefix-20261015
~~~
