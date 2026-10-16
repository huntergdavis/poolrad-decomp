# U6F +1446 cold-return dependency and future native gate

PR #614 composes the 313-byte U6F caller, SYSTEM helpers, warm stub and
reviewed U9B +0000. Its first unsupported cold effect is execution of
`08D0:0025 INT 3Fh`, before the translated handler prefix even starts.
The adapter delegates the entire interrupt/loader/return interval to
`pr_u6f_1446_cold_dispatch`. This source-only map does not close that
dependency or add whole-function, native, identity or strict credit.

The bounded recall query found no indexed matches. This map reuses and
cites [the composition](U6F_1446_FULLSTATE.md), [cold CPU stops](SLOT_A_NATIVE_FIRST_INSERT_COLD_CPU.md),
[handler prefix](RESIDENT_HANDLER_PREFIX_02E6.md), [loader prefix](RESIDENT_LOADER_PREFIX_0338.md),
[0355](RESIDENT_LOADER_0355.md), [0366](RESIDENT_LOADER_0366.md), and
[earlier insertion return](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md).
Original-source disassembly and retained run-038 hashes/frame/pointer
assertions were rechecked without starting an emulator or byte executor.

Inputs are DOS v1.3 START.EXE, 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
and GAME.OVR, 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Saved slot A is 13,137 bytes, SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Original/loaded handler spans are 82 bytes / 42 instructions; original
SHA-256 `6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c`,
loaded SHA-256 `5c69a0e6a0a9141fae2d0381a4ebe4252981d54cbad01d335a3f5f73380fd6ef`.
Loader +0338..03E8 is 177 bytes / 64 instructions, original/loaded SHA-256
`58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991`.

| Boundary | Actual ABI/effect | Remaining obligation |
| --- | --- | --- |
| U6F +1574 to cold stub | Far CALL pushes return 2A8E:1579; argument 449A:0008 remains above frame | Already modeled; destination descriptor is U9B +0000, never substitute +01AF |
| INT 3Fh to 12C6:02E6 | Push FLAGS, CS and IP=0027; clear IF/TF; read live vector 0000:00FC | First unsupported effect in the merged adapter; existing handler API expects this frame already present |
| Handler +02E6..0314 | Save registers; load relocated DS; rewind saved IP to 0025; exchange saved BP/FLAGS around loader | Reviewed prefix stops before CALL +0338 (alternate branch stops at +0303) |
| Loader +0338 onward | Live overlay counters, descriptor state, calls, memory/load/patch effects | Existing prefix/helper stages are separate; they are not a full loader composition |
| +03AA indirect CALL [DS:8504] | Retained saved pointer 12C6:0268; post-call +03AE | Read/load dependencies must execute with actual service events, not presumed success |
| Handler +0317..032A | Reverse temporary exchange, POP descriptor, compare ES:[0012], SBB AX,AX | Tail is outside the prefix API; preserve defined flags and stack order |
| +032A indirect far CALL [DS:43F0] | Retained run-038 pointer 12C6:02E5, whose instruction is RETF | Read live pointer and frame; this one observed target does not prove other dispatch paths |
| +032E..0337 IRET | Restore eight registers and BP; pop rewound IP, CS, FLAGS | Normal measured return is 08D0:0025, original SS:SP restored |
| Patched stub to U9B | Actual far jump 2CCD:0000, same captured RAM | Existing U9B composition then returns to U6F +1579 |
| U6F +1579/+157B/+157C | MOV SP,BP; POP BP; generic RETF 4 | Must honor live outer frame; saved caller successor is +19E6 |

At the retained pre-INT stop, SS:SP=1D16:3796 and FLAGS=7202.
The handler-entry frame at SS:3790/3792/3794 is 0027/08D0/7202;
handler-entry FLAGS=7002. Exactly six captured bytes change in one run.
The later frame retains rewound 0025, even after IRET restores SP=3796.
Run-038 cold-stub to patched-stub changes 1,210 bytes in 20 address runs
net. The separate pre-far-CALL to U9B-entry delta is 1,214 bytes in 20
runs; the two bounds differ by the CALL frame. Endpoint differences are
not an ordered write journal.
In particular, overwritten/restored INT-frame, loader and helper stack
bytes cannot be reconstructed as execution history from these endpoints.

The callback takes read-only pre-INT 16-field CPU and exactly 1MiB RAM,
and supplies complete post-handler CPU plus ordered `(linear, old, new)`
byte writes, including transient INT/handler/loader/patch writes. It must
not mutate inputs. The validator checks count, address bounds and repeated
old-byte chains before committing; projected jump must target 2CCD:0000,
post CS:IP must be 08D0:0025, SS:SP restored, and projected saved INT frame
must agree with returned IP/CS/FLAGS. These checks do not prove journal
completeness or implement the callback. Missing dependency blocks;
arbitrary handler diversion is outside this normal-return contract.
Existing handler/loader APIs have 14 CPU fields and segment-offset word
wrap conventions; composition also requires explicit FS/GS preservation
and reconciliation with the caller's physical-word/A20 conventions.

Future native recipe, only after the host hold is lifted and a slot is
assigned: use a new isolated ignored run of the pinned inputs. Reach the
visible title/menu/load-slot UI naturally, choose slot A, and gate at the
actual first U6F +19E3 caller. Do not patch registers/RAM, jump to a helper,
force the thunk warm, or alter service results. Verify loaded identities,
cold stub descriptor and actual frame before proceeding.

Record all 16 CPU fields and captured RAM in that same invocation at
+1446 entry, +1574, cold stub, handler entry, pre/post loader +0314/+0317,
pre-indirect +032A, patched stub, U9B +0000, U6F +1579, pre-RETF +157C,
and actual caller successor +19E6. Re-arm repeated stub stops so pre-INT
and post-IRET are distinguished by arrival order and patch identity.
Before +157C, obtain actual return IP/CS and argument words from SS:SP;
verify the successor from that frame rather than hardcoding +19E6 for
other callers. At the saved entry SP=37B0; normal RETF4 yields SP=37B8.
Store distinct RAM files, numeric transcripts, hashes and stop order.
Capture coverage must be stated explicitly: existing dumps are 655,360
bytes, with no native evidence for the upper 384KiB. Future whole-RAM
claims need appropriate coverage; do not compare an initialized tail as
though it were captured native RAM. Ordered transient-write claims need
actual instruction/write attribution in addition to endpoint dumps.

Run-037 has full entry/pre-dispatch fields but stops at U9B entry. The
earlier insertion-return run has the actual returned caller stop and RAM,
but only a partial CPU tuple. Neither is a same-invocation 16-field
entry-to-return proof, and joining different runs cannot manufacture one.
Future parity must execute the dependency in C; replaying a native net
delta as an opaque callback is still a supplied dependency, even if its
endpoint matches. Heavy native work remains held; no capture is launched
by this note. The U6F +02AE message/UI implementation is a separate lead.

Parity review: `target_function=U6F1446 cold interrupt-to-return interval`;
`source_candidates=u6f_1446_fullstate.c and existing resident prefixes`;
`signals=hash-locked static call/return graph, retained frame/vector/pointer
observations, explicit callback validation, partial API boundaries`;
`score=four evidence categories, not a probability`; `verdict=unclear`
for complete cold execution. Endpoint and prefix evidence establish the
bounded observations above; missing dependency execution prevents promotion.
