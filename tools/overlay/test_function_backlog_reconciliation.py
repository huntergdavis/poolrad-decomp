#!/usr/bin/env python3
"""Synthetic refusal tests for stale, aliased and overstated backlog evidence."""

import copy
import json
from pathlib import Path
import sys
import tempfile
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
    def stdin_fixture(self, root, opaque=False):
        def write(rel, content):
            path = root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)

        queue = dict(inputs={"synthetic": "no game media"}, rows=[])
        ledger = "unit,offset,decoded\n"
        for offset in range(807):
            row = dict(fixture()["rows"][0], id=f"OVL_0100_{offset:04X}",
                       original_address=f"0x100:{offset:#06x}", aliases=[], grade="B")
            queue["rows"].append(row)
            ledger += f"0x100,{offset:#x},1\n"
        queue["rows"].append(dict(fixture()["rows"][0], id="RES_IMAGE_0896A", domain="resident",
                                  original_address="image:0x0896A", aliases=["RES_0802_094A"], grade="A"))
        write(backlog.CENSUS, "Synthetic census fixture; no original bytes.\n")
        write("docs/re/function_ceiling_census.json", json.dumps(dict(
            count=815, bound_kind="upper_bound", source_digests=queue["inputs"])))
        write(backlog.CHECKPOINT, "{}")
        write(backlog.TREE_INPUTS["ledger"], ledger)
        write("docs/re/resident_callable_ledger.csv", "id,image_offset\nRPRO_0896A,0x0896A\n")
        for manifest in backlog.MANIFESTS + ("reviewed_function_identities.csv",):
            write("docs/re/" + manifest, "id,status,evidence_doc,body_bytes,body_sha256\n")
        for name in ("function_backlog_reconciliation.py", "function_queue.py",
                     "function_queue_audit.py", "check_checker_builds.py", "checker_execution.py"):
            write("tools/overlay/" + name, "# Synthetic dependency fingerprint fixture\n")
        write("tools/overlay/strict_candidate_queue.py", "RUNTIME_COVERED_ENDPOINTS = {}\n")
        write("CMakeLists.txt", "add_library(model src/recovered/inline_model.c)\n")
        write("src/recovered/inline_model.c", "void inline_model(void) {}\n")
        checker = '''from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[2]
CHECKER_ROWS = ("RES_0802_094A",)
C_SOURCE = "src/recovered/inline_model.c"
SOURCES = ["inline_model"]
def library(output):
    subprocess.run(["cc", "-x", "c", "-", "-o", str(output)],
                   input=(ROOT / C_SOURCE).read_text(), text=True, check=True)
'''
        if opaque:
            checker = '''import subprocess
CHECKER_ROWS = ("RES_0802_094A",)
def library():
    subprocess.run(["cc", *unknown_sources], check=True)
'''
        write("tools/overlay/stdin_checker.py", checker)
        return queue

    def test_registered_production_source_compiled_via_stdin_is_drafted(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            queue = self.stdin_fixture(root)
            _, products, _ = backlog.reconcile(queue, root)
            import csv
            rows = list(csv.DictReader(products["function_backlog_reconciliation.csv"].decode().splitlines()))
            row = next(r for r in rows if r["id"] == "RES_IMAGE_0896A")
            self.assertEqual(row["evidence_state"], "C_drafted")
            self.assertEqual(row["c_sources"], "src/recovered/inline_model.c")

    def test_opaque_compiler_cmake_superset_is_not_draft_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            queue = self.stdin_fixture(root, opaque=True)
            _, products, _ = backlog.reconcile(queue, root)
            import csv
            rows = list(csv.DictReader(products["function_backlog_reconciliation.csv"].decode().splitlines()))
            row = next(r for r in rows if r["id"] == "RES_IMAGE_0896A")
            self.assertEqual(row["evidence_state"], "disassembled")
            self.assertEqual(row["c_sources"], "")

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
