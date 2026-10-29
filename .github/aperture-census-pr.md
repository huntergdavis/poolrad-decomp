Automated: `tools/overlay/aperture_census.py --write` on `main`, filed by `.github/workflows/aperture-census.yml` (t582).

The census CSVs are **derived** from the model sources, so no body PR carries them any more. A body PR may add a
model without the row; PR CI tolerates **absent** rows only (`--allow-missing`), never a row with a wrong value.
This PR is the row being written after the merge.

Review this PR on the CSVs alone:

- the only changed files are `docs/re/aperture_observability.csv` and `docs/re/aperture_pairs.csv`;
- the diff is exactly the rows the sources produce — re-running `python3 tools/overlay/aperture_census.py` on this
  head must exit 0 with no `STALE` line, which is the same check CI runs strictly on `main`;
- nothing here reclassifies an existing row. If a diff *changes* a value rather than adding a row, the census input
  changed (a model's callee list, a header's declaration, or `SOURCES`), and that change belongs to the PR that
  caused it, not to this refresh — send it back with the cause named.

Not verified by the job: no build, no ctest, no media. It runs one Python tool over the sources.
