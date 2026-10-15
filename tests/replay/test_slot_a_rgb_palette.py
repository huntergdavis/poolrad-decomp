#!/usr/bin/env python3
"""Synthetic checks for the slot-A RGB frame audit; no game media required."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "replay"))
from slot_a_rgb_palette_audit import doubled_rgb, ega_rgb_frame, palette_state  # noqa: E402


class SlotARgbPaletteTests(unittest.TestCase):
    def test_four_planes_attribute_map_mask_and_six_bit_channels(self):
        # Eight pixels: indices 1, 2, 4, 8, then four zeros.
        page = bytes((0x80, 0x40, 0x20, 0x10))
        ac = list(range(16))
        ac[1], ac[2], ac[4], ac[8] = 0x11, 0x22, 0x33, 0x44
        dac = [(0, 0, 0)] * 256
        dac[0x11] = (0, 21, 42)
        dac[0x22] = (63, 0, 0)
        dac[0x33] = (0, 63, 0)
        dac[0x04] = (42, 42, 63)
        frame, counts = ega_rgb_frame(page, tuple(ac), tuple(dac), 0x3F, 8, 1)
        self.assertEqual(frame, bytes((0, 85, 170, 255, 0, 0, 0, 255, 0,
                                       170, 170, 255, *([0] * 12))))
        self.assertEqual(counts, {0: 4, 1: 1, 2: 1, 4: 1, 8: 1})
        self.assertEqual(doubled_rgb(frame, 8, 1)[:12],
                         bytes((0, 85, 170, 0, 85, 170,
                                255, 0, 0, 255, 0, 0)))
        self.assertEqual(len(doubled_rgb(frame, 8, 1)), 8 * 2 * 2 * 3)

    def test_parser_reads_all_dac_rows_and_rejects_missing_row(self):
        rows = [f"LOG: {i:02x}: " + " ".join(f"{(i+j)&63:02x}0000"
                                               for j in range(8))
                for i in range(0, 256, 8)]
        trace = "\n".join((
            "LOG: mode_control=09h horz-pel-pan=00h overscan-color=00h",
            "LOG: color-plane-en=0fh color-select=00h index=00h",
            "LOG: palette={ 00 01 02 03-04 05 06 07-10 11 12 13-14 15 16 17 }",
            "LOG: VGA DAC info: bits=6 pel_mask=ffh pel_index[RGB]=00h",
            "LOG: mode=M_EGA vref=70.087Hz",
            "LOG: disp-start=0h real-start=0h retrace=0 scanlen=20 cursor-start=0h",
            "LOG: byte-skip=0 addr-shift=0 rd-mode=0 wr-mode=0",
            "LOG: VGA DAC palette (RGB):", *rows,
        ))
        ac, dac, mask = palette_state(trace)
        self.assertEqual(ac, (*range(8), *range(0x10, 0x18)))
        self.assertEqual((dac[0], dac[0x11], dac[255], mask),
                         ((0, 0, 0), (17, 0, 0), (63, 0, 0), 255))
        with self.assertRaisesRegex(ValueError, "incomplete native DAC palette"):
            palette_state(trace.rsplit("\n", 1)[0])
        with self.assertRaisesRegex(ValueError, "attribute map changed"):
            palette_state(trace.replace("07-10", "07-08"))

    def test_rejects_incomplete_or_invalid_planes(self):
        ac = tuple(range(16))
        dac = ((0, 0, 0),) * 256
        with self.assertRaisesRegex(ValueError, "frame layout"):
            ega_rgb_frame(bytes(3), ac, dac, 255, 8, 1)
        with self.assertRaisesRegex(ValueError, "six-bit"):
            ega_rgb_frame(bytes(4), ac, ((64, 0, 0),) * 256, 255, 8, 1)


if __name__ == "__main__":
    unittest.main()
