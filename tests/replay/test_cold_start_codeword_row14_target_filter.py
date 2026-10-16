"""Synthetic target-filter tests; no DOSBox-X or game media are used."""

from hashlib import sha256
import unittest

from tools.replay.cold_start_codeword_row14_target_filter import (
    Frame, debugger_features, filter_frames, parse_frame,
)


CALLER = Frame(0x38D3, 0x02D8, 0, 0)
RETURN = Frame(0x38D3, 0x02DD, 0, 0)


class TargetFilterTests(unittest.TestCase):
    def test_debugger_hash_and_feature_gate(self):
        binary = (b"DEBUGBOX MEMDUMPBIN VGA Graphics controller info "
                  b"VGA Sequencer info VGA mode info OUTP BPM")
        digest = sha256(binary).hexdigest()
        self.assertEqual(debugger_features(binary, digest),
                         ("SOURCE_PREFLIGHT_READY_NO_NATIVE", ()))
        self.assertEqual(debugger_features(binary)[0], "STOP_DEBUGGER_SHA")
        modified = binary.replace(b"MEMDUMPBIN", b"NO_MEMDUMP")
        missing = debugger_features(modified, sha256(modified).hexdigest())
        self.assertEqual(missing[0], "STOP_DEBUGGER_FEATURE")
        self.assertIn("MEMDUMPBIN", missing[1])

    def test_reached_witness_after_unrelated_candidate(self):
        frames = [Frame(0x0F2D, 0x0412, 0xA000, 0x118F), CALLER,
                  Frame(0x0F2D, 0x0445, 0xA000, 0x118E),
                  Frame(0x0F2D, 0x0412, 0xA000, 0x118F),
                  Frame(0x0F2D, 0x0415, 0xA000, 0x118F)]
        result = filter_frames(frames)
        self.assertEqual((result.status, result.candidate_hits,
                          result.target_pc, result.next_pc),
                         ("PAIRED_CPU_SOURCE_ONLY", 2, 0x0412, 0x0415))

    def test_return_and_missing_anchor_are_typed(self):
        self.assertEqual(filter_frames([Frame(0x0F2D, 0x0445, 0xA000, 0x118F)]).status,
                         "STOP_NO_U19_CALLER")
        self.assertEqual(filter_frames([CALLER, RETURN]).status,
                         "STOP_PRINTER_RETURN_BEFORE_WITNESS")

    def test_single_step_is_required(self):
        site = Frame(0x0F2D, 0x0451, 0xA000, 0x118F)
        self.assertEqual(filter_frames([CALLER, site]).status,
                         "STOP_MISSING_SINGLE_STEP")
        self.assertEqual(filter_frames([CALLER, site, RETURN]).status,
                         "STOP_WRONG_SINGLE_STEP")
        self.assertEqual(filter_frames(
            [CALLER, site, Frame(0x0F2D, 0x0455, 0xA000, 0x118F)]
        ).status, "PAIRED_CPU_SOURCE_ONLY")

    def test_candidate_hit_cap(self):
        wrong = Frame(0x0F2D, 0x0445, 0xA000, 0x118E)
        self.assertEqual(filter_frames([CALLER, wrong, wrong], 1).status,
                         "STOP_CANDIDATE_HIT_CAP")
        with self.assertRaises(ValueError):
            filter_frames([CALLER], 0)

    def test_strict_hex_frame_parser(self):
        self.assertEqual(parse_frame({"cs": "0F2D", "ip": "0412",
                                      "es": "A000", "di": "118F"}),
                         Frame(0x0F2D, 0x0412, 0xA000, 0x118F))
        for record in ({"cs": "0F2D", "ip": "0412", "es": "A000"},
                       {"cs": "0F2D", "ip": "ZZZZ", "es": "A000", "di": "118F"},
                       {"cs": "0F2D", "ip": "10412", "es": "A000", "di": "118F"},
                       *({"cs": "0F2D", "ip": value, "es": "A000", "di": "118F"}
                         for value in ("+1", " 1", "0x1", "1\n", "１"))):
            with self.assertRaises(ValueError):
                parse_frame(record)


if __name__ == "__main__":
    unittest.main()
