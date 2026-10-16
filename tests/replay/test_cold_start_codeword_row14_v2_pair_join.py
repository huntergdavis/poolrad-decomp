"""Synthetic integration checks for the separate U19 v2 pair join."""

from copy import deepcopy
import json
import unittest

from tools.replay.cold_start_codeword_row14_candidate_coverage_gate import (
    SUCCESSOR, candidate_frames,
)
from tools.replay.cold_start_codeword_row14_selected_pc_recorder import (
    CandidateRecorder,
)
from tools.replay.cold_start_codeword_row14_target_filter import Frame, NEXT_PC
from tools.replay.cold_start_codeword_row14_v2_producer_adapter import (
    produce_packet,
)
from tools.replay.cold_start_codeword_row14_passive_snapshot_v2 import (
    parse_capture_v2,
)
from tools.replay.cold_start_codeword_row14_v2_pair_join import (
    IDENTITY_FIELDS, JOIN_STATUS, join_pair_v2,
)


SOURCE = b"synthetic U19 source bytes\n"
U19 = 0x38D3
PRINTER = 0x0F2D


def recording(*, prefix: bool = False, active: int = 0xA000,
              draw: int = 0xB000):
    rec = CandidateRecorder(
        run_token="run_v2", arm_token="arm_v2", stream_id="stream_v2",
        plane_event_id="plane_v2", source_bytes=SOURCE,
        u19_segment=U19, printer_segment=PRINTER,
        active_segment=active, draw_segment=draw, enabled=True,
    )
    assert rec.push(Frame(U19, 0x02D8, 0, 0)) == "CALLER"
    for frame in candidate_frames(PRINTER, active, draw):
        assert rec.push(frame) == "CANDIDATE"
        if rec.await_successor:
            assert rec.push(Frame(PRINTER, SUCCESSOR[frame.ip],
                                  frame.es, frame.di)) == "SUCCESSOR"
            if prefix:
                break
    if not prefix:
        assert rec.push(Frame(U19, 0x02DD, 0, 0)) == "FULL_RETURN"
    return rec.finish(selected_prefix=prefix)


def stopped_recording(reason: str):
    active, draw = ((0xB000, 0xC000) if reason == "absent" else
                    (0xA000, 0xB000))
    rec = CandidateRecorder(
        run_token="run_v2", arm_token="arm_v2", stream_id="stream_v2",
        plane_event_id="plane_v2", source_bytes=SOURCE,
        u19_segment=U19, printer_segment=PRINTER,
        active_segment=active, draw_segment=draw, enabled=True,
    )
    assert rec.push(Frame(U19, 0x02D8, 0, 0)) == "CALLER"
    if reason == "truncated":
        assert rec.push(next(candidate_frames(PRINTER, active, draw))) == "CANDIDATE"
        return rec.finish()
    for frame in candidate_frames(PRINTER, active, draw):
        assert rec.push(frame) == "CANDIDATE"
        if rec.await_successor:
            assert rec.push(Frame(PRINTER, SUCCESSOR[frame.ip],
                                  frame.es, frame.di)) == "SUCCESSOR"
    if reason == "absent":
        assert rec.push(Frame(U19, 0x02DD, 0, 0)) == "STOP_TARGET_PAGE_ABSENT"
    elif reason == "overflow":
        assert rec.push(Frame(PRINTER, 0x0412, active, 0x118F)) == (
            "STOP_CANDIDATE_OVERFLOW")
    else:
        raise ValueError("unknown stopped-recorder fixture")
    return rec.finish()


def plane_record(selected: dict) -> dict:
    site = int(selected["pc"][5:], 16)
    state = {
        "cs": "0F2D", "ip": f"{site:04X}", "es": "A000", "di": "118F",
        "gfx": "00000000000000000000", "seq": "000000000000",
        "latch": "12345678",
    }
    value = {
        "version": 2, **selected,
        "probe": "host_backing_word", "extent": "one_byte_per_plane",
        "witness": {"x": 122, "y": 112, "segment": "A000",
                    "offset": "118F", "bit": 0x20},
        "host_map": {
            "handler": "uvga_fast", "physical_address": 0xA118F,
            "page_mask": 0x1FFFF, "bank_read_full": 0,
            "memmask": 0x3FFFF, "memsize": 0x40000,
            "seq_memory_mode": 4, "gfx_miscellaneous": 0,
            "compatible_chain4": False, "ignore_odd_even": False,
            "ignore_extended_memory_bit": False, "is_vga_arch": False,
        },
        "handler_index": 0x118F, "write_count": 1,
    }
    value["before"] = {"planes": "00000000",
                       "state_before_copy": deepcopy(state),
                       "state_after_copy": deepcopy(state)}
    value["after"] = {"planes": "00200000",
                      "state_before_copy": deepcopy(state),
                      "state_after_copy": deepcopy(state)}
    assert value["successor_pc"] == f"0F2D:{NEXT_PC[site]:04X}"
    return value


def raw(value: dict) -> bytes:
    return json.dumps(value, separators=(",", ":")).encode("utf-8")


