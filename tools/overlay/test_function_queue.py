#!/usr/bin/env python3
"""Synthetic queue join and denominator tests; no media or decoder needed."""

import copy
import csv
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import function_queue as fq


def fixture():
    inv = dict(inputs=fq.DIGESTS.copy(), units=[dict(stub_para=0x19, memory_segment=0x1C,
               ovr_offset=100, code_size=0x100, entries=[0x10, 0x820],
               functions=[dict(rel=0x10), dict(rel=0x20), dict(rel=0x30)])])
    ledger, classes = [], []
    for rel, grade, entry, alias, near, cls in (
            (0x10, "A", 1, 0, 0, "game_verified"),
            (0x20, "D", 0, 1, 0, "helper_candidate"),
            (0x30, "B", 0, 0, 2, "game_evidenced")):
        common = dict(id=f"0x19:{rel:#06x}", unit="0x19", offset=hex(rel),
                      ovr_offset=str(100 + rel), grade=grade)
        ledger.append(dict(common, entry=str(entry), entry_alias=str(alias),
                           near_called=str(near), far_called="0", procptr="0"))
        classes.append(dict(common, far_in="0", classification=cls))
    return inv, ledger, classes


def resident_row(rid, image, classification="unresolved", far=1):
    return dict(id=rid, image_offset=image, classification=classification,
                overlay_far_sites=far, loader_far_sites=0, decoded_near_sites=1,
                init_fallthrough=0)


