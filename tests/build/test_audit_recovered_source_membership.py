"""Synthetic source-filename fixtures; no recovered C bodies or CMake execution."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_source_membership.py"


class MembershipTests(unittest.TestCase):
    def invoke(self, cmake, files=("a.c",), expected=None):
        with tempfile.TemporaryDirectory(prefix="source membership ") as directory:
            root = Path(directory)
            (root / "src/recovered").mkdir(parents=True)
            for name in files:
                (root / "src/recovered" / name).write_text("not C")
            (root / "CMakeLists.txt").write_text(cmake)
            output = root / "report.json"
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--root", str(root),
                 "--expected-cmake-sha256", expected if expected is not None else sha256(cmake.encode()).hexdigest(),
                 "--json", str(output)], capture_output=True, text=True, timeout=15, check=False)
            self.assertTrue(output.exists(), result.stderr)
            return result, json.loads(output.read_text())

    def test_literals_brackets_raw_hash_and_additional_sources(self):
        cmake = ('\ufeff#[=[ ignored source ]=]\r\n'
                 'add_library(poolrad_recovered STATIC [=[\r\nsrc/recovered/a.c]=])\r\n'
                 'target_sources(poolrad_recovered PRIVATE "src/recovered/b.c")\r\n')
        result, report = self.invoke(cmake, ("a.c", "b.c"))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["registration_entries"], 2)
        self.assertEqual(report["cmake_sha256"], sha256(cmake.encode()).hexdigest())

    def test_missing_duplicate_and_nonexistent_members(self):
        result, report = self.invoke(
            "add_library(poolrad_recovered STATIC src/recovered/a.c ./src/recovered/a.c src/recovered/gone.c)",
            ("a.c", "missing.c"))
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["missing_members"], ["src/recovered/missing.c"])
        self.assertEqual(report["issues"]["duplicate_members"], ["src/recovered/a.c"])
        self.assertEqual(report["issues"]["nonexistent_members"], ["src/recovered/gone.c"])

    def test_explicit_out_of_scope_entries(self):
        result, report = self.invoke(
            "add_library(poolrad_recovered STATIC src/recovered/a.c src/formats/helper.c src/recovered/sub/nested.c include/public.h /tmp/absolute.c ../outside.c)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(len(report["issues"]["out_of_scope_members"]), 5)
        self.assertEqual(report["issues"]["missing_members"], [])

    def test_stale_or_malformed_expected_hash(self):
        for expected in ("0" * 64, "invalid"):
            with self.subTest(expected=expected):
                result, report = self.invoke("add_library(poolrad_recovered STATIC src/recovered/a.c)", expected=expected)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)

    def test_other_target_cannot_satisfy_membership(self):
        result, report = self.invoke(
            "add_library(poolrad_recovered STATIC)\nadd_library(other STATIC src/recovered/a.c)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["missing_members"], ["src/recovered/a.c"])

    def test_dynamic_and_scoped_forms_fail_closed(self):
        cases = [
            "add_library(poolrad_recovered STATIC \u0024{SOURCES})",
            "if(ON)\nadd_library(poolrad_recovered STATIC src/recovered/a.c)\nendif()",
            "add_library(poolrad_recovered INTERFACE src/recovered/a.c)",
            "target_sources(poolrad_recovered PRIVATE src/recovered/a.c)\nadd_library(poolrad_recovered STATIC)",
            "add_library(poolrad_recovered STATIC)\ntarget_sources(poolrad_recovered PRIVATE FILE_SET sources FILES src/recovered/a.c)",
        ]
        for cmake in cases:
            with self.subTest(cmake=cmake):
                result, report = self.invoke(cmake)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_malformed_commands_fail_closed(self):
        for cmake in ("add_library(poolrad_recovered STATIC src/recovered/a.c", "#[=[ unterminated", 'add_library(poolrad_recovered STATIC "unterminated)'):
            with self.subTest(cmake=cmake):
                result, report = self.invoke(cmake)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)

    def test_empty_inventory_or_missing_selected_library(self):
        for files, cmake in (((), "add_library(poolrad_recovered STATIC)"), (("a.c",), "add_library(other STATIC src/recovered/a.c)")):
            with self.subTest(files=files):
                result, report = self.invoke(cmake, files)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)


if __name__ == "__main__":
    unittest.main()
