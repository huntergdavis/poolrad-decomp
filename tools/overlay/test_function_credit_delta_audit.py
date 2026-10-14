#!/usr/bin/env python3
"""Synthetic before/after accounting contracts and CLI failure modes."""

import copy
from collections import Counter
import csv
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import function_credit_delta_audit as da
from function_queue import DIGESTS, ownership


def summarize(queue):
    rows = queue["rows"]
    queue["summary"] = dict(queue_originals=len(rows),
        strict_full_c_originals=sum(r["strict_full_c"] for r in rows),
        ownership=dict(Counter(r["game_or_library"] for r in rows)),
        classifications=dict(Counter(r["classification"] for r in rows)),
        whole_game_denominator="unknown", completion_percentage=None)
    return queue


def fixture():
    rows = []
    for rel, cls, strict in ((0, "game_verified", 1), (16, "game_verified", 0),
                            (32, "game_or_other_library_unresolved", 0)):
        rows.append(dict(id=f"OVL_0001_{rel:04X}", original_address=f"0x1:{rel:#06x}",
            aliases=[f"0x1:{rel:#06x}", f"STUB_0003_{32+rel:04X}"], domain="overlay",
            classification=cls, game_or_library=ownership(cls), strict_full_c=strict))
    rows.append(dict(id="RES_IMAGE_00100", original_address="image:0x00100",
        aliases=["RPRO_00100"], domain="resident", classification="unresolved",
        game_or_library="unknown", strict_full_c=0))
    return summarize(dict(schema_version=1, inputs=DIGESTS.copy(), rows=rows))


def credits(queue):
    return [dict(id=r["id"], evidence_doc="docs/re/" + r["id"] + ".md")
            for r in queue["rows"] if r["strict_full_c"]]


def expectation(*changes):
    return dict(schema_version=1, changes=list(changes))


def change(name, field="strict_full_c", before=0, after=1):
    return dict(id=name, field=field, before=before, after=after)


def audit(before, after, expected=None):
    return da.audit_delta(before, after, credits(before), credits(after), expected or expectation())


def promote(queue, index, identity=False):
    after = copy.deepcopy(queue)
    row = after["rows"][index]
    row["strict_full_c"] = 1
    if identity:
        row.update(classification="game_verified", game_or_library="game_verified")
    return summarize(after)


def csv_content(rows):
    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=("id", "evidence_doc"), lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue().encode()