class QueueTest(unittest.TestCase):
    def queue(self, credits=(), resident=None, near=None, data=None):
        return fq.export_queue(*(data or fixture()), list(credits), resident, near)

    def test_stable_order_and_aliases(self):
        original = self.queue()
        data = copy.deepcopy(fixture())
        data[0]["units"][0]["functions"].reverse()
        data[1].reverse()
        data[2].reverse()
        self.assertEqual(original, self.queue(data=data))
        self.assertEqual(original["summary"]["overlay_candidates"], 3)
        self.assertEqual(original["summary"]["overlay_public_entry_slots"], 2)
        self.assertEqual(original["rows"][1]["entry_slots"],
                         [dict(slot=1, raw_offset=0x820, normalized_offset=0x20)])
        self.assertEqual(original["rows"][1]["game_or_library"], "unknown")
        self.assertIsNone(original["summary"]["completion_percentage"])

    def test_verified_does_not_automatically_get_strict_credit(self):
        self.assertEqual(self.queue()["summary"]["strict_full_c_originals"], 0)
        credited = self.queue([dict(id="OVL_0019_0010", evidence_doc="docs/re/synthetic.md")])
        self.assertEqual(credited["summary"]["strict_full_c_originals"], 1)
        with self.assertRaisesRegex(ValueError, "verified game"):
            self.queue([dict(id="OVL_0019_0020", evidence_doc="docs/re/synthetic.md")])

    def test_rejects_input_corruption(self):
        mutations = [lambda d: d[1].append(d[1][0]),
                     lambda d: d[2].pop(),
                     lambda d: d[0]["inputs"].update(start_exe_sha256="bad"),
                     lambda d: d[1][0].update(grade="B"),
                     lambda d: d[2][0].update(ovr_offset="500"),
                     lambda d: d[2][0].update(far_in="1"),
                     lambda d: d[0]["units"][0]["entries"].append(0x99),
                     lambda d: d[1][0].update(near_called="-1")]
        for mutate in mutations:
            data = copy.deepcopy(fixture())
            mutate(data)
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                self.queue(data=data)

    def test_near_validation_cannot_be_silently_assumed(self):
        self.assertEqual(self.queue()["rows"][0]["near_call_evidence"], "raw_unvalidated")
        near = dict(inputs=fq.DIGESTS.copy(), invalid_seed_paths=0, truncated_seed_paths=0,
                    raw_exact_not_decoded=0, decoded_exact_not_raw=0,
                    decoded_near_calls_to_prologues=2, raw_near_call_candidates_to_prologues=2)
        self.assertEqual(self.queue(near=near)["rows"][0]["near_call_evidence"], "decoded_validated")
        near["decoded_near_calls_to_prologues"] = 3
        with self.assertRaises(ValueError):
            self.queue(near=near)

    def test_resident_dedupe_and_overlay_stub_alias(self):
        audit = dict(inputs=fq.DIGESTS.copy(), rows=[
            resident_row("RES_0001_0020", 0x200, "game_verified"),
            resident_row("RPRO_00200", 0x200),
            resident_row("STUBLIKE", 0x1C * 16 + 0x20)], runtime_rows=[
            resident_row("SYS_0AF8_0010", 0xAF90, "linked_runtime_evidenced"),
            resident_row("SYSNEAR_0AF90", 0xAF90, "linked_runtime_evidenced")])
        result = self.queue([dict(id="RES_0001_0020", evidence_doc="docs/re/synthetic.md")], audit)
        self.assertEqual(result["summary"]["queue_originals"], 5)
        self.assertEqual(result["summary"]["resident_unique_originals"], 2)
        self.assertIn("STUBLIKE", result["rows"][0]["aliases"])
        runtime = next(r for r in result["rows"] if r["id"] == "RES_IMAGE_0AF90")
        self.assertEqual(len(runtime["aliases"]), 2)
        self.assertEqual(runtime["game_or_library"], "library_inferred")
        self.assertEqual(runtime["evidence"]["overlay_far_sites"], 1)
        audit["rows"].reverse()
        audit["runtime_rows"].reverse()
        self.assertEqual(result, self.queue([dict(id="RES_0001_0020", evidence_doc="docs/re/synthetic.md")], audit))

    def test_conflicts_and_double_credit_are_rejected(self):
        audit = dict(inputs=fq.DIGESTS.copy(), rows=[
            resident_row("RES_0001_0020", 0x200, "game_verified"),
            resident_row("RPRO_00200", 0x200, "linked_crt_inferred")])
        with self.assertRaisesRegex(ValueError, "conflicting"):
            self.queue(resident=audit)
        audit["rows"][1]["classification"] = "unresolved"
        with self.assertRaisesRegex(ValueError, "double-credit"):
            self.queue([dict(id=x, evidence_doc="docs/re/synthetic.md")
                        for x in ("RES_0001_0020", "RPRO_00200")], audit)
        with self.assertRaisesRegex(ValueError, "absent"):
            self.queue([dict(id="OVL_0019_9999", evidence_doc="docs/re/synthetic.md")])

    def test_cli_invalid_join_preserves_existing_output(self):
        data = fixture()
        data[2][0]["grade"] = "U"
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            (directory / "inventory.json").write_text(json.dumps(data[0]))
            for name, rows in (("ledger.csv", data[1]), ("classes.csv", data[2])):
                with (directory / name).open("w") as fh:
                    writer = csv.DictWriter(fh, fieldnames=list(rows[0]))
                    writer.writeheader()
                    writer.writerows(rows)
            (directory / "credits.csv").write_text("id,evidence_doc\n")
            output = directory / "queue.json"
            output.write_text("existing-output\n")
            result = subprocess.run([sys.executable, str(Path(fq.__file__)),
                "--inventory", str(directory / "inventory.json"),
                "--ledger", str(directory / "ledger.csv"),
                "--classification", str(directory / "classes.csv"),
                "--full-c-map", str(directory / "credits.csv"),
                "--json", str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn("inconsistent evidence grade", result.stderr)
            self.assertEqual(output.read_text(), "existing-output\n")

    def test_owner_audit_refuses_unknown_media_before_writing(self):
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            media = directory / "synthetic.bin"
            media.write_bytes(b"synthetic test input")
            output = directory / "output"
            result = subprocess.run([sys.executable,
                str(Path(fq.__file__).with_name("function_queue_audit.py")),
                "--exe", str(media), "--ovr", str(media), "--out-dir", str(output)],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn("owner-media hash mismatch", result.stderr)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
