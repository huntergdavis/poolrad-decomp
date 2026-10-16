"""Cross-packet U19 v2 selected-event association checks."""

from copy import deepcopy
from hashlib import sha256
import unittest

from tests.replay.test_cold_start_codeword_row14_completed_backend_rgb24_v2 import (
    fixture as rgb_fixture, wire as rgb_wire,
)
from tests.replay.test_cold_start_codeword_row14_v2_pair_join import (
    SOURCE, plane_record, raw as plane_wire, recording,
)
from tools.replay.cold_start_codeword_row14_completed_backend_rgb24_v2 import (
    STATUS as RGB_STATUS, parse_completed_backend_frame_v2,
)
from tools.replay.cold_start_codeword_row14_v2_pair_join import (
    JOIN_STATUS as PAIR_STATUS,
)
from tools.replay.cold_start_codeword_row14_v2_producer_adapter import (
    produce_packet,
)
from tools.replay.cold_start_codeword_row14_v2_rgb_association import (
    ASSOCIATION_STATUS, associate_pair_with_supplied_rgb_v2,
)


class SuppliedRgbAssociationV2Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rec = recording()
        cls.producer = produce_packet(cls.rec.raw_jsonl, cls.rec.manifest,
                                      SOURCE, enabled=True)
        cls.plane_raw = plane_wire(plane_record(cls.producer["selected_event"]))
        cls.rgb_manifest, cls.rgb24 = rgb_fixture()
        cls.rgb_manifest["selected_event"] = dict(cls.producer["selected_event"])

    def associate(self, manifest=None, *, enabled=True):
        if manifest is None:
            manifest = self.rgb_manifest
        return associate_pair_with_supplied_rgb_v2(
            self.rec.raw_jsonl, self.rec.manifest, SOURCE, self.producer,
            self.plane_raw, rgb_wire(manifest), self.rgb24, enabled=enabled,
        )

    def test_audited_pair_binds_valid_supplied_rgb_without_promoting_it(self):
        result = self.associate()
        self.assertEqual(result.status, ASSOCIATION_STATUS)
        self.assertEqual((result.pair_status, result.rgb_status,
                          result.producer_status),
                         (PAIR_STATUS, RGB_STATUS,
                          "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED"))
        self.assertEqual((result.plane_event_id, result.backend_event_id),
                         ("plane_v2", "backend_v2"))
        self.assertNotEqual(result.plane_event_id, result.backend_event_id)
        self.assertEqual(result.rgb_sha256, sha256(self.rgb24).hexdigest())
        self.assertEqual(result.rgb.rgb24, self.rgb24)
        self.assertIs(result.native_coverage_proved, False)
        self.assertIs(result.pair.native_coverage_proved, False)

    def test_standalone_valid_label_changes_fail_cross_packet_association(self):
        changes = (
            {"run_token": "other_run"},
            {"arm_token": "other_arm"},
            {"stream_id": "other_stream"},
            {"plane_event_id": "other_plane"},
            {"raw_sha256": "0" * 64},
            {"candidate_ordinal": 898},
            # A valid successor is determined by its PC, so change both.
            {"pc": "0F2D:0438", "successor_pc": "0F2D:043B"},
        )
        for changed in changes:
            with self.subTest(changed=changed):
                manifest = deepcopy(self.rgb_manifest)
                manifest["selected_event"].update(changed)
                parsed = parse_completed_backend_frame_v2(
                    rgb_wire(manifest), self.rgb24)
                self.assertEqual(parsed.status, RGB_STATUS)
                with self.assertRaisesRegex(ValueError,
                                            "RGB selected-event .* differs"):
                    self.associate(manifest)

        foreign = recording(prefix=True, active=0xB000, draw=0xA000)
        foreign_producer = produce_packet(
            foreign.raw_jsonl, foreign.manifest, SOURCE, enabled=True)
        manifest = deepcopy(self.rgb_manifest)
        manifest["selected_event"] = dict(foreign_producer["selected_event"])
        self.assertEqual(parse_completed_backend_frame_v2(
            rgb_wire(manifest), self.rgb24).status, RGB_STATUS)
        with self.assertRaisesRegex(ValueError, "RGB selected-event"):
            self.associate(manifest)

    def test_backend_id_remains_a_distinct_unbound_declaration(self):
        manifest = deepcopy(self.rgb_manifest)
        manifest["backend_event_id"] = "another_backend_event"
        result = self.associate(manifest)
        self.assertEqual(result.backend_event_id, "another_backend_event")
        self.assertEqual(result.plane_event_id, "plane_v2")
        self.assertEqual(result.rgb_status, RGB_STATUS)

    def test_exact_opt_in_and_raw_rgb_validation_remain_required(self):
        for enabled in (False, 1, None, "true"):
            with self.subTest(enabled=enabled):
                with self.assertRaisesRegex(ValueError, "exact opt-in"):
                    self.associate(enabled=enabled)
        bad = rgb_wire(self.rgb_manifest).replace(
            b'"run_token":"run_v2"',
            b'"run_token":"run_v2","run_token":"run_v2"', 1)
        with self.assertRaisesRegex(ValueError, "duplicate RGB24 manifest"):
            associate_pair_with_supplied_rgb_v2(
                self.rec.raw_jsonl, self.rec.manifest, SOURCE, self.producer,
                self.plane_raw, bad, self.rgb24, enabled=True,
            )


if __name__ == "__main__":
    unittest.main()
