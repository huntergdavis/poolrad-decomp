#!/usr/bin/env python3
"""Synthetic boundary tests for the post-Up native/C printer-page audit."""

import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "replay"))
import slot_a_post_up_first_printer_page_audit as audit  # noqa: E402


class PostUpPrinterAuditTests(unittest.TestCase):
    def test_rejects_changed_stack_frame_before_private_source_access(self):
        ram = bytearray(0xA0000)
        struct.pack_into("<7H", ram, 0x1D160 + 0x3C6E, *audit.FRAME)
        ram[0x1D160 + 0x3C6E] ^= 1
        with self.assertRaisesRegex(ValueError, "frame changed"):
            audit.validate_frame(ram)

    def test_accepts_observed_delta_shape_and_rejects_wrong_plane(self):
        before = bytes(32000)
        after = bytearray(before)
        allowed = [plane * 8000 + line * 40 + column
                   for plane in (1, 3) for line in range(120, 128)
                   for column in range(17, 36)]
        for index in allowed[:105] + allowed[152:257]:
            after[index] = 1
        self.assertEqual(audit.validate_delta(before, after), 210)
        after[0] = 1
        with self.assertRaisesRegex(ValueError, "page delta changed"):
            audit.validate_delta(before, after)

    def test_rejects_right_region_but_wrong_plane_distribution(self):
        before = bytes(32000)
        after = bytearray(before)
        for index in [8000 + line * 40 + col
                      for line in range(120, 128) for col in range(17, 36)][:106]:
            after[index] = 1
        for index in [24000 + line * 40 + col
                      for line in range(120, 128) for col in range(17, 36)][:104]:
            after[index] = 1
        with self.assertRaisesRegex(ValueError, "distribution changed"):
            audit.validate_delta(before, after)


if __name__ == "__main__":
    unittest.main()
