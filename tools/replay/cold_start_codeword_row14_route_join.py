#!/usr/bin/env python3
"""Offline structural join for a supplied U19 route and CPU/EGA store pair.

This does not capture native state or authenticate a debugger transcript.
All private input files stay under one ignored run directory.
"""

import argparse
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.replay.cold_start_codeword_row14_event_plan import (
    EventGate, TOKEN_RE, audit_bundle, private_path, require,
)
from tools.replay.cold_start_codeword_row14_target_filter import (
    NEXT_PC, WITNESS, parse_frame,
)


STOP_IDS = (
    "credits_return", "u19_stub", "u19_entry", "u19_pre_printer",
    "printer_entry", "selected_witness_store", "store_successor",
    "u19_post_printer",
)
WORD_RE = re.compile(r"[0-9A-Fa-f]{1,4}")
RAM_BYTES = 655360


@dataclass(frozen=True)
class RouteGate:
    event: EventGate
    selected_frame_index: int


def word(value: object) -> int:
    require(isinstance(value, str) and WORD_RE.fullmatch(value) is not None,
            "segment must be one to four hex digits")
    return int(value, 16)


def read_ram(root: Path, name: object, expected_sha: object) -> bytes:
    require(isinstance(name, str) and name and
            isinstance(expected_sha, str) and
            re.fullmatch(r"[0-9a-f]{64}", expected_sha) is not None,
            "RAM path and SHA-256 required")
    path = private_path(root, root / name)
    data = path.read_bytes()
    require(len(data) == RAM_BYTES and sha256(data).hexdigest() == expected_sha,
            "RAM size or SHA-256 differs")
    return data


def far_target(ram: bytes, address: int, opcode: int) -> tuple[int, int]:
    require(0 <= address <= len(ram) - 5 and ram[address] == opcode,
            "live far-call/jump opcode differs")
    return struct.unpack_from("<HH", ram, address + 1)


