"""Failure paths for the candidate pricer's input handling.

The pricer reads a gitignored, derived queue file, so a fresh checkout does not
have it. These tests pin the two acceptable behaviours: generate it when the
prerequisites exist, and otherwise stop with an actionable message and a
distinct exit code — never a bare traceback.
"""
import io
import contextlib
import tempfile
import unittest
from pathlib import Path

import price_candidates as pricer


class InputHandlingTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.queue = self.root / "queue.json"

    def test_fresh_checkout_reports_actionably(self):
        message = pricer.input_problem(self.queue, self.root)
        self.assertIsNotNone(message)
        self.assertIn("QUEUE INPUT MISSING", message)
        self.assertIn("NOT a tree failure", message)
        # It must name the exact commands, not send the reader to the source.
        self.assertIn("function_queue_audit.py", message)
        self.assertIn("strict_candidate_queue.py", message)
        self.assertIn("resources/DOS/POOLRAD", message)

    def test_present_input_is_not_a_problem(self):
        self.queue.write_text('{"rows": []}')
        self.assertIsNone(pricer.input_problem(self.queue, self.root))

    def test_prerequisites_detected_as_generatable(self):
        accounting = self.root / "resources/function-accounting-current"
        media = self.root / "resources/DOS/POOLRAD"
        accounting.mkdir(parents=True)
        media.mkdir(parents=True)
        for name in ("inventory.json", "callgraph.json", "near.json"):
            (accounting / name).write_text("{}")
        (media / "START.EXE").write_bytes(b"MZ")
        (media / "GAME.OVR").write_bytes(b"OVR")
        self.assertEqual(pricer.input_problem(self.queue, self.root), "generate")

    def test_main_exits_with_input_code_not_a_traceback(self):
        real = pricer.gate.missing_dependencies
        pricer.gate.missing_dependencies = lambda: []   # this test is about the INPUT
        buffer = io.StringIO()
        try:
            with contextlib.redirect_stdout(buffer):
                code = pricer.main(["--root", str(self.root), "--queue", str(self.queue)])
        finally:
            pricer.gate.missing_dependencies = real
        self.assertEqual(code, pricer.INPUT_EXIT)
        self.assertIn("QUEUE INPUT MISSING", buffer.getvalue())

    def test_missing_environment_is_distinct_from_missing_input(self):
        real = pricer.gate.missing_dependencies
        pricer.gate.missing_dependencies = lambda: ["capstone==5.0.7"]
        buffer = io.StringIO()
        try:
            with contextlib.redirect_stdout(buffer):
                code = pricer.main(["--root", str(self.root), "--queue", str(self.queue)])
        finally:
            pricer.gate.missing_dependencies = real
        self.assertEqual(code, pricer.gate.ENVIRONMENT_EXIT)
        self.assertIn("ENVIRONMENT MISSING", buffer.getvalue())


if __name__ == "__main__":
    unittest.main()
