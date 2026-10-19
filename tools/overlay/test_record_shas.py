#!/usr/bin/env python3
"""The SHA-provenance guard must reject an invented commit id.

The failure mode it exists for: a 40-hex id that looks plausible but names no
commit. Shape validation cannot see that; ``git cat-file`` can.
"""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_record_shas as guard

KNOWN = "95b5cfe359e982c6644a726ad60bd9628b89bb46"
FAKE = "95b5cfe3f8e2f6f4f9d1b7f2f1a2c3d4e5f60718"
BODY = "4921365d3d512d870ac24085a63d408b38ab60e6f98e4136f583a7c5c78fe753"


class RecordShaGuardTest(unittest.TestCase):
    def tree(self, rows):
        tmp = tempfile.TemporaryDirectory()
        root = Path(tmp.name)
        (root / "docs/re").mkdir(parents=True)
        (root / "docs/re/reviewed_function_identities.csv").write_text(
            "id,evidence_doc,review_pr,reviewed_commit,body_sha256,bytes\n" + rows)
        self.addCleanup(tmp.cleanup)
        return root

    def test_real_commit_passes(self):
        root = self.tree(f"OVL_0038_25BC,x.md,1102,{KNOWN},{BODY},73\n")
        problems, checked, marked = guard.scan(root, {KNOWN})
        self.assertEqual(problems, [])
        self.assertEqual(checked, 1)
        self.assertEqual(marked, 0)

    def test_invented_commit_fails(self):
        root = self.tree(f"OVL_0099_9999,x.md,1102,{FAKE},{BODY},73\n")
        problems, _, _ = guard.scan(root, {KNOWN})
        self.assertEqual(len(problems), 1)
        self.assertIn(FAKE, problems[0])

    def test_external_marker_allows_unknown(self):
        root = self.tree(f"# external upstream id {FAKE}\n")
        problems, checked, marked = guard.scan(root, {KNOWN})
        self.assertEqual(problems, [])
        self.assertEqual((checked, marked), (1, 1))

    def test_body_digest_is_not_treated_as_commit(self):
        root = self.tree(f"OVL_0038_25BC,x.md,1102,{KNOWN},{BODY},73\n")
        _, checked, _ = guard.scan(root, {KNOWN})
        self.assertEqual(checked, 1, "only the 40-hex commit is a provenance id")


if __name__ == "__main__":
    unittest.main()
