"""Synthetic, source-only U19 fixed-row candidate coverage checks."""

from dataclasses import replace
import json
from pathlib import Path
import tempfile
import unittest

from tools.replay.cold_start_codeword_row14_candidate_coverage_gate import (
    FIRST_TARGET_LOWER_BOUND, LEGACY_CAP, MAX_STREAM_RECORDS,
    ROUTE_CANDIDATES, check_candidate_stream, read_jsonl_frames,
)
from tools.replay.cold_start_codeword_row14_target_filter import Frame


U19 = 0x38D3
PRINTER = 0x0F2D
PAGE = 0xA000


def fixture(active=PAGE, draw=PAGE):
    frames = [Frame(U19, 0x02D8, 0, 0)]
    inserted_successor = False
    for cell in range(1, 37):
        for row in range(8):
            for plane in range(4):
                for page, segment in enumerate((active, draw)):
                    glyph = plane == 1 or plane == 3
                    pc = (0x0412, 0x0438)[page] if glyph else (0x0445, 0x0451)[page]
                    offset = 0x1180 + cell + 40 * row
                    frames.append(Frame(PRINTER, pc, segment, offset))
                    if not inserted_successor and segment == PAGE and offset == 0x118F:
                        frames.append(Frame(PRINTER, {0x0445: 0x0449,
                                                      0x0451: 0x0455}[pc],
                                            segment, offset))
                        inserted_successor = True
    frames.append(Frame(U19, 0x02DD, 0, 0))
    return frames


def check(frames, active=PAGE, draw=PAGE, cap=ROUTE_CANDIDATES):
    return check_candidate_stream(frames, U19, PRINTER, active, draw, cap)


class CandidateCoverageTests(unittest.TestCase):
    def test_full_conditional_route_is_only_source_shape(self):
        frames = fixture()
        self.assertEqual((len(frames), ROUTE_CANDIDATES,
                          FIRST_TARGET_LOWER_BOUND, LEGACY_CAP),
                         (2307, 2304, 897, 128))
        self.assertEqual(frames[1], Frame(PRINTER, 0x0445, PAGE, 0x1181))
        self.assertEqual(frames[897], Frame(PRINTER, 0x0445, PAGE, 0x118F))
        self.assertEqual(frames[898], Frame(PRINTER, 0x0449, PAGE, 0x118F))
        result = check(iter(frames))
        self.assertEqual((result.status, result.candidate_hits,
                          result.selected_ordinal, result.selected_pc),
                         ("SOURCE_ONLY_SHAPE_UNAUTHENTICATED", 2304, 897, 0x0445))
        self.assertFalse(result.native_coverage_proved)

    def test_first_target_second_page_is_ordinal_898(self):
        result = check(fixture(active=0xA200), active=0xA200)
        self.assertEqual((result.status, result.selected_ordinal, result.selected_pc),
                         ("SOURCE_ONLY_SHAPE_UNAUTHENTICATED", 898, 0x0451))
        self.assertFalse(result.native_coverage_proved)

    def test_legacy_128_cap_typed_stops_before_target(self):
        result = check(fixture(), cap=LEGACY_CAP)
        self.assertEqual((result.status, result.candidate_hits,
                          result.selected_ordinal),
                         ("STOP_CANDIDATE_HIT_CAP", 129, None))

    def test_sparse_selected_target_cannot_false_pass(self):
        frames = [Frame(U19, 0x02D8, 0, 0),
                  Frame(PRINTER, 0x0445, PAGE, 0x118F),
                  Frame(PRINTER, 0x0449, PAGE, 0x118F),
                  Frame(U19, 0x02DD, 0, 0)]
        result = check(frames)
        self.assertEqual(result.status, "STOP_CANDIDATE_SEQUENCE")
        self.assertFalse(result.native_coverage_proved)

    def test_missing_or_modified_earlier_hit_cannot_false_pass(self):
        frames = fixture()
        self.assertEqual(check(frames[:100] + frames[101:]).status,
                         "STOP_CANDIDATE_SEQUENCE")
        self.assertEqual(check(frames[:897] +
                               [replace(frames[897], di=0x1190)] +
                               frames[898:]).status, "STOP_CANDIDATE_SEQUENCE")
        self.assertEqual(check(frames[:897]).status, "STOP_INCOMPLETE_ROUTE")

    def test_successor_and_return_are_required(self):
        frames = fixture()
        self.assertEqual(check(frames[:898] + frames[899:]).status,
                         "STOP_WRONG_SINGLE_STEP")
        self.assertEqual(check(frames[:-1]).status, "STOP_RETURN_MISSING")
        self.assertEqual(check(frames[:-1] +
                               [Frame(PRINTER, 0x0445, PAGE, 0x118F), frames[-1]]).status,
                         "STOP_ROUTE_OVERFLOW")
        self.assertEqual(check(frames + [frames[-1]]).status, "STOP_TRAILING_FRAME")

    def test_missing_target_page_is_typed(self):
        result = check(fixture(active=0xA200, draw=0xA400),
                       active=0xA200, draw=0xA400)
        self.assertEqual(result.status, "STOP_TARGET_PAGE_ABSENT")
        self.assertFalse(result.native_coverage_proved)

    def test_stream_record_and_line_caps(self):
        line = json.dumps({"cs": "0F2D", "ip": "0445", "es": "A000",
                           "di": "118F"}) + "\n"
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.jsonl"
            path.write_text(line * (MAX_STREAM_RECORDS + 1))
            with self.assertRaisesRegex(ValueError, "STOP_STREAM_RECORD_CAP"):
                list(read_jsonl_frames(path))
            path.write_text(line[:-1] + " " * 257 + "\n")
            with self.assertRaisesRegex(ValueError, "STOP_STREAM_LINE_CAP"):
                list(read_jsonl_frames(path))


if __name__ == "__main__":
    unittest.main()
