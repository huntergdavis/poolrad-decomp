"""Synthetic tests for the source-only U19 v2 producer adapter."""

import copy
import json
import unittest

from tools.replay.cold_start_codeword_row14_candidate_coverage_gate import (
    ROUTE_CANDIDATES, SUCCESSOR, candidate_frames,
)
from tools.replay.cold_start_codeword_row14_selected_pc_recorder import (
    CandidateRecorder,
)
from tools.replay.cold_start_codeword_row14_target_filter import Frame
from tools.replay.cold_start_codeword_row14_v2_producer_adapter import (
    MAX_PACKET_BYTES, SELECTED_ORDER, audit_packet, produce_packet,
)


SOURCE = b"synthetic pinned debugger source\n"
U19 = 0x38D3
PRINTER = 0x0F2D
CALLER = Frame(U19, 0x02D8, 0, 0)
RETURN = Frame(U19, 0x02DD, 0, 0)


def capture(active=0xA000, draw=0xB000, *, prefix=False, no_return=False,
            overflow=False):
    rec = CandidateRecorder(
        run_token="run_v2", arm_token="arm_v2", stream_id="stream_v2",
        plane_event_id="plane_v2", source_bytes=SOURCE, u19_segment=U19,
        printer_segment=PRINTER, active_segment=active, draw_segment=draw,
        enabled=True,
    )
    assert rec.push(CALLER) == "CALLER"
    for frame in candidate_frames(PRINTER, active, draw):
        assert rec.push(frame) == "CANDIDATE"
        if rec.await_successor:
            assert rec.push(Frame(PRINTER, SUCCESSOR[frame.ip],
                                  frame.es, frame.di)) == "SUCCESSOR"
            if prefix:
                break
    if overflow:
        assert rec.push(Frame(PRINTER, 0x0412, active, 0x118F)) == (
            "STOP_CANDIDATE_OVERFLOW")
    elif not prefix and not no_return:
        assert rec.push(RETURN) in ("FULL_RETURN", "STOP_TARGET_PAGE_ABSENT")
    return rec.finish(selected_prefix=prefix)


class ProducerAdapterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.full = capture()
        cls.prefix = capture(active=0xB000, draw=0xA000, prefix=True)
        cls.no_target = capture(active=0xB000, draw=0xC000)
        cls.overflow = capture(overflow=True)
        cls.truncated = capture(no_return=True)

    def packet(self, recording=None):
        recording = self.full if recording is None else recording
        return produce_packet(recording.raw_jsonl, recording.manifest,
                              SOURCE, enabled=True)

    def audit(self, packet, recording=None):
        recording = self.full if recording is None else recording
        return audit_packet(recording.raw_jsonl, recording.manifest,
                            SOURCE, packet, enabled=True)

    def test_default_off_and_exact_opt_in(self):
        for enabled in (False, 0, 1, None, "true"):
            with self.subTest(enabled=enabled):
                with self.assertRaisesRegex(ValueError, "STOP_V2_DISABLED"):
                    produce_packet(self.full.raw_jsonl, self.full.manifest,
                                   SOURCE, enabled=enabled)
        with self.assertRaisesRegex(ValueError, "STOP_V2_DISABLED"):
            audit_packet(self.full.raw_jsonl, self.full.manifest, SOURCE, {})

    def test_full_return_exact_selected_identity_and_provenance(self):
        packet = self.packet()
        self.assertEqual(packet["version"], 2)
        self.assertEqual(tuple(packet["selected_event"]), SELECTED_ORDER)
        self.assertEqual(packet["selected_event"]["candidate_ordinal"], 897)
        self.assertEqual(packet["selected_event"], self.full.manifest["selected_event"])
        self.assertEqual(packet["status"],
                         "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED")
        self.assertEqual(packet["provenance"]["terminal"], "full_return")
        self.assertEqual(packet["provenance"]["candidate_count"],
                         ROUTE_CANDIDATES)
        self.assertIs(packet["provenance"]["native_coverage_proved"], False)
        self.assertLessEqual(len(json.dumps(packet, separators=(",", ":")).encode()),
                             MAX_PACKET_BYTES)
        self.assertEqual(self.audit(packet), packet["status"])

    def test_selected_prefix_keeps_ordinal_898_and_prefix_status(self):
        packet = self.packet(self.prefix)
        self.assertEqual(packet["selected_event"]["candidate_ordinal"], 898)
        self.assertEqual(packet["provenance"]["terminal"], "selected_prefix")
        self.assertLess(packet["provenance"]["candidate_count"], ROUTE_CANDIDATES)
        self.assertEqual(self.audit(packet, self.prefix),
                         "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED")
        forged = copy.deepcopy(packet)
        forged["status"] = "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED"
        with self.assertRaisesRegex(ValueError, "STOP_V2_MISMATCH"):
            self.audit(forged, self.prefix)

    def test_v1_target_absent_overflow_and_truncated_cannot_produce_v2(self):
        for recording in (self.no_target, self.overflow, self.truncated):
            with self.subTest(reason=recording.manifest["stop_reason"]):
                with self.assertRaisesRegex(ValueError, "STOP_V1_SELECTION"):
                    self.packet(recording)

    def test_all_eight_identity_fields_are_compared(self):
        packet = self.packet()
        changes = {
            "run_token": "other_run", "arm_token": "other_arm",
            "stream_id": "other_stream", "plane_event_id": "other_plane",
            "raw_sha256": "0" * 64, "candidate_ordinal": 898,
            "pc": "0F2D:0438", "successor_pc": "0F2D:043B",
        }
        for name, value in changes.items():
            with self.subTest(name=name):
                forged = copy.deepcopy(packet)
                forged["selected_event"][name] = value
                with self.assertRaisesRegex(ValueError, "STOP_V2_MISMATCH"):
                    self.audit(forged)

    def test_missing_extra_and_noncanonical_identity_rejected(self):
        packet = self.packet()
        for location in ((), ("selected_event",), ("provenance",)):
            with self.subTest(location=location):
                forged = copy.deepcopy(packet)
                parent = forged if not location else forged[location[0]]
                parent["unexpected"] = 1
                with self.assertRaisesRegex(ValueError, "STOP_V2_.*SHAPE"):
                    self.audit(forged)
                missing = copy.deepcopy(packet)
                parent = missing if not location else missing[location[0]]
                parent.pop(next(iter(parent)))
                with self.assertRaisesRegex(ValueError, "STOP_V2_.*SHAPE"):
                    self.audit(missing)
        for name, value in (
            ("candidate_ordinal", True), ("candidate_ordinal", 897.0),
            ("run_token", "bad token"), ("raw_sha256", "A" * 64),
            ("pc", "0f2d:0412"), ("successor_pc", "0F2D:041")):
            with self.subTest(name=name, value=value):
                forged = copy.deepcopy(packet)
                forged["selected_event"][name] = value
                with self.assertRaisesRegex(ValueError, "STOP_V2_SELECTED_SHAPE"):
                    self.audit(forged)
        for value in (True, 2.0, 1, "2"):
            with self.subTest(version=value):
                forged = copy.deepcopy(packet)
                forged["version"] = value
                with self.assertRaisesRegex(ValueError, "STOP_V2_VERSION"):
                    self.audit(forged)
        forged = copy.deepcopy(packet)
        forged["status"] = []
        with self.assertRaisesRegex(ValueError, "STOP_V2_STATUS"):
            self.audit(forged)
        forged = copy.deepcopy(packet)
        forged["provenance"]["source_domain"] = "x" * MAX_PACKET_BYTES
        with self.assertRaisesRegex(ValueError, "STOP_V2_PACKET_SIZE"):
            self.audit(forged)

    def test_provenance_mismatch_and_type_forgery_rejected(self):
        packet = self.packet()
        changes = {
            "source_domain": "other domain",
            "source_sha256": "0" * 64,
            "raw_sha256": "0" * 64,
            "candidate_count": ROUTE_CANDIDATES - 1,
            "frame_count": packet["provenance"]["frame_count"] - 1,
            "terminal": "selected_prefix",
        }
        for name, value in changes.items():
            with self.subTest(name=name):
                forged = copy.deepcopy(packet)
                forged["provenance"][name] = value
                with self.assertRaisesRegex(ValueError, "STOP_V2_MISMATCH"):
                    self.audit(forged)
        for name, value in (("candidate_count", True),
                            ("frame_count", 2307.0),
                            ("native_coverage_proved", True)):
            with self.subTest(name=name):
                forged = copy.deepcopy(packet)
                forged["provenance"][name] = value
                with self.assertRaisesRegex(ValueError, "STOP_V2_PROVENANCE_SHAPE"):
                    self.audit(forged)

    def test_raw_manifest_and_source_tampering_recomputed_before_v2(self):
        packet = self.packet()
        with self.assertRaisesRegex(ValueError, "STOP_RAW_SHA"):
            audit_packet(self.full.raw_jsonl + b" ", self.full.manifest,
                         SOURCE, packet, enabled=True)
        with self.assertRaisesRegex(ValueError, "STOP_SOURCE_SHA"):
            audit_packet(self.full.raw_jsonl, self.full.manifest,
                         SOURCE + b"changed", packet, enabled=True)
        forged_manifest = copy.deepcopy(self.full.manifest)
        forged_manifest["selected_event"]["candidate_ordinal"] = 898
        with self.assertRaisesRegex(ValueError, "STOP_MANIFEST_MISMATCH"):
            audit_packet(self.full.raw_jsonl, forged_manifest, SOURCE,
                         packet, enabled=True)


if __name__ == "__main__":
    unittest.main()
