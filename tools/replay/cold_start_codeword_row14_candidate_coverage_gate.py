#!/usr/bin/env python3
"""Source-only shape check for a complete U19 fixed-row candidate stream.

The supplied frames and their completeness are unauthenticated. This checker
cannot turn an offline transcript into a native writer or coverage claim.
"""

import argparse
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
from typing import Iterable, Iterator

from tools.replay.cold_start_codeword_row14_target_filter import Frame, parse_frame


DEBUG_SHA = "2762251108e54aeec3474b9f495222ce09bec9f246dd0af53a4d10fa7bb2a38d"
DEBUG_WINDOWS = (
    (664, 676, "6e72af0426de681c0273d740b1db5e61e045334832e00abfa7f9854143705e3e"),
    (729, 793, "c9cba40ea45854a6d3323e4c63c0261042d1f570fe2fa6446cf99133de4e9484"),
    (2505, 2522, "acd91f4150f20159f134b787ab8751e1e43d25b2ffc833c2ce3fbfebc48e285e"),
    (2763, 2799, "2b2d8a6ac574290e0ef54b84a72eceabac10a570de6d677f12abffa3b86a9edc"),
    (5430, 5502, "80a18c0cd99c085f3c2b22ea66b3145ccf7d69f4c57211af23d5cc000d9aed62"),
    (6009, 6030, "0e773f59747b8dea6cfc435dbfc2dc78fe0e442e8f250daec4ff4f1393cf8a2a"),
)
FIXED_CELLS = 36
GLYPH_ROWS = 8
PLANES = 4
PAGES = 2
ROUTE_CANDIDATES = FIXED_CELLS * GLYPH_ROWS * PLANES * PAGES
LEGACY_CAP = 128
WITNESS_SEGMENT = 0xA000
WITNESS_OFFSET = 0x118F
FIRST_TARGET_COLUMN = 15
FIRST_TARGET_LOWER_BOUND = (FIRST_TARGET_COLUMN - 1) * GLYPH_ROWS * PLANES * PAGES + 1
MAX_JSONL_LINE = 256
MAX_STREAM_RECORDS = ROUTE_CANDIDATES + 3  # call, one successor, return
SUCCESSOR = {0x0412: 0x0415, 0x0438: 0x043B, 0x0445: 0x0449, 0x0451: 0x0455}


@dataclass(frozen=True)
class CoverageResult:
    status: str
    candidate_hits: int
    selected_ordinal: int | None = None
    selected_pc: int | None = None
    native_coverage_proved: bool = False


def audit_debugger_source(source: Path) -> None:
    data = (source / "src/debug/debug.cpp").read_bytes()
    if sha256(data).hexdigest() != DEBUG_SHA:
        raise ValueError("private debugger source differs")
    lines = data.splitlines(keepends=True)
    for first, last, digest in DEBUG_WINDOWS:
        if sha256(b"".join(lines[first - 1:last])).hexdigest() != digest:
            raise ValueError(f"private debugger window {first}-{last} differs")


def candidate_frames(printer_segment: int, active_segment: int,
                     draw_segment: int) -> Iterator[Frame]:
    """The merged fixed-row C model's accepted EGA path, without game text."""
    for cell in range(FIXED_CELLS):
        for row in range(GLYPH_ROWS):
            offset = 14 * 8 * 40 + cell + 1 + row * 40
            for plane in range(PLANES):
                for page, segment in enumerate((active_segment, draw_segment)):
                    glyph = plane in (1, 3)  # fixed color 10
                    pc = (0x0412 if page == 0 else 0x0438) if glyph else (
                        0x0445 if page == 0 else 0x0451)
                    yield Frame(printer_segment, pc, segment, offset)


