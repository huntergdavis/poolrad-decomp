#!/usr/bin/env python3
"""Synthetic approved-change, stale-count and presentation-scope tests."""

import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import function_accounting_refresh as ar
from classify_overlay import load_reviewed_identities
from function_alias_audit import check_checkpoint


def summarized(rows):
    return dict(rows=rows, inputs=ar.DIGESTS.copy(), artifacts_sha256={}, summary=dict(
        strict_full_c_originals=sum(r["strict_full_c"] for r in rows),
        ownership={"game_verified": sum(r["game_or_library"] == "game_verified" for r in rows)},
        whole_game_denominator="unknown", completion_percentage=None))


def fixture():
    before = summarized([
        dict(id="OVL_0001_0010", original_address="0x1:0x0010", aliases=["STUB_0003_0020"],
             domain="overlay", strict_full_c=1, game_or_library="game_verified", classification="game_verified"),
        dict(id="OVL_0001_0020", original_address="0x1:0x0020", aliases=["STUB_0003_0025"],
             domain="overlay", strict_full_c=0, game_or_library="unknown", classification="game_evidenced")])
    credits = [dict(id="OVL_0001_0010", evidence_doc="docs/re/old.md")]
    approval = dict(status="approved", approved_by="PM", review_pr=11,
                    reviewed_commit="a" * 40, original_id="0x1:0x0020", evidence_doc="docs/re/new.md",
                    body_sha256="b" * 64, body_bytes=5, game_identity=True, strict_full_c=True,
                    native_reachability="controlled synthetic input only")
    return before, credits, approval


def promoted(before, plan):
    rows = copy.deepcopy(before["rows"])
    for row in rows:
        if row["id"] == plan["approval"]["original_id"]:
            row["strict_full_c"] += plan["strict_delta"]
            row["game_or_library"] = "game_verified"
            row["classification"] = "game_verified"
    return summarized(rows)


