"""Synthetic Markdown and registry fixtures; no recovered C bodies or CMake execution."""
from copy import deepcopy
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_test_documentation.py"


class DocumentationTests(unittest.TestCase):
    def invoke(self, documents, mutate=None, change_cmake=False, extra_source=False, selected=None):
        with tempfile.TemporaryDirectory(prefix="documentation metadata ") as directory:
            root = Path(directory)
            (root / "tests/recovered").mkdir(parents=True)
            sources = ["tests/recovered/test_a.c", "tests/recovered/test_b.c"]
            for source in sources:
                (root / source).write_text("deliberately not C")
            cmake = b"synthetic metadata; not executed\n"
            (root / "CMakeLists.txt").write_bytes(cmake + (b"changed" if change_cmake else b""))
            registry = {
                "schema_version": 1, "kind": "recovered_test_registration", "passed": True,
                "cmake_sha256": sha256(cmake).hexdigest(), "inventory": sources,
                "source_files": 2, "focused_executables": 2, "focused_test_names": 2,
                "all_executable_names": ["poolrad_a_tests", "poolrad_b_tests", "poolrad_other_tests"],
                "all_test_names": ["poolrad_a_tests", "poolrad_b_tests", "poolrad_other_tests"],
                "registrations": [
                    {"source": source, "executable": target, "test_names": [target]}
                    for source, target in zip(sources, ["poolrad_a_tests", "poolrad_b_tests"])],
                "issues": {"missing_sources": []},
            }
            if mutate:
                mutate(registry)
            registry_path = root / "registry.json"
            registry_path.write_text(json.dumps(registry))
            if extra_source:
                (root / "tests/recovered/test_added.c").write_text("not C")
            for path, contents in documents.items():
                doc = root / path
                doc.parent.mkdir(parents=True, exist_ok=True)
                doc.write_text(contents)
            output = root / "report.json"
            command = [sys.executable, str(SCRIPT), "--root", str(root), "--registry", str(registry_path), "--json", str(output)]
            if selected is not None:
                command += ["--doc", selected]
            result = subprocess.run(command, capture_output=True, text=True, timeout=15, check=False)
            self.assertTrue(output.exists(), result.stderr)
            return result, json.loads(output.read_text())

    def test_targets_and_relative_source_links_cover_registry(self):
        result, report = self.invoke({
            "README.md": "Run poolrad_a_tests and poolrad_other_tests.",
            "docs/NOTES.md": "[B source](../tests/recovered/test_b.c#L12)",
        })
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["covered_sources"], 2)
        self.assertEqual(len(report["document_hashes"]), 2)
        self.assertEqual(report["stale_references"], [])
        self.assertEqual(report["broken_source_links"], [])

    def test_missing_and_stale_references_are_reported_with_locations(self):
        result, report = self.invoke({"README.md": "poolrad_a_tests\npoolrad_gone_tests tests/recovered/test_removed.c\n"})
        self.assertEqual(result.returncode, 1)
        self.assertEqual([r["source"] for r in report["missing_documentation"]], ["tests/recovered/test_b.c"])
        self.assertEqual({r["reference"] for r in report["stale_references"]},
                         {"poolrad_gone_tests", "tests/recovered/test_removed.c"})
        self.assertEqual({r["line"] for r in report["stale_references"]}, {2})

    def test_wrong_relative_source_link_is_broken(self):
        result, report = self.invoke({
            "README.md": "poolrad_a_tests poolrad_b_tests",
            "docs/NOTES.md": "[wrong](tests/recovered/test_a.c)",
        })
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report["broken_source_links"][0]["document"], "docs/NOTES.md")
        self.assertEqual(report["broken_source_links"][0]["destination"], "tests/recovered/test_a.c")

    def test_external_urls_and_partial_regex_names_are_not_stale(self):
        result, report = self.invoke({"README.md":
            "poolrad_a_tests poolrad_b_tests poolrad_a\n"
            "[archived](https://example.invalid/tests/recovered/test_removed.c)\n"
            "https://example.invalid/poolrad_removed_tests\n"})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["stale_references"], [])

    def test_stale_cmake_or_filename_inventory_requires_fresh_registry(self):
        for options in ({"change_cmake": True}, {"extra_source": True}):
            with self.subTest(options=options):
                result, report = self.invoke({"README.md": "poolrad_a_tests poolrad_b_tests"}, **options)
                self.assertEqual(result.returncode, 2)
                self.assertIn("stale", report["error"])

    def test_invalid_registry_is_configuration_error(self):
        def duplicate(registry):
            registry["registrations"].append(deepcopy(registry["registrations"][0]))
        mutations = [
            lambda r: r.update(schema_version=True),
            lambda r: r.update(passed=False),
            lambda r: r.update(source_files=True),
            lambda r: r.update(issues={"missing_sources": ["missing"]}),
            lambda r: r["registrations"][0].update(executable="undeclared"),
            lambda r: r["registrations"][0].update(test_names=[]),
            duplicate,
        ]
        for mutate in mutations:
            with self.subTest(mutate=mutate):
                result, report = self.invoke({"README.md": "poolrad_a_tests poolrad_b_tests"}, mutate=mutate)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error", report)

    def test_empty_or_invalid_document_selection_fails(self):
        for documents, selected in (({}, None), ({"README.md": "poolrad_a_tests"}, "missing.md"),
                                    ({"README.md": "poolrad_a_tests"}, "../outside.md")):
            with self.subTest(selected=selected):
                result, report = self.invoke(documents, selected=selected)
                self.assertEqual(result.returncode, 2)
                self.assertIn("error", report)

    def test_explicit_document_scope_and_hash(self):
        content = "poolrad_a_tests poolrad_b_tests\n"
        result, report = self.invoke({"README.md": content, "docs/IGNORED.md": "poolrad_stale_tests"}, selected="README.md")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report["document_hashes"], {"README.md": sha256(content.encode()).hexdigest()})


if __name__ == "__main__":
    unittest.main()
