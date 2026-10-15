#!/usr/bin/env python3
"""Synthetic checks for the A40 private-frame comparator's public boundary."""

import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "replay"))
import slot_a_a40_frame_compare as frame  # noqa: E402


class A40FrameCompareTests(unittest.TestCase):
    def test_c_body_and_external_timer_are_separate(self) -> None:
        with tempfile.TemporaryDirectory(prefix="poolrad-a40-test-") as tmp:
            lib = frame.build_shim(Path(tmp))
            status, report = frame.run_model(lib, 2, 248, 3, 9)
        self.assertEqual(status, 0)
        self.assertEqual(report[:6], (2, 2, 7, 2, 6, 24))
        self.assertEqual(report[6:], (248, 248, 3, 0))

        entry = bytearray(frame.RAM_SIZE)
        frame.put_word(entry, frame.SS, frame.ENTRY_SP, 0x0DD1)
        frame.put_word(entry, frame.SS, frame.ENTRY_SP + 2, 0x0F2D)
        frame.put_word(entry, frame.SS, frame.ENTRY_SP + 4, 2)
        frame.put_dword(entry, 0x40, 0x6C, 1000)
        result, cpu = frame.simulate_frame(bytes(entry), frame.NATIVE_ENTRY_CPU,
                                            report, frame.timer_advance, 5)
        self.assertEqual(frame.dword(result, 0x40, 0x6C), 1005)
        self.assertEqual(frame.word(result, frame.SS, frame.ENTRY_SP - 2),
                         0x02B7)
        self.assertEqual((cpu["ax"], cpu["bx"], cpu["cx"], cpu["dx"]),
                         (248, 0, 0, 0))
        self.assertEqual((cpu["cs"], cpu["ip"], cpu["sp"]),
                         (0x0F2D, 0x0DD1, 0x3EDC))
        self.assertEqual(frame.intervals(entry, result),
                         [(0x46C, 0x46D), (0x21034, 0x21036)])

    def test_timer_wrap_and_reached_budget(self) -> None:
        memory = bytearray(frame.RAM_SIZE)
        frame.put_dword(memory, 0x40, 0x6C, 0xFFFFFFFE)
        frame.timer_advance(memory, 5)
        self.assertEqual(frame.dword(memory, 0x40, 0x6C), 3)
        with tempfile.TemporaryDirectory(prefix="poolrad-a40-test-") as tmp:
            lib = frame.build_shim(Path(tmp))
            status, report = frame.run_model(lib, 1, 7, 3, 2)
        self.assertEqual(status, 3)  # PR_A40_DELAY_BUDGET
        self.assertEqual(report[:4], (1, 0, 1, 1))


if __name__ == "__main__":
    unittest.main()