class AccountingRefreshTest(unittest.TestCase):
    def test_exact_one_original_addition_and_alias_normalization(self):
        before, credits, approval = fixture()
        plan = ar.plan_change(before, credits, [], approval)
        self.assertEqual(plan["approval"]["original_id"], "OVL_0001_0020")
        self.assertEqual(ar.verify_transition(before, promoted(before, plan), plan), (2, 2))
        self.assertEqual(len(plan["credits"]), 2)
        self.assertEqual(plan["identities"][0]["reviewed_commit"], "a" * 40)

    def test_repeating_approved_credit_is_rejected(self):
        before, credits, approval = fixture()
        plan = ar.plan_change(before, credits, [], approval)
        with self.assertRaisesRegex(ValueError, "already has strict"):
            ar.plan_change(promoted(before, plan), plan["credits"], plan["identities"], approval)

    def test_duplicate_existing_credit_through_alias_is_rejected(self):
        before, credits, approval = fixture()
        credits.append(dict(id="STUB_0003_0020", evidence_doc="docs/re/old.md"))
        with self.assertRaisesRegex(ValueError, "duplicate existing strict credit"):
            ar.plan_change(before, credits, [], approval)

    def test_missing_approval_bad_head_and_nonboolean_decisions_fail(self):
        for changes in (dict(status="pending"), dict(approved_by="worker"),
                        dict(reviewed_commit="not-exact"), dict(strict_full_c="yes"), dict(body_sha256="bad"),
                        dict(review_pr=True), dict(body_bytes=True)):
            before, credits, approval = fixture()
            approval.update(changes)
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                ar.plan_change(before, credits, [], approval)

    def test_identity_only_does_not_gain_strict_credit(self):
        before, credits, approval = fixture()
        approval["strict_full_c"] = False
        plan = ar.plan_change(before, credits, [], approval)
        self.assertEqual(ar.verify_transition(before, promoted(before, plan), plan), (1, 2))

    def test_strict_only_does_not_duplicate_verified_identity(self):
        before, credits, approval = fixture()
        before["rows"][1]["game_or_library"] = "game_verified"
        before = summarized(before["rows"])
        approval["game_identity"] = False
        plan = ar.plan_change(before, credits, [], approval)
        self.assertEqual(ar.verify_transition(before, promoted(before, plan), plan), (2, 2))
        self.assertEqual(plan["identities"], [])

    def test_unapproved_other_original_or_stale_counts_fail(self):
        for mutate in (lambda q: q["rows"][0].update(strict_full_c=0),
                       lambda q: q["rows"][0].update(game_or_library="unknown"),
                       lambda q: q["summary"].update(strict_full_c_originals=1),
                       lambda q: q["summary"].update(whole_game_denominator="known")):
            before, credits, approval = fixture()
            plan = ar.plan_change(before, credits, [], approval)
            after = promoted(before, plan)
            mutate(after)
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                ar.verify_transition(before, after, plan)

    def test_resident_and_library_promotions_fail_explicitly(self):
        before, credits, approval = fixture()
        before["rows"][1]["domain"] = "resident"
        with self.assertRaisesRegex(ValueError, "resident changes"):
            ar.plan_change(before, credits, [], approval)
        before["rows"][1].update(domain="overlay", game_or_library="library_inferred")
        with self.assertRaisesRegex(ValueError, "ownership conflict"):
            ar.plan_change(before, credits, [], approval)

    def test_machine_checkpoint_replaces_fixed_count_gate(self):
        before, credits, approval = fixture()
        plan = ar.plan_change(before, credits, [], approval)
        after = promoted(before, plan)
        checkpoint = ar.make_checkpoint(after, json.dumps(after).encode(), approval)
        summary = dict(strict_full_c_originals=2, verified_game_identities=2)
        check_checkpoint(summary, checkpoint["strict_full_c_originals"], checkpoint["verified_game_identities"])
        with self.assertRaises(ValueError):
            check_checkpoint(summary, 1, 1)

    def test_card_updates_accounting_and_preserves_pixel_scope(self):
        before, credits, approval = fixture()
        plan = ar.plan_change(before, credits, [], approval)
        after = promoted(before, plan)
        checkpoint = ar.make_checkpoint(after, json.dumps(after).encode(), approval)
        original = f"prefix\n{ar.BEGIN}\nold accounting\n{ar.END}\n128 of 64,000 pixels\n"
        rendered = ar.render_battle_card(original, checkpoint)
        self.assertIn("**2 strict full-C game routines**", rendered)
        self.assertIn("128 of 64,000 pixels", rendered)
        self.assertNotIn("old accounting", rendered)

    def test_dashboard_accounting_preserves_replay_observations(self):
        before, credits, approval = fixture()
        plan = ar.plan_change(before, credits, [], approval)
        after = promoted(before, plan)
        checkpoint = ar.make_checkpoint(after, json.dumps(after).encode(), approval)
        snapshot = dict(queue=[dict(id=r["original_address"], queue_state="triage", full_c="unlinked")
                               for r in before["rows"]],
                        summary=dict(overlay_candidates=2), published_comparisons=[dict(compared=128)], commit="old replay")
        refreshed = ar.refresh_dashboard(snapshot, after, [], checkpoint)
        self.assertEqual(refreshed["strict_full_c_checkpoint"]["count"], 2)
        self.assertTrue(refreshed["queue"][1]["identity_verified"])
        self.assertEqual(refreshed["published_comparisons"], snapshot["published_comparisons"])
        self.assertEqual(refreshed["commit"], "old replay")
        self.assertEqual(snapshot["queue"][1]["full_c"], "unlinked")

    def test_reviewed_registry_rejects_duplicate_identities(self):
        before, credits, approval = fixture()
        plan = ar.plan_change(before, credits, [], approval)
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            doc = root / "docs/re/new.md"
            doc.parent.mkdir(parents=True)
            doc.write_text("synthetic evidence")
            manifest = root / "identities.csv"
            manifest.write_bytes(ar.csv_bytes(plan["identities"], ar.IDENTITY_FIELDS))
            self.assertEqual(load_reviewed_identities(manifest, root), {(1, 0x20)})
            manifest.write_bytes(ar.csv_bytes(plan["identities"] * 2, ar.IDENTITY_FIELDS))
            with self.assertRaisesRegex(ValueError, "duplicate reviewed"):
                load_reviewed_identities(manifest, root)


if __name__ == "__main__":
    unittest.main()
