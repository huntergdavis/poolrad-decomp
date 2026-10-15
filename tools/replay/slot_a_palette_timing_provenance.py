#!/usr/bin/env python3
"""Audit saved-A palette report order and cross-run equality, without colors."""

from __future__ import annotations

import argparse
import csv
from hashlib import sha256
from io import StringIO
from pathlib import Path
import re

import slot_a_palette_write_route_audit as prior
import slot_a_rgb_palette_audit as rgb


ROOT = Path(__file__).resolve().parents[2]
MODE_RUN = ROOT / "resources/oracle-runs/slot-a-palette-mode-p5-002"
RGB_RUN = ROOT / "resources/oracle-runs/slot-a-rgb-palette-p5-001"
MAP = ROOT / "docs/re/slot_a_palette_timing_provenance.csv"
FULL_SHA = "95d6bf83cec40bed147bb1aad176dab7cacdd9733df76fb6217420e4e5d45394"
PREFIX_SIZE = 22030
SUFFIX_SHA = "ac06a70c49fbcf35f2d417487bbf0fe1a3ce0d19674ebcf75638266135eb5fe8"
RGB_TRACE_SHA = "578a577f4891dcb29d8e28eb84b12229727bba9a3989b8520baba3c0f83b5326"
FIRST_SCREEN_SHA = "2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134"
UP_SCREEN_SHA = "ecb89bbc83b4cafa8a50500b3a16f9cd286f0b41191d05c75c4e77db9b17e3c7"
AC_MARKER = "LOG: VGA Attribute Controller info:"


def input_bytes(path: Path, expected: str) -> bytes:
    private = (ROOT / "resources").resolve()
    resolved = path.resolve(strict=True)
    if not resolved.is_relative_to(private):
        raise ValueError("capture must remain under ignored resources/")
    data = resolved.read_bytes()
    if sha256(data).hexdigest() != expected:
        raise ValueError(f"{path.name}: SHA-256 mismatch")
    return data


def render() -> tuple[str, tuple[int, int, int]]:
    full = input_bytes(MODE_RUN / "debugger-mode.typescript", FULL_SHA)
    if len(full) != PREFIX_SIZE + 960:
        raise ValueError("mode transcript length changed")
    prefix, suffix = full[:PREFIX_SIZE], full[PREFIX_SIZE:]
    # The old merged audit's immutable pin remains the evidence source.
    if sha256(prefix).hexdigest() != prior.TRACE_SHA:
        raise ValueError("published mode transcript prefix changed")
    if sha256(suffix).hexdigest() != SUFFIX_SHA:
        raise ValueError("appended terminal suffix changed")
    lower_suffix = suffix.lower()
    if (b"xio" not in lower_suffix or b"fatal" not in lower_suffix or
            any(marker in lower_suffix for marker in (
                b"vga attribute controller info:", b"vga dac info:",
                b"vga dac palette", b"ev of 'cs'", b"ev of 'ip'"))):
        raise ValueError("suffix no longer classifies as teardown-only text")

    mode_trace = prefix.decode("latin1")
    prior.require_stop_order(mode_trace)
    reports = prior.hardware_reports(mode_trace)
    if len(reports) != 4 or prior.changed_channels(reports[0], reports[1]) != (
            9, 49, 68):
        raise ValueError("mode-call report sequence changed")
    if reports[1] != reports[2] or reports[1] != reports[3]:
        raise ValueError("later palette reports differ from post-mode state")
    mode_offsets = [m.start() for m in re.finditer(AC_MARKER, mode_trace)]
    if mode_offsets != [7981, 11323, 15528, 18759]:
        raise ValueError("published transcript report positions changed")

    rgb_trace = input_bytes(
        RGB_RUN / "debugger-palette.typescript", RGB_TRACE_SHA).decode("latin1")
    rgb_offsets = [m.start() for m in re.finditer(AC_MARKER, rgb_trace)]
    if len(rgb_offsets) != 2 or rgb_offsets[-1] != 9044:
        raise ValueError("independent first-view report positions changed")
    # The early RGB-run text-mode report is incomplete. Only its final EGA
    # report is complete and comparable to the mode-run post-call state.
    rgb_reports = prior.hardware_reports(rgb_trace[rgb_offsets[-1]:])
    if len(rgb_reports) != 1 or rgb_reports[0] != reports[1]:
        raise ValueError("independent first-view palette differs")
    rgb.palette_state(rgb_trace)  # also checks display mapping and completeness

    for run, name, expected in (
        (MODE_RUN, "paused-first-view-private.png", FIRST_SCREEN_SHA),
        (MODE_RUN, "post-up-stable-private.png", UP_SCREEN_SHA),
        (MODE_RUN, "post-up-paused-private.png", UP_SCREEN_SHA),
        (RGB_RUN, "stable-first-view-private.png", FIRST_SCREEN_SHA),
        (RGB_RUN, "paused-first-view-private.png", FIRST_SCREEN_SHA),
    ):
        input_bytes(run / name, expected)

    output = StringIO()
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(("run", "order", "anchor", "transcript_byte_offset",
                     "relation_to_post_mode", "screen_relation"))
    for row in (
        ("mode-p5-002", 0, "0C88:00EE", mode_offsets[0],
         "pre_call_baseline", "no_screen_join"),
        ("mode-p5-002", 1, "0C88:00F3", mode_offsets[1],
         "changed_ac_dac", "no_screen_join"),
        ("mode-p5-002", 2, "later_first_view", mode_offsets[2],
         "equal_post_call", "same_first_view_hash"),
        ("mode-p5-002", 3, "later_one_up", mode_offsets[3],
         "equal_post_call", "distinct_one_up_hash"),
        ("rgb-p5-001", 0, "stable_first_view", rgb_offsets[-1],
         "equal_post_call", "same_first_view_hash"),
    ):
        writer.writerow(row)
    return output.getvalue(), prior.changed_channels(reports[0], reports[1])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emit-csv", action="store_true")
    args = parser.parse_args()
    csv_text, counts = render()
    if args.emit_csv:
        print(csv_text, end="")
        return
    if MAP.read_text(encoding="ascii") != csv_text:
        raise ValueError("checked-in timing map differs")
    print("mode full/prefix/suffix and RGB trace hash-locked")
    print("four ordered mode reports; AC/DAC entry/channel deltas", counts)
    print("post-mode, first-view, one-Up and cross-run RGB palette equal")
    print("960-byte terminal append excluded; no extra hardware/CPU reports")
    print("address/order-only CSV SHA-256", sha256(csv_text.encode("ascii")).hexdigest())


if __name__ == "__main__":
    main()
