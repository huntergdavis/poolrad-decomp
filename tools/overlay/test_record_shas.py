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
        problems, checked, marked, unverifiable = guard.scan(root, {KNOWN})
        self.assertEqual(problems, [])
        self.assertEqual(checked, 1)
        self.assertEqual(marked, 0)

    def test_invented_commit_fails(self):
        root = self.tree(f"OVL_0099_9999,x.md,1102,{FAKE},{BODY},73\n")
        problems, _, _, _ = guard.scan(root, {KNOWN})
        self.assertEqual(len(problems), 1)
        self.assertIn(FAKE, problems[0])

    def test_external_marker_allows_unknown(self):
        root = self.tree(f"# external upstream id {FAKE}\n")
        problems, checked, marked, unverifiable = guard.scan(root, {KNOWN})
        self.assertEqual(problems, [])
        self.assertEqual((checked, marked), (1, 1))

    def test_body_digest_is_not_treated_as_commit(self):
        root = self.tree(f"OVL_0038_25BC,x.md,1102,{KNOWN},{BODY},73\n")
        _, checked, _, _ = guard.scan(root, {KNOWN})
        self.assertEqual(checked, 1, "only the 40-hex commit is a provenance id")

    # -- shallow clones (t675) ------------------------------------------------

    def test_an_id_absent_from_a_shallow_clone_is_not_a_finding(self):
        # The CI failure four body PRs generalised into "fails at the base": the
        # object is simply not in a --depth clone, and this guard cannot say whether
        # it is real.
        root = self.tree(f"OVL_0038_25BC,x.md,1102,{FAKE},{BODY},73\n")
        problems, checked, marked, unverifiable = guard.scan(
            root, {KNOWN}, shallow=True, exists=lambda sha: False)
        self.assertEqual([], problems, "a shallow clone must not fail the gate")
        self.assertEqual((checked, marked), (1, 0))
        self.assertEqual(1, len(unverifiable))
        self.assertIn(FAKE, unverifiable[0])

    def test_a_full_clone_still_fails_on_the_same_id(self):
        root = self.tree(f"OVL_0038_25BC,x.md,1102,{FAKE},{BODY},73\n")
        problems, _checked, _marked, unverifiable = guard.scan(
            root, {KNOWN}, shallow=False, exists=lambda sha: False)
        self.assertEqual(len(problems), 1, "an invented id must still fail a full clone")
        self.assertIn(FAKE, problems[0])
        self.assertEqual([], unverifiable)

    def test_a_shallow_clone_still_fails_an_id_whose_object_is_present(self):
        # Not in --all's history but present in the object store: that IS a finding.
        root = self.tree(f"OVL_0038_25BC,x.md,1102,{FAKE},{BODY},73\n")
        problems, _checked, _marked, _unverifiable = guard.scan(
            root, {KNOWN}, shallow=True, exists=lambda sha: True)
        self.assertEqual(1, len(problems))

    def test_the_summary_says_which_history_it_read(self):
        root = self.tree(f"OVL_0038_25BC,x.md,1102,{FAKE},{BODY},73\n")
        original_commits, original_shallow = guard.commits, guard.is_shallow
        guard.commits, guard.is_shallow = (lambda _root: {KNOWN}), (lambda _root: True)
        try:
            self.assertEqual(0, guard.main(["--root", str(root)]),
                             "a shallow clone reports, it does not fail")
        finally:
            guard.commits, guard.is_shallow = original_commits, original_shallow


if __name__ == "__main__":
    unittest.main()
