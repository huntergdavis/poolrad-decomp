# Saved-A contiguous C49 caller and bounded Close prefix

One C execution starts from the naturally reached loader2A8E:C49 input,
builds the file-record argument, executes the real C4F far CALL, and runs
the bounded SYSTEM131C:1491 CPU prefix through its real near signature
CALL/RET to pre-DOS14A6. It stops before DOS effects or cleanup. The state
is never replaced between the caller and Close instructions.

PM i4029 authorized the distinct C49 caller prefix after publication of
[read-return e24fadb](https://github.com/huntergdavis/poolrad-decomp/blob/e24fadbe5138fdd1bf020907ad1ed6a8b7668cb0/docs/re/SYSTEM_READ_152B_RETURN.md).
The selected integration scope was reported in i4032: bind C49 setup and
the real C4F frame to the already evidenced Close CPU prefix, ending at
the first DOS external stop. The standalone C49-to-C4F setup gate already
exists in [loader359f97a](https://github.com/huntergdavis/poolrad-decomp/blob/359f97ac2574eda606a8efaeb5fd29f44b5b0b1e/docs/re/SLOT_A_LOADER_BLOCKREAD_ABI.md);
this adds the same-state call-frame/Close prefix measurement rather than
presenting the earlier three setup steps as a new gate.

The reviewed [Close133baef evidence](https://github.com/huntergdavis/poolrad-decomp/blob/133baef6a9e51b4f02636b0981a01a7ca6abf6f0/docs/re/SYSTEM_CLOSE_1491_PREFIX.md)
provides the signature, unsigned-handle and DOS-boundary leads. The new
C implementation is self-contained and compiled alone; it does not import
or link an unmerged C adapter. The independent original-byte audit reader/
CPU/emulator infrastructure is reused from current main, with the focused
comparison loop adapted from the reviewed Close audit. All address/byte/
frame claims are rechecked against this capture. A bounded prior-session
C49/C4F/1491/14A6 query expired after35s; no recalled result was reused.

Base is fresh main217e809448be096fcc48b2413714856f5b165284. Six new uniquely
named files only; shared integration/scoreboard, PR410 maps,0425 guards
and other lanes' printer/key files are unchanged. Selector0334 post-IOResult
remains a second separate queued branch scope. No continuous read-to-selector,
DOS join, full Close/loader/function-count or playable claim follows.

## Native-first same-run pins

A fresh original DOSBox-X heavy-debugger saved-A run stopped at loaderC26,
then executed original open/read effects naturally to C49. Before writing
C, it captured every instruction boundary from C49 through the real far
call and Close CPU prefix to pre-DOS14A6:16 CPU/full655360-byte RAM states.
No native CPU/RAM was edited. The reached Close DOS instruction was never
executed. Earlier original read/runtime effects establish the initial C49
state and receive no C credit. The native log was frozen and the owned
emulator stopped before C implementation.

| Pin | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original unit6F C49..C53 | 11 | 1da7692556a283d48020cff03bfe4d9749af1f1a495fde4a3d870f4ea78ac67d |
| Original SYSTEM1491..14C4 | 52 | 83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858 |
| Raw native log | private | d44c2d96d1145507b450d74a6f54172ca19a76d50d09e42c7621c793147e0744 |
| Initial C49 full RAM | 655360 | ae54f48945deef576b07897ed13ba5973c677b19cb70fa953f7c265ecd426c7f |
| Pre-DOS14A6 full RAM | 655360 | 191881e1166cea9f92bbcf7dcbbe16fa8b0185ff0fc0cf01eb751ab6b080b075 |

The audit pins full media/every RAM image, compares CPU JSON with raw EV
rows, and verifies both original and loaded code spans in every snapshot.
The C4F call's original AF8 segment is relocated to131C; unit6F is loaded
at2A8E. The52-byte identity fingerprint includes unsupported cleanup/error
bytes; only the documented prefix executes in C. Original media and all
derived/private capture material remain under ignored resources/.

## Measured contiguous ABI

Native C49 input: CS2A8E DS14A0 ES=SS1D16 SP33EC BP37A6 AX011D BX0006
CX0064 DX3F36 SI33C6 DI3624 FLAGS7206. LEA computes file-record local
BP-180=3626; PUSH SS and PUSH DI create the argument at SS33E8. Actual C4F
far CALL pushes real returnC54 and caller CS2A8E, entering131C:1491 with
SP33E4 and four words:

~~~
C54 2A8E 3626 1D16
~~~

Close MOV BX,SP/LES SS:[BX+4] reloads that actual argument. Near CALL1497
pushes149A at SS33E2, enters signature helper14B6, compares ES:[DI+2]
againstD7B3 and returns through real near RET14C4 to149A with SP33E4.
Close then loads the current guest handle and compares it unsigned with4.
Larger handles build AH3E while retaining input AL; native handle6 and
AL1D therefore produce the actual request:

~~~
CS:IP=131C:14A6 DS=14A0 ES=SS=1D16 FS=GS=0000
SP=33E4 BP=37A6 AX=3E1D BX=0006 CX=0064 DX=3F36
SI=33C6 DI=3626 FLAGS=7202
~~~

No native6 or AL1D value is hardcoded. The typed external stop reports
originalAF8:14A6/return14A8 and actual AX/BX, preserving all CPU/RAM.
Repeated calls remain blocked. The guest file record stays handle6,
signatureD7B3,size1, with all128 bytes unchanged; native RAM changes are
limited to the actual stack frames/argument area.

Invalid signature stops at14BE before its error store; handles0..4 stop
at14AD before record cleanup; the independent failed-helper branch stops
at14B3 before final far return. Post-DOS14A8 and loaderC54 are unsupported.
No post-DOS CPU/RAM is accepted as a replacement for the live prefix.

## Verification and adversarial parity review

Strict C11 Wall/Wextra/Wpedantic/Wconversion/Werror focused tests and
complete audit pass; the same focused/complete suites pass ASan/UBSan:

- 763 independent original-byte Unicorn/C full-state comparisons.
- All15 post-input same-run native full CPU/RAM checkpoints match exactly.
- All16 CPU fields, all FLAGS bits and all655360 RAM bytes are compared.
- Real C4F far CALL and1497 near CALL/RET are executed C instructions,
  included in those counts; no separate original-CALL-only credit.
- No flag masks, DOS service/results, cleanup or earlier read credit.

Controlled whole-caller inputs cover unsigned handles0..6/7FFF/8000/FFFF,
AX0/AB00/AB01/FFFF, flag/DF variants, invalid signatures, alternate and
wrapping local-record offsets, three caller-record/argument/far/near-frame
aliases, and contiguous far/near stack wrap from initial SP4/8. These
aliases invalidate the signature when real caller/frame pushes overwrite
it; records are not cached before those writes. One independent failed-
helper branch tests the return-tail gate. Focused tests also check actual
frame bytes, AL retention, NULL arguments and atomic unsupported spans.
Case bounds are100 instructions/10s; process120s plus5s kill allowance.

target_function: reached loader unit6F:C49 caller through bounded
originalSYSTEM AF8:1491 CPU prefix to external14A6.
source_candidates: src/replay/slot_a_c49_close_prefix.c only.
signals: same-run naturally reached caller; original/loaded byte pins;
actual far and near frames in one live C state;15 unmasked native
checkpoints; unsigned-handle and ordered alias failures; atomic blockers.
score: 4/5 ordinal bounded evidence strength, not a probability.
verdict: match within this contiguous pre-DOS scope.
review_notes: no measured native error/reserved-handle runs supplement
the controlled cases. DOS handle effects and post-DOS cleanup/far return
remain external. Earlier read execution and later selector0334 are not
composed into this C path. No full-C identity/count or playable credit;
sidecar, allocator, printer and selector branch scopes stay separate.

~~~
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
bash tests/replay/check_slot_a_c49_close_prefix.sh \
resources/oracle-runs/slot-a-c49-close-prefix-20261015
~~~
