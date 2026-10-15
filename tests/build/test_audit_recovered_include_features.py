"""Synthetic CMake/registry fixtures; no C bodies or configured/compiler execution."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_include_features.py"


class IncludeFeaturesTests(unittest.TestCase):
    def invoke(self, library="", focused="", early="", stale=False, late=""):
        with tempfile.TemporaryDirectory(prefix="compile metadata ") as directory:
            root = Path(directory)
            (root / "tests/recovered").mkdir(parents=True)
            source = "tests/recovered/test_a.c"
            (root / source).write_text("not C")
            cmake = (early + "\nadd_library(poolrad_recovered STATIC)\n" + library +
                     "\nif(BUILD_TESTING)\nadd_executable(poolrad_a_tests tests/recovered/test_a.c)\n" +
                     focused + "\nadd_test(NAME poolrad_a_tests COMMAND poolrad_a_tests)\nendif()\n" + late)
            (root / "CMakeLists.txt").write_text(cmake + ("# stale" if stale else ""))
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
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--root", str(root), "--registry", str(root / "registry.json"), "--json", str(output)],
                capture_output=True, text=True, timeout=15, check=False)
            self.assertTrue(output.exists(), result.stderr)
            return result, json.loads(output.read_text())

    def test_bounded_variable_modifiers_features_and_guard_metadata(self):
        template = "$" + "{CMAKE_CURRENT_SOURCE_DIR}/include"
        result, report = self.invoke(
            library="target_include_directories(poolrad_recovered SYSTEM BEFORE PUBLIC " + template + ")",
            focused="target_compile_features(poolrad_a_tests PRIVATE c_std_11)")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((report["include_declarations"], report["feature_declarations"], report["bounded_variable_items"]), (1, 1, 1))
        self.assertEqual(report["declarations"][0]["modifiers"], ["SYSTEM", "BEFORE"])
        self.assertEqual(report["declarations"][0]["items"][0]["literal"], template)
        self.assertEqual(report["declarations"][0]["items"][0]["form"], "bounded_variable")
        self.assertEqual(report["declarations"][1]["guard"], [{"command": "if", "arguments": ["BUILD_TESTING"]}])

    def test_literal_relative_paths_visibility_and_features(self):
        result, report = self.invoke(
            library="target_include_directories(poolrad_recovered PUBLIC ./include INTERFACE include/portable)",
            focused="target_compile_features(poolrad_a_tests PUBLIC c_std_11 INTERFACE c_restrict)")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["declarations"][0]["items"][0]["normalized"], "include")
        self.assertEqual([i["visibility"] for i in report["declarations"][1]["items"]], ["PUBLIC", "INTERFACE"])
        self.assertEqual([d["call_index"] for d in report["declarations"]], [0, 1])

    def test_zero_direct_metadata(self):
        result, report = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((report["include_declarations"], report["feature_declarations"]), (0, 0))

    def test_unknown_target_and_nonselected_declarations(self):
        result, report = self.invoke(library=
            "add_executable(probe helper.c)\ntarget_include_directories(probe PRIVATE include)\n"
            "target_compile_features(gone PRIVATE c_std_11)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["unknown_targets"][0]["target"], "gone")
        self.assertEqual({d["target"] for d in report["outside_selection"]}, {"probe", "gone"})
        self.assertEqual(report["include_declarations"], 0)

    def test_unresolved_outside_bounds_and_malformed_forms_fail_closed(self):
        cases = [
            "target_include_directories(poolrad_a_tests PRIVATE \u0024{UNBOUNDED}/include)",
            "target_include_directories(poolrad_a_tests PRIVATE \u0024{CMAKE_CURRENT_SOURCE_DIR}/include/private)",
            "target_include_directories(poolrad_a_tests PRIVATE \u0024<BUILD_INTERFACE:include>)",
            "target_include_directories(poolrad_a_tests PRIVATE /tmp/include)",
            "target_include_directories(poolrad_a_tests PRIVATE ../outside)",
            "target_include_directories(poolrad_a_tests BEFORE SYSTEM PRIVATE include)",
            "target_include_directories(poolrad_a_tests PRIVATE PUBLIC include)",
            "target_compile_features(poolrad_a_tests PRIVATE \u0024{FEATURES})",
            "target_compile_features(poolrad_a_tests PRIVATE c-std-11)",
            "target_compile_features(poolrad_a_tests PRIVATE c_std_11",
        ]
        for focused in cases:
            with self.subTest(focused=focused):
                result, report = self.invoke(focused=focused)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_unsupported_and_mismatched_guards_fail_closed(self):
        result, report = self.invoke(focused="if(OFF)\ntarget_include_directories(poolrad_a_tests PRIVATE include)\nendif()")
        self.assertEqual(result.returncode, 2)
        self.assertIn("guard", report["error"])
        result, report = self.invoke(late="target_include_directories(poolrad_a_tests PRIVATE include)")
        self.assertEqual(result.returncode, 2)
        self.assertIn("guard", report["error"])

    def test_declaration_before_definition_is_rejected(self):
        result, report = self.invoke(early="target_include_directories(poolrad_recovered PUBLIC include)")
        self.assertEqual(result.returncode, 2)
        self.assertIn("precedes", report["error"])

    def test_stale_registry_requires_refresh(self):
        result, report = self.invoke(stale=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("stale", report["error"])


if __name__ == "__main__":
    unittest.main()
