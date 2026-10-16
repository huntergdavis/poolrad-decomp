# Held same-invocation U6F1446 cold capture preparation

This offline harness emits a deterministic capture recipe, indexes already
captured numeric EV/RAM artifacts and checks bounded cut consistency. It
does not start, attach to, control or import an emulator/byte executor.
PM must assign scope and a serialized native slot before any future run.
The current host hold remains in effect. No new native evidence or C
handler/loader implementation is produced by this tooling.

The nine required ordered stops are:

| Label | Loaded boundary | Purpose |
| --- | --- | --- |
| entry | 2A8E:1446 | Input state and live outer frame |
| cold | 08D0:0025 | Actual cold INT3F with descriptor0 |
| handler | 12C6:02E6 | Actual vector, INT stack and IF/TF change |
| iret | 12C6:0337 | Live restored-register and IRET frame |
| patched | 08D0:0025 | Distinct post-IRET arrival and patched jump |
| u9b | 2CCD:0000 | Jump target and live callee return frame |
| callee_return | 2A8E:1579 | Actual U9B RETF4 continuation |
| outer_ret | 2A8E:157C | Live final outer far frame |
| outer_return | derived from outer_ret SS:SP | Actual generic RETF4 successor |

This extends the earlier six minimal cuts with explicit handler, IRET
and U9B snapshots. All nine belong to one fresh naturally reached first
saved-slot-A insertion. Initially gate the real caller at2A8E:19E3 after
natural title/menu/load-A input; audit loaded identities and live frames.
Remove only the previous owned breakpoint and arm the next one before
resuming. Distinguish the two0025 arrivals by ordinal and actual stub
contents. Do not force warm state, patch registers/RAM or fake DOS results.
At each stop record all16 raw EV fields and move MEMDUMP.BIN to the
recipe's distinct ignored file before resuming. Derive the final target
from the outer_ret frame; never assume19E6 for another frame. End before
the following02AE call or instruction at that successor.

Offline commands (the destination run directory must already exist):

```sh
python3 tools/replay/u6f_1446_cold_capture_preflight.py plan RUN_ID > resources/oracle-runs/RUN_ID/cold-plan.json
python3 tools/replay/u6f_1446_cold_capture_preflight.py index resources/oracle-runs/RUN_ID/cold-plan.json resources/oracle-runs/RUN_ID/ev-numeric.tsv > resources/oracle-runs/RUN_ID/cold-manifest.json
python3 tools/replay/u6f_1446_cold_capture_preflight.py check resources/oracle-runs/RUN_ID/cold-manifest.json
```

The generated plan remains HELD_NO_LAUNCH; it is not permission. It pins
the existing DOSv1.3 START/GAME/save hashes, fixed loaded route and
debugger EV/MEMDUMPBIN commands. The raw EV file header is label followed
by CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS; values are16-bit
hex words, with exactly the nine ordered labels above. Indexing reads
existing files only and records hashes. It does not authenticate who
captured them, natural UI input, one continuous process or input-media
identity. Retain the independent operator/action transcript and source
media/code identity audit. Declarations and matching run IDs alone do
not prove same-invocation provenance.

Check rejects mixed IDs, missing fields, invalid CPU words/order/PCs,
reused or out-of-run RAM paths, size/hash mismatches, a warm first stub,
incorrect live vector/INT/IRET frames, missing normal restoration,
incorrect jump/frame cleanup or a hardcoded outer successor. The outer
three-instruction cut requires the observed unchanged RAM interval and
defined preserved registers. A rejection may mean an unsupported path,
asynchronous effect or capture problem; it is not a verdict against
native execution. Required frames outside captured memory reject rather
than supplying invented bytes. The fixed loaded route and normal return
are the bounded domain; alternate dispatch/fatal paths need fresh scope.

The recipe uses655360-byte [00000,A0000) dumps. Check can also validate
declared1048576-byte snapshots in a separately supplied manifest. Neither
flat range proves EGA registers/latches/planes, BIOS queues or external
file state. Missing upper384KiB is never compared as initialized zero.
No device, RGB, first-writer or ordered transient-write claim is made.

A separate actual service transcript remains required for complete cold
effects: selected loader call/return, liveDS8504/43F0 indirect targets,
DOS seek/read handle/position/count/buffer and AX/CF/CPU/RAM effects,
optional second read, overlay counters/lists/patches and asynchronous
events. Endpoint snapshots cannot establish an ordered write journal.
The checker does not execute those services, prove their completeness,
audit all loaded instruction bytes or compare any C implementation.
Its successful JSON explicitly leaves native parity, complete-service,
device and ordered-write proofs false, with new strict credit0.

Reused: [merged cold map](U6F_1446_COLD_RETURN_DEPENDENCY_MAP.md),
[retained cold stops](SLOT_A_NATIVE_FIRST_INSERT_COLD_CPU.md),
[U9B return](SLOT_A_NATIVE_U9B_ENTRY_RETURN_PARITY.md),
[outer return](SLOT_A_NATIVE_U6F_POST_U9B_DIRECTORY.md), and private
static B packet PMi7589. Retained038/039/040 remain separate invocations;
none is relabeled as a complete combined run. Focused recall query
'U6F1446 cold capture harness' returned no indexed match; the cited
measured artifacts and merged source contracts are the reused evidence.

Validation uses synthetic offline artifacts and negative cases only.
There is no compiler, native/original-byte execution, shared CMake/API,
renderer or manifest edit in this B gate. Whole/native parity: unclear.
New whole-function, native, identity and strict credit: ZERO.
