#!/usr/bin/env python3
"""Synthetic stack-frame cases for the bounded DELAY residual-read audit."""

import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "replay"))
import slot_a_delay_residual_read_boundary_audit as audit  # noqa: E402


class DelayResidualReadBoundaryTests(unittest.TestCase):
    @staticmethod
    def frame(sp=0x3EDC):
        memory = bytearray(0xA0000)
        at = audit.physical(audit.SS, sp)
        for index, value in enumerate((sp + 6, 0x272B, 0x3582,
                                       sp + 12, 0x3151, 0x3582,
                                       sp + 18, 0x31A8, 0x3582)):
            memory[at + index * 2:at + index * 2 + 2] = value.to_bytes(2, "little")
        return memory

    def test_read_ranges_follow_three_far_return_frames(self):
        self.assertEqual(audit.returned_reads(self.frame()),
                         ((0x3EDC, 0x3EDE), (0x3EDE, 0x3EE2),
                          (0x3EE2, 0x3EE4), (0x3EE4, 0x3EE8),
                          (0x3EE8, 0x3EEA), (0x3EEA, 0x3EEE)))

    def test_changed_far_return_and_residual_overlap_are_rejected(self):
        memory = self.frame()
        memory[audit.physical(audit.SS, 0x3EE0)] ^= 1
        with self.assertRaisesRegex(ValueError, "resident wrapper return frame"):
            audit.returned_reads(memory)
        with self.assertRaisesRegex(ValueError, "overlaps residual stack"):
            audit.returned_reads(self.frame(sp=0x3ECC), sp=0x3ECC)

    def test_changed_caller_return_is_rejected(self):
        memory = self.frame()
        memory[audit.physical(audit.SS, 0x3EEA)] ^= 1
        with self.assertRaisesRegex(ValueError, "U1C caller return frame"):
            audit.returned_reads(memory)


if __name__ == "__main__":
    unittest.main()
