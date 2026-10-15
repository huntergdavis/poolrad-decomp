#!/usr/bin/env python3
"""Synthetic checks for the saved-slot EGA screen/display join."""

import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.replay.slot_a_screen_palette_join_audit import (  # noqa: E402
    EGA_RGB, display_state, ega_indices, screen_indices,
)


class ScreenPaletteJoinTests(unittest.TestCase):
    def setUp(self) -> None:
        planes = [bytearray(8000) for _ in range(4)]
        for pixel in range(16):
            for plane in range(4):
                if pixel & (1 << plane):
                    planes[plane][pixel // 8] |= 0x80 >> (pixel % 8)
        self.planes = tuple(bytes(plane) for plane in planes)

    def test_plane_bit_order_and_significance(self) -> None:
        indices = ega_indices(self.planes)
        self.assertEqual(indices[:16], bytes(range(16)))
        self.assertEqual(indices[16:], bytes(64000 - 16))
        with self.assertRaises(ValueError):
            ega_indices(self.planes[:3])

    def test_screenshot_palette_and_exact_two_times_scaling(self) -> None:
        image = Image.new("RGB", (1280, 1024))
        for pixel, rgb in enumerate(EGA_RGB):
            for dx in range(2):
                for dy in range(2):
                    image.putpixel((320 + 2 * pixel + dx, 312 + dy), rgb)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.png"
            image.save(path)
            indices, colors = screen_indices(path)
            self.assertEqual(indices, ega_indices(self.planes))
            self.assertEqual(colors, 16)
            image.putpixel((321, 312), EGA_RGB[2])
            image.save(path)
            with self.assertRaisesRegex(ValueError, "exact 2x"):
                screen_indices(path)
            image.putpixel((321, 312), EGA_RGB[0])
            image.putpixel((320, 312), (1, 2, 3))
            image.putpixel((321, 312), (1, 2, 3))
            image.putpixel((320, 313), (1, 2, 3))
            image.putpixel((321, 313), (1, 2, 3))
            image.save(path)
            with self.assertRaisesRegex(ValueError, "outside"):
                screen_indices(path)

    def test_display_state_rejects_wrong_page(self) -> None:
        ram = bytearray(655360)
        base = 0x14A00
        ram[base + 0x4A02:base + 0x4A10] = bytes.fromhex("0005" + "00" * 12)
        ram[base + 0x4A16:base + 0x4A1F] = bytes.fromhex("00a000a00000030d01")
        self.assertEqual(display_state(ram), 23)
        ram[base + 0x4A1B] = 1
        with self.assertRaisesRegex(ValueError, "4A16"):
            display_state(ram)


if __name__ == "__main__":
    unittest.main()
