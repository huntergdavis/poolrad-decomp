"""Synthetic path/CMake/registry fixtures; source contents are never read or compiled."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_test_helper_sources.py"


class HelperSourceTests(unittest.TestCase):
    def invoke(self, helpers="", extra="", early="", late="", helper_files=("src/replay/helper.c",),
               second=False, root_symlink=False, stale=False):
        with tempfile.TemporaryDirectory(prefix="helper membership ") as directory:
            root = Path(directory)
            sources = ["tests/recovered/test_a.c"] + (["tests/recovered/test_b.c"] if second else [])
            targets = ["poolrad_a_tests"] + (["poolrad_b_tests"] if second else [])
            for source in sources + list(helper_files):
                path = root / source
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("not C")
            if root_symlink:
                (root / "src/replay").mkdir(parents=True, exist_ok=True)
                (root / "src/replay/root.c").symlink_to(root, target_is_directory=True)
            definitions = "\n".join("add_executable(" + target + " " + source + " " + helpers + ")"
                                    for source, target in zip(sources, targets))
            tests = "\n".join("add_test(NAME " + target + " COMMAND " + target + ")" for target in targets)
            cmake = early + "\nif(BUILD_TESTING)\n" + definitions + "\n" + extra + "\n" + tests + "\nendif()\n" + late
            (root / "CMakeLists.txt").write_text(cmake + ("# changed" if stale else ""))
            registry = {
                "schema_version": 1, "kind": "recovered_test_registration", "passed": True,
                "cmake_sha256": sha256(cmake.encode()).hexdigest(), "inventory": sources,
                "source_files": len(sources), "focused_executables": len(targets), "focused_test_names": len(targets),
                "all_executable_names": targets, "all_test_names": targets,
                "registrations": [{"source": source, "executable": target, "test_names": [target]}
                                  for source, target in zip(sources, targets)],
                "issues": {"missing_sources": []},
            }
            (root / "registry.json").write_text(json.dumps(registry))
            report = root / "report.json"
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--root", str(root), "--registry", str(root / "registry.json"), "--json", str(report)],
                capture_output=True, text=True, timeout=15, check=False)
            self.assertTrue(report.exists(), result.stderr)
            return result, json.loads(report.read_text())

    def test_existing_helper_and_literal_target_sources(self):
        result, report = self.invoke("src/replay/helper.c",
            extra="target_sources(poolrad_a_tests PRIVATE include/helper.h)",
            helper_files=("src/replay/helper.c", "include/helper.h"))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((report["direct_file_arguments"], report["auxiliary_occurrences"], report["auxiliary_target_count"]), (3, 2, 1))

    def test_shared_helper_across_executables_is_allowed(self):
        result, report = self.invoke("src/replay/helper.c", second=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["auxiliary_occurrences"], 2)
        self.assertEqual(report["unique_auxiliary_sources"], ["src/replay/helper.c"])
        self.assertEqual(report["issues"]["duplicate_sources"], [])

    def test_duplicate_aliases_have_one_diagnostic(self):
        result, report = self.invoke("src/replay/helper.c ./src/replay/helper.c")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(len(report["issues"]["duplicate_sources"]), 1)
        self.assertEqual(report["issues"]["duplicate_sources"][0]["source"], "src/replay/helper.c")

    def test_nonexistent_helper_has_one_diagnostic(self):
        result, report = self.invoke("src/replay/missing.c")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(len(report["issues"]["nonexistent_sources"]), 1)
        self.assertEqual(report["issues"]["nonexistent_sources"][0]["source"], "src/replay/missing.c")

    def test_escaping_absolute_and_resource_paths_are_outside_scope(self):
        result, report = self.invoke("../outside.c /tmp/absolute.c resources/media.c")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(len(report["issues"]["out_of_scope_sources"]), 3)
        self.assertEqual(report["issues"]["nonexistent_sources"], [])

    def test_guard_and_order_mismatches_fail_closed(self):
        cases = [
            {"early": "target_sources(poolrad_a_tests PRIVATE src/replay/helper.c)"},
            {"late": "target_sources(poolrad_a_tests PRIVATE src/replay/helper.c)"},
            {"extra": "if(OFF)\ntarget_sources(poolrad_a_tests PRIVATE src/replay/helper.c)\nendif()"},
        ]
        for options in cases:
            with self.subTest(options=options):
                result, report = self.invoke(**options)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)

    def test_dynamic_and_malformed_sources_fail_closed(self):
        cases = [
            {"helpers": "\u0024{HELPERS}"},
            {"extra": "target_sources(poolrad_a_tests PRIVATE FILE_SET files FILES src/replay/helper.c)"},
            {"extra": "target_sources(poolrad_a_tests INTERFACE src/replay/helper.c)"},
            {"extra": "target_sources(poolrad_a_tests PRIVATE src/replay/helper.c"},
        ]
        for options in cases:
            with self.subTest(options=options):
                result, report = self.invoke(**options)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)

    def test_symlink_to_source_tree_root_is_outside_file_scope(self):
        result, report = self.invoke("src/replay/root.c", root_symlink=True)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(len(report["issues"]["out_of_scope_sources"]), 1)

    def test_stale_registry_requires_refresh(self):
        result, report = self.invoke(stale=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("stale", report["error"])


if __name__ == "__main__":
    unittest.main()
