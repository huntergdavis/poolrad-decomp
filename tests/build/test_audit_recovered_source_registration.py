"""Independent metadata-only CMake/source fixtures; no compiler or media."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_source_registration.py"


class SourceRegistrationTests(unittest.TestCase):
    def invoke(self, files, cmake, exclusions=None, manifest=None):
        with tempfile.TemporaryDirectory(prefix="source registration ") as directory:
            root = Path(directory)
            recovered = root / "src/recovered"
            recovered.mkdir(parents=True)
            for name in files:
                (recovered / name).write_text("fixture contents are deliberately not compiled\n")
            (root / "CMakeLists.txt").write_text(cmake)
            (root / "exclusions.json").write_text(json.dumps(
                manifest if manifest is not None else {"schema_version": 1, "exclusions": exclusions or []}))
            report_path = root / "report.json"
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--root", str(root),
                 "--exclusions", str(root / "exclusions.json"), "--json", str(report_path)],
                capture_output=True, text=True, timeout=15, check=False)
            self.assertTrue(report_path.exists(), result.stderr)
            return result, json.loads(report_path.read_text())

    def test_literal_comments_quoted_paths_and_reasoned_exclusion(self):
        result, report = self.invoke(
            ["a.c", "b # space.c", "excluded.c"],
            '''#[=[ add_library(poolrad_recovered STATIC src/recovered/ghost.c) ]=]
            add_library(poolrad_recovered STATIC src/recovered/a.c) # ghost.c
            target_sources(poolrad_recovered PRIVATE "src/recovered/b # space.c")
            ''', [{"path": "src/recovered/excluded.c", "reason": "synthetic intentional exclusion"}])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((report["source_files"], report["registered_files"], report["excluded_files"]), (3, 2, 1))
        self.assertEqual(set(report["registrations"]), {"src/recovered/a.c", "src/recovered/b # space.c"})

    def test_bracket_argument_bom_crlf_and_raw_hash(self):
        cmake = "\ufeffadd_library(poolrad_recovered STATIC [=[\r\nsrc/recovered/a.c]=])\r\n"
        result, report = self.invoke(["a.c"], cmake)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["registered_files"], 1)
        self.assertEqual(report["cmake_sha256"], sha256(cmake.encode("utf-8")).hexdigest())

    def test_missing_duplicate_and_dangling_registration(self):
        result, report = self.invoke(["a.c", "b.c"], '''
            add_library(poolrad_recovered STATIC src/recovered/a.c ./src/recovered/a.c src/recovered/missing.c)
            add_library(other STATIC src/recovered/b.c)
            ''')
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(report["missing_registrations"], ["src/recovered/b.c"])
        self.assertEqual(report["duplicate_registrations"], ["src/recovered/a.c"])
        self.assertEqual(report["dangling_registrations"], ["src/recovered/missing.c"])

    def test_stale_and_registered_exclusions_fail(self):
        result, report = self.invoke(["a.c", "b.c"],
            "add_library(poolrad_recovered STATIC src/recovered/a.c)", [
                {"path": "src/recovered/a.c", "reason": "redundant"},
                {"path": "src/recovered/b.c", "reason": "intentional"},
                {"path": "src/recovered/gone.c", "reason": "stale"},
            ])
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["missing_registrations"], [])
        self.assertEqual(report["registered_exclusions"], ["src/recovered/a.c"])
        self.assertEqual(report["stale_exclusions"], ["src/recovered/gone.c"])

    def test_invalid_exclusions_are_configuration_errors(self):
        cases = [
            {"schema_version": 1, "exclusions": [{"path": "src/recovered/a.c", "reason": " "}]},
            {"schema_version": 1, "exclusions": [{"path": "src/recovered/*.c", "reason": "wildcard"}]},
            {"schema_version": 1, "exclusions": [{"path": "src/recovered/a.c", "reason": "one"}, {"path": "src/recovered/a.c", "reason": "two"}]},
            {"schema_version": True, "exclusions": []},
        ]
        for manifest in cases:
            with self.subTest(manifest=manifest):
                result, report = self.invoke(["a.c"], "add_library(poolrad_recovered STATIC src/recovered/a.c)", manifest=manifest)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_unresolved_registration_is_not_guessed(self):
        cases = [
            "set(SOURCES src/recovered/a.c)\nadd_library(poolrad_recovered STATIC \u0024{SOURCES})",
            "add_library(poolrad_recovered STATIC \u0024<1:src/recovered/a.c>)",
            "if(ON)\nadd_library(poolrad_recovered STATIC src/recovered/a.c)\nendif()",
            "add_library(poolrad_recovered STATIC src/recovered/a.c)\ntarget_sources(poolrad_recovered INTERFACE src/recovered/a.c)",
            "add_library(poolrad_recovered STATIC)\ntarget_sources(poolrad_recovered PRIVATE FILE_SET headers TYPE HEADERS FILES src/recovered/a.c)",
        ]
        for cmake in cases:
            with self.subTest(cmake=cmake):
                result, report = self.invoke(["a.c"], cmake)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_malformed_cmake_does_not_pass(self):
        for cmake in ("add_library(poolrad_recovered STATIC src/recovered/a.c", "#[=[ unterminated", r"add_library(poolrad_recovered STATIC src/recovered/a\q.c)", 'add_library(poolrad_recovered STATIC "unterminated)'):
            with self.subTest(cmake=cmake):
                result, report = self.invoke(["a.c"], cmake)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)

    def test_empty_inventory_or_missing_target_does_not_pass(self):
        for files, cmake in (([], "add_library(poolrad_recovered STATIC src/recovered/a.c)"), (["a.c"], "add_library(other STATIC src/recovered/a.c)")):
            with self.subTest(files=files):
                result, report = self.invoke(files, cmake)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)


if __name__ == "__main__":
    unittest.main()
