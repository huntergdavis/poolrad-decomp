import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import dashboard


class DashboardTest(unittest.TestCase):
    def test_queue_keeps_identity_separate_from_full_c(self):
        entries, counts = dashboard.queue(dashboard.DEFAULT_REPO)
        self.assertEqual(len(entries), counts["overlay_candidates"] + counts["resident_ledger_rows"])
        self.assertEqual(sum(x["identity_verified"] for x in entries if x["domain"] == "overlay"),
                         counts["overlay_verified_identities"])
        self.assertEqual(sum(x["identity_verified"] for x in entries if x["domain"] == "resident"),
                         counts["resident_verified_identities"])
        self.assertTrue(all(x["full_c"] == "unlinked" for x in entries))

    def test_indexed_diff_counts_and_rejects_wrong_length(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            dos, portable = base / "dos.idx", base / "c.idx"
            dos.write_bytes(bytes([0, 1, 2, 3]))
            portable.write_bytes(bytes([0, 1, 9, 3]))
            with patch.object(dashboard, "STATE", base):
                report = dashboard.compare(dos, portable, 2, 2, "synthetic")
                self.assertEqual((report["matched"], report["mismatched"]), (3, 1))
                self.assertNotIn("full_frame_claim", report)
                with self.assertRaises(ValueError):
                    dashboard.compare(dos, portable, 3, 2, "bad length")

    def test_review_tag_is_local_and_requires_known_function(self):
        with tempfile.TemporaryDirectory() as temp:
            with patch.object(dashboard, "STATE", Path(temp)):
                item = dashboard.mark(dashboard.DEFAULT_REPO, "0x17:0x0000",
                                      "needs-evidence", "inspect caller")
                self.assertEqual(item["status"], "needs-evidence")
                self.assertEqual(dashboard.read_json(Path(temp) / "queue_marks.json")
                                 ["0x17:0x0000"]["note"], "inspect caller")
                with self.assertRaises(ValueError):
                    dashboard.mark(dashboard.DEFAULT_REPO, "missing", "watch", "")

    def test_published_evidence_links_resolve_from_dashboard(self):
        page = dashboard.render(dashboard.DEFAULT_REPO)
        body = page.read_text(encoding="utf-8")
        self.assertIn("'state/reports/'+x.source.split('/').pop()", body)
        for evidence in dashboard.cited_evidence(dashboard.DEFAULT_REPO):
            target = page.parent / "state/reports" / Path(evidence["source"]).name
            self.assertEqual(target.read_bytes(),
                             (dashboard.DEFAULT_REPO / evidence["source"]).read_bytes())


if __name__ == "__main__":
    unittest.main()
