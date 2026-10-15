#!/usr/bin/env python3
"""Synthetic AC/DAC transition and RGB comparison checks; no game media."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "replay"))
from slot_a_palette_write_route_audit import (  # noqa: E402
    Hardware, changed_channels, hardware_reports, pixel_differences,
    require_stop_order,
)


def report(mode="M_EGA", palette=None, broken_row=False):
    if palette is None:
        palette = tuple(range(16))
    rows = []
    for address in range(0, 256, 8):
        entries = [f"{(address + j) % 64:02x}0000" for j in range(8)]
        if broken_row and address == 0x18:
            entries.pop()
        rows.append(f"LOG: {address:02x}: " + " ".join(entries))
    return "\n".join((
        "LOG: VGA Attribute Controller info: attrindexmode=0",
        "LOG: mode_control=09h horz-pel-pan=00h overscan-color=00h",
        "LOG: palette={ " + " ".join(f"{v:02x}" for v in palette) + " }",
        "LOG: VGA DAC info: bits=6 pel_mask=ffh pel_index[RGB]=00h",
        "LOG: VGA DAC palette (RGB):", *rows,
        f"LOG: mode={mode} vref=70.087Hz",
    ))


class PaletteWriteRouteTests(unittest.TestCase):
    def test_call_stops_bracket_first_hardware_report(self):
        pre = "LOG: EV of 'CS' is:\nLOG: c88\nLOG: EV of 'IP' is:\nLOG: ee\n"
        post = "LOG: EV of 'CS' is:\nLOG: c88\nLOG: EV of 'IP' is:\nLOG: f3\n"
        reports = ["LOG: VGA Attribute Controller info:" for _ in range(4)]
        require_stop_order(pre + reports[0] + post + "".join(reports[1:]))
        with self.assertRaisesRegex(ValueError, "ordering"):
            require_stop_order(pre + post + "".join(reports))

    def test_complete_ordered_hardware_report_and_rejects_truncation(self):
        pre, post = hardware_reports(report("M_TEXT") + "\n" + report())
        self.assertEqual((pre.mode, post.mode), ("M_TEXT", "M_EGA"))
        self.assertEqual(pre.ac, tuple(range(16)))
        self.assertEqual(pre.dac[0x19], (25, 0, 0))
        self.assertEqual(pre.mask, 255)
        with self.assertRaisesRegex(ValueError, "invalid native DAC row"):
            hardware_reports(report(broken_row=True))
        with self.assertRaisesRegex(ValueError, "incomplete native DAC table"):
            hardware_reports(report().replace("LOG: f8:", "missing:"))

    def test_counts_ac_entries_dac_entries_and_individual_channels(self):
        base = Hardware("M_TEXT", 12, 8, tuple(range(16)),
                        ((0, 0, 0),) * 256, 255)
        ac = list(base.ac)
        ac[6] = 20
        ac[9] = 33
        dac = list(base.dac)
        dac[1] = (1, 2, 0)
        dac[255] = (0, 0, 3)
        after = Hardware("M_EGA", 9, 0, tuple(ac), tuple(dac), 255)
        self.assertEqual(changed_channels(base, after), (2, 2, 3))
        self.assertEqual(changed_channels(after, after), (0, 0, 0))

    def test_rgb_delta_counts_logical_pixels_not_channels(self):
        left = bytes((0, 0, 0, 10, 20, 30, 4, 5, 6))
        right = bytes((0, 0, 1, 10, 20, 30, 7, 8, 9))
        self.assertEqual(pixel_differences(left, right), 2)
        with self.assertRaisesRegex(ValueError, "lengths"):
            pixel_differences(left, right[:-1])


if __name__ == "__main__":
    unittest.main()
