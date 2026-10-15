# Saved-A loader Close post-DOS cleanup

This bounded adapter starts a new C execution from the actual same-run
native post-DOS131C:14A8 input. It branches on the existing carry flag,
stores the full AX error word on the error path, marks the guest record
signature D7B0, and executes real RETF4 to loader2A8E:C54. It stops before
the loader's word clears. It never supplies a DOS result or replaces an
in-flight C state across DOS.

PM i3950 authorized this first distinct follow-on after publication of
[Close-prefix133baef](https://github.com/huntergdavis/poolrad-decomp/blob/133baef6a9e51b4f02636b0981a01a7ca6abf6f0/docs/re/SYSTEM_CLOSE_1491_PREFIX.md).
This branch starts from fresh main217e809448be096fcc48b2413714856f5b165284;
six new files, no dependency on an unmerged source. The cleanup/error/
RETF semantics reuse the corresponding instructions in the earlier
[probe-context Close/IOResult52aee089](https://github.com/huntergdavis/poolrad-decomp/blob/52aee0899389528ca9bf6f80d5d6d5da96f534f0/src/replay/slot_a_close_ioresult_abi.c).
That earlier native probe input does not substitute for this later
loader frame. A bounded recall query for Close14A8/14AD/14B3/C54 expired
after35s; no recalled result was reused.

## Same-run native input and DOS exclusion

A fresh original DOSBox-X heavy-debugger session loaded saved A naturally,
stopped at loaderC26, then ran original open/read effects to C4F. It entered
this later Close1491, captured its actual pre-DOS14A6 state, executed
original DOS Close, and captured actual14A8,14AD,14B3 and loaderC54 states.
All six CPU/full655360-byte RAM states were frozen before writing C, with
no native CPU/RAM edits. The owned emulator then stopped.

The same run's entry/pre-DOS/post-DOS frame words at SS33E4 are all:

~~~
C54 2A8E 3626 1D16
~~~

All three have SS1D16 SP33E4 BP37A6. Original DOS input AX3E1D/BX6 becomes
AX3E01/CF0 at14A8. Apart from IP/AX, the recorded CPU fields are unchanged.
DOS changed24 conventional RAM bytes and left the complete128-byte record
and real return frame unchanged. Those24 bytes and DOS register effects
are measured native evidence only, excluded from C comparisons. No delta
is applied as a journal. Earlier open/read/prefix effects also receive no
C credit from this adapter.

| Pin | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| SYSTEM1491..14C4,START C107..C13A | 52 | 83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858 |
| Raw native transcript | private | e64f033dc2ed33dda733926d192cde94672c96bee907b04fd97e3c37fc131f42 |
| Actual post-DOS14A8 full RAM | 655360 | af3b6bfdf55881ec20d04c8b7124f2228fcf8b13b82cedd10d0b5719ef932c46 |
| Actual loaderC54 full RAM | 655360 | 4ad38419534e7efaea3121836138d04c14e19463b78000b89e2bb54ce7e567c3 |

The audit pins complete media, all six RAM hashes, raw EV rows against CPU
JSON, and all52 original/loaded SYSTEM bytes in every snapshot. It checks
the original/loaded loaderC4F call and AF8->131C relocation. The52-byte
fingerprint includes unsupported prefix/DOS instructions for identity;
only the post-DOS steps listed here execute in C. Original media, derived
content, raw captures and binaries remain under ignored resources/.

## Measured cleanup

Actual14A8 input: DS14A0 ES=SS1D16 SP33E4 BP37A6 AX3E01 BX0006 CX0064
DX3F36 SI33C6 DI3626 FLAGS7202. Carry is clear, so JNC14AD skips the error
store. MOV ES:[DI+2],D7B0 changes only the low signature byte at physical
20788, retaining handle6, record size1 and all remaining record bytes.
RETF4 reads the real IP/CS from the stack, consumes eight stack bytes
(four-byte far return plus four-byte arguments), and reaches2A8E:C54 with
SP33EC BP37A6 AX3E01 FLAGS7202. All other CPU fields are retained; RETF4
does not modify RAM.

The typed loader boundary reports2A8E:C54 and preserves the entire state.
Post-DOS error inputs independently copied from the native initial state
exercise carry-set14AA: store the whole AX word to DS:4426, then the same
record cleanup and real far return. These controlled errors are not
observed DOS failure outcomes. Arbitrary return frames are consumed as
guest data; unknown returned PCs block before further execution. Pre-DOS
14A6, signature helper/error and IOResult are outside this adapter.

## Verification and parity review

Focused strict C11 tests and the complete audit pass
Wall/Wextra/Wpedantic/Wconversion/Werror and ASan/UBSan:

- 136 independent original-byte Unicorn/C full-state comparisons.
- All three post-input native CPU/full-RAM checkpoints match exactly.
- Six same-run states establish the input/frame/DOS provenance; prefix
  and DOS states are excluded from the C count.
- All16 CPU fields, all FLAGS bits and all655360 RAM bytes are compared;
  no flag masks, original-CALL-only counts or continuous DOS join.

Controlled cases cover carry clear/set, full AX words0/1/0100/8000/FFFF,
flags/DF, alternate/wrapping record pointers, signature cleanup aliasing
the real return IP or CS, error storage aliasing return CS or the record
handle, error/cleanup writes to the same word, independent cleanup/return
entries, real RETF4 stack wrap and unknown return. Focused tests also check
NULL arguments, atomic spans and repeated immutable loader stops.
Each case is bounded at100 instructions/10s; process120s plus5s kill.

target_function: reached originalAF8:14A8 Close cleanup to real loaderC54.
source_candidates: src/replay/system_close_14a8_cleanup.c only.
signals: same-run actual DOS return and unchanged real frame; pinned
original/loaded instructions; native unmasked full-state checkpoints;
ordered error/signature writes; real far-return argument consumption;
independent carry/alias/wrap comparisons against original bytes.
score: 4/5 ordinal evidence strength for this bounded slice, not probability.
verdict: match within the stated independently seeded post-DOS scope.
review_notes: controlled error cases have no native failure capture.
DOS Close/handle effects remain external; the prior C prefix and this
execution are not a continuous C path. No complete Close/loader, full-C,
function-count or playable claim follows. SYSTEM14FB post-DOS/IOResult
remains the second queued independent-branch task. Sidecar/allocator/
printer gaps, shared integration, PR410 maps and0425 guards stay separate.

~~~
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
bash tests/replay/check_system_close_14a8_cleanup.sh \
resources/oracle-runs/system-close-14a8-cleanup-20261015
~~~
