# Parallel workstreams

The CTO sets scope and priorities. The primary Codex session coordinates the
project, reviews evidence, and integrates pull requests. Two Claude and two
Codex workers use isolated deep copies of this repository and exchange changes
through the public GitHub repository. Workers use their own branches and open
pull requests; the primary session merges after review.

| Lane | Owner | Delivered; current focus | Owned paths |
| --- | --- | --- | --- |
| A: DOS code recovery | Claude | Thirty-four full overlay translations merged through PR #108; COMPARE and IF are bounded models. Current: issue #113 ECL RETURN opcode 0x13. | `tools/overlay/`, `docs/re/`, `src/recovered/ecl_*`, `include/poolrad/recovered/ecl_*`, `tests/recovered/test_ecl_*` (no edits to Codex's resident files) |
| B: portable data foundation and resident recovery | Codex worker | Format readers and resident inventory/loader/callable audits merged. Nine full resident translations merged through PR #79. PR #110 added a bounded resident LOAD PIECES display/list model; current: issue #115, its field-display callee. | `src/formats/`, `include/poolrad/formats/`, `tests/formats/`, `CMakeLists.txt`, `docs/formats/`; resident-specific files under `src/recovered/`, `include/poolrad/recovered/`, `tests/recovered/`, and `docs/re/` (no edits to Claude's ECL helper files or overlay research) |
| C: separate overlay recovery | Claude worker 2 | Unit `0x3BD` entry 6 model merged in PR #106. Current: issue #112, adjacent entry 7 on the opcode 0x21 wallset path. | New uniquely named overlay recovered source, header, test, and evidence files only; coordinate any shared CMake edit. |
| D: separate resident recovery | Codex worker 2 | Resident LOAD route dispatch model merged in PR #109. Current: issue #111, callee `RES_00BA:290B`. | New uniquely named resident recovered source, header, test, and evidence files only; coordinate any shared CMake edit. |
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
Forty-three verified game-specific routines are fully translated across the
overlay and resident lanes: thirty-four overlay and nine resident. PR #64's
separate `0x464` helper has unresolved source ownership.
Three handler/callee models have unresolved dependencies or heap semantics;
PRs #65/#72 add separate path-complete script-state initializer and ECL block-loader models.
Two string routines have path-complete C translations with a provisional runtime
cap, and five more have extracted C cores whose engine side effects remain to be
translated. The regenerated classifier records 94 verified overlay identities,
including PRs #63/#69's twenty unit `0x38` entries, PR #65's view setter,
PR #106's unit `0x3BD` entry 6 and PR #108's two compare helpers,
and the callable ledger records ten resident identities. The combined
evidence-backed floor is therefore 104 game routines. The full game-function
denominator remains open; the DOS save-walk oracle reaches exploration.