def audit_route(root: Path, template: object, route_doc: object,
                frame_doc: object, plane_manifest: object) -> RouteGate:
    """Check one supplied route against the already bounded store event."""
    require(isinstance(template, dict) and template.get("version") == 1 and
            template.get("status") == "SOURCE_ONLY_TEMPLATE_NO_NATIVE" and
            template.get("candidate_hit_cap") == 128 and
            template.get("witness") == {"segment": "A000", "offset": "118F",
                                        "bit": "20"} and
            isinstance(template.get("ordered_stops"), list) and
            len(template["ordered_stops"]) == len(STOP_IDS) and
            all(isinstance(stop, dict) and "id" in stop for stop in
                template["ordered_stops"]) and
            tuple(stop["id"] for stop in template["ordered_stops"]) ==
            STOP_IDS,
            "public eight-stop template differs")
    require(isinstance(route_doc, dict) and route_doc.get("version") == 1 and
            isinstance(frame_doc, dict) and isinstance(plane_manifest, dict),
            "route and event documents required")
    token = route_doc.get("run_token")
    require(isinstance(token, str) and TOKEN_RE.fullmatch(token) is not None and
            frame_doc.get("run_token") == token and
            plane_manifest.get("run_token") == token,
            "route, CPU, and EGA run tokens differ")
    segments = route_doc.get("segments")
    require(isinstance(segments, dict) and
            set(segments) == {"resident", "u19", "printer"},
            "three measured loaded segments required")
    resident, u19, printer = (word(segments[key]) for key in
                              ("resident", "u19", "printer"))
    require(resident <= 0xFFE3, "stub segment wraps")
    require((u19, printer) == (0x38D3, 0x0F2D),
            "existing CPU target filter pins U19/printer loaded segments")
    stub = resident + 0x1C

    records = frame_doc.get("frames")
    require(isinstance(records, list) and len(records) <= 130,
            "bounded CPU frame stream required")
    frames = [parse_frame(record) for record in records]
    stops = route_doc.get("ordered_stops")
    require(isinstance(stops, list) and len(stops) == len(STOP_IDS) and
            all(isinstance(stop, dict) and
                set(stop) == {"id", "frame_index"} for stop in stops) and
            tuple(stop["id"] for stop in stops) == STOP_IDS,
            "eight ordered stop IDs required")
    indices = [stop["frame_index"] for stop in stops]
    require(all(type(index) is int and 0 <= index < len(frames)
                for index in indices) and
            all(left < right for left, right in zip(indices, indices[1:])),
            "stop indices must be bounded and strictly increasing")
    selected_index = indices[5]
    require(indices[6] == selected_index + 1,
            "selected store successor must be next CPU frame")
    gate = audit_bundle(root, frame_doc, plane_manifest)
    target = gate.selected.target_pc
    successor = gate.selected.next_pc
    expected = (
        (resident, 0x0122), (stub, 0x0025), (u19, 0x00FD),
        (u19, 0x02D8), (printer, 0x0626), (printer, target),
        (printer, successor), (u19, 0x02DD),
    )
    require(all((frames[index].cs, frames[index].ip) == pc
                for index, pc in zip(indices, expected)),
            "route stop PC differs from measured loaded segments")
    first_call = next((index for index, frame in enumerate(frames)
                       if (frame.cs, frame.ip) == (u19, 0x02D8)), None)
    require(first_call == indices[3],
            "route pre-call is not the CPU filter's first caller")
    require((frames[selected_index].es, frames[selected_index].di) == WITNESS,
            "selected store effective address differs")
    require(NEXT_PC[target] == successor, "store successor differs")
    require(not any((frame.cs, frame.ip) == (u19, 0x02DD)
                    for frame in frames[indices[3] + 1:selected_index]),
            "printer returned before selected store")
    first_witness = next((index for index in range(indices[3] + 1,
                                                   indices[7])
                          if frames[index].cs == printer and
                          frames[index].ip in NEXT_PC and
                          (frames[index].es, frames[index].di) == WITNESS),
                         None)
    require(first_witness == selected_index,
            "route index does not select first matching witness frame")
    first_return = next((index for index in range(selected_index + 1,
                                                   len(frames))
                         if (frames[index].cs, frames[index].ip) ==
                         (u19, 0x02DD)), None)
    require(first_return == indices[7],
            "route post-return is not the first U19 return")

    before_name, after_name = route_doc.get("pre_ram"), route_doc.get("post_ram")
    require(isinstance(before_name, str) and
            isinstance(after_name, str) and before_name != after_name,
            "distinct pre-call and post-return RAM paths required")
    require(private_path(root, root / before_name) !=
            private_path(root, root / after_name),
            "pre-call and post-return RAM paths alias")
    event = plane_manifest["events"][0]
    require(before_name not in (event["before"]["ram"],
                                event["after"]["ram"]) and
            after_name not in (event["before"]["ram"],
                               event["after"]["ram"]),
            "route and store RAM samples must be distinct")
    before = read_ram(root, before_name, route_doc.get("pre_ram_sha256"))
    read_ram(root, after_name, route_doc.get("post_ram_sha256"))
    require(far_target(before, (resident << 4) + 0x0122, 0x9A) ==
            (0x0025, stub) and
            far_target(before, (stub << 4) + 0x0025, 0xEA) ==
            (0x00FD, u19) and
            far_target(before, (u19 << 4) + 0x02D8, 0x9A) ==
            (0x0626, printer),
            "pre-call RAM does not connect resident, stub, U19, and printer")
    return RouteGate(gate, selected_index)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--template", type=Path, required=True)
    parser.add_argument("--route", type=Path, required=True)
    parser.add_argument("--frames", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    route = private_path(args.run, args.route)
    frames = private_path(args.run, args.frames)
    manifest = private_path(args.run, args.manifest)
    gate = audit_route(args.run, json.loads(args.template.read_text()),
                       json.loads(route.read_text()), json.loads(frames.read_text()),
                       json.loads(manifest.read_text()))
    print(f"eight ordered stops structurally joined; selected_frame="
          f"{gate.selected_frame_index}; candidate_hits="
          f"{gate.event.selected.candidate_hits}")
    print("supplied transcript authenticity, first guest writer, page and RGB "
          "timing remain unproved")


if __name__ == "__main__":
    main()
