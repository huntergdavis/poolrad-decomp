"""Synthetic CMake/registry fixtures; no C bodies or configured/compiler execution."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_compile_metadata.py"


class CompileMetadataTests(unittest.TestCase):
    def invoke(self, library="", focused="", early="", stale=False):
        with tempfile.TemporaryDirectory(prefix="compile metadata ") as directory:
            root = Path(directory)
            (root / "tests/recovered").mkdir(parents=True)
            source = "tests/recovered/test_a.c"
            (root / source).write_text("not C")
            cmake = (early + "\nadd_library(poolrad_recovered STATIC)\n" + library +
                     "\nif(BUILD_TESTING)\nadd_executable(poolrad_a_tests tests/recovered/test_a.c)\n" +
                     focused + "\nadd_test(NAME poolrad_a_tests COMMAND poolrad_a_tests)\nendif()\n")
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

    def test_literal_compiler_guard_definitions_and_before_order(self):
        result, report = self.invoke(
            library='if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")\ntarget_compile_options(poolrad_recovered PRIVATE -Wall)\nendif()',
            focused='target_compile_definitions(poolrad_a_tests PUBLIC ANSWER=42 -DFLAG "" -D)\ntarget_compile_options(poolrad_a_tests BEFORE PRIVATE -Wextra)')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((report["option_declarations"], report["definition_declarations"]), (2, 1))
        self.assertEqual(report["declarations"][0]["guard"][0]["arguments"], ["CMAKE_C_COMPILER_ID", "MATCHES", "GNU|Clang"])
        self.assertTrue(report["declarations"][2]["before"])
        self.assertEqual([d["call_index"] for d in report["declarations"]], [0, 1, 2])
        self.assertEqual([v["literal"] for v in report["declarations"][1]["items"]], ["ANSWER=42", "-DFLAG", "", "-D"])

    def test_visibility_groups_preserved_without_effective_flag_claim(self):
        result, report = self.invoke(library="target_compile_options(poolrad_recovered PRIVATE -Wall INTERFACE -Wextra)")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual([v["visibility"] for v in report["declarations"][0]["items"]], ["PRIVATE", "INTERFACE"])

    def test_zero_direct_compile_metadata(self):
        result, report = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["literal_item_count"], 0)
        self.assertEqual(report["selected_target_count"], 2)

    def test_unknown_target_and_declared_outside_selection(self):
        result, report = self.invoke(library=
            "add_executable(probe helper.c)\ntarget_compile_definitions(probe PRIVATE PROBE=1)\n"
            "target_compile_options(gone PRIVATE -Wall)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["unknown_compile_targets"][0]["target"], "gone")
        self.assertEqual({d["target"] for d in report["outside_selection"]}, {"probe", "gone"})
        self.assertEqual(report["definition_declarations"], 0)

    def test_unresolved_or_malformed_values_fail_closed(self):
        cases = [
            "target_compile_options(poolrad_a_tests PRIVATE \u0024{FLAGS})",
            "target_compile_definitions(poolrad_a_tests PRIVATE \u0024<1:FLAG>)",
            'target_compile_options(poolrad_a_tests PRIVATE "SHELL:-X a")',
            "target_compile_definitions(poolrad_a_tests PRIVATE 1_BAD=42)",
            "target_compile_definitions(poolrad_a_tests BEFORE PRIVATE FLAG)",
            "target_compile_options(poolrad_a_tests -Wall)",
            "target_compile_options(poolrad_a_tests PRIVATE PUBLIC -Wall)",
            "target_compile_options(poolrad_a_tests PRIVATE -Wall",
        ]
        for focused in cases:
            with self.subTest(focused=focused):
                result, report = self.invoke(focused=focused)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_guard_mismatch_and_unsupported_guard_fail_closed(self):
        cases = [
            {"library": "target_compile_options(poolrad_a_tests PRIVATE -Wall)"},
            {"focused": "if(OFF)\ntarget_compile_options(poolrad_a_tests PRIVATE -Wall)\nendif()"},
        ]
        for options in cases:
            with self.subTest(options=options):
                result, report = self.invoke(**options)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)

    def test_declaration_before_target_is_rejected(self):
        result, report = self.invoke(early="target_compile_options(poolrad_recovered PRIVATE -Wall)")
        self.assertEqual(result.returncode, 2)
        self.assertIn("precedes", report["error"])

    def test_stale_registry_requires_refresh(self):
        result, report = self.invoke(stale=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("stale", report["error"])


if __name__ == "__main__":
    unittest.main()
