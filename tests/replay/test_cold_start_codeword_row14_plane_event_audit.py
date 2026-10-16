"""Synthetic evidence-gate tests for the private U19 four-plane watch."""

from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.replay.cold_start_codeword_row14_plane_event_audit import (
    WINDOW_BYTES, WITNESS_AT, audit, compare_events,
)


def planes(*witness_values):
    return tuple(bytes(WITNESS_AT) + bytes((value,)) +
                 bytes(WINDOW_BYTES - WITNESS_AT - 1)
                 for value in witness_values)


class PlaneEventAuditTests(unittest.TestCase):
    def test_no_op_then_observed_glyph_bit_change(self):
        blank = planes(0, 0, 0, 0)
        glyph = planes(0, 0x20, 0, 0)
        changes = compare_events([
            {"sequence": 1, "pc": "0F2D:0445", "next_pc": "0F2D:0449",
             "before": blank, "after": blank},
            {"sequence": 2, "pc": "0F2D:0412", "next_pc": "0F2D:0415",
             "before": blank, "after": glyph},
        ])
        self.assertEqual(changes[0].changed_planes, ())
        self.assertEqual(changes[1].changed_planes, (1,))
        self.assertEqual(changes[1].changed_witness_bits, (1,))

    def test_byte_change_without_pixel_bit_change_is_distinct(self):
        first = planes(0, 0x01, 0, 0)
        second = planes(0, 0x03, 0, 0)
        change, = compare_events([
            {"sequence": 1, "pc": "0F2D:0412", "next_pc": "0F2D:0415",
             "before": first, "after": second},
        ])
        self.assertEqual(change.changed_planes, (1,))
        self.assertEqual(change.changed_witness_bits, ())

    def test_wrong_successor_and_missing_witness_event_rejected(self):
        blank = planes(0, 0, 0, 0)
        changed = planes(0, 0x20, 0, 0)
        with self.assertRaisesRegex(ValueError, "complete candidate"):
            compare_events([
                {"sequence": 1, "pc": "0F2D:0445", "next_pc": "0F2D:0451",
                 "before": blank, "after": blank},
            ])
        with self.assertRaisesRegex(ValueError, "Unaccounted|unaccounted"):
            compare_events([
                {"sequence": 1, "pc": "0F2D:0445", "next_pc": "0F2D:0449",
                 "before": blank, "after": blank},
                {"sequence": 2, "pc": "0F2D:0412", "next_pc": "0F2D:0415",
                 "before": changed, "after": changed},
            ])

    def test_manifest_rejects_truncated_plane_and_controller_mutation(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            ram = bytearray(655360)
            at = (0x14A0 << 4) + 0x4A16
            ram[at:at + 4] = bytes((0, 0xA0, 0, 0xA0))
            ram[(0x14A0 << 4) + 0x4A1E] = 1
            (root / "ram.bin").write_bytes(ram)
            for plane in range(4):
                (root / f"p{plane}.bin").write_bytes(bytes(WINDOW_BYTES))
            sample = {"ram": "ram.bin",
                      "planes": [f"p{plane}.bin" for plane in range(4)],
                      "gc_before": [5, 0], "gc_after": [5, 0]}
            pair = {"sequence": 1, "pc": "0F2D:0445",
                    "next_pc": "0F2D:0449",
                    "before": sample, "after": sample}
            manifest = {"version": 1, "window_start": 0x1180,
                        "window_bytes": 0x140, "witness_offset": 0x118F,
                        "events": [pair]}
            self.assertEqual(len(audit(root, manifest)), 1)
            (root / "p2.bin").write_bytes(bytes(WINDOW_BYTES - 1))
            with self.assertRaisesRegex(ValueError, "320 bytes"):
                audit(root, manifest)
            (root / "p2.bin").write_bytes(bytes(WINDOW_BYTES))
            sample["gc_after"] = [5, 1]
            with self.assertRaisesRegex(ValueError, "not restored"):
                audit(root, manifest)


if __name__ == "__main__":
    unittest.main()
