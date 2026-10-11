# Parallel workstreams

The CTO sets scope and priorities. The primary Codex session coordinates the
project, reviews evidence, and integrates pull requests. Two Claude and two
Codex workers use isolated deep copies of this repository and exchange changes
through the public GitHub repository. Workers use their own branches and open
pull requests; the primary session merges after review.

| Lane | Owner | Delivered; current focus | Owned paths |
| --- | --- | --- | --- |
| A: DOS code recovery | Claude | Thirty-four full overlay translations merged through PR #108; COMPARE, IF and RETURN are bounded models, the latter merged in PR #119. Idle pending PM assignment. | `tools/overlay/`, `docs/re/`, `src/recovered/ecl_*`, `include/poolrad/recovered/ecl_*`, `tests/recovered/test_ecl_*` (no edits to Codex's resident files) |
| B: portable data foundation and resident recovery | Codex worker | Format readers and resident inventory/loader/callable audits merged. Nine full resident translations merged through PR #79. PRs #110/#120 added bounded LOAD PIECES display callees; issue #122 awaits public review. | `src/formats/`, `include/poolrad/formats/`, `tests/formats/`, `CMakeLists.txt`, `docs/formats/`; resident-specific files under `src/recovered/`, `include/poolrad/recovered/`, `tests/recovered/`, and `docs/re/` (no edits to Claude's ECL helper files or overlay research) |
| C: separate overlay recovery | Claude worker 2 | Unit `0x3BD` entries 6 and 7 models merged in PRs #106/#114; entries 4 and 2 have full translations in PRs #124/#126. Entry 3 (`+0x0350`) is active. | New uniquely named overlay recovered source, header, test, and evidence files only; coordinate any shared CMake edit. |
| D: separate resident recovery | Codex worker 2 | Resident LOAD route dispatch and `RES_00BA:290B` models merged in PRs #109/#118; issue #123 awaits public review. | New uniquely named resident recovered source, header, test, and evidence files only; coordinate any shared CMake edit. |
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
Forty-five verified game-specific routines are fully translated across the
overlay and resident lanes: thirty-six overlay and nine resident. PR #64's
separate `0x464` helper has unresolved source ownership.
Three handler/callee models have unresolved dependencies or heap semantics;
PRs #65/#72 add separate path-complete script-state initializer and ECL block-loader models.
Two string routines have path-complete C translations with a provisional runtime
cap, and five more have extracted C cores whose engine side effects remain to be
translated. The regenerated classifier records 97 verified overlay identities,
including PRs #63/#69's twenty unit `0x38` entries, PR #65's view setter,
PRs #106/#114/#124/#126's unit `0x3BD` entries 6/7/4/2 and PR #108's two compare helpers,
and the callable ledger records ten resident identities. The combined
evidence-backed floor is therefore 107 game routines. The full game-function
denominator remains open; the DOS save-walk oracle reaches exploration.
