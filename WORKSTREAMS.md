# Parallel workstreams

The CTO sets scope and priorities. The primary Codex session coordinates the
project, reviews evidence, and integrates pull requests. Claude and a separate
Codex worker use isolated deep copies of this repository and exchange changes
through the public GitHub repository. Workers use their own branches and open
pull requests; the primary session merges after review.

| Lane | Owner | Delivered; current focus | Owned paths |
| --- | --- | --- | --- |
| A: DOS code recovery | Claude | Three full ECL helper translations and three extracted behavior cores merged in PRs #11, #15, and #20. Current: resolve ECL tag 0x81 engine string windows, issue #22. | `tools/overlay/`, `docs/re/`, `src/recovered/`, `include/poolrad/recovered/`, `tests/recovered/` |
| B: portable data foundation | Codex worker | DAX/RLE/GEO, ECL container/operands/text, DOS EGA images, WALLDEF spans, and DOS character record/name framing merged in PRs #4, #7, #10, #13, #17, #19, and #23. Current: map resident functions for the denominator, issue #24; owns only new resident-inventory files for this task. | `src/formats/`, `include/poolrad/formats/`, `tests/formats/`, `CMakeLists.txt`, `docs/formats/`; issue #24 exception for new `tools/overlay/resident_inventory.py` and `docs/re/RESIDENT_FUNCTION_INVENTORY.md` |
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
Three leaf functions are fully translated. Three more ECL behavior cores are in
C; their engine state changes remain to be translated. The next checkpoint
resolves the loader's engine string windows and establishes a reproducible DOS
gameplay oracle trace.
