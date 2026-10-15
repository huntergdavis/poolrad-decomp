"""Synthetic integration tests; no game media, decoder, or recovered function execution."""
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools/ci/audit_recovered_headers.py"


class HeaderAuditTests(unittest.TestCase):
    def invoke(self, directory, headers, compiler="cc", timeout=10):
        include_root = Path(directory) / "include with spaces"
        recovered = include_root / "poolrad/recovered"
        recovered.mkdir(parents=True)
        for name, source in headers.items():
            (recovered / name).write_text(source)
        report = Path(directory) / "report.json"
        result = subprocess.run(
            [sys.executable, str(SCRIPT), "--include-root", str(include_root),
             "--cc", compiler, "--jobs", "2", "--timeout", str(timeout), "--json", str(report)],
            capture_output=True, text=True, timeout=20, check=False)
        self.assertTrue(report.exists(), result.stderr)
        return result, json.loads(report.read_text())

    @unittest.skipUnless(shutil.which("cc"), "a C11 compiler is required")
    def test_independent_headers_and_warnings_are_errors(self):
        with tempfile.TemporaryDirectory(prefix="header audit ") as directory:
            result, report = self.invoke(directory, {
                "a_good.h": "#include <stdint.h>\nstruct good { uint32_t value; };\n",
                "b_missing_dependency.h": "struct bad { uint32_t value; };\n",
                "c_warning.h": "static inline int warning(void) { int unused; return 0; }\n",
            })
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual((report["headers_checked"], report["passed"], report["failed"]), (3, 1, 2))
        results = report["results"]
        self.assertEqual([r["status"] for r in results], ["passed", "failed", "failed"])
        self.assertIn("uint32_t", results[1]["stderr"])
        self.assertIn("unused", results[2]["stderr"])
        self.assertIn("-std=c11", report["compile_argv"])
        self.assertIn("-Werror", report["compile_argv"])
        self.assertIn("-fsyntax-only", report["compile_argv"])

    def test_missing_compiler_is_a_tool_error(self):
        with tempfile.TemporaryDirectory() as directory:
            result, report = self.invoke(directory, {"good.h": "struct good { int value; };\n"},
                                         compiler=str(Path(directory) / "nonexistent-compiler"))
        self.assertEqual(result.returncode, 2)
        self.assertIn("error", report)
        self.assertEqual(report["headers_checked"], 0)

    def test_empty_inventory_does_not_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            result, report = self.invoke(directory, {})
        self.assertEqual(result.returncode, 2)
        self.assertIn("no public headers", report["error"])

    def test_compiler_timeout_is_a_failed_header(self):
        with tempfile.TemporaryDirectory(prefix="header timeout ") as directory:
            wrapper = Path(directory) / "compiler wrapper.py"
            wrapper.write_text("import sys, time\n"
                               "if '--version' in sys.argv: print('synthetic compiler wrapper')\n"
                               "else: time.sleep(5)\n")
            compiler = shlex.join([sys.executable, str(wrapper)])
            result, report = self.invoke(directory, {"good.h": "struct good { int value; };\n"},
                                         compiler=compiler, timeout=1)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual((report["passed"], report["failed"], report["timed_out"]), (0, 1, 1))
        self.assertEqual(report["results"][0]["status"], "timeout")


if __name__ == "__main__":
    unittest.main()
