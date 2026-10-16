"""A later supplied plane receipt cannot replace the audited U19 selection."""

from copy import deepcopy
import unittest

from tests.replay import test_cold_start_codeword_row14_v2_pair_join as fixtures
from tools.replay.cold_start_codeword_row14_passive_snapshot_v2 import parse_capture_v2
from tools.replay.cold_start_codeword_row14_v2_pair_join import (
    IDENTITY_FIELDS, JOIN_STATUS, join_pair_v2,
)


class LaterReselectionTests(unittest.TestCase):
    def test_coherent_later_pair_rejects_against_unchanged_actual_stream(self):
        recording = fixtures.recording()
        producer = fixtures.produce_packet(
            recording.raw_jsonl, recording.manifest, fixtures.SOURCE,
            enabled=True,
        )

        def join(packet, plane_wire):
            return join_pair_v2(
                recording.raw_jsonl, recording.manifest, fixtures.SOURCE,
                packet, plane_wire, enabled=True,
            )

        canonical_plane = fixtures.plane_record(producer["selected_event"])
        control = join(producer, fixtures.raw(canonical_plane))
        self.assertEqual((control.candidate_ordinal, control.pc,
                          control.successor_pc),
                         (897, "0F2D:0445", "0F2D:0449"))
        self.assertEqual(control.status, JOIN_STATUS)
        self.assertIs(control.native_coverage_proved, False)

        later = deepcopy(producer)
        later["selected_event"].update(
            candidate_ordinal=899, pc="0F2D:0412", successor_pc="0F2D:0415",
        )
        later_plane = fixtures.plane_record(later["selected_event"])
        later_wire = fixtures.raw(later_plane)
        parsed = parse_capture_v2(later_wire)
        self.assertEqual((parsed.candidate_ordinal, parsed.site, parsed.successor),
                         (899, 0x0412, 0x0415))
        self.assertEqual(later["provenance"], producer["provenance"])
        self.assertEqual(later["status"], producer["status"])
        self.assertEqual(later["version"], producer["version"])
        for name in IDENTITY_FIELDS:
            with self.subTest(identity=name):
                self.assertEqual(getattr(parsed, name),
                                 later["selected_event"][name])
                if name not in ("candidate_ordinal", "pc", "successor_pc"):
                    self.assertEqual(later["selected_event"][name],
                                     producer["selected_event"][name])
        self.assertEqual(producer["selected_event"],
                         recording.manifest["selected_event"])

        # The two supplied envelopes agree, including the original raw hash.
        # Re-auditing the unchanged actual stream must still select ordinal 897.
        # A receipt at 899 carries no first-change or native-state assertion.
        with self.assertRaisesRegex(ValueError, "^STOP_V2_MISMATCH$"):
            join(later, later_wire)


if __name__ == "__main__":
    unittest.main()
