"""Fail-closed U19 selected-PC return and overflow controls; synthetic only."""

import copy
import unittest

from tools.replay.cold_start_codeword_row14_candidate_coverage_gate import (
    MAX_STREAM_RECORDS, ROUTE_CANDIDATES, SUCCESSOR, candidate_frames,
)
from tools.replay.cold_start_codeword_row14_selected_pc_recorder import (
    CandidateRecorder, audit_recording,
)
from tools.replay.cold_start_codeword_row14_target_filter import Frame


SOURCE = b"synthetic source for negative controls\n"
U19 = 0x38D3
PRINTER = 0x0F2D
CALLER = Frame(U19, 0x02D8, 0, 0)
RETURN = Frame(U19, 0x02DD, 0, 0)


def recorder(active=0xA000, draw=0xB000):
    return CandidateRecorder(
        run_token="run_negative", arm_token="arm_negative",
        stream_id="stream_negative", plane_event_id="plane_negative",
        source_bytes=SOURCE, u19_segment=U19, printer_segment=PRINTER,
        active_segment=active, draw_segment=draw, enabled=True,
    )


class SelectedPcNegativeControls(unittest.TestCase):
    def candidates(self, rec, active, draw, count=ROUTE_CANDIDATES):
        for ordinal, frame in enumerate(candidate_frames(PRINTER, active, draw), 1):
            if ordinal > count:
                break
            self.assertEqual(rec.push(frame), "CANDIDATE")
            if rec.await_successor:
                successor = Frame(PRINTER, SUCCESSOR[frame.ip], frame.es, frame.di)
                self.assertEqual(rec.push(successor), "SUCCESSOR")

    def test_early_return_before_and_after_selected_successor(self):
        for count in (0, 896, 897, 2303):
            with self.subTest(candidate_count=count):
                rec = recorder()
                self.assertEqual(rec.push(CALLER), "CALLER")
                self.candidates(rec, 0xA000, 0xB000, count)
                self.assertEqual(rec.push(RETURN), "STOP_INCOMPLETE_ROUTE")
                recording = rec.finish()
                self.assertIsNone(recording.manifest["selected_event"])
                self.assertEqual(recording.manifest["candidate_count"], count)
                with self.assertRaisesRegex(ValueError, "STOP_TERMINAL"):
                    audit_recording(recording.raw_jsonl, recording.manifest, SOURCE)

    def test_complete_candidate_stream_without_return_is_truncated(self):
        rec = recorder()
        self.assertEqual(rec.push(CALLER), "CALLER")
        self.candidates(rec, 0xA000, 0xB000)
        recording = rec.finish()
        self.assertEqual(recording.manifest["candidate_count"], ROUTE_CANDIDATES)
        self.assertEqual(recording.manifest["frame_count"], MAX_STREAM_RECORDS - 1)
        self.assertIsNone(recording.manifest["selected_event"])
        self.assertEqual(audit_recording(recording.raw_jsonl, recording.manifest,
                                         SOURCE).status, "STOP_TRUNCATED")

    def test_duplicate_return_and_candidate_after_return_fail_closed(self):
        for trailing in (RETURN, Frame(PRINTER, 0x0412, 0xA000, 0x118F)):
            with self.subTest(trailing=trailing):
                rec = recorder()
                self.assertEqual(rec.push(CALLER), "CALLER")
                self.candidates(rec, 0xA000, 0xB000)
                self.assertEqual(rec.push(RETURN), "FULL_RETURN")
                self.assertEqual(rec.push(trailing), "STOP_AFTER_END")
                recording = rec.finish()
                self.assertIsNone(recording.manifest["selected_event"])
                with self.assertRaisesRegex(ValueError, "STOP_TERMINAL"):
                    audit_recording(recording.raw_jsonl, recording.manifest, SOURCE)

    def test_return_after_selected_prefix_invalidates_selection(self):
        rec = recorder()
        self.assertEqual(rec.push(CALLER), "CALLER")
        self.candidates(rec, 0xA000, 0xB000, 897)
        prefix = rec.finish(selected_prefix=True)
        self.assertEqual(audit_recording(prefix.raw_jsonl, prefix.manifest,
                                         SOURCE).status,
                         "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED")
        self.assertEqual(rec.push(RETURN), "STOP_AFTER_END")
        invalid = rec.finish()
        self.assertIsNone(invalid.manifest["selected_event"])
        with self.assertRaisesRegex(ValueError, "STOP_TERMINAL"):
            audit_recording(invalid.raw_jsonl, invalid.manifest, SOURCE)

    def test_candidate_2305_overflow_on_either_page_order(self):
        for active, draw, selected in ((0xA000, 0xB000, 897),
                                       (0xB000, 0xA000, 898)):
            with self.subTest(selected=selected):
                rec = recorder(active, draw)
                self.assertEqual(rec.push(CALLER), "CALLER")
                self.candidates(rec, active, draw)
                self.assertEqual(rec.selected_ordinal, selected)
                extra = Frame(PRINTER, 0x0412, active, 0x118F)
                self.assertEqual(rec.push(extra), "STOP_CANDIDATE_OVERFLOW")
                recording = rec.finish()
                self.assertEqual(recording.manifest["candidate_count"],
                                 ROUTE_CANDIDATES + 1)
                self.assertEqual(recording.manifest["frame_count"],
                                 MAX_STREAM_RECORDS - 1)
                self.assertIsNone(recording.manifest["selected_event"])
                self.assertEqual(audit_recording(recording.raw_jsonl,
                                                 recording.manifest, SOURCE).status,
                                 "STOP_CANDIDATE_OVERFLOW")
                forged = copy.deepcopy(recording.manifest)
                forged["candidate_count"] = ROUTE_CANDIDATES
                with self.assertRaisesRegex(ValueError, "STOP_OVERFLOW_MANIFEST"):
                    audit_recording(recording.raw_jsonl, forged, SOURCE)


if __name__ == "__main__":
    unittest.main()