def check_candidate_stream(frames: Iterable[Frame], u19_segment: int,
                           printer_segment: int, active_segment: int,
                           draw_segment: int,
                           cap: int = ROUTE_CANDIDATES) -> CoverageResult:
    """Check one claimed complete stream; success remains source-only.

    The stream has a caller, all 2,304 selected-PC frames on the conditional
    accepted EGA path, the first target's direct successor, then the return.
    A later capture owner must separately prove that the recording mechanism
    retained every stop and that this path was actually reached.
    """
    if (any(type(word) is not int or not 0 <= word <= 0xFFFF for word in
            (u19_segment, printer_segment, active_segment, draw_segment)) or
            type(cap) is not int or not 1 <= cap <= ROUTE_CANDIDATES):
        raise ValueError("invalid segment or candidate cap")
    if WITNESS_SEGMENT not in (active_segment, draw_segment):
        return CoverageResult("STOP_TARGET_PAGE_ABSENT", 0)

    stream = iter(frames)
    if not _is_pc(next(stream, None), u19_segment, 0x02D8):
        return CoverageResult("STOP_CALLER_MISSING", 0)
    selected_ordinal = None
    selected_pc = None
    for ordinal, expected in enumerate(candidate_frames(
            printer_segment, active_segment, draw_segment), 1):
        frame = next(stream, None)
        if frame is None or _is_pc(frame, u19_segment, 0x02DD):
            return CoverageResult("STOP_INCOMPLETE_ROUTE", ordinal - 1)
        if ordinal > cap:
            return CoverageResult("STOP_CANDIDATE_HIT_CAP", ordinal)
        if frame != expected:
            return CoverageResult("STOP_CANDIDATE_SEQUENCE", ordinal)
        if (selected_ordinal is None and frame.es == WITNESS_SEGMENT and
                frame.di == WITNESS_OFFSET):
            selected_ordinal, selected_pc = ordinal, frame.ip
            successor = next(stream, None)
            if (successor != Frame(printer_segment, SUCCESSOR[frame.ip],
                                   frame.es, frame.di)):
                return CoverageResult("STOP_WRONG_SINGLE_STEP", ordinal,
                                      selected_ordinal, selected_pc)

    terminal = next(stream, None)
    if terminal is None:
        return CoverageResult("STOP_RETURN_MISSING", ROUTE_CANDIDATES,
                              selected_ordinal, selected_pc)
    if not _is_pc(terminal, u19_segment, 0x02DD):
        return CoverageResult("STOP_ROUTE_OVERFLOW", ROUTE_CANDIDATES + 1,
                              selected_ordinal, selected_pc)
    if next(stream, None) is not None:
        return CoverageResult("STOP_TRAILING_FRAME", ROUTE_CANDIDATES,
                              selected_ordinal, selected_pc)
    if selected_ordinal is None:
        return CoverageResult("STOP_TARGET_MISSING", ROUTE_CANDIDATES)
    return CoverageResult("SOURCE_ONLY_SHAPE_UNAUTHENTICATED", ROUTE_CANDIDATES,
                          selected_ordinal, selected_pc)


def _is_pc(frame: Frame | None, segment: int, offset: int) -> bool:
    return isinstance(frame, Frame) and (frame.cs, frame.ip) == (segment, offset)


def read_jsonl_frames(path: Path) -> Iterator[Frame]:
    """Bound line size and count while streaming an ignored private record."""
    with path.open("rb") as source:
        for count, line in enumerate(source, 1):
            if count > MAX_STREAM_RECORDS:
                raise ValueError("STOP_STREAM_RECORD_CAP")
            if len(line) > MAX_JSONL_LINE:
                raise ValueError("STOP_STREAM_LINE_CAP")
            yield parse_frame(json.loads(line))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True,
                        help="ignored pinned DOSBox-X source root")
    parser.add_argument("--frames", type=Path,
                        help="ignored JSONL caller/candidate/successor/return frames")
    for name in ("u19-segment", "printer-segment", "active-segment", "draw-segment"):
        parser.add_argument(f"--{name}", type=lambda value: int(value, 16))
    args = parser.parse_args()
    audit_debugger_source(args.source)
    print("debugger source: six exact windows and full file PASS")
    print(f"conditional accepted route: {ROUTE_CANDIDATES} candidates; "
          f"first target >= {FIRST_TARGET_LOWER_BOUND}; legacy cap {LEGACY_CAP} insufficient")
    words = (args.u19_segment, args.printer_segment,
             args.active_segment, args.draw_segment)
    if args.frames is None:
        if any(word is not None for word in words):
            parser.error("all measured segments require --frames")
        print("no private stream or native coverage supplied")
        return
    if any(word is None for word in words):
        parser.error("--frames requires all four measured segments")
    result = check_candidate_stream(read_jsonl_frames(args.frames), *words)
    print(f"{result.status}; supplied_candidate_hits={result.candidate_hits}; "
          f"selected_ordinal={result.selected_ordinal}; native_coverage_proved=false")
    if result.status != "SOURCE_ONLY_SHAPE_UNAUTHENTICATED":
        raise SystemExit(3)


if __name__ == "__main__":
    main()
