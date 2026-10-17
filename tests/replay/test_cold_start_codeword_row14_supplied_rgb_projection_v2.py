"""Source-only checks for distinct full and canonical supplied RGB digests."""

from dataclasses import replace
from hashlib import sha256
import unittest

from tools.replay.cold_start_codeword_row14_completed_backend_rgb24_v2 import (
    validate_completed_backend_frame_v2,
)
from tools.replay.cold_start_codeword_row14_supplied_rgb_projection_v2 import (
    CANONICAL_BYTE_LENGTH, STATUS, project_supplied_rgb24_rows_v2,
)


def supplied_frame(rgb24: bytes, stride: int):
    manifest = {
        "version": 2,
        "selected_event": {
            "run_token": "run_v2", "arm_token": "arm_v2",
            "stream_id": "stream_v2", "plane_event_id": "plane_v2",
            "raw_sha256": "a" * 64, "candidate_ordinal": 897,
            "pc": "0F2D:0445", "successor_pc": "0F2D:0449",
        },
        "backend_event_id": "backend_v2", "backend": "surface",
        "completion_kind": "backend_complete", "width": 640, "height": 400,
        "row_stride": stride, "pixel_format": "RGB24", "channel_order": "RGB",
        "rgb_byte_length": len(rgb24), "rgb_sha256": sha256(rgb24).hexdigest(),
    }
    return validate_completed_backend_frame_v2(manifest, rgb24)


class SuppliedRgbProjectionV2Tests(unittest.TestCase):
    def test_padding_changes_full_digest_but_not_canonical_rows(self) -> None:
        stride = 1924
        row = bytes(range(256)) * 7 + bytes(range(128))
        original = supplied_frame((row + b"abcd") * 400, stride)
        changed = supplied_frame((row + b"wxyz") * 400, stride)
        first = project_supplied_rgb24_rows_v2(original, enabled=True)
        second = project_supplied_rgb24_rows_v2(changed, enabled=True)
        self.assertEqual(first.status, STATUS)
        self.assertFalse(first.native_coverage_proved)
        self.assertEqual(len(first.canonical_rgb24), CANONICAL_BYTE_LENGTH)
        self.assertNotEqual(first.full_sha256, second.full_sha256)
        self.assertEqual(first.canonical_rgb24, second.canonical_rgb24)
        self.assertEqual(first.canonical_sha256, second.canonical_sha256)

    def test_active_pixel_changes_canonical_and_invalid_full_hash_rejects(self) -> None:
        stride = 1924
        row = bytes(range(256)) * 7 + bytes(range(128))
        original = supplied_frame((row + b"abcd") * 400, stride)
        changed_bytes = bytearray(original.rgb24)
        changed_bytes[stride + 5] ^= 1
        changed = supplied_frame(bytes(changed_bytes), stride)
        first = project_supplied_rgb24_rows_v2(original, enabled=True)
        second = project_supplied_rgb24_rows_v2(changed, enabled=True)
        self.assertNotEqual(first.canonical_sha256, second.canonical_sha256)
        with self.assertRaises(ValueError):
            project_supplied_rgb24_rows_v2(
                replace(changed, rgb_sha256=original.rgb_sha256), enabled=True)
        with self.assertRaises(ValueError):
            project_supplied_rgb24_rows_v2(original)


if __name__ == "__main__":
    unittest.main()
