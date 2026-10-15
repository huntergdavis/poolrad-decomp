# Actual saved-A CBA GetMem frame and first return boundaries

Two independent bounded C inputs bind the actual CAF allocation occurrence:
CBA's real far CALL and wrapper prefix stop at SYSTEM 245 before the allocator
core near CALL; separately captured actual core return248 supplies the wrapper
tail through RETF2 to CBF, before any loader pointer store. The allocator
interval245->248 changes 25 RAM bytes in original execution and is excluded
from C coverage. No allocator result is invented or inserted into a continuous
C prefix-to-return chain.

This adapter reuses the merged [SYSTEM23F wrapper](SYSTEM_GETMEM_23F_NATIVE_ABI.md)
through src/replay/system_getmem_23f_abi.c and its declarations. An explicit PC
whitelist accepts only the wrapper prefix/return/error instructions, plus the
new CBA far CALL and CBF stop. It cannot enter the existing heap-tail or
requested-size models. The earlier wrapper occurrence at caller19B5 allocated
011D and returned449A:0008. This actual CAF occurrence requests003F and returns
44B2:0008; it needs its own original-only frame and native state evidence.
The preceding CAF adapter at 2e92653f4685a42409c447e000e029484f36c7b2 is
cited context, not a linked unmerged C dependency. No new function identity,
count, integration, or playable credit is claimed. The required bounded deja
query for actual CAF/CBA GetMem entry expired35s without reusable output;
merged wrapper code and evidence were inspected directly and reused as cited.

## Native capture before C implementation

The pinned original saved A ran without guest CPU/RAM edits on 2026-10-15.
A C26 stop selected the later loader context before CBA. Nine original-only
CPU/full655360RAM states were frozen: CBA,23F,240,242,245,248,249,24B,CBF.
The owned emulator was then stopped before any new C was written. The heavy
Unicorn and sanitizer runs occurred afterward and sequentially.

Actual CBA input: CS2A8E DS14A0 ES=SS1D16 FS=GS0 SP33EA BP37A6 AX003F
BX0006 CX0000 DX3368 SI33D0 DI3338 FLAGS7202. Its argument is SS33EA=003F.

| Original instruction | Actual post-instruction state |
| --- | --- |
| 2A8E:CBA far CALL131C:023F | SP33E6, frame words [0CBF,2A8E,003F] |
| 131C:23F PUSH BP | SP33E4, saved BP37A6 |
| 240 MOV BP,SP | BP33E4 |
| 242 MOV AX,[BP+6] | AX003F, IP245 |

At245 the adapter returns typed BLOCKED_CORE with original AF8:0308 near
call, return248 metadata. No core near frame is pushed by C. Original native
execution continues across this excluded interval and captures its first
return248. The old near return word248 remains at SS33E2 in that snapshot.
The measured interval changes RAM at physical1D154..1D155 (requested-size
scratch),20536..20543 (stack),44ACA,44B68..44B6F. These25 changed bytes are
pinned and excluded; they do not establish C coverage of allocator effects.

Independent actual248 input: CS131C DS14A0 ES44AC SS1D16 FS=GS0
SP=BP33E4 AX0008 BX44B6 CX0008 DX44B2 SI44B2 DI0008 FLAGS7202.

| Original instruction | Actual post-instruction state |
| --- | --- |
| 248 POP BP | BP37A6,SP33E6 |
| 249 JC24E | CF0 chooses24B |
| 24B RETF2 | CS:IP2A8E:CBF,SP33EC |

The wrapper tail changes no RAM and preserves returned DX:AX=44B2:0008.
This value comes from the actual native248 input; C does not compute or fake
an allocation. CBF blocks before its next loader instruction. Controlled CF1
takes24E MOV AX,00CB then251 JMP E2 and stops before runtime-error effects.
The failure route is controlled evidence, not a second native outcome.

## Pinned identity and reproduction

| Input or code span | SHA-256 |
| --- | --- |
| START.EXE,65231 bytes | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR,206105 bytes | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT,13137 bytes | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original/loaded SYSTEM23F..253,21 bytes | d4ceab56ec05612e8ba6c90dab11d9da62f1f38235503781f80d636daa63d346 |
| Raw nine-row transcript | d37b7f81c8ac7ecc5f866a376cbed7d4b7c5c4f8936258e3608dfd8ea72458e5 |
| RAM at actualCBA | faf24e60219a63671c2b44eb532a28edc1d803b69f7506f4b3592a1714358a2c |
| RAM at actualGetMem23F | e67d7dca4163ea8c12ec26a7933f0d17c104d771459877e739b1ce5110712e53 |
| RAM at pre-core245 | ce25e587715c7a5081ab377f5e9686bf8dc61ded5d8594aa460f3833ce72550b |
| RAM at actual248/CBF | d6ef2d838eaeb89cc4875cfe9ec4fae591f067e719e34957a6d8fc83af2c0356 |

The audit pins every raw transcript CPU row and native RAM checksum, checks
CPU JSON against the raw rows, and verifies original/loaded call and wrapper
bytes in every image. The CBA call's original AF8 relocates to131C. Original
SYSTEM code starts at START+AC76; unit6F at GAME+1DFE6. All media, captures,
builds and logs remain under ignored resources/.

~~~sh
POOLRAD_UNICORN_PYTHON=resources/venv-string0356/bin/python \
  tests/replay/check_slot_a_cba_getmem_frame.sh \
  resources/oracle-runs/slot-a-cba-getmem-frame-20261015
~~~

The focused C tests and strict C11/Wconversion/Werror audit pass138 C versus
original-byte Unicorn2.1.4 comparisons over all16CPU fields/full655360RAM,
with seven native post-input checkpoints: four prefix and three independent
tail. Full ASan/UBSan repeats the same suite. No flag masks, state rewriting,
or snapshot bridge is used. Controlled cases cover four full-word sizes,
AH/argument disagreement, flags/DF/carry, wrapped prefixSP and RETF2SP,
BP-relative argument offset wrap, alternate DX:AX payloads and real unknown
far-return targets. Unsupported crossing far frames at SP1/2/3, host bounds,
A20 spans, NULL, wrong CS and out-of-slice core PC block atomically. The
merged wrapper's existing one-segment multi-byte span restriction is retained.

Parity report: target_function=actual unit6F CBA frame and independent
SYSTEM248 return wrapper; source_candidates=slot_a_cba_getmem_frame.c plus
merged system_getmem_23f_abi.c; signals=original/loaded identities, actual far
frame, actual first core return, full-state oracle, native checkpoints,
controlled error/wrap/return cases; score=strong (ordinal,not probability);
verdict=match within stated fragments and memory bounds. review_notes:
wrapper semantics reuse prior recovery;25 allocator interval changes are
native-only; no allocator core/body/output implementation, prefix-to-return
C join, CBF pointer stores, D8F context, DOS join, callback/journal replacement,
identity/count/playable claim. Shared code, CMake, immutablePR410 map, printer,
key dispatch,0425 progress guards and scoreboards are untouched.
