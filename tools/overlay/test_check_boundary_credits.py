"""The declared-boundary credit guard must refuse the exact injection that broke it.

The VP proved the hole by appending a boundary row to the credited CSV and
watching every gate stay green. These tests reproduce that injection on a
synthetic tree and require a refusal that names the row and the class.
"""
import contextlib
import csv
import io
import tempfile
import unittest
from pathlib import Path

import check_boundary_credits as gate
from strict_candidate_queue import DECLARED_BOUNDARY_ENDPOINTS

HEADER = ("id", "evidence_doc", "review_pr", "reviewed_commit",
          "body_sha256", "body_bytes")


class BoundaryCreditGuardTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        (self.root / "docs/re").mkdir(parents=True)
        for table in gate.CREDIT_TABLES:
            self.write(table, [])
        self.endpoint = sorted(DECLARED_BOUNDARY_ENDPOINTS)[0]

    def write(self, table, rows):
        with (self.root / table).open("w", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(HEADER)
            writer.writerows(rows)

    def run_gate(self):
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            code = gate.main(["--root", str(self.root)])
        return code, buffer.getvalue()

    def test_clean_tables_pass(self):
        self.write(gate.CREDIT_TABLES[0], [("OVL_0038_12B2", "docs/re/X.md")])
        self.assertEqual(gate.offences(self.root), [])
        code, output = self.run_gate()
        self.assertEqual(code, 0)
        self.assertIn("OK:", output)

    def test_the_exact_injection_is_refused(self):
        """Append a boundary endpoint to the credited table, as the VP did."""
        self.write(gate.CREDIT_TABLES[0],
                   [("OVL_0038_12B2", "docs/re/X.md"),
                    (self.endpoint, "docs/re/DECLARED_BOUNDARY.md")])
        found = gate.offences(self.root)
        self.assertEqual(found, [(gate.CREDIT_TABLES[0], self.endpoint)])
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            code = gate.main(["--root", str(self.root)])
        self.assertEqual(code, 1)
        output = buffer.getvalue()
        self.assertIn("DECLARED BOUNDARY CREDITED", output)
        self.assertIn(self.endpoint, output)

    def test_identity_table_is_guarded_too(self):
        self.write(gate.CREDIT_TABLES[1], [(self.endpoint, "docs/re/X.md")])
        code, output = self.run_gate()
        self.assertEqual(code, 1)
        self.assertIn("DECLARED BOUNDARY CREDITED", output)

    def test_missing_table_fails_closed(self):
        (self.root / gate.CREDIT_TABLES[1]).unlink()
        code, _ = self.run_gate()
        self.assertEqual(code, 1)


if __name__ == "__main__":
    unittest.main()
