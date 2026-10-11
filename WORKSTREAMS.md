# Parallel workstreams

The CTO sets scope and priorities. The primary Codex session coordinates the
project, reviews evidence, and integrates pull requests. Claude and a separate
Codex worker use isolated deep copies of this repository and exchange changes
through the public GitHub repository. Workers use their own branches and open
pull requests; the primary session merges after review.

| Lane | Owner | Delivered; current focus | Owned paths |
| --- | --- | --- | --- |
| A: DOS code recovery | Claude | Three full leaf translations, two path-complete string translations with a provisional cap, and five extracted cores merged in PRs #11, #15, #20, and #25. Current: validate overlay routine starts and game-code classification, issue #26 under #24. | `tools/overlay/`, `docs/re/`, `src/recovered/`, `include/poolrad/recovered/`, `tests/recovered/` |
| B: portable data foundation | Codex worker | DAX/RLE/GEO, ECL container/operands/text, DOS EGA images, WALLDEF spans, DOS character record/name framing, and the resident called-target inventory merged in PRs #4, #7, #10, #13, #17, #19, #23, and #27. Current: recover resident loader mapping and classify called code, issue #28 under #24. | `src/formats/`, `include/poolrad/formats/`, `tests/formats/`, `CMakeLists.txt`, `docs/formats/`; issue #28 exception for focused resident mapping files under `tools/overlay/` and `docs/re/` |
| Integration | Primary Codex session | Review PRs, maintain roadmap and issue tracker, establish DOS replay/oracle cases, validate overlay function candidates for issue #24, and connect both lanes at the first playable route. | `README.md`, `ROADMAP.md`, `WORKSTREAMS.md`, `.github/`, `tools/overlay/function_evidence.py`, `docs/re/OVERLAY_FUNCTION_INVENTORY.md` |

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
Three leaf functions are fully translated. Two string routines have path-complete
C translations with a provisional runtime cap, and five more have extracted C
cores whose engine side effects remain to be translated. The current checkpoint
measures the original game-function denominator across overlay and resident code
and establishes a reproducible DOS gameplay oracle trace.
