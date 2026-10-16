"""Whole-byte plane deltas do not imply a changed supplied witness bit."""

import unittest

from tests.replay import test_cold_start_codeword_row14_passive_snapshot_v2 as fixtures
from tools.replay.cold_start_codeword_row14_passive_snapshot_v2 import (
    IDENTITY_FIELDS, parse_capture_v2,
)


class PlaneDeltaSemanticsTests(unittest.TestCase):
    def test_non_witness_bits_change_two_plane_bytes(self):
        packet = fixtures.record()
        packet["before"]["planes"] = "20202020"
        packet["after"]["planes"] = "21202220"
        self.assertEqual(packet["witness"]["bit"], 0x20)

        result = parse_capture_v2(fixtures.raw(packet))
        self.assertEqual(result.before_planes, bytes.fromhex("20202020"))
        self.assertEqual(result.after_planes, bytes.fromhex("21202220"))
        # Hardcoded expectations distinguish the entire byte from bit 0x20.
        self.assertEqual(result.changed_planes, (0, 2))
        self.assertEqual(tuple(byte & 0x20 for byte in result.before_planes),
                         (0x20, 0x20, 0x20, 0x20))
        self.assertEqual(tuple(byte & 0x20 for byte in result.after_planes),
                         (0x20, 0x20, 0x20, 0x20))
        for name in IDENTITY_FIELDS:
            with self.subTest(identity=name):
                self.assertEqual(getattr(result, name), packet[name])
        # These are supplied packet labels and bytes, with no native, page,
        # RGB, or first-change inference from the differing plane indices.


if __name__ == "__main__":
    unittest.main()
