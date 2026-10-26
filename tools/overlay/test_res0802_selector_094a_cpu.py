"""Media-dependent regression checks for the isolated 094A evidence harness."""
from collections import Counter
from pathlib import Path
import tempfile
from unittest import mock
import unittest

import res0802_selector_094a_cpu_audit as audit


class SelectorEvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = audit.audit(smoke=True)

    def test_smoke_compares_complete_cpu_and_ram(self):
        self.assertEqual(self.report["cases"], 2773)
        self.assertEqual(self.report["cpu_words"], 14)
        self.assertEqual(self.report["ram_bytes"], 1 << 20)
        self.assertEqual(self.report["masks"], [])
        self.assertEqual(self.report["source"]["bytes"], 19)
        self.assertEqual(self.report["source"]["instructions"], 8)
        self.assertEqual(len(self.report["source"]["callers"]), 2)

    def test_flag_mutant_is_detected_by_flags(self):
        witness = self.report["mutants"]["flip_cf"]
        self.assertTrue(witness["killed"])
        self.assertIn("flags", witness["differences"]["cpu"])
        self.assertTrue(witness["differences"]["ram_equal"])

    def test_read_order_mutant_requires_alias_witness(self):
        witness = self.report["mutants"]["early_argument"]
        self.assertTrue(witness["killed"])
        self.assertIn(witness["group"], ("frame_alias_old", "frame_alias_argument", "segment_alias"))
        self.assertFalse(witness["differences"]["ram_equal"])

    def test_complete_declared_dimension_enumeration(self):
        counts = Counter()
        words = {n: set() for n in ("bp", "ds", "ss", "ax", "return_ip", "return_cs", "argument", "sp")}
        flags = set()
        pairs = set()
        for group, change in audit.cases():
            counts[group] += 1
            if group in words:
                words[group].add(change[group])
            if group == "flags":
                flags.add(change["flags"])
            if group == "selector_argument_product":
                pairs.add((change["old"], change["argument"] & 255))
        self.assertEqual(sum(counts.values()), 600505)
        for name, seen in words.items():
            expected = set(range(2, 0xFFFA)) if name == "sp" else set(range(65536))
            self.assertEqual(seen, expected, name)
        self.assertEqual(len(flags), 2048)
        self.assertTrue(all((f & 0x100) == 0 for f in flags))
        self.assertEqual(len(pairs), 65536)
        self.assertEqual(counts["frame_alias_old"], 2048)
        self.assertEqual(counts["frame_alias_argument"], 2048)
        self.assertEqual(counts["segment_alias"], 4096)

    def test_incorrect_media_identity_fails_closed(self):
        with mock.patch.object(audit, "START_SHA", "0" * 64):
            with self.assertRaisesRegex(ValueError, "START.EXE hash"):
                audit.pin_source()

    def test_gate_selects_only_the_actual_production_source(self):
        import check_checker_builds
        import checker_execution
        path = Path("tools/overlay/res0802_selector_094a_cpu_audit.py")
        receipt = checker_execution.source_map(audit.ROOT, [path], check_checker_builds._cc_commands)[path]
        self.assertEqual(receipt["sources"], [audit.C_SOURCE])
        self.assertEqual(receipt["conservative"], [])

    def test_reused_ram_matches_fresh_cases_and_rejects_divergence(self):
        body, _ = audit.pin_source()
        with tempfile.TemporaryDirectory(prefix="094a-fixture-") as td:
            lib = audit.library(Path(td))
            reused, fresh = audit.Engine(body), audit.Engine(body, fresh_memory=True)
            for change in ({}, {"ds": 0x1360}, {"ss": 0xFFFF, "sp": 0x11},
                           {"sp": 2}, {"sp": 0xFFF9}, {"flags": 0x7ED7},
                           {"ds": 0x2000, "ss": 0x2000, "sp": 0x5373, "old": 0},
                           {"ds": 0x2000, "ss": 0x2000, "sp": 0x5378, "bp": 0xFFFF}):
                self.assertEqual(reused.run(change, lib), {"cpu": {}, "ram_equal": True})
                self.assertEqual(fresh.run(change, lib), {"cpu": {}, "ram_equal": True})
                self.assertEqual(reused.last_original_cpu, fresh.last_original_cpu)

            class BadStore:
                @staticmethod
                def audit_selector(ram, cpu):
                    lib.audit_selector(ram, cpu)
                    ram[0xFFEED] ^= 1  # unrelated byte, outside this case's inputs/writes

            self.assertFalse(reused.run({}, BadStore)["ram_equal"])
            with self.assertRaisesRegex(ValueError, "previous case failed"):
                reused.run({}, lib)


if __name__ == "__main__":
    unittest.main()

