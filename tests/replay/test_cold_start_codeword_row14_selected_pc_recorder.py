"""Synthetic source-only tests for the opt-in 2,304-candidate collector."""

import copy
import unittest

from tools.replay.cold_start_codeword_row14_candidate_coverage_gate import (
    MAX_STREAM_RECORDS, ROUTE_CANDIDATES, SUCCESSOR, candidate_frames,
)
from tools.replay.cold_start_codeword_row14_selected_pc_recorder import (
    CandidateRecorder, audit_recording,
)
from tools.replay.cold_start_codeword_row14_target_filter import (
    Frame, filter_frames,
)


SOURCE = b"synthetic debugger source bytes\r\n"
U19 = 0x38D3
PRINTER = 0x0F2D
CALLER = Frame(U19, 0x02D8, 0, 0)
RETURN = Frame(U19, 0x02DD, 0, 0)


def recorder(active=0xA000, draw=0xB000, enabled=True):
    return CandidateRecorder(
        run_token="run_1", arm_token="arm_1", stream_id="stream_1",
        plane_event_id="plane_1", source_bytes=SOURCE,
        u19_segment=U19, printer_segment=PRINTER,
        active_segment=active, draw_segment=draw, enabled=enabled,
    )


def capture(active=0xA000, draw=0xB000, *, prefix=False):
    rec = recorder(active, draw)
    assert rec.push(CALLER) == "CALLER"
    for frame in candidate_frames(PRINTER, active, draw):
        assert rec.push(frame) == "CANDIDATE"
        if rec.await_successor:
            successor = Frame(PRINTER, SUCCESSOR[frame.ip], frame.es, frame.di)
            assert rec.push(successor) == "SUCCESSOR"
            if prefix:
                break
    if not prefix:
        assert rec.push(RETURN) == "FULL_RETURN"
    return rec.finish(selected_prefix=prefix)


