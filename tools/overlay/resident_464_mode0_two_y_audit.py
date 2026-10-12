#!/usr/bin/env python3
"""Hash-lock the bounded two-y route of RES_0464:02A5."""

import argparse
import json
from pathlib import Path

from resident_inventory import require
from resident_464_mode0_single_audit import collect as collect_route
from resident_464_mode0_loops_audit import collect as collect_loops


def collect(exe):
    route = collect_route(exe)
    loops = collect_loops(exe)
    backedge = next(s for s in loops["spans"]
                    if s["name"] == "y_backedge")
    require(route["route_bytes"] == 796 and
            route["route_sha256"] ==
            "5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66",
            "route-0 body changed")
    require(backedge["sha256"] ==
            "836fb65002fd742d43b2733338df4103d534419c2f3a08eed093b6cd5a46d0e0"
            and loops["next_move_offset"] == 0x0328,
            "y backedge changed")
    require({(e["offset"], e["kind"], e["target"])
             for e in loops["gate_edges"]} >= {
                 (0x05D3, "jmp", "0x4965"),
                 (0x05EC, "jmp", "0x4f0e")},
            "loop gate targets changed")
    require(loops["common_return_offset"] == 0x08CE and
            route["common_return_sha256"] ==
            "90f66eae99f51c421d8db68fa3d7a7629f728669d6c3fd06db0bb96e74651939",
            "common return changed")
    return {
        "start_sha256": route["start_sha256"],
        "routine": route["original_routine"],
        "route_image_start": route["route_image_start"],
        "route_image_end_exclusive": route["route_image_end_exclusive"],
        "route_bytes": route["route_bytes"],
        "route_sha256": route["route_sha256"],
        "y_backedge_image_start": backedge["image_start"],
        "y_backedge_sha256": backedge["sha256"],
        "next_move_offset": loops["next_move_offset"],
        "common_return_offset": loops["common_return_offset"],
        "supported_shape": {"count": 1, "width": 1, "height": 2},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
