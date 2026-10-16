"""Synthetic selected-event negatives at the source-only U19 v2 join boundary."""

import json
import unittest

from tests.replay import test_cold_start_codeword_row14_v2_pair_join as fixtures
from tools.replay.cold_start_codeword_row14_passive_snapshot_v2 import (
    MAX_PACKET_BYTES, parse_capture_v2,
)
from tools.replay.cold_start_codeword_row14_v2_pair_join import (
    IDENTITY_FIELDS, join_pair_v2,
)


class SelectedEventWireNegativeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.full = fixtures.recording()
        cls.prefix = fixtures.recording(
            prefix=True, active=0xB000, draw=0xA000,
        )

    def packets(self, recording):
        producer = fixtures.produce_packet(
            recording.raw_jsonl, recording.manifest, fixtures.SOURCE,
            enabled=True,
        )
        plane = fixtures.plane_record(producer["selected_event"])
        return producer, plane, fixtures.raw(plane)

    def join_wire(self, recording, producer, payload):
        return join_pair_v2(
            recording.raw_jsonl, recording.manifest, fixtures.SOURCE,
            producer, payload, enabled=True,
        )

    def test_equal_duplicate_plane_event_identity_rejects_on_raw_wire(self):
        producer, plane, compact = self.packets(self.full)
        control = self.join_wire(self.full, producer, compact)
        self.assertIs(control.native_coverage_proved, False)
        self.assertEqual(compact[-1:], b"}")
        duplicate = (compact[:-1] + b',"plane_event_id":' +
                     json.dumps(plane["plane_event_id"]).encode("ascii") + b"}")
        self.assertLess(len(duplicate), MAX_PACKET_BYTES)
        self.assertEqual(duplicate.count(b'"plane_event_id":'), 2)
        # A decoder that collapses duplicates sees the unchanged valid object.
        self.assertEqual(json.loads(duplicate), plane)
        with self.assertRaisesRegex(ValueError, "duplicate packet JSON key"):
            self.join_wire(self.full, producer, duplicate)

    def test_coherent_prefix_pair_cannot_replace_full_audited_selection(self):
        full_producer, _, full_raw = self.packets(self.full)
        prefix_producer, prefix_plane, prefix_raw = self.packets(self.prefix)
        full_control = self.join_wire(self.full, full_producer, full_raw)
        prefix_control = self.join_wire(self.prefix, prefix_producer, prefix_raw)
        self.assertEqual(full_control.terminal, "full_return")
        self.assertEqual(prefix_control.terminal, "selected_prefix")
        self.assertEqual(full_control.candidate_ordinal, 897)
        self.assertEqual(prefix_control.candidate_ordinal, 898)
        self.assertIs(full_control.native_coverage_proved, False)
        self.assertIs(prefix_control.native_coverage_proved, False)
        self.assertNotEqual(full_producer["selected_event"],
                            prefix_producer["selected_event"])
        self.assertLess(len(prefix_raw), MAX_PACKET_BYTES)
        parsed = parse_capture_v2(prefix_raw)
        for name in IDENTITY_FIELDS:
            with self.subTest(identity=name):
                selected = prefix_producer["selected_event"][name]
                self.assertEqual(prefix_plane[name], selected)
                self.assertEqual(getattr(parsed, name), selected)

        # Both supplied envelopes agree; the actual full recorder input stays
        # unchanged. Re-auditing that input must reject the foreign selection
        # before the join reaches passive-packet parsing or identity equality.
        with self.assertRaisesRegex(ValueError, "^STOP_V2_MISMATCH$"):
            self.join_wire(self.full, prefix_producer, prefix_raw)


if __name__ == "__main__":
    unittest.main()
