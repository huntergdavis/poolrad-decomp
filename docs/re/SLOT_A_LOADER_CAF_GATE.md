# Actual saved-A selector return: loader CAF to external GetMem gate

This bounded adapter starts at the actual returned selector state in loaded
unit 6F, 2A8E:0CAF, and stops before CBA's far allocator CALL. It models four
instructions on the measured positive path. No allocator return, heap change,
DOS operation, or next-sidecar composition is supplied.

This branch already has evidence in local predecessor commit
00755c3e98a966b56a39d0dbf0b10d173c1fac7e, its standalone
slot_a_loader_sidecar_abi adapter and SLOT_A_LOADER_SIDECAR_ABI.md.
This slice revalidates the actual selector-return context and isolates its
external gate; it adds no new game-function identity or count credit.
The selector work at de573decddb1672c7c0ee9a7ec298f1541c51eb5 provides
preceding evidence, not a linked C dependency. The new source compiles alone,
using only the merged shared state/report declarations and oracle utilities.
A bounded deja recall query for loader CAF/CB1/CBA/CBF/D8F expired after 35
seconds without output; these references were inspected directly through git.

## Original-only capture before implementation

On 2026-10-15, run the pinned original saved A, first breaking at loader C26,
then CAA, then actual post-IOResult selector 1026:0334. Continue the unchanged
original selector tail to CAF, and the original branch to CBA. Seven CPU and
655,360-byte conventional RAM states were frozen before C was written:
CAA, 0334, CAF, CB1, CB6, CB9, CBA. The owned emulator was stopped afterward.
No synthetic state was written into that run.

The actual outer selector frame at SS:33E2 contains words
37A6, 0CAF, 2A8E, 34ED, 1D16. Selector RETF 4 restores BP=37A6 and SP=33EC.
An independent original-byte Unicorn run executes its eleven instructions
from captured 0334 and matches captured CAF across all 16 CPU fields and all
RAM. This is one original-only endpoint provenance check, excluded from C
instruction and native-checkpoint counts. The actual C initial state is:

~~~text
CS:IP 2A8E:0CAF  DS 14A0  ES=SS 1D16  FS=GS 0000
SP 33EC BP 37A6 AX 0001 BX 0006 CX 0000 DX 3368
SI 33D0 DI 3338 FLAGS 7206
~~~

| Original instruction | Reached next state |
| --- | --- |
| CAF OR AL,AL | CB1, FLAGS=7202; AX unchanged |
| CB1 JNE CB6 | CB6 |
| CB6 MOV AX,003F | CB9, entire AX=003F |
| CB9 PUSH AX | CBA, SP=33EA; word SS:33EA=003F |

Only RAM bytes 2054A and 2054B change between CAF and CBA. The CALL at CBA
remains unexecuted; no far return frame is pushed. The typed external stop
reports original AF8:023F (loaded 131C:023F), return CBF, caller overlay 6F.
The zero-AL route is controlled evidence only: CAF -> CB1 -> CB3 JMP D8F.
D8F is a separate next-context blocker, with no invented callee metadata.
AH does not select the branch: AX=0100 or AB00 takes the zero-AL path.

## Pinned identity

| Input or code region | SHA-256 |
| --- | --- |
| START.EXE, 65,231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR, 206,105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT, 13,137 bytes | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original CAF..CBE, 16 bytes | 6a6980b142121eccd959ff3ea69319c89a5a3e843ab69af6f26cc16721cffae1 |
| Loaded CAF..CBE | ac945d9129af9a75f2e11fc2d7b6a232eb6cafd8d108bdbf740981325f0c3f6d |
| Original selector 0334..0355, 34 bytes | 86e4cb84d29f3d8d6c422459c06563bb6218c34e1897c7d05628954c021c5014 |
| Frozen raw native transcript | 08265abbc8ad732886593dfea948cda18223331ff6580d1cb6498972bb20ebc8 |
| RAM at actual CAF | 329be84d4ca69bce633a209f1af59555af98b24f97679dd8604d11e75f718751 |
| RAM at pre-CALL CBA | 641c398f51ef0288b4623de51ecb959fe13331b77a76a408cc1a0c2c0b578ab8 |

The audit pins every native RAM state and raw CPU transcript row, checks
CPU JSON against the raw transcript, and checks original/loaded selector,
CAA call, and CAF branch bytes in every snapshot. Original unit 6F begins at
GAME.OVR+1DFE6; resident 0802 at START.EXE+8057. Relocation is 0824 paragraphs.
All media, screenshots, raw CPU/RAM captures, and builds remain ignored under
resources/oracle-runs/slot-a-loader-caf-gate-20261015 and resources/build-loader-caf-gate.

## Verification and adversarial limits

Strict C11 with Wconversion/Werror and full ASan/UBSan runs pass, including
139 C/original-byte Unicorn 2.1.4 instruction comparisons over all 16 CPU
fields and full RAM, plus four native checkpoints from the measured branch.
No flags are masked and execution state is never patched for comparison.
Controlled cases cover low AL 00/01/7F/80/FF, AH 00/AB, three incoming flag
sets, DF preservation, incoming ZF branch selection, independent full-word
PUSH, and SP 0000/0002/8000. SP=0000 wraps to FFFE; SP=0001 would produce a
split word at FFFF and blocks atomically before PUSH. Host bounds, A20 spans,
NULL arguments, unknown PC and wrong CS are also tested. Logic AF follows
the observed zero policy; multi-byte segment crossings and A20 wrap remain
unsupported typed memory blockers.

Reproduce with:

~~~sh
POOLRAD_UNICORN_PYTHON=resources/venv-string0356/bin/python \
  tests/replay/check_slot_a_loader_caf_gate.sh \
  resources/oracle-runs/slot-a-loader-caf-gate-20261015
~~~

Parity report: target_function=bounded unit6F CAF branch;
source_candidates=src/replay/slot_a_loader_caf_gate.c;
signals=original/relocated hashes, actual selector return frame, original-only
endpoint, native flags and stack bytes, controlled branches, full-state oracle;
score=strong (ordinal, not a probability); verdict=match within the stated
instruction and host-memory bounds. review_notes: the branch semantics repeat
prior evidence; zero AL is not a measured native outcome; allocator service,
allocator frame/body/return, heap state, D8F continuation, continuous read to
selector/allocator joins, callback/journal replacement, integration and playable
credit remain outside this slice. No shared code, immutable PR410 map, printer
or key code, 0425 progress guards, scoreboard, or identity ledger is changed.
