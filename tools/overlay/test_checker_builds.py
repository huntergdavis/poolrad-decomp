"""Failure paths for the pre-handoff gate, using synthetic checker programs."""
from pathlib import Path
import shutil
import tempfile
import unittest

import check_checker_builds as gate


class CheckerBuildGateTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        (self.root / "tests/recovered").mkdir(parents=True)
        self.path = Path("tests/recovered/check_synthetic.py")

    def checker(self, program):
        (self.root / self.path).write_text(
            'CHECKER_ROWS = ("OVL_0038_12B2",)\n' + program)

    def test_unknown_row_blocks_instead_of_skipping(self):
        self.checker('print("CHECKER_BUILD_OK")\n')
        with self.assertRaisesRegex(ValueError, "unregistered.*handoff blocked"):
            gate.select(gate.discover(self.root), ["OVL_0038_2B20"])

    def test_empty_selection_blocks(self):
        with self.assertRaisesRegex(ValueError, "no registered rows"):
            gate.select({}, [])

    def test_new_or_changed_manifest_row_requires_registration(self):
        header = "id,status,evidence_doc\n"
        before = header + "OVL_0038_12B2,partial,docs/re/a.md\n"
        after = header + "OVL_0038_12B2,full,docs/re/a.md\n" + \
            "OVL_0038_2B20,full,docs/re/b.md\n"
        self.assertEqual(gate.changed_rows(before, after),
                         {"OVL_0038_12B2", "OVL_0038_2B20"})
        with self.assertRaisesRegex(ValueError, "unregistered"):
            gate.select({}, gate.changed_rows(before, after))

    def test_unchanged_legacy_rows_do_not_claim_build_coverage(self):
        data = "id,status,evidence_doc\nOVL_0038_12B2,full,docs/re/a.md\n"
        self.assertEqual(gate.changed_rows(data, data), set())

    def test_registration_is_in_actual_checker(self):
        self.checker('import sys\nassert sys.argv[1:] == ["--compile-only"]\n'
                     'print("CHECKER_BUILD_OK")\n')
        paths = gate.select(gate.discover(self.root), ["OVL_0038_12B2"])
        self.assertEqual(paths, [self.path])
        self.assertEqual(gate.run_checker(self.root, paths[0])["checker"], str(self.path))

    def test_success_without_completion_marker_blocks(self):
        self.checker('print("not a compile-only checker")\n')
        with self.assertRaisesRegex(ValueError, "checker build failed"):
            gate.run_checker(self.root, self.path)

    def test_compile_error_blocks(self):
        self.checker('import subprocess\nsubprocess.run(["cc", "missing.c"], check=True)\n')
        with self.assertRaisesRegex(ValueError, "checker build failed"):
            gate.run_checker(self.root, self.path)

    @unittest.skipUnless(shutil.which("cc"), "C compiler required")
    def test_shared_library_with_unresolved_symbol_blocks_at_load(self):
        (self.root / "missing.c").write_text(
            'extern int missing_dependency(void);\n'
            'int model(void) { return missing_dependency(); }\n')
        self.checker('import ctypes, subprocess\n'
                     'subprocess.run(["cc", "-shared", "-fPIC", "missing.c", '
                     '"-o", "model.so"], check=True)\n'
                     'ctypes.CDLL("./model.so").model\nprint("CHECKER_BUILD_OK")\n')
        with self.assertRaisesRegex(ValueError, "undefined symbol: missing_dependency"):
            gate.run_checker(self.root, self.path)

    def test_malformed_registration_blocks(self):
        (self.root / self.path).write_text('CHECKER_ROWS = ("not-a-row",)\n')
        with self.assertRaisesRegex(ValueError, "invalid CHECKER_ROWS"):
            gate.discover(self.root)


if __name__ == "__main__":
    unittest.main()
