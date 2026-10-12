#!/usr/bin/env python3
"""Hash-lock RES_0464:02A5 route-0 pixel and y/x/layer loop coverage."""

import argparse
import json
from pathlib import Path

from resident_inventory import require
from resident_464_mode0_single_audit import collect as collect_route
from resident_464_mode0_loops_audit import collect as collect_loops


BACKEDGES = {
    "y_backedge": "836fb65002fd742d43b2733338df4103d534419c2f3a08eed093b6cd5a46d0e0",
    "x_backedge": "0e579be0b811f9fa79302e6125dec28616dca2204d8bdeb8bd6e21711ed85753",
    "layer_backedge": "9ec211aeff74791df4c55e89e758139066bf9f278d09a6487835be3abaa4308e",
}


def collect(exe):
    route = collect_route(exe)
    loops = collect_loops(exe)
    spans = {s["name"]: s for s in loops["spans"]}
    require(route["route_bytes"] == 796 and
            route["route_sha256"] ==
            "5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66",
            "route-0 body changed")
    require(all(spans[name]["sha256"] == expected
                for name, expected in BACKEDGES.items()),
            "loop backedge changed")
    require(loops["next_move_offset"] == 0x0328 and
            loops["common_return_offset"] == 0x08CE,
            "loop continuation changed")
    require({(edge["offset"], edge["kind"], edge["target"])
             for edge in loops["gate_edges"]} >= {
                 (0x05D3, "jmp", "0x4965"),
                 (0x05DE, "jmp", "0x4948"),
                 (0x05E9, "jmp", "0x492c"),
                 (0x05EC, "jmp", "0x4f0e")},
            "loop gate targets changed")
    return {
        "start_sha256": route["start_sha256"],
        "routine": route["original_routine"],
        "route_image_start": route["route_image_start"],
        "route_image_end_exclusive": route["route_image_end_exclusive"],
        "route_bytes": route["route_bytes"],
        "route_sha256": route["route_sha256"],
        "backedges": [
            {"name": name, "image_start": spans[name]["image_start"],
             "bytes": spans[name]["bytes"], "sha256": spans[name]["sha256"]}
            for name in ("y_backedge", "x_backedge", "layer_backedge")
        ],
        "next_move_offset": loops["next_move_offset"],
        "common_return_offset": loops["common_return_offset"],
        "common_return_sha256": route["common_return_sha256"],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