class CandidateRecorderTests(unittest.TestCase):
    def test_default_off_and_legacy_cap_unchanged(self):
        rec = recorder(enabled=False)
        self.assertEqual(rec.push(CALLER), "DISABLED")
        self.assertEqual(rec.frames, [])
        with self.assertRaisesRegex(ValueError, "disabled"):
            rec.finish()
        with self.assertRaisesRegex(ValueError, "1..128"):
            filter_frames([], max_candidate_hits=ROUTE_CANDIDATES)
        for value in ("false", 1, [], None):
            with self.subTest(enabled=value):
                with self.assertRaisesRegex(ValueError, "exact bool"):
                    recorder(enabled=value)

    def test_no_a000_target_is_typed_stop(self):
        rec = recorder(active=0xB000, draw=0xC000)
        self.assertEqual(rec.push(CALLER), "CALLER")
        for frame in candidate_frames(PRINTER, 0xB000, 0xC000):
            self.assertEqual(rec.push(frame), "CANDIDATE")
        self.assertEqual(rec.push(RETURN), "STOP_TARGET_PAGE_ABSENT")
        recording = rec.finish()
        self.assertEqual(recording.manifest["terminal"], "stopped")
        self.assertIsNone(recording.manifest["selected_event"])
        self.assertEqual(audit_recording(recording.raw_jsonl, recording.manifest,
                                         SOURCE).status, "STOP_TARGET_PAGE_ABSENT")

    def test_full_first_page_target_897(self):
        recording = capture()
        manifest = recording.manifest
        self.assertEqual((manifest["frame_count"], manifest["candidate_count"]),
                         (MAX_STREAM_RECORDS, ROUTE_CANDIDATES))
        self.assertEqual(manifest["selected_event"]["candidate_ordinal"], 897)
        self.assertEqual(manifest["selected_event"]["plane_event_id"], "plane_1")
        self.assertEqual(manifest["terminal"], "full_return")
        self.assertIs(manifest["native_coverage_proved"], False)
        result = audit_recording(recording.raw_jsonl, manifest, SOURCE)
        self.assertEqual((result.status, result.candidate_count,
                          result.selected_candidate_ordinal),
                         ("SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED", 2304, 897))
        self.assertIs(result.native_coverage_proved, False)

    def test_second_page_target_898_and_prefix_is_not_full(self):
        recording = capture(active=0xB000, draw=0xA000, prefix=True)
        self.assertEqual(recording.manifest["selected_event"]["candidate_ordinal"], 898)
        self.assertEqual(recording.manifest["terminal"], "selected_prefix")
        self.assertLess(recording.manifest["candidate_count"], ROUTE_CANDIDATES)
        result = audit_recording(recording.raw_jsonl, recording.manifest, SOURCE)
        self.assertEqual(result.status, "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED")
        self.assertEqual(result.selected_candidate_ordinal, 898)

    def test_prefix_requires_exact_bool_without_sealing_valid_prefix(self):
        rec = recorder()
        self.assertEqual(rec.push(CALLER), "CALLER")
        for frame in candidate_frames(PRINTER, 0xA000, 0xB000):
            self.assertEqual(rec.push(frame), "CANDIDATE")
            if rec.await_successor:
                self.assertEqual(rec.push(Frame(PRINTER, SUCCESSOR[frame.ip],
                                                frame.es, frame.di)), "SUCCESSOR")
                break
        for value in (1, 0, "false", None):
            with self.subTest(selected_prefix=value):
                with self.assertRaisesRegex(ValueError, "exact bool"):
                    rec.finish(selected_prefix=value)
                self.assertIsNone(rec.terminal)
        recording = rec.finish(selected_prefix=True)
        self.assertEqual(recording.manifest["terminal"], "selected_prefix")
        self.assertEqual(recording.manifest["selected_event"]["candidate_ordinal"], 897)
        self.assertEqual(audit_recording(recording.raw_jsonl, recording.manifest,
                                         SOURCE).status,
                         "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED")

    def test_full_return_cannot_be_downgraded_to_prefix(self):
        rec = recorder(active=0xB000, draw=0xA000)
        self.assertEqual(rec.push(CALLER), "CALLER")
        for frame in candidate_frames(PRINTER, 0xB000, 0xA000):
            self.assertEqual(rec.push(frame), "CANDIDATE")
            if rec.await_successor:
                self.assertEqual(rec.push(Frame(PRINTER, SUCCESSOR[frame.ip],
                                                frame.es, frame.di)), "SUCCESSOR")
        self.assertEqual(rec.push(RETURN), "FULL_RETURN")
        recording = rec.finish(selected_prefix=True)
        self.assertEqual(recording.manifest["terminal"], "full_return")
        self.assertEqual(recording.manifest["candidate_count"], ROUTE_CANDIDATES)
        self.assertEqual(recording.manifest["selected_event"]["candidate_ordinal"], 898)
        self.assertEqual(audit_recording(recording.raw_jsonl, recording.manifest,
                                         SOURCE).status,
                         "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED")

    def test_candidate_2305_fails_closed_without_frame_2308(self):
        rec = recorder()
        self.assertEqual(rec.push(CALLER), "CALLER")
        for frame in candidate_frames(PRINTER, 0xA000, 0xB000):
            self.assertEqual(rec.push(frame), "CANDIDATE")
            if rec.await_successor:
                self.assertEqual(rec.push(Frame(PRINTER, SUCCESSOR[frame.ip],
                                                frame.es, frame.di)), "SUCCESSOR")
        self.assertEqual(rec.push(Frame(PRINTER, 0x0412, 0xA000, 0x118F)),
                         "STOP_CANDIDATE_OVERFLOW")
        recording = rec.finish()
        self.assertEqual(recording.manifest["candidate_count"], 2305)
        self.assertEqual(recording.manifest["frame_count"], 2306)
        self.assertTrue(recording.manifest["overflow"])
        self.assertIsNone(recording.manifest["selected_event"])
        self.assertEqual(audit_recording(recording.raw_jsonl, recording.manifest,
                                         SOURCE).status, "STOP_CANDIDATE_OVERFLOW")

    def test_truncation_wrong_successor_and_after_end(self):
        rec = recorder()
        rec.push(CALLER)
        rec.push(next(candidate_frames(PRINTER, 0xA000, 0xB000)))
        recording = rec.finish()
        self.assertEqual(recording.manifest["stop_reason"], "STOP_TRUNCATED")
        self.assertIsNone(recording.manifest["selected_event"])
        self.assertEqual(audit_recording(recording.raw_jsonl, recording.manifest,
                                         SOURCE).status, "STOP_TRUNCATED")
        forged = copy.deepcopy(recording.manifest)
        forged.update(candidate_count=2305, overflow=True, truncated=False,
                      stop_reason="STOP_CANDIDATE_OVERFLOW")
        with self.assertRaisesRegex(ValueError, "STOP_OVERFLOW_PREFIX"):
            audit_recording(recording.raw_jsonl, forged, SOURCE)

        selected = recorder()
        selected.push(CALLER)
        for frame in candidate_frames(PRINTER, 0xA000, 0xB000):
            selected.push(frame)
            if selected.await_successor:
                break
        self.assertEqual(selected.push(Frame(PRINTER, 0x0415, 0xA000, 0x1190)),
                         "STOP_WRONG_SINGLE_STEP")
        self.assertIsNone(selected.finish().manifest["selected_event"])

        completed = recorder()
        completed.push(CALLER)
        for frame in candidate_frames(PRINTER, 0xA000, 0xB000):
            completed.push(frame)
            if completed.await_successor:
                completed.push(Frame(PRINTER, SUCCESSOR[frame.ip], frame.es,
                                     frame.di))
        completed.push(RETURN)
        self.assertEqual(completed.push(RETURN), "STOP_AFTER_END")

    def test_manifest_and_raw_tampering_fail(self):
        recording = capture(prefix=True)
        raw, manifest = recording.raw_jsonl, recording.manifest
        tampered = copy.deepcopy(manifest)
        tampered["selected_event"]["candidate_ordinal"] = 2304
        with self.assertRaisesRegex(ValueError, "STOP_MANIFEST_MISMATCH"):
            audit_recording(raw, tampered, SOURCE)
        for value in (897.0, True):
            tampered = copy.deepcopy(manifest)
            tampered["selected_event"]["candidate_ordinal"] = value
            with self.subTest(candidate_ordinal=value):
                with self.assertRaisesRegex(ValueError, "STOP_SELECTED_EVENT_SCHEMA"):
                    audit_recording(raw, tampered, SOURCE)
        for value in (True, 1.0):
            tampered = copy.deepcopy(manifest)
            tampered["version"] = value
            with self.subTest(version=value):
                with self.assertRaisesRegex(ValueError, "STOP_MANIFEST_VERSION_OR_TOKEN"):
                    audit_recording(raw, tampered, SOURCE)
        tampered = copy.deepcopy(manifest)
        tampered["selected_event"] = None
        with self.assertRaisesRegex(ValueError, "STOP_SELECTED_EVENT_MISSING"):
            audit_recording(raw, tampered, SOURCE)
        tampered = copy.deepcopy(manifest)
        tampered["selected_event"]["unreviewed"] = "value"
        with self.assertRaisesRegex(ValueError, "STOP_SELECTED_EVENT_SCHEMA"):
            audit_recording(raw, tampered, SOURCE)
        with self.assertRaisesRegex(ValueError, "STOP_SOURCE_SHA"):
            audit_recording(raw, manifest, SOURCE + b"changed")
        tampered = copy.deepcopy(manifest)
        tampered["source_sha256"] = 1
        with self.assertRaisesRegex(ValueError, "STOP_SOURCE_SHA"):
            audit_recording(raw, tampered, SOURCE)
        tampered = copy.deepcopy(manifest)
        tampered["segments"]["active"] = object()
        with self.assertRaisesRegex(ValueError, "STOP_MANIFEST_SHAPE"):
            audit_recording(raw, tampered, SOURCE)
        with self.assertRaisesRegex(ValueError, "STOP_RAW_SHA"):
            audit_recording(raw + b" ", manifest, SOURCE)
        tampered = copy.deepcopy(manifest)
        tampered["frame_count"] += 1
        with self.assertRaisesRegex(ValueError, "STOP_FRAME_COUNT"):
            audit_recording(raw, tampered, SOURCE)
        tampered = copy.deepcopy(manifest)
        tampered["native_coverage_proved"] = True
        with self.assertRaisesRegex(ValueError, "STOP_MANIFEST_VERSION_OR_TOKEN"):
            audit_recording(raw, tampered, SOURCE)


if __name__ == "__main__":
    unittest.main()
