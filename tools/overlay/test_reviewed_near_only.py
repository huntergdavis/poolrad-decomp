#!/usr/bin/env python3
"""Reviewed RES_ names for near-only resident bodies (t30, VP i20369): a naming change, never an identity grant.

Synthetic tests need no media; the audit tests run on the owner media and skip without it.
"""

import copy
import csv
import sys
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import function_queue as fq
import resident_callable_coverage as rc
import resident_inventory as ri
import strict_candidate_queue as scq
from test_function_queue import fixture, resident_row

ROOT = Path(__file__).resolve().parents[2]
EXE, OVR = ROOT / "resources/DOS/POOLRAD/START.EXE", ROOT / "resources/DOS/POOLRAD/GAME.OVR"
SIX_0802 = {"RES_0802_0040", "RES_0802_0105", "RES_0802_0193", "RES_0802_050F", "RES_0802_05CD", "RES_0802_06E7"}
EIGHT_00BA = {f"RES_00BA_{o}" for o in ("0000", "01FB", "0282", "03A0", "10E6", "11EA", "1282", "1325")}
NINE_NEAR_ONLY = SIX_0802 | EIGHT_00BA                      # fourteen entries (name kept from the first six-entry change)


def renamed_audit(classification="unresolved"):
    row = resident_row("RES_0802_0040", 0x8060, classification, far=0)
    row["discovery_id"] = "RPRO_08060"
    return dict(inputs=fq.DIGESTS.copy(), rows=[row, resident_row("RPRO_08125", 0x8125, far=0)], runtime_rows=[])


class QueueNamingTest(unittest.TestCase):
    def queue(self, audit, credits=()):
        return fq.export_queue(*fixture(), list(credits), audit)

    def test_a_reviewed_name_adds_an_alias_and_no_row(self):
        plain = dict(renamed_audit())
        plain["rows"] = [dict(r) for r in plain["rows"]]
        plain["rows"][0].pop("discovery_id"); plain["rows"][0]["id"] = "RPRO_08060"
        before, after = self.queue(plain), self.queue(renamed_audit())
        self.assertEqual(len(before["rows"]), len(after["rows"]))          # condition 2: the denominator is unchanged
        self.assertEqual(before["summary"], after["summary"])
        row = next(r for r in after["rows"] if r["id"] == "RES_IMAGE_08060")
        self.assertEqual(row["aliases"], ["RES_0802_0040", "RPRO_08060"])  # the discovery name survives
        self.assertEqual([r for r in before["rows"] if r["id"] != "RES_IMAGE_08060"],
                         [r for r in after["rows"] if r["id"] != "RES_IMAGE_08060"])

    def test_an_unreviewed_body_is_not_creditable_by_default(self):
        # Condition 3: the alias exists, ownership is still unresolved, so the credit is refused.
        with self.assertRaisesRegex(ValueError, "lacks verified game identity"):
            self.queue(renamed_audit(), [dict(id="RES_0802_0040", evidence_doc="docs/re/synthetic.md")])
        # An unrenamed raw-prologue body has no RES_ name to credit by at all.
        with self.assertRaisesRegex(ValueError, "absent from queue"):
            self.queue(renamed_audit(), [dict(id="RES_0802_0105", evidence_doc="docs/re/synthetic.md")])

    def test_a_decided_identity_is_creditable_once_and_by_either_name(self):
        audit = renamed_audit("game_verified")
        done = self.queue(audit, [dict(id="RES_0802_0040", evidence_doc="docs/re/synthetic.md")])
        self.assertEqual(sum(r["strict_full_c"] for r in done["rows"]), 1)
        with self.assertRaisesRegex(ValueError, "double-credit"):
            self.queue(audit, [dict(id=x, evidence_doc="docs/re/synthetic.md") for x in ("RES_0802_0040", "RPRO_08060")])

    def test_a_reused_discovery_name_is_refused(self):
        audit = renamed_audit()
        audit["rows"][1]["id"] = "RPRO_08060"
        with self.assertRaisesRegex(ValueError, "duplicate resident alias"):
            self.queue(audit)


class LedgerIdsTest(unittest.TestCase):
    def test_only_overlay_called_rows_are_overlay_called_entries(self):
        ids = scq.resident_ledger_ids(ROOT)
        rows = list(csv.DictReader((ROOT / "docs/re/resident_callable_ledger.csv").read_text().splitlines()))
        self.assertEqual(ids, {r["id"] for r in rows if r["kind"] == "overlay_called"})
        self.assertTrue(NINE_NEAR_ONLY.isdisjoint(ids))                      # near-only names are not overlay entries
        self.assertIn("RES_0802_0716", ids)


