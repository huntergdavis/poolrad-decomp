# U6F1446 cold INT3F/handler/IRET source and ownership map

This supplements the merged preflight with source/API ownership and
launcher prerequisites. It does not duplicate or modify the preflight,
translate a handler/loader, assign a native slot or permit launch.
PMi7789 authorizes this separate read-only map. Existing API ownership
is retained; relative clearances do not establish global absence.

## Source boundary and responsibility matrix

| Actual boundary | Existing source contract | Ownership/remaining gate |
| --- | --- | --- |
| Original00AC:0025, loaded08D0:0025 INT3F | First effect unresolved by u6f_1446_fullstate; cold callback covers entire event | No new implementation assigned; requires PM gate for real vector/frame entry |
| Original0AA2:02E6, loaded12C6:02E6 | Handler prefix expects INT frame ALREADY present | Existing resident API unchanged; cannot substitute it for INT setup |
| Handler02F6 STI | Enables later asynchronous interrupts | Preserve raw flags/event ordering; no unsupported no-interrupt assumption |
| Handler0303 or0314 CALL0338 | Near return0306 or0317 according to source branch | Prefix stops BEFORE call; selected live frame must be pushed by caller |
| Loader0338 onward | Prefix stops BEFORE0553 or0435; other leaves/stages separate | Existing helper ownership unchanged; complete load/list/patch/services open |
| Loader03AA CALL[DS:8504] ->03AE | Retained pointer targets0268 on measured route | Actual indirect target plus seek/read/optional second-read effects required |
| Handler0317..032A | Reverse BP/FLAGS exchange on nonzero branch; POPdescriptor; comparison/SBB | Tail not covered by handler prefix; source/defined flags remain obligations |
| Handler032A CALL[DS:43F0] ->032E | Retained route points12C6:02E5 RETF | One measured target does not implement other targets or identify global owner |
| Handler032E..0337 | POP ES,DS,DI,SI,DX,CX,BX,AX,BP; IRET0337 | All saved registers plus real rewound IP/CS/FLAGS must be restored |
| Patched08D0:0025 ->2CCD:0000 | Existing caller composition then reviewed U9B | Complete cold effects cannot be replaced by a recorded net delta |

Original source was rechecked without execution from pinned START.EXE:
handler[02E6,0338)82B/42; loader[0338,03E9)177B/64;
post-loader[0317,032A)19B/7; restore/IRET[032E,0338)10B/10.
Exact SHA256s and11 unchanged dependency git blobs/hashes are recorded
in U6F_1446_HANDLER_OWNERSHIP_FACTS.json. Loaded handler identity from
retained038 also matches its existing relocated82-byte SHA. Raw spans
and disassembly remain ignored; this map exports addresses/hash/counts.

The source branches on rewound savedIP:0303 path returns0306 then0320;
the observed nonzero path addsBP6 and exchanges saved BP/FLAGS around
CALL0314, then reverses at0317/031A/031D. Do not apply its exchange to
the alternate branch or force a source branch with a chosen constant.

## Confirmed scope and CPU/RAM compatibility

pN owns only this new source-map document/metadata and its separate
02AE return fixture. Native peer i7809, p5i7810 and censusi7811 explicitly
report no overlap/reservation relative to this READ-ONLY handler/loader
map. Existing resident/helper/service/launcher files remain unchanged
and are not reassigned. A future implementation needs PM's explicit
target/API/file assignment and any additional relevant peer screens.
No implementation owner or global availability is inferred from silence.

The resident handler/loader CPU type has14 fields; the caller and
preflight require16 includingFS/GS. Future composition must carry those
values and reconcile resident segment-offset word wrapping with the
caller's consecutive-physical-byte/A20 convention. Ordinary retained
frames do not decide vector-read/push ordering for alias edge cases.
A prefix checkpoint is not a successful complete cold callback.

The existing complete-event validator requires normal return to08D0:0025,
restored SS:SP, a projected far jump to2CCD:0000 and saved INT frame
agreeing with returned CPU. It validates supplied old-byte chains but
does not implement or prove complete loader/IRET/service effects.
The measured indirect targets and restored snapshots are bounded facts.

## Checkpoints derived from the source

Keep the existing nine required cuts unchanged:1446 entry, cold0025,
handler02E6, pre-IRET0337, patched0025, U9B0000,1579,157C and live-derived
outer successor. They cover actual INT frame creation and IRET/frame
consumption, rather than assuming a helper started with valid state.

For attribution, record additional ordered diagnostic boundaries:
selected pre-loader0303 or0314, loader-entry0338, matching return0306 or0317,
pre-indirect032A and its live far target/return. Record actual DS8504
load service and DOSseek/read/optional-second-read boundaries separately.
These diagnostic requirements supplement the existing tool; they are
not a second preflight or newly invented complete-service proof.

Use the original source/data and actual loaded identity at each stop.
Handler-prefix AT_LOADER is BEFORE CALL; loader-prefix results likewise
stop before unresolved calls. Do not feed a pre-call state to an API
requiring post-CALL entry or feed an intermediate state to a complete
cold callback. Repeated0025 stops need distinct ordinal/stub identity.
Derived outer successor comes from the final live SS frame, not a fixed
19E6 assumption. Every required artifact belongs to one natural run.

## Launcher prerequisites; current hold remains

Before any future launch PM must assign the serialized slot/operator and
scope. Pin the DOSBox-X executable/build digest, debugger-capable build,
PTY attachment, isolated display/session, actual arguments/configuration
and run action transcript. The existing capture evidence requires a PTY;
no --version, GUI, preparation copy or launcher process was run here.

The unchanged prepare_run.sh requires a new valid run name, owner-supplied
ignored input tree, pinned START/GAME hashes and no symlink inputs. It
creates an isolated run and input manifest. The unchanged launch.sh
requires a prepared run beneath this repo's ignored oracle-runs, its
manifest, START and POOL.CFG, bwrap and dosbox-x. It isolates networking,
binds the run writable and redirects XDG/cache/save paths there.
These are read source contracts, not checks of current host readiness
or authorization to invoke either script.

Input save identity, natural title/menu/load-A sequence and first19E3
caller must be retained independently. Audit actual IVT, relocated code,
cold descriptor and live frames; no register/RAM patches, forced warm
thunk, helper jump or invented service success. Store distinct raw16CPU
and declared RAM files plus original action/service transcript.640KiB
omits upper memory/video aperture; flat RAM omits EGA registers/latches/
planes and queues/file state. Endpoint differences are not write journals.
The merged checker proves only cut consistency, not these missing proofs.

Queued B lead for a NEW PM gate: the actual INT3F entry/vector/frame
transition before existing02E6 prefix, with explicit16CPU and bounded
alias/A20 semantics. No first-entry fixture may return complete cold
success. Full loader, tail/indirect, IRET and actual services stay open.

Reused: merged [cold dependency map](U6F_1446_COLD_RETURN_DEPENDENCY_MAP.md),
[held preflight](U6F_1446_COLD_CAPTURE_PREFLIGHT.md),
[handler prefix](RESIDENT_HANDLER_PREFIX_02E6.md),
[composed read](RESIDENT_LOADER_COMPOSED_028F.md), retained038 evidence
and private B packet PMi7589. Source spans and retained loaded-code hash
were rechecked statically; focused recall timed out and supplies no
absence/parity evidence. Existing preflight/API/launcher blobs are pinned
unchanged in the companion JSON. Complete parity remains unclear.
No native, complete-service, device, ordered-write or strict credit added.
