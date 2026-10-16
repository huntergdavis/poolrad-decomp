"""Synthetic checks for the supplied U19 completed-backend RGB24 packet."""

from copy import deepcopy
from hashlib import sha256
import json
import unittest

from tools.replay.cold_start_codeword_row14_completed_backend_rgb24_v2 import (
    MAX_MANIFEST_BYTES, STATUS, parse_completed_backend_frame_v2,
    validate_completed_backend_frame_v2,
)


def fixture(stride: int = 1920) -> tuple[dict, bytes]:
    rgb24 = bytes(range(256)) * (stride * 400 // 256)
    rgb24 += b"\x80" * (stride * 400 - len(rgb24))
    manifest = {
        "version": 2,
        "selected_event": {
            "run_token": "run_v2", "arm_token": "arm_v2",
            "stream_id": "stream_v2", "plane_event_id": "plane_v2",
            "raw_sha256": "a" * 64, "candidate_ordinal": 897,
            "pc": "0F2D:0412", "successor_pc": "0F2D:0415",
        },
        "backend_event_id": "backend_v2", "backend": "surface",
        "completion_kind": "backend_complete",
        "width": 640, "height": 400, "row_stride": stride,
        "pixel_format": "RGB24", "channel_order": "RGB",
        "rgb_byte_length": len(rgb24), "rgb_sha256": sha256(rgb24).hexdigest(),
    }
    return manifest, rgb24


def wire(manifest: dict) -> bytes:
    return json.dumps(manifest, separators=(",", ":")).encode("utf-8")


class CompletedBackendRgb24V2Tests(unittest.TestCase):
    def test_actual_bytes_and_padded_stride_are_preserved(self) -> None:
        for stride in (1920, 1924):
            with self.subTest(stride=stride):
                manifest, rgb24 = fixture(stride)
                result = parse_completed_backend_frame_v2(wire(manifest), rgb24)
                self.assertEqual(result.status, STATUS)
                self.assertEqual((result.width, result.height, result.row_stride),
                                 (640, 400, stride))
                self.assertEqual((result.rgb_byte_length, result.rgb_sha256),
                                 (len(rgb24), sha256(rgb24).hexdigest()))
                self.assertEqual(result.rgb24, rgb24)
                self.assertEqual((result.candidate_ordinal, result.pc,
                                  result.successor_pc),
                                 (897, "0F2D:0412", "0F2D:0415"))

    def test_selected_labels_and_completion_remain_declarations(self) -> None:
        manifest, rgb24 = fixture()
        selected = manifest["selected_event"]
        selected["run_token"], selected["arm_token"] = (
            selected["arm_token"], selected["run_token"])
        result = parse_completed_backend_frame_v2(wire(manifest), rgb24)
        self.assertEqual((result.run_token, result.arm_token),
                         ("arm_v2", "run_v2"))
        self.assertNotEqual(result.backend_event_id, result.plane_event_id)
        self.assertEqual(result.status, STATUS)
        self.assertFalse(hasattr(result, "frame"))
        self.assertFalse(hasattr(result, "real_start"))
        self.assertFalse(hasattr(result, "palette_sha256"))

    def test_reject_changed_rgb_bytes_length_hash_and_format(self) -> None:
        manifest, rgb24 = fixture()
        for bad_manifest, bad_rgb in (
            (manifest, rgb24[:-1]),
            (manifest, rgb24[:-1] + b"\x00"),
            ({**manifest, "rgb_byte_length": len(rgb24) + 1}, rgb24),
            ({**manifest, "row_stride": 1919}, rgb24),
            ({**manifest, "row_stride": True}, rgb24),
            ({**manifest, "width": 640.0}, rgb24),
            ({**manifest, "pixel_format": "BGRA32"}, rgb24),
            ({**manifest, "channel_order": "BGR"}, rgb24),
            ({**manifest, "completion_kind": "renderer_end"}, rgb24),
            ({**manifest, "backend_event_id": ""}, rgb24),
        ):
            with self.subTest(changed=bad_manifest, length=len(bad_rgb)):
                with self.assertRaises(ValueError):
                    validate_completed_backend_frame_v2(bad_manifest, bad_rgb)
        with self.assertRaises(ValueError):
            validate_completed_backend_frame_v2(manifest, bytearray(rgb24))

    def test_selected_identity_types_and_unsourced_fields_reject(self) -> None:
        manifest, rgb24 = fixture()
        changes = (
            ("run_token", "x" * 65),
            ("arm_token", ""),
            ("stream_id", 1),
            ("plane_event_id", "bad space"),
            ("raw_sha256", "A" * 64),
            ("candidate_ordinal", True),
            ("candidate_ordinal", 2305),
            ("pc", "0F2D:FFFF"),
            ("successor_pc", "0F2D:043B"),
        )
        for name, value in changes:
            with self.subTest(name=name, value=value):
                forged = deepcopy(manifest)
                forged["selected_event"][name] = value
                with self.assertRaises(ValueError):
                    parse_completed_backend_frame_v2(wire(forged), rgb24)
        for name in ("frame", "real_start", "palette_sha256", "phase_clock"):
            with self.subTest(unsourced=name):
                forged = {**manifest, name: 1}
                with self.assertRaisesRegex(ValueError, "incomplete or extra"):
                    validate_completed_backend_frame_v2(forged, rgb24)

    def test_manifest_original_byte_bound_and_nested_duplicates(self) -> None:
        manifest, rgb24 = fixture()
        encoded = wire(manifest)
        exact = encoded + b" " * (MAX_MANIFEST_BYTES - len(encoded))
        self.assertEqual(parse_completed_backend_frame_v2(exact, rgb24).status,
                         STATUS)
        for bad in (
            exact + b" ",
            encoded.replace(b'{"version":2', b'{"version":2,"version":2', 1),
            encoded.replace(b'{"run_token":"run_v2"',
                            b'{"run_token":"run_v2","run_token":"run_v2"', 1),
            encoded + b"{}",
            b"\xff" + encoded,
            encoded.replace(b'"version":2', b'"version":NaN', 1),
        ):
            with self.subTest(length=len(bad)):
                with self.assertRaises(ValueError):
                    parse_completed_backend_frame_v2(bad, rgb24)
        with self.assertRaises(ValueError):
            parse_completed_backend_frame_v2(encoded.decode("utf-8"), rgb24)


if __name__ == "__main__":
    unittest.main()
