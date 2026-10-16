# U19 selected backing-assignment hook placement source gate

This is a local source-only placement note. It adds no hook, clock, packet,
runtime capture, or first value-changing writer. A bounded 12-second deja
search for the U19 assignment and callback paths returned no indexed result.
It reuses the [original guest store bound](U19_ROW14_FIRST_PLANE_CHANGE_SOURCE_BOUND.md),
[selected assignment bound](U19_ROW14_ACTUAL_STORE_ASSIGNMENT_GATE.md), and
[provisional receipt bound](U19_ROW14_PROVISIONAL_WRITE_RECEIPT_GATE.md).

## Original guest and host source pins

The expanded START drawer is loaded image `[0x7417,0x7501)`, 234 bytes,
SHA-256 `3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
Its candidate byte-store sites are RES_0709:+0412/+0438/+0445/+0451
(loaded 0x74A2/0x74C8/0x74D5/0x74E1). The first two are three-byte sites
with SHA-256 `4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e`;
the latter two are four-byte sites with SHA-256
`5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29`.
These are candidate guest stores, not proof of a reached host handler.

The ignored DOSBox-X reference is committed revision
`5a9c7c53481c2e933948039c33256776653f4faf`. The windows below are
inclusive and hashed from that commit, not its locally instrumented copy.

| Source window | Bytes | SHA-256 | Bounded role |
| --- | ---: | --- | --- |
| `include/paging.h:463-467` | 240 | `4363344295a437947e2b00d654c0cccfc71584051e46b533eef0a69fa307b49a` | Guest byte write may use a direct TLB host pointer or dispatch to the page handler. |
| `src/hardware/vga_memory.cpp:2694-2758` | 2,770 | `891693a6a306953b8176f1a742f9b3e2760fc8d694b0135bf02faa90abb9453d` | Mode/chained/complexity selection chooses `uvga_fast` or `uvga` only on the unchained EGA/VGA branch. |
| `src/hardware/vga_memory.cpp:2766-2829` | 2,267 | `15e0304c61444c5305903655008f8e15493fd11cc1b839026abf19358a9bd15a` | GC map selection installs a page range; another page handler can override it. |
| `src/hardware/vga_memory.cpp:579-629` | 2,211 | `01c545faf450be21ea04391d23602d5433e9112934c66a2fc83f3da38b13c7cd` | Generic unchained `writeb` maps the address and enters `VGA_Generic_Write_Handler<false>`. |
| `src/hardware/vga_memory.cpp:636-668` | 1,405 | `029a87e8d754cfcd53e564ce48603cbc915da1597459c8fbc25d6a09b5e5f69a` | Fast unchained `writeb` maps the address and uses its separate masked replacement. |
| `src/hardware/vga_memory.cpp:376-384` | 203 | `a67274a7a8f66dcecb01d6ad08ff9c5680fdfdef47d40d290eb4fb471d182230` | Generic assignment to the resolved backing word is line 383. |
| `src/hardware/vga_memory.cpp:640-649` | 510 | `cfd70630f5674d777f197c9ffce1ff8732a6d5c4fb9cc5e010b27f31a4c4fd75` | Fast backing-word assignment is lines 647-648. |

## Placement and resolved-index requirements

The merged observer in `tools/replay/u19_row14_in_handler_capture.hpp:99-153`
samples around an opaque `apply_write` call. Its entry and exit can bound
an assignment but cannot timestamp the assignment itself. On the generic
branch, the prospective interior hook belongs at line 383 after the
`ModeOperation`, live mask, and prior backing-word combination. The resolved
word index is the final `planeaddr` after the odd/even and memory-mask
transforms at lines 341-374. On the fast branch, the corresponding hook
belongs at lines 647-648, using the mapped `addr` from lines 640-642.
Neither source path is selected for the saved run by these static windows.

A future issuer must bind the selected guest PC/ES:DI and candidate ordinal
to the *reached* page handler, bank, resolved word index, live GC/Sequencer
state and latch, and complete four-plane bytes immediately before and after
the interior assignment. It must preserve the original write once and must
not make an extra guest memory read that changes a latch or controller state.
The pre-assignment `vga_vram_write_trigger_update_planar_mem` calls at lines
590/646 are separate effects; a backing assignment is not a proof that it
was the route's first visible effect.

Reject a handler claim when the direct-TLB path at `paging.h:464-466` ran,
when the installed page was empty/RAM/MMIO or another override, or when the
mode/chained/complexity state chose another handler. Guest addresses can
alias after the page mask, bank, memory mask, and odd/even transform; the
guest address or page alone does not determine a unique backing word.
Likewise a selected A000:118F event cannot be silently joined to A200:118F.
Even a measured selected assignment cannot establish the first changing
store without complete earlier-writer coverage in a named scope.

The next gate is a reviewed, default-off hook proposal that names both
generic and fast paths and records an explicit unsupported/alias outcome.
Shared issuer, wire, and consumer changes require p9/pC/pR coordination.
Status: `SOURCE_ONLY_HOOK_PLACEMENT`; reached handler and first change
remain unproven.
