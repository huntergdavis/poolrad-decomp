# Saved-A loader SYSTEM1491 Close prefix

The isolated adapter starts at the actual loader C4F call's reached
SYSTEM131C:1491 entry and stops before DOS INT21h14A6. It executes the real
near signature CALL/RET and builds the request from guest registers and the
guest file record. DOS handle effects, results, error stores, cleanup and
the final far return remain external stops. No supplied DOS return,
callback, journal or replacement of an in-flight C state joins this prefix
to a later snapshot.

## Scope and provenance

PM i3924 authorized this distinct prefix after publication of approved
[SYSTEM14FB6de2dc8](https://github.com/huntergdavis/poolrad-decomp/blob/6de2dc8e72721c0aebe49131df31369ae3a4de59/docs/re/SYSTEM_READ_14FB_PREFIX.md).
Base is fresh main217e809448be096fcc48b2413714856f5b165284. Six new uniquely
named files only; no dependency on an unmerged adapter or integration edit.

The reviewed CPU semantics and signature-helper flag calculations reuse
the corresponding prefix from [Close/IOResult52aee089](https://github.com/huntergdavis/poolrad-decomp/blob/52aee0899389528ca9bf6f80d5d6d5da96f534f0/src/replay/slot_a_close_ioresult_abi.c),
whose native input was an earlier resident probe Close with AX0080 and a
128-byte record size. This task separately captures the later loader
Close after its real read: AX011D and record size1. The [loader359f97a
evidence](https://github.com/huntergdavis/poolrad-decomp/blob/359f97ac2574eda606a8efaeb5fd29f44b5b0b1e/docs/re/SLOT_A_LOADER_BLOCKREAD_ABI.md)
supplies the earlier measured C4F/Close-entry lead. A bounded prior-session
query for loader1491/C4F/14A6 expired after35s; no recalled result was reused.
The C API is standalone and stops before the earlier adapter's DOS/error/
cleanup/return/IOResult fragments.

A fresh original DOSBox-X heavy-debugger session loaded saved A naturally.
It first stopped at loaderC26, ran original open/read effects to C4F, then
entered this precise1491 occurrence with14A6 guarded. Before C was written,
all12 CPU/full conventional-RAM states were captured and the native log
frozen. Native CPU/RAM was never edited. The reached Close DOS instruction
was never executed in this capture. Earlier native open/read effects only
establish the input; they receive no C credit from this task.

## Pins and reached state

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| SYSTEM1491..14C4,START C107..C13A | 52 | 83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858 |
| Native raw transcript | private | 95afb1580bb125f7dde266a4f04e57c0eeb3c43fdaa9d60ea0ac86b0cc4f2e32 |
| Initial complete conventional RAM | 655360 | d936d445902f6169fdd50f96a06def50aa0e90801a6f4db71e1baef935177298 |
| Pre-DOS complete conventional RAM | 655360 | 216900d0a02b1894941501c0e19af182392374742682d7ed49c57744bd119355 |

The audit pins full media, every RAM image and raw EV row, checks each CPU
JSON against that raw row, compares all52 original SYSTEM bytes to every
loaded image, and verifies original/loaded loaderC4F call relocation
AF8->131C. Original1491 is loaded at131C by the observed +0824 relocation.
The52-byte fingerprint includes unsupported tail/error instructions for
identity; it does not grant coverage to those bytes. Original media,
derived content, screenshots, logs, dumps and binaries remain ignored.

At1491: SS=ES1D16 DS14A0, SP33E4 BP37A6, AX011D BX0006 CX0064 DX3F36,
SI33C6 DI3626 FLAGS7206. The four far-call frame words at SS33E4 are:

~~~
C54 2A8E 3626 1D16
~~~

MOV BX,SP and LES SS:[BX+4] load file-record1D16:3626. Its initial words
are handle0006, signatureD7B3, record size0001. Near CALL1497 pushes149A
at SS33E2, enters14B6 and compares signature+2 againstD7B3. The valid
signature branches to real near RET14C4, restoring SP33E4.

The prefix loads the current guest handle, compares it unsigned with4 and,
for larger handles, replaces AH with3E while retaining AL. The reached
pre-DOS state is:

~~~
CS:IP=131C:14A6 DS=14A0 ES=SS=1D16 FS=GS=0000
SP=33E4 BP=37A6 AX=3E1D BX=0006 CX=0064 DX=3F36
SI=33C6 DI=3626 FLAGS=7202
~~~

The typed stop reports originalAF8:14A6, return14A8 and actual AX/BX,
preserving every CPU field and RAM byte. No native6/AL1D value is hardcoded.
The only prefix RAM write is the two-byte near return at SS:[SP-2].
The file record, DS error word and outer far frame remain unchanged on
the measured route. Reserved handles0..4 stop at14AD before signature
cleanup; invalid signatures stop at14BE before the error store. An
independent failed-helper branch stops at14B3 before far-return cleanup.
Post-DOS14A8 and adjacent entry14C5 are unsupported PCs.

## Verification and parity review

The focused test and complete audit pass strict C11 with
Wall/Wextra/Wpedantic/Wconversion/Werror and also ASan/UBSan:

- 544 independent original-byte Unicorn/C full-state instruction comparisons.
- All11 post-input native full CPU/RAM checkpoints match, from12 pinned states.
- Every comparison uses all16 CPU fields, all FLAGS bits and all655360 RAM bytes.
- No DOS execution, runtime result, original-CALL-only frame check, flag mask
  or post-DOS tail is included in these counts.

Controlled inputs are fresh native-state copies: handles0..6/7FFF/8000/FFFF,
initial AX0/AB00/AB01/FFFF and flag/DF variants, invalid signatures around
D7B3, independent failed-helper branch, alternate record address, effective
offset wrap, real near-stack wrap, and record/near-frame aliases that change
the signature or handle. No snapshot replaces a live C state. Focused tests
also check immutable typed stops, AL retention, actual near CALL/RET,
NULL arguments and atomic RAM/segment failures. Cases are bounded at100
instructions/10s; the audit process at120s plus5s kill allowance.

target_function: originalAF8:1491 loader-context Close-entry prefix.
source_candidates: src/replay/system_close_1491_prefix.c.
signals: pinned original/loaded code; actual later loader frame; real
near CALL/RET; unmasked native/original-byte CPU/RAM parity; unsigned
handle boundary; ordered alias loads/writes; blocked failure paths.
score: 4/5 ordinal strength for this bounded prefix, not a probability.
verdict: match within the stated entry-to-external-stop scope.
review_notes: the earlier probe context does not substitute for this
capture. Controlled failure paths have no additional native captures.
DOS Close effects, errors, record cleanup, final RETF4 and continuous
loader execution remain unsupported here. No full-C/function-count,
library ownership or playable credit follows. Sidecar, allocator and
printer gaps remain separate. Shared integration, PR410 map,0425 guards
and other lanes' key/printer files are unchanged.

~~~
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
bash tests/replay/check_system_close_1491_prefix.sh \
resources/oracle-runs/system-close-1491-prefix-20261015
~~~
