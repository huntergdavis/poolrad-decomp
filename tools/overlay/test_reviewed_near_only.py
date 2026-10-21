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
TWO_0709 = {"RES_0709_06A7", "RES_0709_0714"}
NINE_NEAR_ONLY = SIX_0802 | EIGHT_00BA | TWO_0709           # the sixteen near-only entries (name kept from the first change)
LOADER_CALLED = {"RES_0709_00B7"}                              # the second door
ALL_NAMED = NINE_NEAR_ONLY | LOADER_CALLED


def renamed_audit(classification="unresolved"):
    row = resident_row("RES_0802_0040", 0x8060, classification, far=0)
    row["discovery_id"] = "RPRO_08060"
    return dict(inputs=fq.DIGESTS.copy(), rows=[row, resident_row("RPRO_08125", 0x8125, far=0)], runtime_rows=[])


# Reviewed near-only bodies granted game_verified by their acceptance (par-e). Exact set: each grant
# commit adds one, deliberately.
GRANTED_NEAR_ONLY = frozenset({(0xBA, 0x0000), (0xBA, 0x01FB), (0xBA, 0x0282), (0xBA, 0x03A0), (0xBA, 0x10E6), (0xBA, 0x11EA), (0xBA, 0x1282), (0xBA, 0x1325), (0x802, 0x0040), (0x802, 0x0105), (0x802, 0x0193), (0x802, 0x050F), (0x802, 0x05CD), (0x802, 0x06E7)})


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


class VerdictTest(unittest.TestCase):
    """verdict_of reads the LAST verdict whose first cell is the body; a mention elsewhere is not a verdict."""
    PAGE = "\n".join([
        "# Page", "", "| Body | Bytes |", "| `RES_0001_0001` (`RPRO_00001`) | 10 |",
        "## PASS: the nested bodies", "| `RES_0001_0001` (`RPRO_00001`) | 10 | role |",
        "## Pass 2", "| `RES_0001_0002` | 0001_0001 | **PASS.** role |", "| `RES_0001_0003` | **FAIL, factor 2.** constants |",
        "| `RES_0001_0004` | **BLOCKED.** needs a caller |", "| `RES_0001_0005` | not assessed |",
        "| `RES_0001_0006` | x | mentions `RES_0001_0002` and **FAIL** in its text |",
        "## Pass 3", "| `RES_0001_0004` | **PASS, with a limit.** |", "| `RES_0001_0002` | **FAIL.** reversed later |"])

    def test_verdicts(self):
        v = lambda n: rc.verdict_of(self.PAGE, n)
        self.assertEqual(v("RES_0001_0001"), "PASS")        # under a PASS heading, no bold needed
        self.assertEqual(v("RES_0001_0003"), "FAIL")
        self.assertEqual(v("RES_0001_0004"), "PASS")        # a later pass supersedes BLOCKED
        self.assertEqual(v("RES_0001_0002"), "FAIL")        # a later FAIL supersedes PASS; the mention in 0006's text is not a verdict
        self.assertIsNone(v("RES_0001_0009"))
        self.assertEqual(rc.verdict_of("| `RES_0001_0005` | not assessed |", "RES_0001_0005"), "NOT_ASSESSED")

    @unittest.skipUnless((ROOT / rc.VERDICTS).is_file(), "verdict page absent")
    def test_the_real_page_passes_every_listed_body_and_refuses_the_failed_ones(self):
        text = (ROOT / rc.VERDICTS).read_text()
        listed = [f"RES_{s:04X}_{o:04X}" for s, o in rc.REVIEWED_RESIDENTS.values() if s != 0x802]
        self.assertEqual({n: rc.verdict_of(text, n) for n in listed}, {n: "PASS" for n in listed})
        for failed in ("RES_0464_1509", "RES_0464_1484", "RES_0464_1596", "RES_0709_0000"):    # negative controls
            self.assertEqual(rc.verdict_of(text, failed), "FAIL", failed)
        self.assertEqual(rc.verdict_of(text, "RES_0464_007B"), "BLOCKED")


