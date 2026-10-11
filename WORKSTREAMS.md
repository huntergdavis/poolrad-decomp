# Parallel workstreams

The CTO sets scope and priorities. The primary Codex session coordinates the
project, reviews evidence, and integrates pull requests. Claude and a separate
Codex worker use isolated deep copies of this repository and exchange changes
through the public GitHub repository. Workers use their own branches and open
pull requests; the primary session merges after review.

| Lane | Owner | Delivered; current focus | Owned paths |
| --- | --- | --- | --- |
| A: DOS code recovery | Claude | Overlay inventory, fixup-aware candidate call graph, Ghidra pilot, and ECL contracts merged in PR #6. Current: three behavior-checked C function translations, issue #8. | `tools/overlay/`, `docs/re/`; for #8 also `src/recovered/`, `include/poolrad/recovered/`, `tests/recovered/` |
| B: portable data foundation | Codex worker | DAX/RLE/GEO reader merged in PR #4; ECL container/text reader merged in PR #7. Current: bounded DOS EGA image reader, issue #9. | `src/formats/`, `include/poolrad/formats/`, `tests/formats/`, `CMakeLists.txt`, `docs/formats/` |
| Integration | Primary Codex session | Review PRs, maintain roadmap and issue tracker, establish DOS replay/oracle cases, and connect both lanes at the first playable route. | `README.md`, `ROADMAP.md`, `WORKSTREAMS.md`, `.github/` |

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

The first joint checkpoint is complete: the C parser audits all local DOS DAX
archives, and the overlay map and Ghidra feasibility pilot are published. The
ECL container reader and recovered byte contract are also merged. The next
checkpoint establishes the first behavior-checked C function translations
while widening the portable asset layer to DOS EGA images.
