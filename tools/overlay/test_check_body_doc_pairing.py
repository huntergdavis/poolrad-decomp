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

    GENERATOR = "tools/overlay/function_queue_audit.py"
    MARKER = "# NOT_A_ROW_CHECKER: queue generator; it produces the accounting pin\n"

    def test_marked_generator_is_exempt_and_reported(self):
        self.write(self.GENERATOR, "x = 1\n" + self.MARKER)
        self.write(DOC, "# doc")
        code, output = self.run_gate([self.GENERATOR, DOC])
        self.assertEqual(code, 0)
        self.assertIn("EXEMPT (not a row checker): " + self.GENERATOR, output)

    def test_marker_without_a_reason_exempts_nothing(self):
        self.write(self.GENERATOR, "# NOT_A_ROW_CHECKER:\n")
        self.write(DOC, "# doc")
        code, output = self.run_gate([self.GENERATOR, DOC])
        self.assertEqual(code, 1)
        self.assertIn("no changed checker declares CHECKER_ROWS", output)

    def test_marker_inside_a_string_exempts_nothing(self):
        self.write(self.GENERATOR, "note = '# NOT_A_ROW_CHECKER: queue generator here'\n")
        self.write(DOC, "# doc")
        code, _ = self.run_gate([self.GENERATOR, DOC])
        self.assertEqual(code, 1)

    def test_marker_and_checker_rows_together_are_refused(self):
        self.write(self.GENERATOR, self.MARKER + "CHECKER_ROWS = ('OVL_0001_0000',)\n")
        self.write(DOC, "# doc")
        code, output = self.run_gate([self.GENERATOR, DOC])
        self.assertEqual(code, 1)
        self.assertIn("a file is a row checker or it is not", output)

    def test_exempt_generator_does_not_cover_an_unpaired_checker(self):
        """A real checker in the same change still needs its own CHECKER_ROWS."""
        self.write(self.GENERATOR, self.MARKER)
        self.write(AUDIT, "print('no registration')")
        self.write(DOC, "# doc")
        code, output = self.run_gate([self.GENERATOR, AUDIT, DOC])
        self.assertEqual(code, 1)
        self.assertIn(AUDIT, output)
        self.assertNotIn(self.GENERATOR + ",", output)


if __name__ == "__main__":
    unittest.main()
