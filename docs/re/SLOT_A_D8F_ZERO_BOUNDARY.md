# Controlled saved-A zero-AL D8F setup boundary

This bounded C adapter starts at actual D8F reached through the original
CAF zero-AL branch. Six LEA/PUSH instructions prepare the B0B arguments;
D9B stops before its far CALL, frame push, string copy or downstream effects.
The original run deliberately withheld the first probed sidecar in its ignored
DOS input copy. This is a controlled file-absent original-native case, not
an untouched baseline. The pinned save and executables remain unchanged.
No guest CPU/RAM, selector result, or allocator outcome was patched.

Prior standalone D8F setup evidence at
00755c3e98a966b56a39d0dbf0b10d173c1fac7e came from the normal positive
sidecar-processing path. The approved CAF adapter at
2e92653f4685a42409c447e000e029484f36c7b2 tested its false branch synthetically.
This slice supplies fresh original-native zero-branch provenance and a finite
external-copy boundary; neither predecessor is a C dependency. Only merged
state/report declarations and audit utilities are reused. No new function
identity/count or playable credit is claimed. The required deja query
completed with no indexed matches (zero sessions searched); the repository
leads above were inspected directly and are cited as the reused evidence.

## Coordinated original-only native capture

PM i4259 granted one bounded capture slot after pR/pP/pG notifications.
The original session stopped at the first finite B0B boundary; the owned
emulator then exited and slot release was routed to PM, peers and p9.
Eleven CPU/full655360RAM states were frozen before C implementation:
0334,CAF,CB1,CB3,D8F,D93,D94,D95,D99,D9A,D9B.

Original file handling produced selector AX=0067 at0334. The original tail
stores that full word in DS:49CE and returns AL=0, AX=0000 toCAF. Its ten
original instructions reproduce the captured CAF CPU/full RAM. The next
three original instructions OR AL,AL; JNE (not taken); JMP D8F reproduce
captured D8F. These two original-only endpoint checks and13 instructions are
excluded from C comparison/native-checkpoint counts. The underlying I/O
implementation and numeric error interpretation remain outside this slice.

Actual D8F input: CS2A8E DS14A0 ES=SS1D16 FS=GS0 SP33EC BP37A6
AX0000 BX3330 CX0000 DX3368 SI33D0 DI3338 FLAGS7246. DS:49CE=0067.
This differs from the earlier baseline D8F input after successful sidecar I/O.

| Original instruction | Actual next state |
| --- | --- |
| D8F LEA DI,[BP-2B9] | D93, DI34ED |
| D93 PUSH SS | D94, SP33EA |
| D94 PUSH DI | D95, SP33E8 |
| D95 LEA DI,[BP-100] | D99, DI36A6 |
| D99 PUSH SS | D9A, SP33E6 |
| D9A PUSH DI | D9B, SP33E4 |

At the pre-call boundary, stack words are [36A6,1D16,34ED,1D16]: source
far pointer followed by destination far pointer. Four net RAM bytes change
at physical20544..20547; the first two pushes execute idempotent writes.
AX and FLAGS7246 remain unchanged. The destination string is not copied.
The typed external dependency is original AF8:0B0B, returnDA0, caller overlay6F;
loaded call target131C:0B0B is identity-checked but never entered.

## Pinned inputs and reproduction

| Input or span | SHA-256 |
| --- | --- |
| START.EXE,65231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR,206105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT,13137 bytes | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Withheld sidecar,126 bytes, retained privately | a1927b99e61de81f910cae15428e8bce17fd1857197a666e605980f8ba3e8332 |
| Original D8F..D9F including unexecuted CALL,17 bytes | ff578f2fa82bb3b6d20b45411dd317b4ccb5a9611253e76c0b923ee7e51bc4e3 |
| Original selector334..355,34 bytes | 86e4cb84d29f3d8d6c422459c06563bb6218c34e1897c7d05628954c021c5014 |
| Raw11-row transcript | ca7583396bd1358af2cde731f59bc908fe2b8b470380a51d10cee9fc92c6a024 |

The audit pins every native RAM checksum and raw CPU row, validates CPU JSON
against raw rows, and checks original/loaded D8F, CAF branch and selector bytes
in every image. It also checks the withheld input hash, its absence from the
DOS directory, and its filename against the captured original selector name.
Original source starts at GAME+1DFE6 and resident0802 at START+8057; the B0B
call segment relocates AF8 to131C. The untouched original input manifest and
controlled fixture metadata remain private alongside the retained asset.
All original/derived media, raw snapshots, screenshots and builds stay under
ignored resources/. Native root: resources/oracle-runs/slot-a-d8f-zero-boundary-20261015.

~~~sh
POOLRAD_UNICORN_PYTHON=resources/venv-string0356/bin/python \
  tests/replay/check_slot_a_d8f_zero_boundary.sh \
  resources/oracle-runs/slot-a-d8f-zero-boundary-20261015
~~~

## Focused verification and limits

Strict C11 with Wconversion/Werror passes the C tests and one focused native
case: six C/original-byte Unicorn2.1.4 comparisons over all16CPU fields and
full655360RAM, with six corresponding controlled-original-native checkpoints.
ASan/UBSan passes the C tests and the same six-step case. No heavy parity
matrix was run. Native case stepping is capped at2seconds; the runner has a
20second process cap plus5seconds termination allowance. No flags are masked
and no execution state is rewritten for comparison.

Separate lightweight C tests verify argument order, AH/flags preservation,
wrapped LEA offsets and wrapped stack writes, typed pre-call metadata, NULL,
wrong CS/unknown PC, host bounds, A20 spans and split words atFFFF. These
synthetic C tests are not counted as original-byte or native comparisons.
Each failing instruction preserves CPU/RAM; earlier successful effects remain.
The adapter requires audited original/loaded identity, actual independent live
CPU/RAM, at most1MiB RAM, nonoverlapping state/report/RAM, and every word
inside one segment and host image. Unsupported A20 or word crossings block.

Parity report: target_function=actual zero-AL D8F setup before B0B;
source_candidates=src/replay/slot_a_d8f_zero_boundary.c;
signals=controlled file-absence provenance, original/loaded identities,
original-only selector/branch endpoints, actual retained-pointer arguments,
six native full-state checks and focused failure/wrap tests;
score=strong for this measured input (ordinal,not probability);
verdict=match within the six-instruction and memory bounds. review_notes:
the file-absent fixture is deliberate; baseline zero-AL reachability remains
unclaimed; branch and setup instructions reuse prior recovery; no string
copy/append/probe, allocator, downstream D8F effects, continuous runtime join,
CCC reload/use, callback/journal replacement or new function/count/playable
credit. CBF stays on its separately approved exact head; CCC remains a
separate queued follow-on requiring its own measured input. Shared code,
CMake, immutablePR410 map, printer/key code,0425 guards and ledgers are untouched.
