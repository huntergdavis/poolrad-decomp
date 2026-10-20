"""Failure paths for the evidence-link gate, on a synthetic record tree."""
import csv
from pathlib import Path
import tempfile
import unittest

import check_evidence_docs as gate


class EvidenceGateTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        (self.root / "docs/re").mkdir(parents=True)
        for record in gate.RECORDS:
            self.write_record(record, [])
        self.doc("docs/re/BODY.md", "this document models the routine +0x0295 here")

    def write_record(self, record, rows):
        path = self.root / record
        with path.open("w", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(("id", "evidence_doc"))
            writer.writerows(rows)

    def doc(self, relative, text):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    def test_clean_record_passes(self):
        self.write_record(gate.RECORDS[0], [("OVL_0068_0295", "docs/re/BODY.md")])
        self.assertEqual(gate.problems(self.root), [])

    def test_every_offset_form_is_accepted(self):
        for form in ("OVL_0068_0295", "0x68:0x0295", "+0x0295", "+0295",
                     "0x0295", "104:0295"):
            self.doc("docs/re/BODY.md", f"routine {form} documented")
            self.write_record(gate.RECORDS[0], [("OVL_0068_0295", "docs/re/BODY.md")])
            self.assertEqual(gate.problems(self.root), [], form)

    def test_missing_document_blocks(self):
        self.write_record(gate.RECORDS[1],
                          [("OVL_0068_0295", "docs/re/ABSENT.md")])
        found = gate.problems(self.root)
        self.assertEqual(len(found), 1)
        self.assertIn("names a missing document", found[0])

    def test_document_that_never_names_the_row_blocks(self):
        self.doc("docs/re/OTHER.md", "this document is about routine +0x0AAA")
        self.write_record(gate.RECORDS[0], [("OVL_0068_0295", "docs/re/OTHER.md")])
        found = gate.problems(self.root)
        self.assertEqual(len(found), 1)
        self.assertIn("is not named anywhere", found[0])

    def test_missing_record_blocks(self):
        (self.root / gate.RECORDS[2]).unlink()
        found = gate.problems(self.root)
        self.assertTrue(any("record is missing" in line for line in found), found)

    def test_resident_row_forms(self):
        self.doc("docs/re/RES.md", "resident 0464:2595 releases the bitmap")
        self.write_record(gate.RECORDS[2],
                          [("RES_0464_2595", "docs/re/RES.md")])
        self.assertEqual(gate.problems(self.root), [])

    def test_row_without_document_field_blocks(self):
        path = self.root / gate.RECORDS[0]
        with path.open("w", newline="") as handle:
            csv.writer(handle).writerow(("id", "evidence_doc"))
            csv.writer(handle).writerow(("OVL_0068_0295", ""))
        found = gate.problems(self.root)
        self.assertTrue(any("without id or evidence_doc" in line for line in found), found)

    def test_cli_exit_codes(self):
        self.write_record(gate.RECORDS[0], [("OVL_0068_0295", "docs/re/BODY.md")])
        self.assertEqual(gate.main(["--root", str(self.root), "--quiet"]), 0)
        (self.root / "docs/re/BODY.md").unlink()
        self.assertEqual(gate.main(["--root", str(self.root), "--quiet"]), 1)


if __name__ == "__main__":
    unittest.main()