class CreditDeltaAuditTest(unittest.TestCase):
    def test_no_change_and_separate_strict_identity_totals(self):
        before = fixture()
        result = audit(before, copy.deepcopy(before))
        self.assertTrue(result["passed"])
        self.assertEqual(result["summary"]["delta"], dict(queue_originals=0,
            strict_full_c_originals=0, verified_game_identities=0))
        self.assertEqual(result["summary"]["whole_game_denominator"], "unknown")

    def test_strict_only_expected_promotion_does_not_add_identity(self):
        before = fixture()
        after = promote(before, 1)
        result = audit(before, after, expectation(change("OVL_0001_0010")))
        self.assertTrue(result["passed"])
        self.assertEqual(result["summary"]["delta"]["strict_full_c_originals"], 1)
        self.assertEqual(result["summary"]["delta"]["verified_game_identities"], 0)

    def test_correct_strict_total_does_not_hide_unexpected_identity_class(self):
        before = fixture()
        after = promote(before, 2, identity=True)
        result = audit(before, after, expectation(change("OVL_0001_0020")))
        self.assertFalse(result["passed"])
        self.assertEqual({r["field"] for r in result["unexpected_changes"]},
                         {"classification", "game_or_library"})

    def test_caller_class_change_must_be_expected_explicitly(self):
        before = fixture()
        after = promote(before, 1)
        after["rows"][2]["classification"] = "game_evidenced"
        summarize(after)
        expected = expectation(change("OVL_0001_0010"))
        result = audit(before, after, expected)
        self.assertEqual(len(result["unexpected_changes"]), 1)
        expected["changes"].append(change("0x1:0x0020", "classification",
            "game_or_other_library_unresolved", "game_evidenced"))
        self.assertTrue(audit(before, after, expected)["passed"])

    def test_balanced_credit_swap_is_flagged_despite_zero_total_delta(self):
        before = fixture()
        after = promote(before, 1)
        after["rows"][0]["strict_full_c"] = 0
        summarize(after)
        result = audit(before, after)
        self.assertFalse(result["passed"])
        self.assertEqual(result["summary"]["delta"]["strict_full_c_originals"], 0)
        self.assertEqual(len(result["unexpected_changes"]), 2)

    def test_identity_only_then_balanced_identity_swap_preserves_strict_count(self):
        before = fixture()
        after = copy.deepcopy(before)
        after["rows"][2].update(classification="game_verified", game_or_library="game_verified")
        summarize(after)
        expected = expectation(
            change("OVL_0001_0020", "game_or_library", "unknown", "game_verified"),
            change("OVL_0001_0020", "classification", "game_or_other_library_unresolved", "game_verified"))
        result = audit(before, after, expected)
        self.assertTrue(result["passed"])
        self.assertEqual(result["summary"]["delta"]["strict_full_c_originals"], 0)
        after["rows"][1].update(classification="game_evidenced", game_or_library="unknown")
        summarize(after)
        result = audit(before, after)
        self.assertFalse(result["passed"])
        self.assertEqual(result["summary"]["delta"]["verified_game_identities"], 0)
        self.assertEqual(len(result["unexpected_changes"]), 4)

    def test_missing_and_wrong_direction_expectations_are_flagged(self):
        before = fixture()
        expected = expectation(change("OVL_0001_0010"))
        self.assertEqual(len(audit(before, before, expected)["missing_expected_changes"]), 1)
        after = promote(before, 1)
        result = audit(before, after, expectation(change("OVL_0001_0010", before=1, after=0)))
        self.assertEqual(len(result["unexpected_changes"]), 1)
        self.assertEqual(len(result["missing_expected_changes"]), 1)

    def test_expected_aliases_normalize_and_duplicates_fail(self):
        before = fixture()
        after = promote(before, 1)
        expected = expectation(change("STUB_0003_0030"))
        self.assertTrue(audit(before, after, expected)["passed"])
        expected["changes"].append(change("OVL_0001_0010"))
        with self.assertRaisesRegex(ValueError, "duplicate"):
            audit(before, after, expected)

    def test_candidate_set_swap_is_flagged_without_count_change(self):
        before = fixture()
        after = copy.deepcopy(before)
        after["rows"][2].update(id="OVL_0001_0030", original_address="0x1:0x0030", aliases=["0x1:0x0030"])
        summarize(after)
        result = audit(before, after)
        self.assertFalse(result["passed"])
        self.assertEqual(result["summary"]["delta"]["queue_originals"], 0)
        self.assertEqual(result["original_set_changes"],
                         dict(added=["OVL_0001_0030"], removed=["OVL_0001_0020"]))

    def test_alias_rebinding_flags_originals_without_exporting_alias_text(self):
        before = fixture()
        after = copy.deepcopy(before)
        alias = after["rows"][1]["aliases"].pop()
        after["rows"][2]["aliases"].append(alias)
        result = audit(before, after)
        self.assertFalse(result["passed"])
        self.assertEqual(len(result["alias_changes"]), 2)
        self.assertNotIn(alias, json.dumps(result))

    def test_resident_promotions_are_audited_without_overlay_assumptions(self):
        before = fixture()
        after = promote(before, 3, identity=True)
        expected = expectation(change("RPRO_00100"),
            change("RPRO_00100", "game_or_library", "unknown", "game_verified"),
            change("RPRO_00100", "classification", "unresolved", "game_verified"))
        self.assertTrue(audit(before, after, expected)["passed"])

    def test_invalid_snapshot_and_stale_summaries_fail(self):
        mutations = (
            lambda q: q["rows"].append(copy.deepcopy(q["rows"][1])),
            lambda q: q["rows"][1]["aliases"].append(q["rows"][0]["aliases"][0]),
            lambda q: q["rows"][1].update(strict_full_c=True),
            lambda q: q["rows"][2].update(game_or_library="game_verified"),
            lambda q: q["summary"].update(classifications={}),
            lambda q: q["summary"].update(whole_game_denominator=4),
            lambda q: q["inputs"].update(start_exe_sha256="0" * 64))
        for mutate in mutations:
            before, after = fixture(), fixture()
            mutate(after)
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                audit(before, after)

    def test_invalid_expectation_fields_values_and_noops_fail(self):
        before, after = fixture(), promote(fixture(), 1)
        for transition in (change("OVL_0001_0010", "grade", "A", "B"),
                           change("OVL_0001_0010", before=True),
                           change("OVL_0001_0010", before=1),
                           change("OVL_0001_0010", "classification", "unknown", "game_verified")):
            with self.subTest(transition=transition), self.assertRaises(ValueError):
                audit(before, after, expectation(transition))

    def test_deterministic_report_ignores_input_order_and_extra_media_fields(self):
        before, after = fixture(), promote(fixture(), 1)
        expected = expectation(change("OVL_0001_0010"))
        first = audit(before, after, expected)
        before["rows"].reverse()
        after["rows"].reverse()
        for row in after["rows"]:
            row["aliases"].reverse()
            row["raw_media"] = "synthetic private text"
        self.assertEqual(audit(before, after, expected), first)
        self.assertNotIn("synthetic private text", json.dumps(first))

    def test_strict_evidence_path_change_is_reported_separately_from_credit(self):
        before = fixture()
        old = credits(before)
        new = copy.deepcopy(old)
        new[0]["evidence_doc"] = "docs/re/renamed.md"
        result = da.audit_delta(before, before, old, new, expectation())
        self.assertTrue(result["passed"])
        self.assertEqual(len(result["strict_evidence_changes"]), 1)
        self.assertEqual(result["summary"]["delta"]["strict_full_c_originals"], 0)

    def test_cli_exit_codes_provenance_determinism_and_no_overwrite_on_invalid_input(self):
        before, after = fixture(), promote(fixture(), 1)
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            paths = {}
            for side, queue in (("before", before), ("after", after)):
                csv_path, queue_path = root / (side + ".csv"), root / (side + ".json")
                csv_path.write_bytes(csv_content(credits(queue)))
                queue["artifacts_sha256"] = dict(full_c_map=hashlib.sha256(csv_path.read_bytes()).hexdigest())
                queue_path.write_text(json.dumps(queue))
                paths[side] = (queue_path, csv_path)
            output, expected_path = root / "report.json", root / "expected.json"
            cmd = [sys.executable, str(Path(da.__file__)), "--json", str(output)]
            for side, (queue_path, csv_path) in paths.items():
                cmd.extend(["--" + side + "-queue", str(queue_path), "--" + side + "-full-c-map", str(csv_path)])
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertEqual(json.loads(output.read_text())["summary"]["unexpected_changes"], 1)
            expected_path.write_text(json.dumps(expectation(change("OVL_0001_0010"))))
            cmd.extend(["--expected-changes", str(expected_path), "--before-head", "a" * 40, "--after-head", "b" * 40])
            first = None
            for _ in range(2):
                result = subprocess.run(cmd, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                current = output.read_bytes()
                if first is None:
                    first = current
                self.assertEqual(current, first)
            paths["after"][1].write_bytes(paths["after"][1].read_bytes() + b"\n")
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 2, result.stderr)
            self.assertEqual(output.read_bytes(), first)


if __name__ == "__main__":
    unittest.main()
