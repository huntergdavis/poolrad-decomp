#!/usr/bin/env python3
"""Source-only RES_0464 mode-1 Move destination-span predicate.

This checks supplied guest row geometry. It cannot attest a reached Move,
EGA handler, plane-byte change, or displayed frame.
"""

import argparse
import json
from pathlib import Path

ROW_COUNT = 168
TARGET = 0x118F
UNKNOWN = "UNKNOWN_METADATA"
NOT_MATCH = "NOT_MATCH"
INTERSECTS = "CONDITIONAL_SPAN_INTERSECTION"


def word(value):
    if type(value) is not int or not 0 <= value <= 0xFFFF:
        raise ValueError("missing or invalid DOS word")
    return value


def classify(frame):
    """Return (status, row/span hits); unknown inputs fail closed."""
    try:
        if not isinstance(frame, dict):
            raise ValueError("missing frame")
        state = word(frame["video_state"])
        source = word(frame["source_segment"])
        dest = word(frame["dest_segment"])
        page_base = word(frame["page_base"])
        rows = frame["rows"]
        if state != 1 or source != 0xA200 or dest != 0xA000:
            raise ValueError("outside the supported mode-1 page route")
        if not isinstance(rows, list) or len(rows) != ROW_COUNT:
            raise ValueError("incomplete row table")
        hits = []
        for index, row in enumerate(rows):
            if not isinstance(row, dict) or type(row.get("dirty")) is not bool:
                raise ValueError("unknown dirty state")
            if not row["dirty"]:
                continue
            first = word(row["first"])
            last = word(row["last"])
            low = word(row["low"])
            if last < first or first % 4 or (last - first) % 4 != 3:
                raise ValueError("unsupported interleaved extent")
            length = ((last - first) >> 2) + 1
            start = page_base + 1 + index * 40 + low
            end = start + length
            if start > 0xFFFF or end > 0x10000:
                raise ValueError("unsupported 16-bit span wrap")
            if start <= TARGET < end:
                hits.append((index, start, end))
        return (INTERSECTS if hits else NOT_MATCH), tuple(hits)
    except (KeyError, TypeError, ValueError):
        return UNKNOWN, ()


def empty_frame():
    return {
        "video_state": 1,
        "source_segment": 0xA200,
        "dest_segment": 0xA000,
        "page_base": 0x0140,
        "rows": [{"dirty": False} for _ in range(ROW_COUNT)],
    }


def self_check():
    saved = empty_frame()
    for row in range(16, 104):
        first = 1352 + 84 * (row - 16)
        saved["rows"][row] = {
            "dirty": True, "first": first, "last": first + 43, "low": 2
        }
    short = empty_frame()
    short["rows"][104] = {"dirty": True, "first": 0, "last": 43, "low": 2}
    wide = empty_frame()
    wide["rows"][104] = {"dirty": True, "first": 0, "last": 51, "low": 2}
    unknown = empty_frame()
    unknown["rows"][104] = {"dirty": None}
    cases = (
        ("saved-other-run-r16-103", saved, (NOT_MATCH, ())),
        ("synthetic-r104-L11", short, (NOT_MATCH, ())),
        ("synthetic-r104-L13", wide, (INTERSECTS, ((104, 0x1183, 0x1190),))),
        ("unknown-live-metadata", unknown, (UNKNOWN, ())),
    )
    for name, frame, expected in cases:
        actual = classify(frame)
        if actual != expected:
            raise AssertionError(f"{name}: {actual!r} != {expected!r}")
        print(f"{name}: {actual[0]} {actual[1]}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--frame", type=Path, help="private complete 168-row JSON frame")
    args = parser.parse_args()
    if args.frame is None:
        self_check()
        return 0
    try:
        frame = json.loads(args.frame.read_text())
    except (OSError, UnicodeError, json.JSONDecodeError):
        frame = None
    status, hits = classify(frame)
    print(f"{status} {hits}")
    return 2 if status == UNKNOWN else 0


if __name__ == "__main__":
    raise SystemExit(main())
