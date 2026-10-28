"""Media-dependent checks for the 0126 caller composition and evidence boundary."""
from pathlib import Path
import unittest
from unittest import mock

import u1c_compare_0126_cpu_audit as audit


class CompareCallerEvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = audit.audit(smoke=True)

    def test_composed_body_branches_and_full_state(self):
        self.assertEqual(self.report["source"]["bytes"], 125)
        self.assertEqual(self.report["starts"], 50)
        self.assertEqual(self.report["outcomes"], 4)
        self.assertEqual(self.report["cpu_words"], 14)
        self.assertEqual(self.report["ram_bytes"], 1 << 20)
        self.assertEqual(self.report["masks"], [])
        self.assertTrue({0, 2, 3, 4} <= set(self.report["stops"]))
        self.assertEqual(self.report["c_only_atomic_entry_refusals"], 7)

    def test_overwritten_sub_flags_are_observed_before_the_callee(self):
        witness = self.report["mutants"]["wrong_sub_flags"]["witness"]
        self.assertEqual(witness["cpu"], {})
        self.assertTrue(witness["ram_equal"])
        self.assertFalse(witness["trace_equal"])
        self.assertEqual(witness["first_observation"]["ip"], 0x012D)
        self.assertIn("flags", witness["first_observation"]["cpu"])

    def test_argument_order_and_second_tag_mutants_are_killed(self):
        for name in ("reversed_words", "second_tag_ignored"):
            self.assertTrue(self.report["mutants"][name]["killed"], name)

    def test_media_pin_refuses_wrong_executable(self):
        with mock.patch.object(audit, "EXE_SHA", "0" * 64):
            with self.assertRaisesRegex(ValueError, "START.EXE hash"):
                audit.source()

    def test_actual_registry_has_one_owner_and_exact_source_selection(self):
        import check_checker_builds
        import checker_execution
        path = Path("tools/overlay/u1c_compare_0126_cpu_audit.py")
        registry = check_checker_builds.discover(audit.ROOT)
        self.assertEqual(registry["OVL_001C_0126"], [path])
        selected = checker_execution.source_map(audit.ROOT, [path], check_checker_builds._cc_commands)[path]
        self.assertEqual(selected["conservative"], [])
        self.assertEqual(set(selected["sources"]), {"src/recovered/" + name for name in audit.SOURCES})

    def test_full_length_and_byte_sweeps_are_enumerated(self):
        from collections import Counter
        counts = Counter(group for group, _ in audit.cases())
        self.assertEqual(counts["literal-byte"], 512)
        self.assertEqual(counts["tag-byte"], 512)
        self.assertEqual(counts["string-length"], 256)
        self.assertEqual(counts["literal-word"], 25)


if __name__ == "__main__":
    unittest.main()
