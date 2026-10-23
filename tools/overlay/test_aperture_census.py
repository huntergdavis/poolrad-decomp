#!/usr/bin/env python3
"""The aperture census: the committed CSVs must equal what the sources say, and the classification must be complete."""
import csv
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import aperture_census as ac


class ApertureCensusTest(unittest.TestCase):
    def test_the_committed_csvs_equal_the_sources(self):
        rows = ac.models()
        for path, want in ((ac.OBSERVABILITY, ac.observability(rows)), (ac.PAIRS, ac.pairs(rows))):
            have = list(csv.DictReader(path.open()))
            self.assertEqual(have, [{k: str(v) for k, v in r.items()} for r in want], f"{path.name} is stale: run aperture_census.py --write")

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
