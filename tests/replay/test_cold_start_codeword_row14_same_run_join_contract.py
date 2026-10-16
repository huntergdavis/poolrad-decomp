"""Synthetic U19 source join; no private capture, emulator, or game bytes."""

from dataclasses import replace
from types import SimpleNamespace
import unittest

from tools.replay.cold_start_codeword_row14_event_plan import EventGate
from tools.replay.cold_start_codeword_row14_plane_event_audit import Change
from tools.replay.cold_start_codeword_row14_route_join import RouteGate
from tools.replay.cold_start_codeword_row14_same_run_join_contract import (
    ExpectedRgbPatch, join_source_contract,
)
from tools.replay.cold_start_codeword_row14_target_filter import FilterResult


def fixture():
    selected = FilterResult("PAIRED_CPU_SOURCE_ONLY", 7, 0x0445, 0x0449)
    route = RouteGate("run_42", EventGate(selected, Change(1, "0F2D:0445", (), ())), 12)
    host = SimpleNamespace(
        run="run_42", plane_event_id="store_7", arm_token="arm_7",
        store_ordinal=7,
        drawer_offset=0x0445, frame=3, real_start=0, same_page=True,
        backend="software", backend_event_id="backend_3",
        rgb_sha256="a" * 64, palette_sha256="b" * 64,
        palette_generation=2, rgb_width=640, rgb_height=400,
    )
    plane = SimpleNamespace(
        run_token="run_42", plane_event_id="store_7", arm_token="arm_7",
        store_ordinal=7,
        site=0x0445, successor=0x0449, backing_index=0x118F,
        before_planes=bytes((1, 2, 3, 4)),
        after_planes=bytes((1, 2, 3, 4)),
    )
    expected = ExpectedRgbPatch(bytes(27648), "c" * 64, "d" * 64)
    return route, host, plane, expected


class SameRunJoinTests(unittest.TestCase):
    def test_equal_page_and_unchanged_planes_are_structural_only(self):
        route, host, plane, expected = fixture()
        joined = join_source_contract(route, host, plane, expected)
        self.assertEqual((joined.store_ordinal, joined.site, joined.successor),
                         (7, 0x0445, 0x0449))
        self.assertEqual(joined.arm_token, "arm_7")
        self.assertTrue(joined.same_page)
        self.assertEqual(joined.status,
                         "SOURCE_ONLY_JOIN_NO_PIXEL_OR_WRITE_ASSERTION")

    def test_rejects_cross_run_or_different_selected_event(self):
        route, host, plane, expected = fixture()
        plane.run_token = "run_43"
        with self.assertRaisesRegex(ValueError, "key"):
            join_source_contract(route, host, plane, expected)
        plane.run_token = "run_42"
        plane.plane_event_id = "store_8"
        with self.assertRaisesRegex(ValueError, "key"):
            join_source_contract(route, host, plane, expected)
        plane.plane_event_id = "store_7"
        plane.arm_token = "arm_8"
        with self.assertRaisesRegex(ValueError, "key"):
            join_source_contract(route, host, plane, expected)
        plane.arm_token = "arm_7"
        host.store_ordinal = 0
        with self.assertRaisesRegex(ValueError, "key"):
            join_source_contract(route, host, plane, expected)
        host.store_ordinal = 7
        host.arm_token = ""
        with self.assertRaisesRegex(ValueError, "key"):
            join_source_contract(route, host, plane, expected)

    def test_rejects_wrong_drawer_successor_and_noncanonical_rgb(self):
        route, host, plane, expected = fixture()
        plane.successor = 0x0455
        with self.assertRaisesRegex(ValueError, "address"):
            join_source_contract(route, host, plane, expected)
        plane.successor = 0x0449
        host.rgb_width = 576
        with self.assertRaisesRegex(ValueError, "canonical"):
            join_source_contract(route, host, plane, expected)

    def test_rejects_missing_palette_or_wrong_expected_patch(self):
        route, host, plane, expected = fixture()
        host.palette_sha256 = ""
        with self.assertRaisesRegex(ValueError, "canonical"):
            join_source_contract(route, host, plane, expected)
        host.palette_sha256 = "b" * 64
        with self.assertRaisesRegex(ValueError, "patch"):
            join_source_contract(route, host, plane,
                                 replace(expected, rgb24=bytes(27647)))


if __name__ == "__main__":
    unittest.main()
