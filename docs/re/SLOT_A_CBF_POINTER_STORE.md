# Actual saved-A CBF returned-pointer stores

This independent bounded adapter starts at actual unit6F CBF after native
GetMem returns and models three instructions: LESDI,[BP+A], ES:[DI+C8]=AX,
and ES:[DI+CA]=DX. It stops at CCC before the next pointer reload or use.
No allocator result is supplied or inserted into a continuous C replay.

Reused context is the approved CBA frame/return evidence at
0eecc140b30a13bdcefaec6e96410665cd0ad999 and merged
[SYSTEM23F wrapper evidence](SYSTEM_GETMEM_23F_NATIVE_ABI.md).
The CBA adapter is not a linked C dependency. The new source compiles alone
with merged state declarations and unchanged shared audit utilities. The
bounded deja query for CBF/CC2/CC7/CCC pointer stores expired after35seconds
without output; inspected repository code/evidence is cited here instead.
This slice adds no game-function identity/count or playable-route credit.

## Fresh original-only capture before C

On 2026-10-15, unchanged original saved A reached C26, then actual CBA.
Eight CPU/full655360RAM states were frozen before new C was written:
CBA,23F,245,248,CBF,CC2,CC7,CCC. The owned emulator was stopped before the
heavy C/original-byte and sanitizer runs, which ran sequentially.

Three independent original-byte Unicorn endpoint checks establish provenance:
CBA->23F (one far CALL),23F->245 (three prefix instructions), and248->CBF
(three wrapper-return instructions). All CPU fields and RAM match native
endpoints. These seven original-only instructions and three endpoint checks
are excluded from C instruction/native-checkpoint counts. The native-only
allocator245->248 interval changes25 RAM bytes; its exact difference set is
pinned and excluded. There is no C join across that interval.

Actual CBF input: CS2A8E DS14A0 ES44AC SS1D16 FS=GS0 SP33EC BP37A6
AX0008 BX44B6 CX0008 DX44B2 SI44B2 DI0008 FLAGS7202. The far pointer at
SS:[BP+A]=SS:37B0 contains449A:0008. Returned DX:AX is44B2:0008.

| Original instruction | Actual result |
| --- | --- |
| CBF LES DI,[BP+A] | ES=449A,DI=0008; entire RAM unchanged |
| CC2 MOV ES:[DI+C8],AX | word449A:00D0=0008 |
| CC7 MOV ES:[DI+CA],DX | word449A:00D2=44B2 |

Exactly three net RAM bytes change: physical44A70,44A72,44A73. The four-byte
field was0000:0000 and becomes44B2:0008. Flags7202 and all other registers
remain unchanged, except IP and the explicitly loaded ES/DI. C stops atCCC
with continuation metadata2A8E:CCC before that next LES. No meaning beyond
these observed record offsets is assigned to the fields.

## Input/code identity

| Input or span | SHA-256 |
| --- | --- |
| START.EXE,65231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR,206105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT,13137 bytes | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original/loaded CBF..CCB,13 bytes | 891fc163483ebfe79aac69d2a2e17acf3a3af3bb9fed5c19cf822301774177dc |
| SYSTEM23F..253 wrapper,21 bytes | d4ceab56ec05612e8ba6c90dab11d9da62f1f38235503781f80d636daa63d346 |
| Frozen raw transcript | 4dd8420de126889728142c9ca8893411f7b12b8abf358434d0c3a1dc7ab85466 |
| RAM at CBF | e521c5b17873f8e5af668c8a5d15dbd0dcee21ff87df226d07c315b928451b41 |
| RAM at CCC | a8f0716b57a0298e2532a388f1f99e1fcd9e62df87f784b5a4e8de38db2094a0 |

The audit pins every native RAM hash and raw transcript CPU row, validates
CPU JSON against the raw rows, and checks original/loaded CBF, CBA call, and
SYSTEM wrapper bytes in every snapshot. SYSTEM starts at START+AC76 and
unit6F at GAME+1DFE6; the CBA call relocates AF8 to131C. The native root is
resources/oracle-runs/slot-a-cbf-pointer-store-20261015. All media, raw
captures, screenshots, build products and logs stay under ignored resources/.

## Verification and adversarial limits

Strict C11 with Wconversion/Werror passes focused tests and124 C versus
original-byte Unicorn2.1.4 instruction comparisons of all16 CPU fields and
full655360 RAM bytes. Three post-input native checkpoints match CC2,CC7,CCC.
The complete suite also passes ASan/UBSan. No flag masks or execution-state
rewrites are used. Reports never invent a call target for the CCC continuation.

Controlled cases vary full AX/DX words (zero, high bytes, mixed words,FFFF),
incoming flags/DF, record pointers in different segments, and wrapped effective
offsets. Source-frame aliases prove LES reads the pointer once: stores can
replace its original stack words, and the second store still uses retained
ES/DI. A second alias uses a different segment with the same physical target.
BP=FFF6 wraps the LES offset to0000; DI=FF38 wraps the store offsets to0000
and0002. Independent CC7 input proves DX supplies the complete segment word.

Unsupported multi-byte crossings block atomically at the current instruction:
LES at offsetFFFE, first store atFFFF, or second store atFFFF after a valid
first store. Earlier completed effects remain; the failing instruction changes
neither CPU nor RAM. Host-image bounds, A20 spans, NULL arguments, wrong CS,
and unknown PC also stop explicitly. The contract requires independent live
state and audited code, RAM at most1MiB, nonoverlapping state/report/RAM,
and each multi-byte span inside one segment and the host image.

~~~sh
POOLRAD_UNICORN_PYTHON=resources/venv-string0356/bin/python \
  tests/replay/check_slot_a_cbf_pointer_store.sh \
  resources/oracle-runs/slot-a-cbf-pointer-store-20261015
~~~

Parity report: target_function=bounded unit6F CBF pointer stores;
source_candidates=src/replay/slot_a_cbf_pointer_store.c;
signals=original/loaded hashes, actual GetMem return and record pointer,
three original-only provenance endpoints, native store addresses/bytes,
full-state differential, alias/wrap/blocker checks;
score=strong (ordinal, not probability); verdict=match within the stated
three-instruction and memory bounds. review_notes: original allocator effects
and25 changed bytes remain excluded; no allocator implementation, continuous
C allocation-to-store join, CCC pointer reload/use, D8F context, DOS join,
callback/journal replacement, new function/count/playable credit. Shared code,
CMake, immutable PR410 map, printer/key code,0425 progress guards, identity
ledger and scoreboard remain untouched.
