# U19 row-14 host four-plane snapshot gate

This source-only gate prepares a stopped-state witness for guest A000:118F. It
does not report a reached writer, live plane value, page change, or RGB result.
The host native slot remains held.

The bounded deja lookup for the U19 host backing-plane and latch capture found
a damaged local index and timed out while rebuilding. This gate reuses the
merged #709 selected-store recipe, #722 host-plane resolver, and #732 plane
index proof. The pinned private DOSBox-X revision is
`5a9c7c53481c2e933948039c33256776653f4faf`.

`tools/replay/cold_start_codeword_row14_host_snapshot_gate.py` checks the
original source file and four windows already identified by the merged
resolver. It also pins four ignored private helper/test files by SHA-256. The
private adapter obtains the active A000 page handler, physical address, bank
state, backing pointer and latch. The core accepts only `uvga` or `uvga_fast`,
the selected address, expected read/write banks, and an in-range four-byte
backing word. It reads the four plane bytes directly from host backing memory,
then compares the latch before and after. It performs no guest VGA read.

The adapter has only been compiled against a fake host. It has not been
inserted into the pinned DOSBox-X build or used during a native run. Its
paging-address query may have host paging side effects that a live audit must
separately rule out. Synthetic C++ and Python tests cover four-plane indexing,
unchanged latch, target/handler/bank guards, short backing, and rejected latch
mutation. They establish the helper contract, not live nonperturbation.

Read-only source reproduction:

    python3 -m tools.replay.cold_start_codeword_row14_host_snapshot_gate --source resources/dosbox-x-16a1-private-src --helper-dir resources/u19-host-capture
    python3 -m unittest tests.replay.test_cold_start_codeword_row14_host_snapshot_gate

The helper and all original media remain ignored under `resources/`. Only
addresses, hashes, source relations, and synthetic results are public.
