#!/usr/bin/env python3
"""Synthetic IRQ frame and residual-bound checks; no game data."""

import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "replay"))
import slot_a_delay_irq_stack_audit as audit  # noqa: E402


class DelayIrqStackAuditTests(unittest.TestCase):
    def make_frames(self):
        entry = bytearray(0x10000)
        ss, sp = 0x0100, 0x0200
        base = audit.physical(ss, sp)

        def put(memory, address, value):
            memory[address:address + 2] = value.to_bytes(2, "little")

        put(entry, base, 0x0DD1)
        put(entry, base + 2, 0x0F2D)
        put(entry, base + 4, 900)
        for i in range(base - 0x14, base):
            entry[i] = 0xFF
        entry[base - 3] = 0x04
        irq = bytearray(entry)
        put(irq, base - 2, 0x02B7)  # Game near call, outside residual range.
        put(irq, base - 8, 0x02C1)  # CPU interrupt push order: IP, CS, FLAGS.
        put(irq, base - 6, 0x1264)
        put(irq, base - 4, 0x7206)
        returned = bytearray(irq)
        for i in list(range(base - 0x14, base - 0x0B)) + list(range(base - 0x0A, base - 2)):
            returned[i] = 0
        return entry, irq, returned, ss, sp

    def test_irq_frame_and_residual_bounds(self):
        entry, irq, returned, ss, sp = self.make_frames()
        self.assertEqual(audit.verify_frame(entry, irq, returned, ss, sp),
                         (6, 17))

    def test_wrong_saved_cs_or_extra_write_is_rejected(self):
        entry, irq, returned, ss, sp = self.make_frames()
        wrong_cs = bytearray(irq)
        wrong_cs[audit.physical(ss, sp - 6)] ^= 1
        with self.assertRaisesRegex(ValueError, "IRQ0 saved"):
            audit.verify_frame(entry, wrong_cs, returned, ss, sp)
        extra = bytearray(irq)
        extra[audit.physical(ss, sp - 9)] = 1
        with self.assertRaisesRegex(ValueError, "first IRQ stop"):
            audit.verify_frame(entry, extra, returned, ss, sp)


if __name__ == "__main__":
    unittest.main()
