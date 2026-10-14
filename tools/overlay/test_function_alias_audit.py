#!/usr/bin/env python3
"""Synthetic alias-credit invariants and 16-bit leaf CFG checks."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import function_alias_audit as fa

try:
    import capstone
except ImportError:
    capstone = None


def fixture():
    inventory = dict(units=[dict(stub_para=0x19, memory_segment=0x1C, entries=[0x810],
                                functions=[dict(rel=0x10)], ovr_offset=0, code_size=0x30)])
    resident = dict(rows=[dict(id="RES_0001_0000", image_offset=0x200, classification="unresolved")],
                    runtime_rows=[dict(id="SYS_0AF8_0010", image_offset=0xAF90, classification="linked_runtime_evidenced")])
    queue = dict(rows=[
        dict(id="OVL_0019_0010", original_address="0x19:0x0010", domain="overlay", grade="A",
             aliases=["0x19:0x0010", "STUB_001C_0020"], strict_full_c=1,
             classification="game_verified", game_or_library="game_verified"),
        dict(id="RES_IMAGE_00200", original_address="image:0x00200", domain="resident",
             aliases=["RES_0001_0000"], strict_full_c=0, game_or_library="unknown"),
        dict(id="RES_IMAGE_0AF90", original_address="image:0x0AF90", domain="resident",
             aliases=["SYS_0AF8_0010"], strict_full_c=0, game_or_library="library_inferred")])
    return queue, inventory, resident


class AliasTest(unittest.TestCase):
    def test_reconcile_independent_aliases_and_preserve_counts(self):
        queue, inventory, resident = fixture()
        result = fa.audit_aliases(queue, inventory, resident)
        self.assertEqual(result["original_starts"], 3)
        self.assertEqual(result["strict_full_c_originals"], 1)
        self.assertEqual(result["verified_game_identities"], 1)
        queue["rows"].reverse()
        self.assertEqual(result, fa.audit_aliases(queue, inventory, resident))

    def test_duplicate_credit_through_raw_entry_alias_is_rejected(self):
        queue, inventory, resident = fixture()
        queue["rows"].append(dict(queue["rows"][0], id="OVL_0019_0810",
                                  original_address="0x19:0x0810", aliases=[]))
        with self.assertRaisesRegex(ValueError, "duplicate strict full-C credit"):
            fa.audit_aliases(queue, inventory, resident)

    def test_duplicate_stub_original_without_credit_is_rejected(self):
        queue, inventory, resident = fixture()
        image = 16 * 0x1C + 0x20
        queue["rows"].append(dict(queue["rows"][0], id=f"RES_IMAGE_{image:05X}",
                                  original_address=f"image:0x{image:05X}", aliases=[], strict_full_c=0))
        with self.assertRaisesRegex(ValueError, "duplicate logical original"):
            fa.audit_aliases(queue, inventory, resident)

    def test_wrong_alias_and_missing_original_fail(self):
        queue, inventory, resident = fixture()
        queue["rows"][0]["aliases"].append("SYS_0AF8_0010")
        with self.assertRaisesRegex(ValueError, "another original"):
            fa.audit_aliases(queue, inventory, resident)
        queue, inventory, resident = fixture()
        queue["rows"].pop()
        with self.assertRaisesRegex(ValueError, "supplied starts"):
            fa.audit_aliases(queue, inventory, resident)

    def test_runtime_and_resident_alias_share_loaded_identity(self):
        queue, inventory, resident = fixture()
        resident["rows"].append(dict(id="RNEAR_0AF90", image_offset=0xAF90, classification="unresolved"))
        queue["rows"][-1]["aliases"].append("RNEAR_0AF90")
        self.assertEqual(fa.audit_aliases(queue, inventory, resident)["original_starts"], 3)

    def test_unknown_ownership_cannot_receive_strict_credit(self):
        queue, inventory, resident = fixture()
        queue["rows"][0]["game_or_library"] = "unknown"
        with self.assertRaisesRegex(ValueError, "strict credit on unknown"):
            fa.audit_aliases(queue, inventory, resident)


@unittest.skipUnless(capstone, "Capstone required for synthetic 16-bit CFG tests")
class LeafTest(unittest.TestCase):
    def test_complete_leaf_and_conditional_return_paths(self):
        self.assertEqual(fa.walk_leaf(b"\x55\x89\xe5\x5d\xcb", 0, 5)["span_bytes"], 5)
        result = fa.walk_leaf(b"\x75\x03\x90\x5d\xcb\x5d\xcb", 0, 7)
        self.assertEqual(result["return_sites"], 2)
        self.assertEqual(result["span_bytes"], 7)

    def test_calls_interrupts_indirect_jumps_and_external_edges_fail(self):
        for code in (b"\xe8\x00\x00\xcb", b"\xcd\x21\xcb", b"\xff\xe0", b"\xeb\x7f", b"\xeb\xfe", b"\x90", b"\xec\xcb", b"\xf4\xcb"):
            with self.subTest(code=code):
                self.assertIsNone(fa.walk_leaf(code, 0, len(code)))

    def test_far_return_cleanup_is_recorded(self):
        result = fa.walk_leaf(b"\x55\x89\xe5\x5d\xca\x04\x00", 0, 7)
        self.assertEqual(result["return_cleanup_bytes"], [4])
        self.assertEqual(result["far_return_sites"], 1)
        self.assertEqual(result["near_return_sites"], 0)

    def test_rank_requires_claim_snapshot_and_preserves_unknown_ownership(self):
        queue, inventory, _ = fixture()
        row = queue["rows"][0]
        row.update(strict_full_c=0, classification="game_evidenced", game_or_library="unknown")
        features = [dict(id=row["original_address"], classification="game_evidenced", bound="5",
                         calls="0", doc_globals="party")]
        overlay = b"\x90" * 0x10 + b"\x55\x89\xe5\x5d\xcb" + b"\x90" * 27
        claims = [dict(id=row["id"], status="unclaimed_in_snapshot", evidence="synthetic snapshot")]
        result = fa.rank_leaves(queue, features, claims, inventory, overlay)
        self.assertEqual(len(result["ranked_leaf_candidates"]), 1)
        self.assertEqual(result["ranked_leaf_candidates"][0]["game_or_library"], "unknown")
        claims[0]["status"] = "existing_model"
        self.assertFalse(fa.rank_leaves(queue, features, claims, inventory, overlay)["ranked_leaf_candidates"])
        self.assertFalse(fa.rank_leaves(queue, features, [], inventory, overlay)["ranked_leaf_candidates"])

    def test_equal_bytes_at_distinct_originals_are_not_merged(self):
        queue, inventory, _ = fixture()
        inventory["units"][0]["functions"].append(dict(rel=0x20))
        row = queue["rows"][0]
        row.update(strict_full_c=0, classification="game_evidenced", game_or_library="unknown")
        queue["rows"].append(dict(row, id="OVL_0019_0020", original_address="0x19:0x0020", aliases=[]))
        features = [dict(id=r["original_address"], classification="game_evidenced", bound="5", calls="0", doc_globals="party")
                    for r in queue["rows"] if r["domain"] == "overlay"]
        claims = [dict(id=r["id"], status="unclaimed_in_snapshot", evidence="synthetic snapshot")
                  for r in queue["rows"] if r["domain"] == "overlay"]
        overlay = bytearray(b"\x90" * 0x30)
        overlay[0x10:0x15] = overlay[0x20:0x25] = b"\x55\x89\xe5\x5d\xcb"
        result = fa.rank_leaves(queue, features, claims, inventory, bytes(overlay))
        self.assertEqual(len(result["ranked_leaf_candidates"]), 2)
        self.assertEqual(len(result["equal_bounded_byte_leads"]), 1)
        self.assertFalse(result["equal_bounded_byte_leads"][0]["identity_merge_proven"])


if __name__ == "__main__":
    unittest.main()
