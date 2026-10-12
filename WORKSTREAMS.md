# Parallel workstreams

The CTO sets scope and priorities. The primary Codex session coordinates the
project, reviews evidence, and integrates pull requests. Four worker lanes are
defined. Active workers use isolated deep copies and exchange changes through
the public GitHub repository. The active lanes are
one Claude C worker, one Codex C worker, and one Codex native replay worker,
each in an isolated copy.
Active workers use their own branches and open pull requests; the primary
session merges after review.

| Lane | Owner | Delivered; current focus | Owned paths |
| --- | --- | --- | --- |
| A: DOS code recovery | Claude | Thirty-four full overlay translations merged through PR #108; COMPARE, IF and RETURN are bounded models, the latter merged in PR #119. Idle pending PM assignment. | `tools/overlay/`, `docs/re/`, `src/recovered/ecl_*`, `include/poolrad/recovered/ecl_*`, `tests/recovered/test_ecl_*` (no edits to Codex's resident files) |
| B: portable data foundation and resident recovery | Codex worker (p5) | Nine full resident translations merged through PR #79. Unit `0x68:+0x05EE` list search merged in PR #149; current isolated target is adjacent record-clear caller `+0x0648`. | `src/formats/`, `include/poolrad/formats/`, `tests/formats/`, `CMakeLists.txt`, `docs/formats/`; resident-specific files under `src/recovered/`, `include/poolrad/recovered/`, `tests/recovered/`, and `docs/re/` (no edits to Claude's ECL helper files or overlay research) |
| C: separate overlay recovery | Claude worker 2 (p6) | Unit `0x7D:+0x0000` and unit `0x9E` entry 2 bounded models merged in PRs #144/#147; current target is connected unit `0x9E:+0x0160`. | New uniquely named overlay recovered source, header, test, and evidence files only; coordinate any shared CMake edit. |
| D: native replay evidence | Codex worker 2 (p9) | Isolated exactly-one-Up native write watch measured window-0 `+0x018E: 0→1`, `+0x0190: 5→5`, and the unit `0x7D:+0x045D` value-changing writer via unit `0x68:+0x07ED`; public address-only PR in preparation. | Private `resources/` and ignored captures in its own clone; public output only address/hash/count evidence. |
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
Fifty-two verified game-specific routines are fully translated across the
overlay and resident lanes: forty-three overlay and nine resident. PR #64's
separate `0x464` helper has unresolved source ownership.
Three handler/callee models have unresolved dependencies or heap semantics;
PRs #65/#72 add separate path-complete script-state initializer and ECL block-loader models.
Two string routines have path-complete C translations with a provisional runtime
cap, and five more have extracted C cores whose engine side effects remain to be
translated. The regenerated classifier records 104 verified overlay identities,
including PRs #63/#69's twenty unit `0x38` entries, PR #65's view setter,
PRs #106/#114/#124/#126/#128's unit `0x3BD` entries 6/7/4/2/3, PRs #130/#132's unit `0x7F` entries 10/11, PR #108's two compare helpers, PR #136's unit `0x25` entry 19, PR #138's unit `0x68:+0x068D` preflight, PR #145's `0x68:+0x05AB` scan, and PR #149's `0x68:+0x05EE` list search,
and the callable ledger records ten resident identities. The combined
evidence-backed floor is therefore 114 game routines. The full game-function
denominator remains open; the DOS one-Up watch reaches the first value-changing writer.
