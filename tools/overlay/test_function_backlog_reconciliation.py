#!/usr/bin/env python3
"""Synthetic refusal tests for stale, aliased and overstated backlog evidence."""

import copy
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import function_backlog_reconciliation as backlog


def fixture():
    row = dict(id="OVL_0017_0000", domain="overlay", original_address="0x17:0x0000",
               aliases=["0x17:0x0000"], classification="game_evidenced",
               game_or_library="unknown", evidence={"near_called": 2}, strict_full_c=0)
    queue = dict(inputs={"exe": "hash"}, artifacts_sha256={"ledger": "current"}, rows=[row])
    return queue


def pin(queue):
    blob = json.dumps(queue).encode()
    return blob, dict(queue_sha256=backlog.sha(blob), inputs=queue["inputs"],
                      artifacts_sha256=queue["artifacts_sha256"], queue_originals=len(queue["rows"]),
                      strict_full_c_originals=sum(r["strict_full_c"] for r in queue["rows"]),
                      verified_game_identities=sum(r["game_or_library"] == "game_verified" for r in queue["rows"]))


class BacklogRefusalTest(unittest.TestCase):
    def test_stale_queue_and_stale_tree_are_refused(self):
        blob, checkpoint = pin(fixture())
        with self.assertRaisesRegex(ValueError, "queue SHA"):
            backlog.validate_queue(blob + b" ", checkpoint, {"ledger": "current"})
        with self.assertRaisesRegex(ValueError, "stale tree input"):
            backlog.validate_queue(blob, checkpoint, {"ledger": "changed"})
        self.assertEqual(backlog.validate_queue(blob, checkpoint, {"ledger": "current"})["rows"][0]["strict_full_c"], 0)

    def test_alias_collision_cannot_be_reported_as_two_functions(self):
        queue = fixture()
        queue["rows"].append(dict(queue["rows"][0], id="OVL_0017_0020", original_address="0x17:0x0020"))
        blob, checkpoint = pin(queue)
        with self.assertRaisesRegex(ValueError, "alias belongs"):
            backlog.validate_queue(blob, checkpoint, {})

    def test_duplicate_address_refused_even_with_different_alias(self):
        queue = fixture()
        queue["rows"].append(dict(queue["rows"][0], id="OVL_0017_0020", aliases=["different"]))
        blob, checkpoint = pin(queue)
        with self.assertRaisesRegex(ValueError, "duplicate original"):
            backlog.validate_queue(blob, checkpoint, {})

    def test_ceiling_library_exclusion_preserves_unknown_ownership(self):
        row = dict(fixture()["rows"][0], id="RES_IMAGE_0AA20", domain="resident",
                   original_address="image:0x0AA20", classification="unresolved")
        self.assertEqual(backlog.census_kind(row)[:2], ("library_inferred", "overlay_manager"))
        self.assertEqual(row["game_or_library"], "unknown")
        row["strict_full_c"] = 1
        row["game_or_library"] = "game_verified"
        queue = fixture()
        queue["rows"] = [row]
        blob, checkpoint = pin(queue)
        with self.assertRaisesRegex(ValueError, "excluded library"):
            backlog.validate_queue(blob, checkpoint, {})

    def test_unverified_credit_is_refused(self):
        queue = fixture()
        queue["rows"][0]["strict_full_c"] = 1
        blob, checkpoint = pin(queue)
        with self.assertRaisesRegex(ValueError, "lacks game identity"):
            backlog.validate_queue(blob, checkpoint, {})

    def test_source_mention_callback_and_span_are_not_credit(self):
        row = fixture()["rows"][0]
        model = dict(status="callback", sources=["src/recovered/callback.c"], declaration="manifest")
        self.assertEqual(backlog.evidence_state(row, "decoded", [model], None)[0], "disassembled")
        model["status"] = "partial"
        self.assertEqual(backlog.evidence_state(row, "decoded", [model], None)[0], "C_drafted")
        model["sources"] = []
        self.assertEqual(backlog.evidence_state(row, "decoded", [model], {"declaration": "span"})[0], "bytes_recovered")
        self.assertEqual(backlog.evidence_state(row, "", [], None)[0], "address_only")
        self.assertEqual(row["strict_full_c"], 0)

    def test_equal_bytes_at_distinct_addresses_are_not_aliases(self):
        queue = fixture()
        second = copy.deepcopy(queue["rows"][0])
        second.update(id="OVL_0017_0020", original_address="0x17:0x0020", aliases=["0x17:0x0020"])
        for row in (queue["rows"][0], second):
            row["body_sha256"] = "a" * 64
        queue["rows"].append(second)
        blob, checkpoint = pin(queue)
        self.assertEqual(len(backlog.validate_queue(blob, checkpoint, {})["rows"]), 2)


if __name__ == "__main__":
    unittest.main()
