"""Synthetic contracts only: no game bytes, fonts, planes or RGB captures."""

from dataclasses import replace
from hashlib import sha256
import unittest

from tools.replay.u19_visual_witness import (
    CaptureState, PageIdentity, PlaneObservation, PresentedFrame,
    RGB24Observation, RGB24Patch, audit_visual_witness, compare_patch,
)


def cpu_frame(cs, ip, es=0, di=0):
    return dict(zip(("cs", "ip", "es", "di"),
                    (f"{n:04X}" for n in (cs, ip, es, di))))


class VisualWitnessTests(unittest.TestCase):
    def setUp(self):
        self.cpu = dict(version=1, run_token="synthetic_u19", frames=[
            cpu_frame(0x38D3, 0x02D8),
            cpu_frame(0x0F2D, 0x0412, 0xA000, 0x118F),
            cpu_frame(0x0F2D, 0x0415, 0xA000, 0x118F),
        ])
        self.state = CaptureState(0x12345678, 15, 0, 2, 15, 4)
        self.page = PageIdentity(0, 0, 0, sha256(b"synthetic palette").hexdigest())
        empty = (bytes(320),) * 4
        plane1 = bytearray(320)
        plane1[15] = 0x20
        planes = (empty[0], bytes(plane1), empty[2], empty[3])
        self.before = PlaneObservation("synthetic_u19", 10, "0F2D:0412",
                                       empty, self.state, self.state, self.page)
        self.after = PlaneObservation("synthetic_u19", 11, "0F2D:0415",
                                      planes, self.state, self.state, self.page)
        self.frame = PresentedFrame("synthetic_u19", 7, 12, 13, 11, self.page, True)
        self.rgb = RGB24Observation("synthetic_u19", 14, 7, self.page, 2, 2, 8,
                                    bytes(range(16)), self.state, self.state)

    def audit(self, **changes):
        values = dict(cpu_doc=self.cpu, before=self.before, after=self.after,
                      frame=self.frame, rgb=self.rgb)
        values.update(changes)
        return audit_visual_witness(**values)

    def test_equal_page_redraw_requires_and_accepts_completed_frame(self):
        self.assertEqual(self.before.page, self.after.page)
        witness = self.audit()
        self.assertEqual(witness.changed_planes, (1,))
        self.assertEqual(witness.changed_witness_bits, (1,))
        self.assertEqual(witness.rgb_sha256, sha256(self.rgb.pixels).hexdigest())
        with self.assertRaisesRegex(ValueError, "backend completion"):
            self.audit(frame=replace(self.frame, backend_completed=False))

    def test_each_plane_frame_and_rgb_record_must_share_cpu_run(self):
        for name in ("before", "after", "frame", "rgb"):
            with self.subTest(record=name):
                with self.assertRaisesRegex(ValueError, "same-run token"):
                    self.audit(**{name: replace(getattr(self, name), run_token="old_run")})

    def test_observation_cannot_change_latch_or_any_retained_controller_field(self):
        for name in ("before", "after", "rgb"):
            for field, value in (("latch", 0), ("gc_index", 4), ("read_map", 1),
                                 ("seq_index", 3), ("map_mask", 3), ("memory_mode", 6)):
                with self.subTest(record=name, field=field):
                    sample = replace(getattr(self, name),
                                     state_after=replace(self.state, **{field: value}))
                    with self.assertRaisesRegex(ValueError, "changed latch or controller"):
                        self.audit(**{name: sample})

    def test_guest_can_change_latch_between_passive_observations(self):
        next_state = replace(self.state, latch=0x98765432)
        self.audit(after=replace(self.after, state_before=next_state,
                                 state_after=next_state))

    def test_debugger_memory_reads_and_unknown_or_failed_reads_rejected(self):
        for change in (dict(method="MEMDUMPBIN"), dict(read_failures=1),
                       dict(read_failures=None), dict(read_failures=False)):
            with self.subTest(change=change):
                with self.assertRaisesRegex(ValueError, "nonperturbing successful"):
                    self.audit(before=replace(self.before, **change))

    def test_plane_count_length_and_selected_pc_are_required(self):
        for change in (dict(planes=self.before.planes[:3]),
                       dict(planes=(b"short",) * 4)):
            with self.assertRaisesRegex(ValueError, "four complete plane"):
                self.audit(before=replace(self.before, **change))
        for name, pc in (("before", "38D3:02D8"), ("after", "0F2D:043B")):
            with self.assertRaisesRegex(ValueError, "selected store and successor"):
                self.audit(**{name: replace(getattr(self, name), pc=pc)})
        cpu = dict(self.cpu, frames=self.cpu["frames"][:-1])
        with self.assertRaisesRegex(ValueError, "STOP_MISSING_SINGLE_STEP"):
            self.audit(cpu_doc=cpu)

    def test_preassignment_old_frame_stale_backend_and_rgb_order_rejected(self):
        for change in (dict(start_event=9), dict(backend_event=12),
                       dict(backend_event=15)):
            with self.subTest(change=change):
                with self.assertRaisesRegex(ValueError, "events out of order"):
                    self.audit(frame=replace(self.frame, **change))
        with self.assertRaisesRegex(ValueError, "events out of order"):
            self.audit(rgb=replace(self.rgb, event_index=13))

    def test_plane_frame_and_rgb_identity_cannot_be_substituted(self):
        for change in (dict(plane_event=10), dict(frame_id=8)):
            with self.assertRaisesRegex(ValueError, "identity mismatch"):
                self.audit(frame=replace(self.frame, **change))

    def test_page_fields_and_palette_must_match_frame_and_rgb(self):
        for name in ("after", "frame", "rgb"):
            for field, value in (("crtc_start", 1), ("display_start", 1),
                                 ("real_start", 1), ("palette_sha256", "a" * 64)):
                with self.subTest(record=name, field=field):
                    page = replace(self.page, **{field: value})
                    with self.assertRaisesRegex(ValueError, "page or palette mismatch"):
                        self.audit(**{name: replace(getattr(self, name), page=page)})

    def test_unknown_and_malformed_state_and_rgb_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "missing passive"):
            self.audit(before=replace(self.before, state_before=None))
        for field, value in (("latch", -1), ("read_map", 4), ("latch", True)):
            invalid = replace(self.state, **{field: value})
            with self.assertRaisesRegex(ValueError, "invalid passive"):
                self.audit(before=replace(self.before, state_before=invalid,
                                          state_after=invalid))
        baseline = replace(self.state, read_map=1)
        with self.assertRaisesRegex(ValueError, "invalid passive"):
            self.audit(before=replace(self.before, state_before=baseline,
                                      state_after=replace(baseline, read_map=True)))
        for change in (dict(width=0), dict(row_stride=5), dict(pixels=b"short")):
            with self.assertRaisesRegex(ValueError, "invalid RGB24"):
                self.audit(rgb=replace(self.rgb, **change))
        with self.assertRaisesRegex(ValueError, "invalid event or frame"):
            self.audit(rgb=replace(self.rgb, event_index=True))

    def test_crop_uses_rgb_order_stride_and_pixel_mismatch_count(self):
        patch = RGB24Patch(1, 0, 1, 2, bytes((3, 4, 5, 11, 12, 13)))
        self.assertEqual(compare_patch(self.rgb, patch), 0)
        self.assertEqual(compare_patch(self.rgb,
                                       replace(patch, pixels=bytes((0, 0, 0, 0, 0, 0)))), 2)
        for change in (dict(x=-1), dict(x=2), dict(y=2), dict(width=0),
                       dict(pixels=b"short")):
            with self.assertRaisesRegex(ValueError, "outside host bounds"):
                compare_patch(self.rgb, replace(patch, **change))

    def test_synthetic_row14_crop_geometry_without_original_pixels(self):
        pixels = bytes((17, 31, 47)) * (640 * 400)
        host = replace(self.rgb, width=640, height=400, row_stride=1920,
                       pixels=pixels)
        patch = RGB24Patch(16, 224, 576, 16, bytes((17, 31, 47)) * (576 * 16))
        self.assertEqual(len(patch.pixels), 27648)
        self.assertEqual(compare_patch(host, patch), 0)


if __name__ == "__main__":
    unittest.main()
