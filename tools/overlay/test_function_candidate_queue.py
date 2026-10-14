#!/usr/bin/env python3
"""Synthetic ranking and CLI provenance tests; no owner media."""

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
import function_candidate_queue as cq
from function_queue import DIGESTS


def fixture():
    rows, features = [], []
    for rel, cls, strict, span, calls in (
            (0, "game_verified", 1, 5, 0),
            (16, "game_verified", 0, 40, 1),
            (32, "game_evidenced", 0, 80, 0),
            (48, "game_evidenced", 0, 10, 1),
            (64, "game_or_other_library_unresolved", 0, 8, 0)):
        alias = f"0x1:{rel:#06x}"
        rows.append(dict(id=f"OVL_0001_{rel:04X}", original_address=alias, aliases=[alias],
            domain="overlay", classification=cls, game_or_library="game_verified" if cls == "game_verified" else "unknown",
            grade="B", strict_full_c=strict, evidence=dict(entry=0, entry_alias=0, far_called=0, near_called=2, procptr=0),
            near_call_evidence="decoded_validated"))
        features.append(dict(id=alias, classification=cls, grade="B", bound=str(span), decoded=str(span), calls=str(calls)))
    for image, cls in ((256, "game_verified"), (512, "linked_runtime_evidenced")):
        rows.append(dict(id=f"RES_IMAGE_{image:05X}", original_address=f"image:0x{image:05X}",
            aliases=[f"SYS_{image:05X}"], domain="resident", classification=cls,
            game_or_library="game_verified" if cls == "game_verified" else "library_inferred",
            grade="A", strict_full_c=0, evidence=dict(overlay_far_sites=1, loader_far_sites=0,
                decoded_near_sites=0, init_fallthrough=0), near_call_evidence="decoded_candidate_seeded"))
    queue = dict(schema_version=1, inputs=DIGESTS.copy(), rows=rows,
        summary=dict(queue_originals=len(rows), strict_full_c_originals=1,
            ownership=dict(Counter(r["game_or_library"] for r in rows)),
            whole_game_denominator="unknown", completion_percentage=None))
    credits = [dict(id=rows[0]["id"], evidence_doc="docs/re/synthetic.md")]
    return queue, credits, features


def encoded_csv(rows):
    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=list(rows[0]), lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue().encode()


