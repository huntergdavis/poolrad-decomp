"""The body/checker pairing gate must refuse an unpaired body.

The failure it prevents already happened once: an untested model was committed
into a reviewable PR by `git add -A`, and only a human going to do something else
caught it. These tests drive the refusal from synthetic change sets, which is the
same shape as the real diff but does not need a git repository.
"""
import contextlib
import io
import subprocess
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


# The shape of tonight's #1477 refusal: a one-line change to a body that is already
# paired on main (a docs/re note names its stem), with no new document.
PAIRED = "src/recovered/overlay_u9e_entry16_10e4.c"
PAIRED_DOC = "docs/re/U9E_ENTRY16_10E4.md"
NOTE = "Evidence-note: overlay_u9e_entry16_10e4: XOR AH,AH writes CF := 0 explicitly; the differential is unchanged\n"
CHECKER = "tools/overlay/u6c_record_roll_sum_4235_audit.py"
CHECKER_DOC = "docs/re/U6C_4235.md"


class EvidenceNoteTest(unittest.TestCase):
    """An edit to an already-paired body or checker may land on a commit-message note;
    a new body or checker, a registration change or a credit table never may."""

    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)

    def write(self, path, text="x"):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)

    def run_gate(self, changed, base_files, messages=""):
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            code = gate.report(changed, str(self.root), gate.Base(base_files, messages))
        lines = buffer.getvalue().splitlines()
        return code, [l for l in lines if l.startswith("UNPAIRED")], [l for l in lines if l.startswith(("EVIDENCE-NOTE", "OK"))]

    def paired_body_base(self):
        return {PAIRED: "old body", PAIRED_DOC: "# 10E4\nmodel: overlay_u9e_entry16_10e4 (OVL_009E_10E4)\n"}

    def test_paired_body_edit_without_a_note_is_refused_exactly(self):
        self.write(PAIRED, "new body")
        code, unpaired, _ = self.run_gate([PAIRED], self.paired_body_base())
        self.assertEqual(code, 1)
        self.assertEqual(unpaired, [
            "UNPAIRED BODY: no evidence document under docs/re/ is part of this change (bodies: 1, checkers: 0)",
            f"UNPAIRED BODY: Evidence-note path closed: {PAIRED} is paired on the base but no "
            "'Evidence-note: <id>: <why>' line in this change's commit messages names it "
            "(accepted ids: overlay_u9e_entry16_10e4)"])

    def test_paired_body_edit_with_a_note_passes_exactly(self):
        self.write(PAIRED, "new body")
        code, unpaired, ok = self.run_gate([PAIRED, "docs/re/FLAG_SEMANTICS_CENSUS.tsv"], self.paired_body_base(), "fix\n\n" + NOTE)
        self.assertEqual((code, unpaired), (0, []))
        self.assertEqual(ok, [
            f"EVIDENCE-NOTE: {PAIRED} - overlay_u9e_entry16_10e4: XOR AH,AH writes CF := 0 explicitly; the differential is unchanged",
            "OK: 2 changed path(s); every body/checker is paired with evidence and registration"])

    def test_body_note_may_name_the_row_its_document_names(self):
        self.write(PAIRED, "new body")
        code, unpaired, _ = self.run_gate([PAIRED], self.paired_body_base(), "Evidence-note: OVL_009E_10E4: explicit CF write, same behaviour\n")
        self.assertEqual((code, unpaired), (0, []))

    def test_a_note_naming_another_file_or_too_short_covers_nothing(self):
        self.write(PAIRED, "new body")
        for message in ("Evidence-note: overlay_u9e_entry17_110d: a different body entirely\n",
                        "Evidence-note: overlay_u9e_entry16_10e4: short\n",
                        "  Evidence-note: overlay_u9e_entry16_10e4: indented, so not a trailer line\n"):
            code, unpaired, _ = self.run_gate([PAIRED], self.paired_body_base(), message)
            self.assertEqual(code, 1, message)
            self.assertEqual(len(unpaired), 2, message)

    def test_new_body_is_refused_even_with_a_note(self):
        """The non-negotiable half: a new body needs its document in the same change."""
        self.write(BODY, "new")
        code, unpaired, _ = self.run_gate([BODY], {}, "Evidence-note: overlay_0068_0294_cpu: brand new body, notes cannot pair it\n")
        self.assertEqual(code, 1)
        self.assertEqual(unpaired[1:], [f"UNPAIRED BODY: Evidence-note path closed: {BODY} is a new body: it needs its evidence document in this change"])

    def test_new_checker_is_refused_even_with_a_note(self):
        self.write(AUDIT, "CHECKER_ROWS = ('OVL_0068_0294',)\n")
        code, unpaired, _ = self.run_gate([AUDIT], {}, "Evidence-note: OVL_0068_0294: brand new checker with rows\n")
        self.assertEqual(code, 1)
        self.assertEqual(unpaired[1:], [f"UNPAIRED BODY: Evidence-note path closed: {AUDIT} is a new checker: "
                                        "it needs its evidence document and CHECKER_ROWS in this change"])

    def paired_checker_base(self, rows="('OVL_006C_4235',)"):
        return {CHECKER: f"CHECKER_ROWS = {rows}\nold\n", CHECKER_DOC: "checker u6c_record_roll_sum_4235_audit.py for OVL_006C_4235\n"}

    def test_paired_checker_edit_with_a_row_note_passes(self):
        """#1437's shape: an import fix to a registered checker."""
        self.write(CHECKER, "CHECKER_ROWS = ('OVL_006C_4235',)\nfixed import\n")
        code, unpaired, ok = self.run_gate([CHECKER], self.paired_checker_base(), "Evidence-note: OVL_006C_4235: import the DX/AX/BX constants it reads\n")
        self.assertEqual((code, unpaired), (0, []))
        self.assertEqual(ok[0], f"EVIDENCE-NOTE: {CHECKER} - OVL_006C_4235: import the DX/AX/BX constants it reads")

    def test_adding_a_checker_row_is_refused_even_with_a_note(self):
        """#1452's shape: declaring CHECKER_ROWS is a registration change."""
        self.write(CHECKER, "CHECKER_ROWS = ('OVL_006C_4235', 'OVL_006C_4236')\n")
        code, unpaired, _ = self.run_gate([CHECKER], self.paired_checker_base(), "Evidence-note: OVL_006C_4235: also registers a second row here\n")
        self.assertEqual(code, 1)
        self.assertEqual(unpaired[1:], [f"UNPAIRED BODY: Evidence-note path closed: {CHECKER} changes CHECKER_ROWS "
                                        "(['OVL_006C_4235'] -> ['OVL_006C_4235', 'OVL_006C_4236']): a registration change needs its evidence document"])

    def test_checker_unregistered_on_the_base_is_refused(self):
        self.write(CHECKER, "CHECKER_ROWS = ()\nedit\n")
        code, unpaired, _ = self.run_gate([CHECKER], self.paired_checker_base("()"), "Evidence-note: u6c_record_roll_sum_4235_audit: an edit to an unregistered checker\n")
        self.assertEqual(code, 1)
        self.assertEqual(unpaired[1:], [f"UNPAIRED BODY: Evidence-note path closed: {CHECKER} is not paired on the base: it declares no CHECKER_ROWS there"])

    def test_body_no_base_document_names_is_refused(self):
        self.write(PAIRED, "new body")
        code, unpaired, _ = self.run_gate([PAIRED], {PAIRED: "old"}, NOTE)
        self.assertEqual(code, 1)
        self.assertEqual(unpaired[1:], [f"UNPAIRED BODY: Evidence-note path closed: {PAIRED} is not paired on the base: no docs/re/*.md there names it"])

    def test_credit_table_closes_the_note_path(self):
        self.write(PAIRED, "new body")
        for table in gate.CREDIT_TABLES:
            code, unpaired, _ = self.run_gate([PAIRED, table], self.paired_body_base(), NOTE)
            self.assertEqual(code, 1, table)
            self.assertEqual(unpaired[1:], [f"UNPAIRED BODY: Evidence-note path closed: {table} is a credit table; an Evidence-note never covers it"])

    def test_deleting_a_paired_body_is_refused_even_with_a_note(self):
        code, unpaired, _ = self.run_gate([PAIRED], self.paired_body_base(), NOTE)
        self.assertEqual(code, 1)
        self.assertEqual(unpaired[1:], [f"UNPAIRED BODY: Evidence-note path closed: {PAIRED} is deleted: a deletion needs its evidence document"])

    def test_one_unnoted_file_refuses_the_whole_change(self):
        self.write(PAIRED, "new body")
        self.write(CHECKER, "CHECKER_ROWS = ('OVL_006C_4235',)\nedit\n")
        base = {**self.paired_body_base(), **self.paired_checker_base()}
        code, unpaired, _ = self.run_gate([PAIRED, CHECKER], base, NOTE)
        self.assertEqual(code, 1)
        self.assertEqual(len(unpaired), 2)
        self.assertIn(CHECKER, unpaired[1])

    def test_a_document_in_the_change_still_pairs_without_notes(self):
        """The original path is unchanged: a body with a changed document passes, even touching a credit table."""
        self.write(PAIRED, "new body")
        code, unpaired, _ = self.run_gate([PAIRED, PAIRED_DOC, gate.CREDIT_TABLES[0]], self.paired_body_base())
        self.assertEqual((code, unpaired), (0, []))


