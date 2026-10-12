#!/usr/bin/env python3
"""Synthetic semantic tests: no original ECL or save bytes are included."""

import unittest

from slot_a_ecl_gate import RUN_LAYOUT, branch_path, validate_layout


class SlotAGateTests(unittest.TestCase):
    def test_saved_zero_gate_skips_call_and_exits_after_and(self):
        path = branch_path(10, 0)
        self.assertEqual(path, [
            0x9914, 0x991A, 0x9920, 0x9927, 0x992E, 0x9934,
            0x993A, 0x993B, 0x996B, 0x9974,
        ])
        self.assertNotIn(0x993F, path)
        self.assertNotIn(0x9945, path)

    def test_nonzero_gate_reaches_call_for_both_hour_sides(self):
        for hour in (0, 13, 14, 23):
            with self.subTest(hour=hour):
                path = branch_path(hour, 1)
                self.assertEqual(0x9921 in path, hour >= 14)
                self.assertEqual(path[-2:], [0x993F, 0x9945])
                self.assertNotIn(0x996B, path)

    def test_layout_rejects_mutated_transfer_and_operand(self):
        validate_layout(RUN_LAYOUT)
        for pc, altered in (
            (0x993B, (0x01, 0x993F, (("word", 0x9945),))),
            (0x9945, (0x2D, 0x9949, (("word", 0xC01F),))),
            (0x996B, (0x2F, 0x9974, (("byte", 0x7F), ("word", 0xC04F), ("word", 0x49C7)))),
            (0x997B, (0x0E, 0x997E, (("byte", 0x00),))),
        ):
            with self.subTest(pc=pc), self.assertRaisesRegex(ValueError, f"{pc:#06x}"):
                validate_layout({**RUN_LAYOUT, pc: altered})


if __name__ == "__main__":
    unittest.main()
