# U19 selected store hook API and byte boundary proposal

This is a source-only proposal for a future, default-off selected-store
observer. It adds no live hook, issuer, wire field, packet, native capture, or
first-writer claim. A bounded 12-second `deja` lookup for the U19 selected
assignment and callback paths returned no indexed result. Reused checked-in
work is the [hook placement](U19_ROW14_ACTUAL_STORE_HOOK_PLACEMENT_SOURCE.md),
[provisional receipt](U19_ROW14_PROVISIONAL_WRITE_RECEIPT_GATE.md),
[consumer contract](U19_ROW14_ACTUAL_STORE_INPUT_CONSUMER_CONTRACT.md), and
[common issuer proposal](U19_ROW14_HOST_ISSUER_PROPAGATION_PROPOSAL.md).

## Pinned source boundary

The original guest candidate span is loaded START `[0x7417,0x7501)`, 234
bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
Its four candidate byte-store PCs are RES_0709:+0412/+0438/+0445/+0451.
These are candidate instructions, not measured selections for the saved run.
The ignored DOSBox-X source revision is
`5a9c7c53481c2e933948039c33256776653f4faf`. Inclusive line windows
retain original newlines; local source-copy hashes matched the prior note:

| Source window | Bytes | SHA-256 | Proposed observation boundary |
| --- | ---: | --- | --- |
| `include/paging.h:463-467` | 240 | `4363344295a437947e2b00d654c0cccfc71584051e46b533eef0a69fa307b49a` | A direct TLB host write bypasses a page-handler callback. |
| `vga_memory.cpp:579-629` | 2,211 | `01c545faf450be21ea04391d23602d5433e9112934c66a2fc83f3da38b13c7cd` | Generic unchained handler; update trigger and address mapping precede assignment. |
| `vga_memory.cpp:376-384` | 203 | `a67274a7a8f66dcecb01d6ad08ff9c5680fdfdef47d40d290eb4fb471d182230` | Generic final backing-word assignment at line 383. |
| `vga_memory.cpp:636-668` | 1,405 | `029a87e8d754cfcd53e564ce48603cbc915da1597459c8fbc25d6a09b5e5f69a` | Fast unchained handler maps the address and performs its own assignment. |
| `vga_memory.cpp:640-649` | 510 | `cfd70630f5674d777f197c9ffce1ff8732a6d5c4fb9cc5e010b27f31a4c4fd75` | Fast resolved backing-word assignment at lines 647-648. |

The generic callback must receive the *final* `planeaddr`, after memory mask
and odd/even transforms at lines 341-374. The fast callback must receive the
mapped `addr` from lines 640-642. Both refer to the actual four-byte backing
word; guest A000:118F or a page number alone cannot identify it because bank,
mask, and alias transforms intervene. The `vga_vram_write_trigger_update_planar_mem`
calls at lines 590/646 are earlier, separate effects. Static source does not
prove that either handler, rather than a direct-TLB or overridden page path,
ran for the selected saved U19 instruction.

## Proposed opt-in interface and timing semantics

The existing `Capture::armed_frame` and `site` identify one guest attempt.
An interior adapter would pass an immutable record of the armed and observed
`CS:IP/ES:DI`, guest linear address and width, actual handler kind, final
backing pointer and word index, and live GC/Sequencer/latch state to a
handler-local observer. It would copy four backing bytes before and after
**the original single assignment**, without an extra guest read. It must
preserve the original `apply_write` exactly once on success and all rejection
paths; `Capture::write_count` counts wrapper callbacks, not native backing
assignments.

The minimal conceptual calls are `assignment_enter(attempt, location, state)`
immediately before the generic or fast assignment and
`assignment_exit(attempt, location, state)` immediately after it. Their
same-issuer indices delimit `assignment_enter < assignment <
assignment_exit`. The first index is **before** the store; the second is
**after** the store. Neither callback can retroactively mint an exact
`actual_store` instant. A future reviewed operation must make the assignment
itself a one-to-one ordered event under p9's named run-wide
`clock_origin`/`clock_domain` before any `actual_store` index or accepted
write reference is exposed to pC. A callback made later, a wrapper
`apply_call_enter/exit` stamp, emission time, host nanoseconds, CPU candidate
ordinal, or backing index is not that event.

Until then, the observer may retain only diagnostic boundaries. If a future
single issuer supports the assignment event, the proposed order is
`before_sample < apply_call_enter < assignment_enter < actual_store <
assignment_exit < apply_call_exit < after_sample`, with strictly increasing
indices in the same run, origin, and domain. The code must establish which
handler executed and that exactly one interior assignment belongs to this
arm. An exact-store sidecar is a separately reviewed version; the current v2
producer, plane, pair, and RGB packet shapes stay untouched.

## Failure and selected-receipt boundary

Reject an exact-store claim for direct-TLB bypass, empty/RAM/MMIO/overridden
handler, unsupported mode, wrong guest frame/target/width, invalid backing,
alias ambiguity, zero or multiple interior hooks, unstable state sample,
wrong immediate successor, or failed/duplicate adapter emission. An armed
attempt that only calls the opaque wrapper has a diagnostic call interval,
not an accepted actual store. Failed, idle, and finished passthrough attempts
may retain only indices truly issued for diagnosis; they cannot reuse a
reference or receive an accepted selected receipt.

After a sourced one-to-one assignment event, successful `finish` at the
audited successor, and one audited adapter emission, a versioned consumer
could late-bind exactly `run_token`, `arm_token`, `stream_id`,
`plane_event_id`, `raw_sha256`, `candidate_ordinal`, `pc`, and
`successor_pc`. The four complete plane bytes before and after determine
`changed_planes_mask`. A zero mask is a valid no-value-change result only for
that accepted event; it cannot encode an absent or failed assignment. Neither
one accepted event nor its mask identifies the first value-changing store
across the saved route, a displayed page, or an RGB frame.

pN cleared this one-doc source scope in durable `i10776`, pR in `i10788`,
and pC in `i10799`. pR and pC specifically required the before/after timing
distinction above. p9 retains common-issuer/backend ownership, pC the
versioned consumer, and pR the passive plane/RGB path. These relative
clearances do not authorize a live hook, native run, schema change, or
publication. Status: `SOURCE_ONLY_ASSIGNMENT_INTERVAL_PROPOSAL`.
