"""Synthetic CMake/registry metadata only; never execute CMake or recovered C."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_test_properties.py"


class RecoveredPropertyTests(unittest.TestCase):
    def invoke(self, properties, allowed=(), stale=False):
        with tempfile.TemporaryDirectory(prefix="CTest property metadata ") as directory:
            root = Path(directory)
            (root / "tests/recovered").mkdir(parents=True)
            source = "tests/recovered/test_a.c"
            (root / source).write_text("not C")
            cmake = ("if(BUILD_TESTING)\nadd_executable(poolrad_a_tests tests/recovered/test_a.c)\n"
                     "add_test(NAME poolrad_a_tests COMMAND poolrad_a_tests)\n" + properties + "\nendif()\n")
            (root / "CMakeLists.txt").write_text(cmake + ("# changed" if stale else ""))
            registry = {
                "schema_version": 1, "kind": "recovered_test_registration", "passed": True,
                "cmake_sha256": sha256(cmake.encode()).hexdigest(), "inventory": [source],
                "source_files": 1, "focused_executables": 1, "focused_test_names": 1,
                "all_executable_names": ["poolrad_a_tests"], "all_test_names": ["poolrad_a_tests"],
                "registrations": [{"source": source, "executable": "poolrad_a_tests", "test_names": ["poolrad_a_tests"]}],
                "issues": {"missing_sources": []},
            }
            (root / "registry.json").write_text(json.dumps(registry))
            output = root / "report.json"
            args = [sys.executable, str(SCRIPT), "--root", str(root), "--registry", str(root / "registry.json"),
                    "--json", str(output)]
            for name in allowed:
                args += ["--allow-property", name]
            result = subprocess.run(args, capture_output=True, text=True, timeout=15, check=False)
            self.assertTrue(output.exists(), result.stderr)
            return result, json.loads(output.read_text())

    def test_zero_test_property_applicability(self):
        result, report = self.invoke("set_property(TARGET poolrad_a_tests PROPERTY FOLDER unit)")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["property_declaration_count"], 0)
        self.assertEqual(report["property_assignment_count"], 0)

    def test_literal_properties_and_false_disabled(self):
        result, report = self.invoke(
            "set_tests_properties(poolrad_a_tests PROPERTIES DISABLED OFF TIMEOUT 30 LABELS unit)\n"
            "set_property(TEST poolrad_a_tests PROPERTY WILL_FAIL FALSE)")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["property_declaration_count"], 2)
        self.assertEqual(report["focused_property_assignment_count"], 4)
        self.assertFalse(any(report["issues"].values()))

    def test_explicit_disabled_true_forms_are_flagged(self):
        for value in ("ON", "1", "True", "YES", "y"):
            with self.subTest(value=value):
                result, report = self.invoke("set_tests_properties(poolrad_a_tests PROPERTIES DISABLED " + value + ")")
                self.assertEqual(result.returncode, 1)
                self.assertEqual(report["issues"]["disabled_focused_tests"], ["poolrad_a_tests"])

    def test_unknown_test_and_unknown_focused_property(self):
        result, report = self.invoke(
            "set_tests_properties(poolrad_gone_tests PROPERTIES TIMEOUT 30)\n"
            "set_property(TEST poolrad_a_tests PROPERTY TIMEUOT 30)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["unknown_tests"][0]["test"], "poolrad_gone_tests")
        self.assertEqual(report["issues"]["unknown_focused_properties"][0]["property"], "TIMEUOT")

    def test_duplicate_properties_in_one_or_multiple_declarations(self):
        cases = [
            "set_tests_properties(poolrad_a_tests PROPERTIES TIMEOUT 10 TIMEOUT 20)",
            "set_tests_properties(poolrad_a_tests PROPERTIES TIMEOUT 10)\nset_property(TEST poolrad_a_tests PROPERTY TIMEOUT 20)",
            "set_tests_properties(poolrad_a_tests poolrad_a_tests PROPERTIES TIMEOUT 10)",
        ]
        for properties in cases:
            with self.subTest(properties=properties):
                result, report = self.invoke(properties)
                self.assertEqual(result.returncode, 1)
                self.assertEqual(report["issues"]["duplicate_focused_properties"][0]["property"], "TIMEOUT")

    def test_explicit_custom_property_allowance(self):
        properties = "set_property(TEST poolrad_a_tests PROPERTY CUSTOM_METADATA unit)"
        result, report = self.invoke(properties)
        self.assertEqual(result.returncode, 1)
        result, report = self.invoke(properties, allowed=("CUSTOM_METADATA",))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["explicit_allowed_properties"], ["CUSTOM_METADATA"])

    def test_unsupported_or_malformed_forms_fail_closed(self):
        cases = [
            "set_tests_properties(poolrad_a_tests PROPERTIES DISABLED \u0024{DISABLE})",
            "set_tests_properties(poolrad_a_tests DIRECTORY .. PROPERTIES TIMEOUT 30)",
            "set_property(TEST poolrad_a_tests APPEND PROPERTY LABELS unit)",
            "set_property(TEST poolrad_a_tests PROPERTY LABELS unit other)",
            "set_tests_properties(poolrad_a_tests PROPERTIES DISABLED 2)",
            "set_tests_properties(poolrad_a_tests PROPERTIES TIMEOUT)",
            "set_tests_properties(poolrad_a_tests TIMEOUT 30)",
            "if(OFF)\nset_tests_properties(poolrad_a_tests PROPERTIES DISABLED ON)\nendif()",
            "set_tests_properties(poolrad_a_tests PROPERTIES TIMEOUT 30",
        ]
        for properties in cases:
            with self.subTest(properties=properties):
                result, report = self.invoke(properties)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_stale_registry_requires_refresh(self):
        result, report = self.invoke("", stale=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("stale", report["error"])


if __name__ == "__main__":
    unittest.main()
