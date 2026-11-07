#!/usr/bin/env python3
"""The aperture census: the committed CSVs must equal what the sources say, and the classification must be complete."""
import csv
import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import aperture_census as ac


class ApertureCensusTest(unittest.TestCase):
    def test_the_committed_csvs_equal_the_sources(self):
        # APERTURE_CENSUS_ALLOW_MISSING is set by CI on a PULL REQUEST only: a body PR no longer has to carry the
        # census row, the refresh job on main writes it (docs/re/APERTURE_CENSUS.md). Absent rows are tolerated
        # there; a row with a wrong value is not, in either mode.
        allow_missing = os.environ.get("APERTURE_CENSUS_ALLOW_MISSING") == "1"
        rows = ac.models()
        for path, want in ((ac.OBSERVABILITY, ac.observability(rows)), (ac.PAIRS, ac.pairs(rows))):
            have = list(csv.DictReader(path.open()))
            verdict = ac.compare(have, want, allow_missing=allow_missing)
            self.assertNotEqual(verdict, "stale", f"{path.name} is stale: run aperture_census.py --write")

    def test_allow_missing_tolerates_an_absent_row_but_never_a_wrong_one(self):
        rows = ac.models()
        want = ac.observability(rows)
        have = [{k: str(v) for k, v in r.items()} for r in want]
        self.assertEqual(ac.compare(have, want, allow_missing=True), "current")
        self.assertEqual(ac.compare(have, want, allow_missing=False), "current")
        absent = have[:-1]
        self.assertEqual(ac.compare(absent, want, allow_missing=True), "missing")
        self.assertEqual(ac.compare(absent, want, allow_missing=False), "stale")
        wrong = [dict(row) for row in have]
        wrong[0]["aperture_observable"] = "no" if wrong[0]["aperture_observable"] != "no" else "yes-with-ega-oracle"
        self.assertEqual(ac.compare(wrong, want, allow_missing=True), "stale")
        self.assertEqual(ac.compare(wrong, want, allow_missing=False), "stale")

    def test_every_flat_model_is_marked_not_observable_and_every_bus_model_is_not(self):
        rows = list(csv.DictReader(ac.OBSERVABILITY.open()))
        self.assertTrue(rows)
        for row in rows:
            self.assertEqual(row["aperture_observable"], "yes-with-ega-oracle" if row["kind"] == "bus" else "no", row["model"])

    def test_the_two_undeclared_reachable_pairs_are_exactly_the_two_found(self):
        rows = [r for r in csv.DictReader(ac.PAIRS.open())
                if r["reach"] == "can-reach-under-a-corrupt-pointer" and r["caller_declares_exclusion"] == "no"]
        self.assertEqual({(r["caller"], r["flat_callee"]) for r in rows},
                         {("overlay_u7f_number_0294_cpu.c", "pr_system_strstore_0b25"), ("resident_0709_printer_0768_cpu.c", "pr_system_strstore_0b25")})

    def test_an_unclassified_pointer_pair_fails_closed(self):
        rows = [dict(model="new_caller_cpu.c", kind="bus", declared_exclusion="yes", callees=["pr_system_strload_0b0b"])]
        with self.assertRaises(SystemExit):
            ac.pairs(rows)


if __name__ == "__main__":
    unittest.main()
