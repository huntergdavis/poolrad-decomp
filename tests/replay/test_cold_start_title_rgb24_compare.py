"""Synthetic checks for the private title frame comparator."""

from contextlib import redirect_stdout
import hashlib
from io import StringIO
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

from PIL import Image

from tools.replay.cold_start_title_rgb24_compare import (
    HEIGHT, RGB_BYTES, WIDTH, Difference, compare_rgb24, main,
)


class TitleFrameComparisonTest(unittest.TestCase):
    def setUp(self) -> None:
        self.reference = bytes(RGB_BYTES)

    def test_equal_frames(self) -> None:
        self.assertEqual(compare_rgb24(self.reference, self.reference),
                         Difference(0, 0, None))

    def test_changed_channels_and_exclusive_bbox(self) -> None:
        candidate = bytearray(self.reference)
        first = 3 * (3 * WIDTH + 7)
        last = 3 * ((HEIGHT - 1) * WIDTH + WIDTH - 1)
        candidate[first:first + 3] = bytes((5, 0, 9))
        candidate[last:last + 3] = bytes((1, 2, 3))
        self.assertEqual(compare_rgb24(self.reference, candidate),
                         Difference(5, 2, (7, 3, WIDTH, HEIGHT)))

    def test_rejects_truncated_frame(self) -> None:
        with self.assertRaisesRegex(ValueError, "exactly 768000"):
            compare_rgb24(self.reference, self.reference[:-1])

    def test_cli_reports_mismatch_and_exit_one(self) -> None:
        with TemporaryDirectory() as directory:
            private = Path(directory) / "resources"
            private.mkdir()
            reference = private / "native.png"
            candidate = private / "candidate.rgb24"
            Image.frombytes("RGB", (WIDTH, HEIGHT), self.reference).save(reference)
            changed = bytearray(self.reference)
            changed[3 * (2 * WIDTH + 4) + 1] = 255
            candidate.write_bytes(changed)
            arguments = ["compare", "--reference", str(reference),
                         "--candidate", str(candidate), "--candidate-format", "rgb24"]
            output = StringIO()
            with patch("tools.replay.cold_start_title_rgb24_compare.NATIVE_RGB_SHA256",
                       hashlib.sha256(self.reference).hexdigest()), \
                 patch.object(sys, "argv", arguments), redirect_stdout(output):
                self.assertEqual(main(), 1)
            self.assertIn("channel_bytes_different=1 pixels_different=1", output.getvalue())
            self.assertIn("bbox_xyxy_exclusive=4,2,5,3", output.getvalue())


if __name__ == "__main__":
    unittest.main()
