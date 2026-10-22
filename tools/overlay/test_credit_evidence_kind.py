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
        self.table = list(csv.DictReader((ROOT / cek.TABLE).read_text().splitlines()))
        self.strict = [r["id"] for r in csv.DictReader((ROOT / cek.STRICT).read_text().splitlines())]

    def test_every_credit_has_exactly_one_row(self):
        ids = [r["id"] for r in self.table]
        self.assertEqual(len(ids), len(set(ids)), "duplicate ids")
        self.assertEqual(set(ids), set(self.strict))

    def test_labels_are_a_closed_set(self):
        self.assertTrue({r["kind"] for r in self.table} <= set(cek.KINDS))
        self.assertEqual(tuple(csv.DictReader((ROOT / cek.TABLE).read_text().splitlines()).fieldnames), cek.FIELDS)

    def test_checker_equals_ci_registry(self):
        """'checker' is exactly the credits CI's check_checker_builds registry names, in both folders."""
        from check_checker_builds import discover
        checker = {r["id"] for r in self.table if r["kind"] == "checker"}
        self.assertEqual(checker, set(discover(ROOT)) & set(self.strict))
        # #1411 regression: these two are registered only from tests/recovered.
        self.assertTrue({"OVL_0038_12B2", "OVL_0038_25BC"} <= checker)

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
            ids = ("0010", "0020", "0030", "0040", "0050", "0060", "0070", "0080", "0090")
            (root / cek.STRICT).write_text("id,evidence_doc\n" + "".join(
                f"OVL_0019_{i},docs/re/D{i}.md\n" for i in ids) + "OVL_0019_00A0,docs/re/gone.md\n")
            (root / "CMakeLists.txt").write_text("add_test(NAME poolrad_u19_four_tests COMMAND x)\n")
            (root / "tools/overlay/u19_one_0010_audit.py").write_text("CHECKER_ROWS = ('OVL_0019_0010',)\n")
            (root / "tests/recovered/check_u19_seven.py").write_text("CHECKER_ROWS = ('OVL_0019_0070',)\n")
            (root / "tools/overlay/u19_two_0020_audit.py").write_text("# audits 0x20 in unit 0x19\n")
            (root / "tools/overlay/callgraph.py").write_text("# shared tool\n")
            docs = {"0010": "python3 tools/overlay/u19_one_0010_audit.py",
                    "0020": "python3 tools/overlay/u19_two_0020_audit.py --unicorn",
                    "0030": "python3 tools/overlay/callgraph.py",
                    "0040": "ctest --test-dir build -R '^poolrad_u19_four_tests$' --output-on-failure",
                    "0050": "prose only; tests/recovered/test_overlay_u19_five_0050.c exists but is not cited",
                    "0060": "python3 tools/overlay/gone.py",
                    "0070": "prose only",
                    "0080": "ctest --test-dir build -R '^poolrad_u19_unregistered_tests$'",
                    "0090": "python3 tools/overlay/callgraph.py; ctest --test-dir build -R poolrad_u19_four_tests"}
            for i, text in docs.items():
                (root / f"docs/re/D{i}.md").write_text(text + "\n")
            kinds = {rid: kind for rid, kind, _ in cek.classify(root)}
        self.assertEqual(kinds, {"OVL_0019_0010": "checker", "OVL_0019_0020": "audit",
                                 "OVL_0019_0030": "cites-tool", "OVL_0019_0040": "unit-test",
                                 "OVL_0019_0050": "prose-only", "OVL_0019_0060": "stale",
                                 "OVL_0019_0070": "checker", "OVL_0019_0080": "stale",
                                 "OVL_0019_0090": "unit-test", "OVL_0019_00A0": "missing-doc"})


if __name__ == "__main__":
    unittest.main()