class PairJoinV2Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.full = recording()
        cls.prefix = recording(prefix=True, active=0xB000, draw=0xA000)

    def inputs(self, rec=None):
        rec = self.full if rec is None else rec
        producer = produce_packet(rec.raw_jsonl, rec.manifest, SOURCE,
                                  enabled=True)
        plane = plane_record(producer["selected_event"])
        return rec, producer, plane

    def join(self, rec, producer, plane):
        return join_pair_v2(rec.raw_jsonl, rec.manifest, SOURCE,
                            producer, raw(plane), enabled=True)

    def test_full_and_prefix_remain_distinct_source_only(self):
        for rec, ordinal, terminal, producer_status in (
            (self.full, 897, "full_return",
             "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED"),
            (self.prefix, 898, "selected_prefix",
             "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED"),
        ):
            with self.subTest(terminal=terminal):
                recording, producer, plane = self.inputs(rec)
                result = self.join(recording, producer, plane)
                self.assertEqual((result.version, result.status,
                                  result.producer_status),
                                 (2, JOIN_STATUS, producer_status))
                self.assertEqual((result.candidate_ordinal, result.terminal),
                                 (ordinal, terminal))
                self.assertEqual((result.site, result.successor),
                                 (int(result.pc[5:], 16),
                                  int(result.successor_pc[5:], 16)))
                self.assertEqual(result.source_sha256,
                                 producer["provenance"]["source_sha256"])
                self.assertEqual(result.before_planes, bytes(4))
                self.assertEqual(result.changed_planes, (1,))
                self.assertIs(result.native_coverage_proved, False)

    def test_all_eight_plane_identity_fields_bind_to_audited_selection(self):
        rec, producer, plane = self.inputs()
        changes = {
            "run_token": "arm_v2", "arm_token": "run_v2",
            "stream_id": "other_stream", "plane_event_id": "other_plane",
            "raw_sha256": "0" * 64, "candidate_ordinal": 898,
            "pc": "0F2D:0438", "successor_pc": "0F2D:043B",
        }
        self.assertEqual(tuple(producer["selected_event"]), IDENTITY_FIELDS)
        for name, changed in changes.items():
            with self.subTest(name=name):
                forged = deepcopy(plane)
                forged[name] = changed
                if name == "pc":
                    forged["successor_pc"] = "0F2D:043B"
                    for sample in (forged["before"], forged["after"]):
                        for state in ("state_before_copy", "state_after_copy"):
                            sample[state]["ip"] = "0438"
                with self.assertRaises(ValueError):
                    self.join(rec, producer, forged)
        swapped = deepcopy(plane)
        swapped["run_token"], swapped["arm_token"] = (
            swapped["arm_token"], swapped["run_token"])
        self.assertEqual(parse_capture_v2(raw(swapped)).run_token, "arm_v2")
        with self.assertRaisesRegex(ValueError, "run_token differs"):
            self.join(rec, producer, swapped)

    def test_raw_plane_packet_bounds_duplicates_and_typed_values_reject(self):
        rec, producer, plane = self.inputs()
        payload = raw(plane)
        for bad in (
            payload + b" " * (4097 - len(payload)),
            payload.replace(b'{"version":2', b'{"version":2,"version":2', 1),
            payload + b"{}",
        ):
            with self.subTest(size=len(bad)):
                with self.assertRaises(ValueError):
                    join_pair_v2(rec.raw_jsonl, rec.manifest, SOURCE,
                                 producer, bad, enabled=True)
        for name, bad in (("version", True), ("candidate_ordinal", True),
                          ("candidate_ordinal", 897.0),
                          ("raw_sha256", "A" * 64)):
            with self.subTest(name=name, bad=bad):
                forged = deepcopy(plane)
                forged[name] = bad
                with self.assertRaises(ValueError):
                    self.join(rec, producer, forged)
        for change in (lambda value: value.pop("stream_id"),
                       lambda value: value.update(extra="x")):
            forged = deepcopy(plane)
            change(forged)
            with self.assertRaises(ValueError):
                self.join(rec, producer, forged)

    def test_producer_raw_manifest_source_and_prefix_promotion_reject(self):
        rec, producer, plane = self.inputs(self.prefix)
        promoted = deepcopy(producer)
        promoted["status"] = "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED"
        with self.assertRaises(ValueError):
            self.join(rec, promoted, plane)
        for raw_jsonl, manifest, source_bytes, packet in (
            (rec.raw_jsonl + b" ", rec.manifest, SOURCE, producer),
            (rec.raw_jsonl, rec.manifest, SOURCE + b"changed", producer),
            (rec.raw_jsonl, {**rec.manifest, "frame_count": 2307},
             SOURCE, producer),
            (rec.raw_jsonl, rec.manifest, SOURCE,
             {**producer, "unexpected": 1}),
        ):
            with self.assertRaises(ValueError):
                join_pair_v2(raw_jsonl, manifest, source_bytes,
                             packet, raw(plane), enabled=True)

    def test_stopped_recorder_inputs_cannot_inherit_a_valid_envelope(self):
        _, producer, plane = self.inputs()
        for reason, stop in (("absent", "STOP_TARGET_PAGE_ABSENT"),
                             ("overflow", "STOP_CANDIDATE_OVERFLOW"),
                             ("truncated", "STOP_TRUNCATED")):
            with self.subTest(reason=reason):
                rec = stopped_recording(reason)
                self.assertEqual(rec.manifest["stop_reason"], stop)
                with self.assertRaises(ValueError):
                    self.join(rec, producer, plane)

    def test_exact_opt_in_and_no_plane_delta_are_source_only(self):
        rec, producer, plane = self.inputs()
        for enabled in (False, 1, None, "true"):
            with self.subTest(enabled=enabled):
                with self.assertRaisesRegex(ValueError, "exact opt-in"):
                    join_pair_v2(rec.raw_jsonl, rec.manifest, SOURCE,
                                 producer, raw(plane), enabled=enabled)
        plane["after"]["planes"] = plane["before"]["planes"]
        result = self.join(rec, producer, plane)
        self.assertEqual(result.changed_planes, ())
        self.assertEqual(result.status, JOIN_STATUS)


if __name__ == "__main__":
    unittest.main()
