"""Synthetic registry/CMake link fixtures; no C bodies are read or executed."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_focused_recovered_links.py"


class FocusedRecoveredLinkTests(unittest.TestCase):
    def invoke(self, links, allowed=(), required="STATIC", stale=False, early=""):
        with tempfile.TemporaryDirectory(prefix="link metadata ") as directory:
            root = Path(directory)
            (root / "tests/recovered").mkdir(parents=True)
            source = "tests/recovered/test_a.c"
            (root / source).write_text("not C")
            cmake = (early + "\nadd_library(poolrad_recovered " + required + ")\n"
                     "add_library(poolrad_formats STATIC)\nif(BUILD_TESTING)\n"
                     "add_executable(poolrad_a_tests tests/recovered/test_a.c)\n" + links +
                     "\nadd_test(NAME poolrad_a_tests COMMAND poolrad_a_tests)\nendif()\n")
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
            report = root / "report.json"
            command = [sys.executable, str(SCRIPT), "--root", str(root),
                       "--registry", str(root / "registry.json"), "--json", str(report)]
            for library in allowed:
                command += ["--allow-library", library]
            result = subprocess.run(command, capture_output=True, text=True, timeout=15, check=False)
            self.assertTrue(report.exists(), result.stderr)
            return result, json.loads(report.read_text())

    def test_private_required_and_declared_extra_library(self):
        result, report = self.invoke("target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered poolrad_formats)")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["required_link_count"], 1)
        self.assertFalse(any(report["issues"].values()))

    def test_missing_required_link(self):
        result, report = self.invoke("target_link_libraries(poolrad_a_tests PRIVATE poolrad_formats)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["missing_required_links"], ["poolrad_a_tests"])

    def test_duplicate_required_links_in_one_or_multiple_declarations(self):
        for links in (
            "target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered poolrad_recovered)",
            "target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered)\ntarget_link_libraries(poolrad_a_tests PUBLIC poolrad_recovered)",
        ):
            with self.subTest(links=links):
                result, report = self.invoke(links)
                self.assertEqual(result.returncode, 1)
                self.assertEqual(report["issues"]["duplicate_required_links"], ["poolrad_a_tests"])

    def test_unknown_target_and_library(self):
        result, report = self.invoke(
            "target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered poolrad_typo)\n"
            "target_link_libraries(poolrad_gone_tests PRIVATE poolrad_recovered)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["unknown_libraries"][0]["library"], "poolrad_typo")
        self.assertEqual(report["issues"]["unknown_link_targets"][0]["target"], "poolrad_gone_tests")

    def test_explicit_external_allowance(self):
        links = "target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered m)"
        result, report = self.invoke(links)
        self.assertEqual(result.returncode, 1)
        result, report = self.invoke(links, allowed=("m",))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["allowed_auxiliary_libraries"], ["m", "poolrad_formats"])

    def test_interface_link_does_not_satisfy_required_compile_link(self):
        result, report = self.invoke("target_link_libraries(poolrad_a_tests INTERFACE poolrad_recovered)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["required_link_count"], 0)
        self.assertEqual(report["issues"]["interface_required_links"], ["poolrad_a_tests"])

    def test_unsupported_or_malformed_links_fail_closed(self):
        cases = [
            "target_link_libraries(poolrad_a_tests poolrad_recovered)",
            "target_link_libraries(poolrad_a_tests PRIVATE \u0024{LIBS})",
            "target_link_libraries(poolrad_a_tests PRIVATE optimized poolrad_recovered)",
            "target_link_libraries(poolrad_a_tests PRIVATE -lm poolrad_recovered)",
            "target_link_libraries(poolrad_a_tests PRIVATE /tmp/library.a)",
            "target_link_libraries(poolrad_a_tests PRIVATE PUBLIC poolrad_recovered)",
            "if(OFF)\ntarget_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered)\nendif()",
            "target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered",
        ]
        for links in cases:
            with self.subTest(links=links):
                result, report = self.invoke(links)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_stale_registry_and_noncompiling_required_library_fail(self):
        for options in ({"stale": True}, {"required": "INTERFACE"}, {"required": "ALIAS other"}):
            with self.subTest(options=options):
                result, report = self.invoke("target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered)", **options)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)


    def test_declared_library_outside_profile_is_flagged(self):
        result, report = self.invoke(
            "add_library(other_library STATIC)\ntarget_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered other_library)")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["issues"]["out_of_scope_links"][0]["library"], "other_library")
        self.assertEqual(report["issues"]["unknown_libraries"], [])

    def test_outside_selection_and_before_definition(self):
        result, report = self.invoke(
            "add_executable(probe helper.c)\ntarget_link_libraries(probe PRIVATE poolrad_recovered)\n"
            "target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered)")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["outside_selection"][0]["target"], "probe")
        result, report = self.invoke("", early="target_link_libraries(poolrad_a_tests PRIVATE poolrad_recovered)")
        self.assertEqual(result.returncode, 2)
        self.assertIn("scope/order", report["error"])


if __name__ == "__main__":
    unittest.main()
