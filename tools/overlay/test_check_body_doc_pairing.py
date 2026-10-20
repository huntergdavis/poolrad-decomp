"""The body/checker pairing gate must refuse an unpaired body.

The failure it prevents already happened once: an untested model was committed
into a reviewable PR by `git add -A`, and only a human going to do something else
caught it. These tests drive the refusal from synthetic change sets, which is the
same shape as the real diff but does not need a git repository.
"""
import contextlib
import io
import tempfile
import unittest
from pathlib import Path

import check_body_doc_pairing as gate

BODY = "src/recovered/overlay_0068_0294_cpu.c"
AUDIT = "tools/overlay/u68_roll_gate_0294_audit.py"
DOC = "docs/re/U68_ROLL_GATE_0294_CPU.md"


class PairingGateTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)

    def write(self, path, text="x"):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)

    def run_gate(self, changed):
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            code = gate.main(["--root", str(self.root), "--changed", *changed])
        return code, buffer.getvalue()

    def test_body_without_document_is_refused(self):
        self.write(BODY)
        code, output = self.run_gate([BODY])
        self.assertEqual(code, 1)
        self.assertIn("UNPAIRED BODY", output)
        self.assertIn("no evidence document", output)

    def test_body_with_document_passes(self):
        self.write(BODY)
        self.write(DOC, "# doc")
        code, output = self.run_gate([BODY, DOC])
        self.assertEqual(code, 0)
        self.assertIn("OK:", output)

    def test_checker_without_checker_rows_is_refused(self):
        self.write(AUDIT, "print('no registration')")
        self.write(DOC, "# doc")
        code, output = self.run_gate([AUDIT, DOC])
        self.assertEqual(code, 1)
        self.assertIn("CHECKER_ROWS", output)

    def test_checker_with_registration_passes(self):
        self.write(AUDIT, "CHECKER_ROWS = ('OVL_0068_0294',)\n")
        self.write(DOC, "# doc")
        code, _ = self.run_gate([AUDIT, DOC])
        self.assertEqual(code, 0)

    def test_gate_scripts_and_tests_are_not_checkers(self):
        """A tooling change (a gate or a test) needs no body evidence."""
        self.write("tools/overlay/check_evidence_docs.py", "x")
        self.write("tools/overlay/test_evidence_docs.py", "x")
        code, _ = self.run_gate(["tools/overlay/check_evidence_docs.py",
                                 "tools/overlay/test_evidence_docs.py"])
        self.assertEqual(code, 0)


if __name__ == "__main__":
    unittest.main()
