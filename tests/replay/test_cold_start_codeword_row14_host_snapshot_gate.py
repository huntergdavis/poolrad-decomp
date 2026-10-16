"""Synthetic direct-backing snapshots; no emulator or game media."""

import unittest

from tools.replay.cold_start_codeword_row14_host_snapshot_gate import capture_backing
from tools.replay.cold_start_codeword_row14_internal_plane_resolver import HostMap


class HostSnapshotTests(unittest.TestCase):
    def setUp(self):
        self.state = HostMap("uvga", 0xA118F, 0xFFFF, 0, 0x1FFFF,
                             0x20000, 4, 0)
        self.backing = bytearray(self.state.memsize)
        self.backing[4 * 0x118F:4 * 0x118F + 4] = bytes((0x11, 0x22, 0x33, 0x44))
        self.latch = 0xA5B6C7D8

    def capture(self, state=None, backing=None, latch_read=None, **changes):
        args = dict(guest_linear=0xA118F, expected_read_bank=0,
                    observed_write_bank=0, expected_write_bank=0)
        args.update(changes)
        return capture_backing(state or self.state,
                               self.backing if backing is None else backing,
                               latch_read or (lambda: self.latch), **args)

    def test_four_planes_and_unchanged_latch(self):
        observed = self.capture()
        self.assertEqual(observed.plane_index, 0x118F)
        self.assertEqual(observed.planes, bytes((0x11, 0x22, 0x33, 0x44)))
        self.assertEqual((observed.latch_before, observed.latch_after),
                         (self.latch, self.latch))
        self.assertEqual(self.latch, 0xA5B6C7D8)
        self.assertEqual(self.capture(state=HostMap("uvga_fast", 0xA118F,
                         0xFFFF, 0, 0x1FFFF, 0x20000, 4, 0)).planes,
                         observed.planes)

    def test_target_and_handler_guards(self):
        with self.assertRaisesRegex(ValueError, "target"):
            self.capture(guest_linear=0xA1190)
        with self.assertRaisesRegex(ValueError, "handler"):
            self.capture(state=HostMap("chained", 0xA118F, 0xFFFF, 0,
                                       0x1FFFF, 0x20000, 4, 0))

    def test_bank_guards(self):
        with self.assertRaisesRegex(ValueError, "bank"):
            self.capture(expected_read_bank=1)
        with self.assertRaisesRegex(ValueError, "bank"):
            self.capture(observed_write_bank=1)

    def test_short_backing_guard(self):
        with self.assertRaisesRegex(ValueError, "backing"):
            self.capture(backing=bytes(32))

    def test_latch_mutation_is_rejected(self):
        reads = iter((0xA5B6C7D8, 0xA5B6C7D9))
        with self.assertRaisesRegex(ValueError, "latch"):
            self.capture(latch_read=lambda: next(reads))


if __name__ == "__main__":
    unittest.main()
