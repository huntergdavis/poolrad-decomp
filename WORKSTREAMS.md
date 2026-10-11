# Parallel workstreams

The CTO sets scope and priorities. The primary Codex session coordinates the
project, reviews evidence, and integrates pull requests. Two Claude and two
Codex workers use isolated deep copies of this repository and exchange changes
through the public GitHub repository. Workers use their own branches and open
pull requests; the primary session merges after review.

| Lane | Owner | Delivered; current focus | Owned paths |
| --- | --- | --- | --- |
| A: DOS code recovery | Claude | Twenty-six full overlay translations merged through PR #65, including sixteen unit `0x38` entries from PR #63 and the view setter from PR #65; three path-complete handler/callee models, the PR #65 script-state initializer model, two path-complete string translations with a provisional cap, and five extracted cores. Next task pending. | `tools/overlay/`, `docs/re/`, `src/recovered/ecl_*`, `include/poolrad/recovered/ecl_*`, `tests/recovered/test_ecl_*` (no edits to Codex's resident files) |
| B: portable data foundation and resident recovery | Codex worker | Format readers merged in PRs #4, #7, #10, #13, #17, #19, and #23; resident inventory, loader map, and callable coverage merged in PRs #27, #30, and #33. Seven full resident translations merged through PR #62. Current: issue #24 resident accounting update. | `src/formats/`, `include/poolrad/formats/`, `tests/formats/`, `CMakeLists.txt`, `docs/formats/`; resident-specific files under `src/recovered/`, `include/poolrad/recovered/`, `tests/recovered/`, and `docs/re/` (no edits to Claude's ECL helper files or overlay research) |
| C: separate overlay recovery | Claude worker 2 | Issue #58's sixteen bounded unit `0x38` routines merged in PR #63; next task pending. | New uniquely named overlay recovered source, header, test, and evidence files only; coordinate any shared CMake edit. |
| D: separate resident recovery | Codex worker 2 | Issue #59 bitmap release helper merged in PR #64; its game/library source ownership remains unresolved and it is outside the verified game-specific count. | New uniquely named resident recovered source, header, test, and evidence files only; coordinate any shared CMake edit. |
| Integration | Primary Codex session | Review PRs, maintain roadmap and issue tracker, establish DOS replay/oracle cases, track the game-function denominator in issue #24, and connect the lanes at the first playable route. | `README.md`, `ROADMAP.md`, `WORKSTREAMS.md`, `.github/`, `tools/overlay/function_evidence.py`, `docs/re/OVERLAY_FUNCTION_INVENTORY.md` |

## Shared rules

1. Use branches named `claude/<topic>` or `codex/<topic>` and open a PR to
   `main`. Put the target DOS executable hashes, input files, measured output,
   and unresolved questions in the PR. Keep changes within the lane's paths
   unless the primary session agrees to an interface change.
2. Coordinate interface changes in GitHub issues or PR comments. Lane A owns
   overlay addresses and behavioral claims; lane B owns portable file-format
   APIs. Both cite evidence and distinguish verified behavior from guesses.
3. Never commit `resources/`, original executables, saves, extracted game
   text, art, audio, or private captures. Tests use synthetic data. A local
   integration audit may read owner-supplied files but must report only counts,
   checksums, and errors. Retain notices when adapting reference code.
4. The primary session checks that branches rebase on current `main`, reviews
   code and reports, runs relevant verification, and merges in dependency
   order. A completed PR updates the roadmap or an issue with what is now
   known and what remains uncertain.

The C parser audits all local DOS DAX archives, the overlay map and Ghidra
feasibility pilot are published, and the ECL, EGA, and WALLDEF readers are merged.
Thirty-three verified game-specific routines are fully translated across the
overlay and resident lanes: twenty-six overlay and seven resident. PR #64's
separate `0x464` helper has unresolved source ownership.
Three handler/callee models have unresolved dependencies or heap semantics;
PR #65 adds a separate path-complete script-state initializer model.
two string routines have path-complete C translations with a provisional runtime
cap, and five more have extracted C cores whose engine side effects remain to be
translated. The regenerated classifier records 84 verified overlay identities,
including PR #63's sixteen unit `0x38` entries and PR #65's view setter, and the callable ledger records
eight resident identities. The combined evidence-backed floor is therefore
92 game routines. The full
game-function denominator and a DOS gameplay oracle trace remain open.
