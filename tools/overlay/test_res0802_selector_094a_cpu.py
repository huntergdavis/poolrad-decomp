"""Media-dependent regression checks for the isolated 094A evidence harness."""
from collections import Counter
from unittest import mock
import unittest

import res0802_selector_094a_cpu_audit as audit


class SelectorEvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = audit.audit(smoke=True)

    def test_smoke_compares_complete_cpu_and_ram(self):
        self.assertEqual(self.report["cases"], 981)
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
        self.assertEqual(sum(counts.values()), 598713)
        for name, seen in words.items():
            expected = set(range(2, 0xFFFA)) if name == "sp" else set(range(65536))
            self.assertEqual(seen, expected, name)
        self.assertEqual(len(flags), 256)
        self.assertTrue(all((f & 0x100) == 0 for f in flags))
        self.assertEqual(len(pairs), 65536)
        self.assertEqual(counts["frame_alias_old"], 2048)
        self.assertEqual(counts["frame_alias_argument"], 2048)
        self.assertEqual(counts["segment_alias"], 4096)

    def test_incorrect_media_identity_fails_closed(self):
        with mock.patch.object(audit, "START_SHA", "0" * 64):
            with self.assertRaisesRegex(ValueError, "START.EXE hash"):
                audit.pin_source()


if __name__ == "__main__":
    unittest.main()

