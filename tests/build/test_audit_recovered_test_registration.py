"""Synthetic filename/CMake fixtures; never read or compile recovered C."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_test_registration.py"


class TestRegistrationTests(unittest.TestCase):
    def invoke(self, cmake, files=("test_a.c",)):
        with tempfile.TemporaryDirectory(prefix="test metadata ") as directory:
            root = Path(directory)
            (root / "tests/recovered").mkdir(parents=True)
            for name in files:
                (root / "tests/recovered" / name).write_text("not C; metadata only\n")
            (root / "CMakeLists.txt").write_text(cmake)
            report = root / "report.json"
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--root", str(root), "--json", str(report)],
                capture_output=True, text=True, timeout=15, check=False)
            self.assertTrue(report.exists(), result.stderr)
            return result, json.loads(report.read_text())

    def test_guarded_sources_and_helper_sources(self):
        result, report = self.invoke('''
            if(BUILD_TESTING)
            add_executable(poolrad_a tests/recovered/test_a.c src/replay/helper.c)
            target_sources(poolrad_a PRIVATE tests/recovered/test_b.c)
            add_test(NAME poolrad_a_tests COMMAND poolrad_a)
            endif()
        ''', ("test_a.c", "test_b.c"))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((report["source_files"], report["focused_executables"], report["focused_test_names"]), (2, 1, 1))
        self.assertEqual(report["registrations"][1]["test_names"], ["poolrad_a_tests"])

    def test_missing_dangling_and_duplicate_sources(self):
        result, report = self.invoke('''
            add_executable(a tests/recovered/test_a.c ./tests/recovered/test_a.c tests/recovered/test_gone.c)
            add_test(NAME a COMMAND a)
        ''', ("test_a.c", "test_missing.c"))
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["missing_sources"], ["tests/recovered/test_missing.c"])
        self.assertEqual(report["issues"]["dangling_sources"], ["tests/recovered/test_gone.c"])
        self.assertEqual(report["issues"]["duplicate_sources"], ["tests/recovered/test_a.c"])

    def test_duplicate_executable_and_test_name(self):
        result, report = self.invoke('''
            add_executable(a tests/recovered/test_a.c)
            add_executable(a tests/recovered/test_a.c)
            add_test(NAME repeated COMMAND a)
            add_test(NAME repeated COMMAND a)
        ''')
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["duplicate_executables"], ["a"])
        self.assertEqual(report["issues"]["duplicate_test_names"], ["repeated"])
        self.assertEqual(report["issues"]["multiple_tests_per_executable"], ["a"])

    def test_unknown_command_and_untested_executable(self):
        result, report = self.invoke('''
            add_executable(a tests/recovered/test_a.c)
            add_test(NAME a COMMAND absent)
        ''')
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["unknown_test_commands"], ["absent"])
        self.assertEqual(report["issues"]["untested_executables"], ["a"])

    def test_two_names_for_one_focused_executable(self):
        result, report = self.invoke('''
            add_executable(a tests/recovered/test_a.c)
            add_test(NAME first COMMAND a)
            add_test(NAME second COMMAND a)
        ''')
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["multiple_tests_per_executable"], ["a"])

    def test_comments_brackets_bom_crlf_and_hash(self):
        cmake = ('\ufeff#[=[ ignored add_test(NAME ghost COMMAND ghost) ]=]\r\n'
                 'add_executable(a [=[\r\ntests/recovered/test_a.c]=])\r\n'
                 'add_test(NAME "named test" COMMAND a)\r\n')
        result, report = self.invoke(cmake)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["cmake_sha256"], sha256(cmake.encode()).hexdigest())
        self.assertEqual(report["registrations"][0]["test_names"], ["named test"])

    def test_dynamic_or_mismatched_scopes_are_configuration_errors(self):
        cases = [
            'add_executable(a \u0024{SOURCES})',
            'if(OFF)\nadd_executable(a tests/recovered/test_a.c)\nendif()',
            'add_executable(a tests/recovered/test_a.c)\nif(BUILD_TESTING)\nadd_test(NAME a COMMAND a)\nendif()',
            'if(BUILD_TESTING)\nadd_executable(a tests/recovered/test_a.c)\nelse()\nendif()',
            'add_executable(a)\ntarget_sources(a PRIVATE FILE_SET headers TYPE HEADERS FILES tests/recovered/test_a.c)',
            'add_executable(a tests/recovered/test_a.c)\nadd_test(a a)',
            'add_executable(a tests/recovered/test_a.c)\nadd_test(NAME a COMMAND \u0024<TARGET_FILE:a>)',
        ]
        for cmake in cases:
            with self.subTest(cmake=cmake):
                result, report = self.invoke(cmake)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_malformed_commands_fail_closed(self):
        for cmake in ('add_executable(a tests/recovered/test_a.c', '#[=[ missing end',
                      'add_executable(a "missing quote)', 'endif()'):
            with self.subTest(cmake=cmake):
                result, report = self.invoke(cmake)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_empty_inventory_is_configuration_error(self):
        result, report = self.invoke("add_executable(a elsewhere.c)", files=())
        self.assertEqual(result.returncode, 2)
        self.assertIn("error", report)


if __name__ == "__main__":
    unittest.main()
