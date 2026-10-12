#!/usr/bin/env python3
"""Audit contiguous coverage of the four RES_0464:02A5 route-0 slices."""

import argparse
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_464_mode0_prefix_audit import collect as collect_prefix
from resident_464_mode0_primary_audit import collect as collect_first
from resident_464_mode0_second_audit import collect as collect_second
from resident_464_mode0_loops_audit import collect as collect_loops


def collect(exe):
    require(digest(exe) == START_SHA256, "unsupported START.EXE")
    prefix = collect_prefix(exe)
    first = collect_first(exe)
    second = collect_second(exe)
    loops = collect_loops(exe)
    gate = next(s for s in loops["spans"] if s["name"] == "loop_gate")
    terminal = next(s for s in loops["spans"]
                    if s["name"] == "common_return")
    spans = (
        ("auxiliary_prefix", prefix["image_start"],
         prefix["prefix_image_end_exclusive"], prefix["prefix_sha256"]),
        ("first_primary_byte", first["image_start"],
         first["image_end_exclusive"], first["sha256"]),
        ("second_primary_byte", second["image_start"],
         second["image_end_exclusive"], second["sha256"]),
        ("loop_gate", gate["image_start"],
         gate["image_end_exclusive"], gate["sha256"]),
    )
    require(prefix["sha256"] ==
            "5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66",
            "route-0 identity changed")
    require(spans[0][1] == prefix["image_start"] and
            spans[-1][2] == prefix["image_end_exclusive"] and
            all(spans[i][2] == spans[i + 1][1]
                for i in range(len(spans) - 1)) and
            sum(end - start for _, start, end, _ in spans) ==
            prefix["bytes"] == 796,
            "route-0 slice coverage changed")
    require(terminal["image_start"] == 0x04F0E and
            terminal["image_end_exclusive"] == 0x04F14,
            "common return changed")
    return {
        "start_sha256": START_SHA256,
        "original_routine": "RES_0464:02A5",
        "route": "video_state_0",
        "route_image_start": prefix["image_start"],
        "route_image_end_exclusive": prefix["image_end_exclusive"],
        "route_bytes": prefix["bytes"],
        "route_sha256": prefix["sha256"],
        "reviewed_slices": [
            {"name": name, "image_start": start,
             "image_end_exclusive": end, "bytes": end - start,
             "sha256": span_hash}
            for name, start, end, span_hash in spans
        ],
        "common_return_image_start": terminal["image_start"],
        "common_return_sha256": terminal["sha256"],
        "supported_shape": {"count": 1, "width": 1, "height": 1},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