@unittest.skipUnless(EXE.is_file() and OVR.is_file(), "owner media absent")
class AuditTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.exe, cls.ovr = EXE.read_bytes(), OVR.read_bytes()
        cls.base = rc.audit_coverage(cls.exe, cls.ovr)

    def other(self, result=None):
        return {r["image_offset"]: r for r in (result or self.base)["rows"] if r["kind"] == "other_prologue"}

    def test_the_reviewed_list_is_exactly_the_fourteen_and_carries_arguments(self):
        self.assertEqual({f"RES_{s:04X}_{o:04X}" for s, o in rc.REVIEWED_NEAR_ONLY.values()}, NINE_NEAR_ONLY)
        image = bytes(rc.unpack(self.exe)[0])
        self.assertEqual(set(rc.reviewed_near_only(image).values()), NINE_NEAR_ONLY)

    def test_the_00ba_hashes_equal_the_115c_checker_pins(self):
        import re
        pins = re.findall(r"\('res',(0x[0-9a-f]+),(0x[0-9a-f]+),\d+,'([0-9a-f]{64})'\)",
                          (ROOT / "tools/overlay/u9e_character_pair_115c_audit.py").read_text())
        independent = {int(a, 16): (int(b, 16) - int(a, 16), h) for a, b, h in pins}
        text = (ROOT / "docs/re/REVIEWED_NEAR_ONLY_ALIAS.md").read_text()
        for name in sorted(EIGHT_00BA):
            row = re.search(rf"\| {name} \|[^\n]*\| ([0-9a-f]{{64}}) \| (\d+) \|", text)
            size, digest = independent[int(name[-4:], 16)]
            self.assertEqual((row.group(1), int(row.group(2))), (digest, size), name)

    def test_an_00ba_entry_needs_a_pass_verdict(self):
        image = bytes(rc.unpack(self.exe)[0])
        with mock.patch.object(rc, "VERDICTS", "docs/re/RES_0802_IDENTITY_ARGUMENTS.md"):
            with self.assertRaises(SystemExit) as caught:
                rc.reviewed_near_only(image)
        self.assertIn("no PASS verdict", str(caught.exception))

    def test_row_count_and_every_other_raw_prologue_are_unchanged(self):
        rows = self.other()
        self.assertEqual(len(self.base["rows"]), 145)
        named = {start for start, r in rows.items() if r["id"].startswith("RES_")}
        self.assertEqual(named, set(rc.REVIEWED_NEAR_ONLY))
        for start, row in rows.items():
            if start not in rc.REVIEWED_NEAR_ONLY:
                self.assertTrue(row["id"].startswith("RPRO_"), row["id"])    # condition 1: never a pattern over RPRO ids
                self.assertNotIn("discovery_id", row)
        for start in rc.REVIEWED_NEAR_ONLY:
            self.assertEqual(rows[start]["discovery_id"], "RPRO_%05X" % start)

    def test_the_alias_is_not_an_identity_grant(self):
        # Condition 4: with none of the six in GAME_SPECIFIC_TARGETS they stay unresolved, as before the change.
        self.assertTrue(set(rc.REVIEWED_NEAR_ONLY.values()).isdisjoint(ri.GAME_SPECIFIC_TARGETS))
        for start in rc.REVIEWED_NEAR_ONLY:
            self.assertEqual(self.other()[start]["classification"], "unresolved")
        self.assertEqual(self.base["summary"]["game_verified"], 30)   # 29 + RES_0802_02EB (overlay-called, accepted in #1341)

    def test_ownership_comes_only_from_game_specific_targets(self):
        grant = ri.GAME_SPECIFIC_TARGETS | {(0x802, 0x0040)}
        with mock.patch.object(rc, "GAME_SPECIFIC_TARGETS", grant):
            changed = rc.audit_coverage(self.exe, self.ovr)
        before, after = self.other(), self.other(changed)
        self.assertEqual({s for s in before if before[s]["classification"] != after[s]["classification"]}, {0x8060})
        self.assertEqual(after[0x8060]["classification"], "game_verified")
        self.assertEqual(changed["summary"]["game_verified"], 31)
        self.assertEqual(changed["summary"]["unresolved"], self.base["summary"]["unresolved"] - 1)

    def test_a_listing_without_an_argument_or_with_a_wrong_body_is_refused(self):
        image = bytes(rc.unpack(self.exe)[0])
        with self.assertRaises(SystemExit) as caught:
            rc.reviewed_near_only(image, listing={0x8040: (0x802, 0x0020)})   # 0020 is side (ii): no identity row
        self.assertIn("no reviewed identity argument", str(caught.exception))
        with self.assertRaises(SystemExit) as caught:
            rc.reviewed_near_only(image, listing={0x8061: (0x802, 0x0040)})
        self.assertIn("body hash differs", str(caught.exception))
        listing = dict(rc.REVIEWED_NEAR_ONLY); listing[0x8736] = (0x802, 0x0716)   # overlay-called, not a raw prologue
        with mock.patch.object(rc, "REVIEWED_NEAR_ONLY", listing):
            with self.assertRaises(SystemExit):
                rc.audit_coverage(self.exe, self.ovr)


if __name__ == "__main__":
    unittest.main()
