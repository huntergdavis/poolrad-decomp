#!/usr/bin/env python3
"""Synthetic guards for the post-Up row-15 clear and printer bridge audit."""

import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "replay"))
import slot_a_post_up_row15_clear_bridge_audit as audit  # noqa: E402


class PostUpRow15BridgeTests(unittest.TestCase):
    @staticmethod
    def frame() -> bytearray:
        memory = bytearray(0xA0000)
        struct.pack_into("<8H", memory, 0x1D160 + 0x3B62, *audit.RECT_FRAME)
        struct.pack_into("<2H", memory, 0x1D160 + 0x3B80,
                         0x2B47, 0x08DE)
        return memory

    def test_reached_rectangle_frame_and_caller(self):
        audit.validate_rect_frame(self.frame())
        memory = self.frame()
        memory[0x1D160 + 0x3B62 + 14] ^= 1
        with self.assertRaisesRegex(ValueError, "rectangle frame changed"):
            audit.validate_rect_frame(memory)
        memory = self.frame()
        memory[0x1D160 + 0x3B80] ^= 1
        with self.assertRaisesRegex(ValueError, "parent return changed"):
            audit.validate_rect_frame(memory)

    def test_delta_rejects_wrong_plane_and_count(self):
        before = bytes(32000)
        after = bytearray(before)
        allowed = [lane * 8000 + row * 40 + col
                   for lane in (1, 3) for row in range(120, 128)
                   for col in range(17, 36)]
        for index in allowed[:105] + allowed[152:257]:
            after[index] = 1
        self.assertEqual(audit.validate_delta(before, after), 210)
        after[0] = 1
        with self.assertRaisesRegex(ValueError, "page delta changed"):
            audit.validate_delta(before, after)

    def test_delta_rejects_wrong_plane_distribution(self):
        before = bytes(32000)
        after = bytearray(before)
        for index in [8000 + row * 40 + col
                      for row in range(120, 128) for col in range(17, 36)][:106]:
            after[index] = 1
        for index in [24000 + row * 40 + col
                      for row in range(120, 128) for col in range(17, 36)][:104]:
            after[index] = 1
        with self.assertRaisesRegex(ValueError, "distribution changed"):
            audit.validate_delta(before, after)

    def test_native_fields_use_actual_far_pointer(self):
        memory = bytearray(0xA0000)
        struct.pack_into("<2H", memory, 0x14A00 + 0x49D2, 0, 0x3996)
        for offset, value in zip((0x6AAD, 0x6AAE, 0x6AAF), (14, 4, 6)):
            memory[0x14A00 + offset] = value
        struct.pack_into("<3H", memory, 0x39960 + 0x18E, 0, 0, 11)
        self.assertEqual(audit.native_route_fields(memory),
                         ((14, 4, 6), (0, 0, 11)))
        memory[0x14A00 + 0x49D4] ^= 1
        with self.assertRaisesRegex(ValueError, "far pointer changed"):
            audit.native_route_fields(memory)


if __name__ == "__main__":
    unittest.main()
