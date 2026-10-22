#!/usr/bin/env python3
"""The evidence-kind table covers every credit exactly once, uses a closed label set, and cannot drift."""
import csv
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import credit_evidence_kind as cek  # noqa: E402

ROOT = cek.ROOT


class CreditEvidenceKindTest(unittest.TestCase):
    def setUp(self):
        self.table = list(csv.DictReader(open(ROOT / cek.TABLE)))
        self.strict = [r["id"] for r in csv.DictReader(open(ROOT / cek.STRICT))]

    def test_every_credit_has_exactly_one_row(self):
        ids = [r["id"] for r in self.table]
        self.assertEqual(len(ids), len(set(ids)), "duplicate ids")
        self.assertEqual(set(ids), set(self.strict))

    def test_labels_are_a_closed_set(self):
        self.assertTrue({r["kind"] for r in self.table} <= set(cek.KINDS))
        self.assertEqual(tuple(csv.DictReader(open(ROOT / cek.TABLE)).fieldnames), cek.FIELDS)

    def test_checker_equals_registered_credits(self):
        """'checker' is not an opinion: it is exactly the credits named in some CHECKER_ROWS."""
        checker = {r["id"] for r in self.table if r["kind"] == "checker"}
        self.assertEqual(checker, set(cek.registered(ROOT)) & set(self.strict))

    def test_no_stale_or_missing_evidence(self):
        bad = [(r["id"], r["kind"]) for r in self.table if r["kind"] in ("stale", "missing-doc")]
        self.assertEqual(bad, [])

    def test_committed_table_equals_a_fresh_classification(self):
        self.assertEqual((ROOT / cek.TABLE).read_text(), cek.render(cek.classify(ROOT)))

    def test_classifier_on_synthetic_tree(self):
        import tempfile
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            for d in ("docs/re", "tools/overlay", "tests/recovered"):
                (root / d).mkdir(parents=True)
            (root / cek.STRICT).write_text(
                "id,evidence_doc\nOVL_0019_0010,docs/re/A.md\nOVL_0019_0020,docs/re/B.md\n"
                "OVL_0019_0030,docs/re/C.md\nOVL_0019_0040,docs/re/D.md\nOVL_0019_0050,docs/re/E.md\n"
                "OVL_0019_0060,docs/re/F.md\n")
            (root / "tools/overlay/u19_one_0010_audit.py").write_text("CHECKER_ROWS = ('OVL_0019_0010',)\n")
            (root / "tools/overlay/u19_two_0020_audit.py").write_text("# audits 0x20 in unit 0x19\n")
            (root / "tools/overlay/callgraph.py").write_text("# shared tool\n")
            (root / "tests/recovered/test_overlay_u19_four_0040.c").write_text("")
            (root / "docs/re/A.md").write_text("python3 tools/overlay/u19_one_0010_audit.py\n")
            (root / "docs/re/B.md").write_text("python3 tools/overlay/u19_two_0020_audit.py --unicorn\n")
            (root / "docs/re/C.md").write_text("python3 tools/overlay/callgraph.py\n")
            (root / "docs/re/D.md").write_text("prose only\n")
            (root / "docs/re/E.md").write_text("prose only\n")
            (root / "docs/re/F.md").write_text("python3 tools/overlay/gone.py\n")
            kinds = {rid: kind for rid, kind, _ in cek.classify(root)}
        self.assertEqual(kinds, {"OVL_0019_0010": "checker", "OVL_0019_0020": "audit",
                                 "OVL_0019_0030": "cites-tool", "OVL_0019_0040": "unit-test",
                                 "OVL_0019_0050": "document", "OVL_0019_0060": "stale"})


if __name__ == "__main__":
    unittest.main()