class CandidateQueueTest(unittest.TestCase):
    def test_credit_exclusion_and_explicit_lexicographic_order(self):
        q, credits, features = fixture()
        result = cq.rank_candidates(q, credits, features)
        self.assertEqual([r["id"] for r in result["rows"]],
            ["OVL_0001_0010", "RES_IMAGE_00100", "OVL_0001_0020",
             "OVL_0001_0030", "OVL_0001_0040", "RES_IMAGE_00200"])
        self.assertEqual(result["summary"]["uncredited_originals"], 6)
        self.assertEqual(result["summary"]["whole_game_denominator"], "unknown")
        self.assertIsNone(result["summary"]["completion_percentage"])

    def test_resident_measurements_are_unknown_and_library_is_labeled(self):
        q, credits, features = fixture()
        result = cq.rank_candidates(q, credits, features, "resident")
        self.assertEqual(len(result["rows"]), 2)
        for row in result["rows"]:
            self.assertIsNone(row["span_upper_bound"])
            self.assertIsNone(row["outgoing_calls"])
        self.assertEqual(result["rows"][-1]["game_or_library"], "library_inferred")

    def test_span_grade_incoming_and_id_tie_breaks(self):
        q, credits, features = fixture()
        features[3]["calls"] = "0"
        self.assertEqual(cq.rank_candidates(q, credits, features)["rows"][2]["id"], "OVL_0001_0030")
        features[3]["bound"] = features[3]["decoded"] = "80"
        q["rows"][3]["evidence"]["far_called"] = 1
        q["rows"][3]["grade"] = features[3]["grade"] = "A"
        self.assertEqual(cq.rank_candidates(q, credits, features)["rows"][2]["id"], "OVL_0001_0030")
        q["rows"][2]["evidence"]["far_called"] = 2
        q["rows"][2]["grade"] = features[2]["grade"] = "A"
        self.assertEqual(cq.rank_candidates(q, credits, features)["rows"][2]["id"], "OVL_0001_0020")
        q["rows"][3]["evidence"]["far_called"] = 2
        self.assertEqual(cq.rank_candidates(q, credits, features)["rows"][2]["id"], "OVL_0001_0020")

    def test_input_order_does_not_change_output(self):
        q, credits, features = fixture()
        expected = cq.rank_candidates(q, credits, features)
        q["rows"].reverse()
        features.reverse()
        self.assertEqual(cq.rank_candidates(q, credits, features), expected)

    def test_extra_input_text_is_not_exported(self):
        q, credits, features = fixture()
        q["raw_media"] = "synthetic private text"
        q["rows"][1]["instructions"] = "synthetic instruction text"
        q["rows"][1]["evidence"]["synthetic private text"] = 3
        features[1]["strings"] = "synthetic private text"
        result = json.dumps(cq.rank_candidates(q, credits, features))
        self.assertNotIn("synthetic", result)
        self.assertNotIn("instructions", result)

    def test_alias_credit_is_excluded_once_and_duplicate_alias_credit_fails(self):
        q, credits, features = fixture()
        credits[0]["id"] = q["rows"][0]["aliases"][0]
        self.assertEqual(len(cq.rank_candidates(q, credits, features)["rows"]), 6)
        credits.append(dict(id=q["rows"][0]["id"], evidence_doc="docs/re/synthetic.md"))
        with self.assertRaisesRegex(ValueError, "duplicate strict credit"):
            cq.rank_candidates(q, credits, features)

    def test_stale_manifest_summary_and_denominator_fail(self):
        mutations = (
            lambda q, c, f: c.clear(),
            lambda q, c, f: q["summary"].update(strict_full_c_originals=2),
            lambda q, c, f: q["summary"].update(whole_game_denominator=7),
            lambda q, c, f: q["inputs"].update(game_ovr_sha256="0" * 64),
            lambda q, c, f: q["rows"][2].update(game_or_library="game_verified"))
        for mutate in mutations:
            q, credits, features = fixture()
            mutate(q, credits, features)
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                cq.rank_candidates(q, credits, features)

    def test_duplicate_identity_alias_and_classifier_sets_fail(self):
        mutations = (
            lambda q, f: q["rows"].append(copy.deepcopy(q["rows"][1])),
            lambda q, f: q["rows"][2]["aliases"].append(q["rows"][1]["aliases"][0]),
            lambda q, f: f.append(copy.deepcopy(f[0])),
            lambda q, f: f.pop())
        for mutate in mutations:
            q, credits, features = fixture()
            mutate(q, features)
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                cq.rank_candidates(q, credits, features)

    def test_invalid_span_measurement_and_classifier_class_fail(self):
        for values in (dict(bound="0"), dict(decoded="999"), dict(calls="-1"),
                       dict(calls=True), dict(classification="game_verified")):
            q, credits, features = fixture()
            features[2].update(values)
            with self.subTest(values=values), self.assertRaises(ValueError):
                cq.rank_candidates(q, credits, features)

    def test_filters_and_limit_do_not_redefine_source_counts(self):
        q, credits, features = fixture()
        result = cq.rank_candidates(q, credits, features, "overlay", 2)
        self.assertEqual(result["summary"]["queue_originals"], 7)
        self.assertEqual(result["summary"]["uncredited_originals"], 6)
        self.assertEqual(result["summary"]["eligible_originals"], 4)
        self.assertEqual(len(result["rows"]), 2)
        for limit in (0, -1, True):
            with self.assertRaises(ValueError):
                cq.rank_candidates(q, credits, features, limit=limit)

    def test_cli_repeats_identically_and_refuses_stale_provenance_without_overwrite(self):
        q, credits, features = fixture()
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            credit_path, feature_path = root / "strict.csv", root / "features.csv"
            credit_path.write_bytes(encoded_csv(credits))
            feature_path.write_bytes(encoded_csv(features))
            q["artifacts_sha256"] = {key: hashlib.sha256(path.read_bytes()).hexdigest()
                for key, path in (("full_c_map", credit_path), ("classification", feature_path))}
            queue_path, output_path = root / "queue.json", root / "result.json"
            queue_path.write_text(json.dumps(q))
            cmd = [sys.executable, str(Path(cq.__file__)), "--queue", str(queue_path),
                "--full-c-map", str(credit_path), "--classification", str(feature_path), "--json", str(output_path)]
            for _ in range(2):
                result = subprocess.run(cmd, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                if _ == 0:
                    first = output_path.read_bytes()
                else:
                    self.assertEqual(output_path.read_bytes(), first)
            feature_path.write_bytes(feature_path.read_bytes() + b"\n")
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn("provenance", result.stderr)
            self.assertEqual(output_path.read_bytes(), first)


if __name__ == "__main__":
    unittest.main()
