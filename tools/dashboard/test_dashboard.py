import tempfile
import unittest
import json
from pathlib import Path
from unittest.mock import patch

import dashboard


class DashboardTest(unittest.TestCase):
    def test_function_progress_waits_for_reviewed_census(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            folder = repo / "docs/re"
            folder.mkdir(parents=True)
            checkpoint = dashboard.read_json(dashboard.DEFAULT_REPO /
                                             "docs/re/function_accounting_checkpoint.json")
            (folder / "function_accounting_checkpoint.json").write_text(
                json.dumps(checkpoint), encoding="utf-8")
            (folder / "function_ceiling_census.json").write_text(
                json.dumps({"status": "pending"}), encoding="utf-8")
            progress = dashboard.function_progress(repo)
            self.assertEqual(progress["status"], "pending")
            self.assertIsNone(progress["strict_percent"])
            self.assertIsNone(progress["identity_percent"])

    def test_function_progress_distinguishes_exact_from_ceiling(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            evidence = repo / "docs/re/CEILING_EVIDENCE.md"
            evidence.parent.mkdir(parents=True)
            evidence.write_text("Test evidence", encoding="utf-8")
            source = dashboard.read_json(dashboard.DEFAULT_REPO /
                                         "docs/re/function_accounting_checkpoint.json")
            (repo / "docs/re/function_accounting_checkpoint.json").write_text(
                json.dumps(source), encoding="utf-8")
            census = {
                "status": "reviewed", "count": 1000, "bound_kind": "upper_bound",
                "scope": "original-game-functions-overlay-and-resident",
                "source_digests": source["inputs"], "reviewed_by": "PM",
                "evidence_doc": "docs/re/CEILING_EVIDENCE.md",
            }
            path = repo / "docs/re/function_ceiling_census.json"
            path.write_text(json.dumps(census), encoding="utf-8")
            progress = dashboard.function_progress(repo)
            self.assertEqual((progress["strict_percent"], progress["identity_percent"]),
                             (8.2, 14.3))
            self.assertEqual(progress["bound_kind"], "upper_bound")
            census["bound_kind"] = "exact"
            path.write_text(json.dumps(census), encoding="utf-8")
            self.assertEqual(dashboard.function_progress(repo)["bound_kind"], "exact")
            census["bound_kind"] = "upper_bound"
            census["count"] = 815
            path.write_text(json.dumps(census), encoding="utf-8")
            self.assertEqual(dashboard.function_progress(repo)["strict_percent"], 10.0)
            census["source_digests"] = {"game_ovr_sha256": "wrong"}
            path.write_text(json.dumps(census), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "different original binaries"):
                dashboard.function_progress(repo)

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
        self.assertIn("POOLRAD // PORT PROGRESS", body)
        self.assertNotIn("<table", body)
        self.assertNotIn('id="queue"', body)
        snapshot = dashboard.read_json(dashboard.STATE / "snapshot.json")
        self.assertTrue(snapshot["queue"])
        self.assertIn("'state/reports/'+x.source.split('/').pop()", body)
        for evidence in dashboard.cited_evidence(dashboard.DEFAULT_REPO):
            target = page.parent / "state/reports" / Path(evidence["source"]).name
            self.assertEqual(target.read_bytes(),
                             (dashboard.DEFAULT_REPO / evidence["source"]).read_bytes())


if __name__ == "__main__":
    unittest.main()