class GitEndToEndTest(unittest.TestCase):
    """main() on a real repository: the merge-base side and the commit messages come from git."""

    def git(self, *args):
        subprocess.run(["git", "-C", str(self.root), "-c", "user.name=t", "-c", "user.email=t@example.invalid", *args],
                       check=True, capture_output=True)

    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.git("init", "-q", "-b", "main")
        for path, text in ((PAIRED, "old body\n"), (PAIRED_DOC, "model overlay_u9e_entry16_10e4\n")):
            (self.root / path).parent.mkdir(parents=True, exist_ok=True)
            (self.root / path).write_text(text)
        self.git("add", "-A")
        self.git("commit", "-q", "-m", "base")
        self.git("checkout", "-q", "-b", "topic")
        (self.root / PAIRED).write_text("new body\n")
        self.git("commit", "-q", "-am", "edit the body")

    def run_gate(self):
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            code = gate.main(["--base", "main", "--root", str(self.root)])
        return code, buffer.getvalue()

    def test_refused_then_passes_once_a_commit_carries_the_note(self):
        code, output = self.run_gate()
        self.assertEqual(code, 1)
        self.assertIn("REFUSED: 2 reason(s)", output)
        self.git("commit", "-q", "--allow-empty", "-m", "note\n\n" + NOTE)
        code, output = self.run_gate()
        self.assertEqual(code, 0)
        self.assertIn(f"EVIDENCE-NOTE: {PAIRED} - overlay_u9e_entry16_10e4:", output)

    def test_a_new_body_on_a_real_branch_is_refused_with_a_note(self):
        (self.root / BODY).parent.mkdir(parents=True, exist_ok=True)
        (self.root / BODY).write_text("new\n")
        self.git("add", BODY)
        self.git("commit", "-q", "-m", "new body\n\n" + NOTE + "Evidence-note: overlay_0068_0294_cpu: notes cannot pair a new body\n")
        code, output = self.run_gate()
        self.assertEqual(code, 1)
        self.assertIn(f"{BODY} is a new body", output)


if __name__ == "__main__":
    unittest.main()