class LedgerIdsTest(unittest.TestCase):
    def test_only_overlay_called_rows_are_overlay_called_entries(self):
        ids = scq.resident_ledger_ids(ROOT)
        rows = list(csv.DictReader((ROOT / "docs/re/resident_callable_ledger.csv").read_text().splitlines()))
        self.assertEqual(ids, {r["id"] for r in rows if r["kind"] == "overlay_called"})
        self.assertTrue(ALL_NAMED.isdisjoint(ids))                           # neither door's names are overlay entries
        self.assertIn("RES_0802_0716", ids)


@unittest.skipUnless(EXE.is_file() and OVR.is_file(), "owner media absent")
class AuditTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.exe, cls.ovr = EXE.read_bytes(), OVR.read_bytes()
        cls.base = rc.audit_coverage(cls.exe, cls.ovr)

    def other(self, result=None):
        return {r["image_offset"]: r for r in (result or self.base)["rows"] if r["kind"] == "other_prologue"}

    def test_the_reviewed_list_is_exactly_the_sixteen_and_carries_arguments(self):
        self.assertEqual({f"RES_{s:04X}_{o:04X}" for s, o in rc.REVIEWED_NEAR_ONLY.values()}, NINE_NEAR_ONLY)
        image = bytes(rc.unpack(self.exe)[0])
        self.assertEqual(set(rc.reviewed_residents(image).values()), ALL_NAMED)
        self.assertEqual({f"RES_{s:04X}_{o:04X}" for s, o in rc.REVIEWED_LOADER_CALLED.values()}, LOADER_CALLED)

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

    def test_an_entry_outside_0802_needs_a_pass_verdict(self):
        image = bytes(rc.unpack(self.exe)[0])
        with mock.patch.object(rc, "VERDICTS", "docs/re/RES_0802_IDENTITY_ARGUMENTS.md"):
            with self.assertRaises(SystemExit) as caught:
                rc.reviewed_residents(image)
        self.assertIn("no PASS verdict", str(caught.exception))
        self.assertIn("last verdict: None", str(caught.exception))

    def test_row_count_and_every_other_raw_prologue_are_unchanged(self):
        rows = self.other()
        self.assertEqual(len(self.base["rows"]), 145)
        named = {start for start, r in rows.items() if r["id"].startswith("RES_")}
        self.assertEqual(named, set(rc.REVIEWED_RESIDENTS))
        for start, row in rows.items():
            if start not in rc.REVIEWED_RESIDENTS:
                self.assertTrue(row["id"].startswith("RPRO_"), row["id"])    # condition 1: never a pattern over RPRO ids
                self.assertNotIn("discovery_id", row)
        for start in rc.REVIEWED_RESIDENTS:
            self.assertEqual(rows[start]["discovery_id"], "RPRO_%05X" % start)

    def test_the_alias_is_not_an_identity_grant(self):
        # Condition 4: a listed body is game_verified exactly when its acceptance added it to GAME_SPECIFIC_TARGETS.
        # GRANTED_NEAR_ONLY is the exact set granted so far; each grant commit moves it (no grant comes from the name).
        self.assertEqual(set(rc.REVIEWED_RESIDENTS.values()) & ri.GAME_SPECIFIC_TARGETS, set(GRANTED_NEAR_ONLY))
        for start, target in rc.REVIEWED_RESIDENTS.items():
            want = "game_verified" if target in GRANTED_NEAR_ONLY else "unresolved"
            self.assertEqual(self.other()[start]["classification"], want)
        # 29 + RES_0802_02EB (overlay-called, accepted in #1341) + the granted near-only bodies
        self.assertEqual(self.base["summary"]["game_verified"], 30 + len(GRANTED_NEAR_ONLY))

    def test_ownership_comes_only_from_game_specific_targets(self):
        # Simulate one more grant: the first listed near-only body that is not yet granted (moves with the grants).
        start, target = next((s, t) for s, t in sorted(rc.REVIEWED_NEAR_ONLY.items()) if t not in ri.GAME_SPECIFIC_TARGETS)
        grant = ri.GAME_SPECIFIC_TARGETS | {target}
        with mock.patch.object(rc, "GAME_SPECIFIC_TARGETS", grant):
            changed = rc.audit_coverage(self.exe, self.ovr)
        before, after = self.other(), self.other(changed)
        self.assertEqual({s for s in before if before[s]["classification"] != after[s]["classification"]}, {start})
        self.assertEqual(after[start]["classification"], "game_verified")
        self.assertEqual(changed["summary"]["game_verified"], 31 + len(GRANTED_NEAR_ONLY))
        self.assertEqual(changed["summary"]["unresolved"], self.base["summary"]["unresolved"] - 1)

    def test_the_loader_called_door_is_one_entry_with_its_own_condition(self):
        rows = self.other()
        row = rows[0x7147]
        self.assertEqual((row["id"], row["discovery_id"], row["classification"]), ("RES_0709_00B7", "RPRO_07147", "unresolved"))
        self.assertGreater(row["loader_far_sites"], 0)               # reached by far calls relocated inside the image
        self.assertEqual(row["overlay_far_sites"], 0)                # and by no overlay
        # The conditions are per door: a near-only body cannot ride the loader-called list, nor the reverse.
        with mock.patch.object(rc, "REVIEWED_LOADER_CALLED", {**rc.REVIEWED_LOADER_CALLED, 0x8060: (0x802, 0x0040)}), \
                mock.patch.object(rc, "REVIEWED_RESIDENTS", {**rc.REVIEWED_RESIDENTS}):
            with self.assertRaises(SystemExit) as caught:
                rc.audit_coverage(self.exe, self.ovr)
        self.assertIn("is not loader-called", str(caught.exception))
        moved = dict(rc.REVIEWED_NEAR_ONLY); moved[0x7147] = (0x709, 0x00B7)
        with mock.patch.object(rc, "REVIEWED_NEAR_ONLY", moved), mock.patch.object(rc, "REVIEWED_LOADER_CALLED", {}), \
                mock.patch.object(rc, "REVIEWED_RESIDENTS", moved):
            with self.assertRaises(SystemExit) as caught:
                rc.audit_coverage(self.exe, self.ovr)
        self.assertIn("is not near-only", str(caught.exception))

    def test_the_00b7_hash_equals_its_evidence_document(self):
        import re
        evidence = (ROOT / "docs/re/RES_0709_RECT_FILL_00B7_CPU.md").read_text()
        theirs = re.search(r"\|0709:00B7\|186 / 89\|([0-9a-f]{64})\|", evidence).group(1)
        ours = re.search(r"\| RES_0709_00B7 \|[^\n]*\| ([0-9a-f]{64}) \| 186 \|",
                         (ROOT / "docs/re/REVIEWED_NEAR_ONLY_ALIAS.md").read_text()).group(1)
        self.assertEqual(ours, theirs)

    def test_a_listing_without_an_argument_or_with_a_wrong_body_is_refused(self):
        image = bytes(rc.unpack(self.exe)[0])
        with self.assertRaises(SystemExit) as caught:
            rc.reviewed_residents(image, listing={0x8040: (0x802, 0x0020)})   # 0020 is side (ii): no identity row
        self.assertIn("no reviewed identity argument", str(caught.exception))
        with self.assertRaises(SystemExit) as caught:
            rc.reviewed_residents(image, listing={0x8061: (0x802, 0x0040)})
        self.assertIn("body hash differs", str(caught.exception))
        listing = dict(rc.REVIEWED_RESIDENTS); listing[0x8736] = (0x802, 0x0716)   # overlay-called, not a raw prologue
        with mock.patch.object(rc, "REVIEWED_RESIDENTS", listing):
            with self.assertRaises(SystemExit):
                rc.audit_coverage(self.exe, self.ovr)


if __name__ == "__main__":
    unittest.main()
