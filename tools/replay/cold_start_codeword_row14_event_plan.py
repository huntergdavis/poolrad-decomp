#!/usr/bin/env python3
"""Read-only bridge from a U19 CPU target frame to one four-plane event.

Supplied files must share a private run directory and token. This checks
selection and manifest consistency, not debugger-log authenticity or native
coverage. It never launches an emulator or writes capture artifacts.
"""

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import re

from tools.replay.cold_start_codeword_row14_plane_event_audit import (
    Change, audit as audit_planes,
)
from tools.replay.cold_start_codeword_row14_target_filter import (
    FilterResult, debugger_features, filter_frames, parse_frame,
)


TOKEN_RE = re.compile(r"[A-Za-z0-9_-]{1,64}")


@dataclass(frozen=True)
class EventGate:
    selected: FilterResult
    change: Change


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def private_path(root: Path, path: Path) -> Path:
    root = root.resolve()
    path = path.resolve()
    require(path.is_relative_to(root), "capture file must stay in one private run")
    return path


def audit_bundle(root: Path, frame_doc: object, manifest: object) -> EventGate:
    """Validate one supplied same-run event after the strict CPU frame filter."""
    require(isinstance(frame_doc, dict) and frame_doc.get("version") == 1,
            "unsupported CPU frame document")
    require(isinstance(manifest, dict) and manifest.get("version") == 1,
            "unsupported EGA manifest")
    token = frame_doc.get("run_token")
    require(isinstance(token, str) and TOKEN_RE.fullmatch(token) is not None and
            manifest.get("run_token") == token, "same-run token mismatch")
    records = frame_doc.get("frames")
    require(isinstance(records, list) and len(records) <= 130,
            "CPU frame stream missing or above bounded stop cap")
    selected = filter_frames([parse_frame(record) for record in records])
    require(selected.status == "PAIRED_CPU_SOURCE_ONLY", selected.status)
    events = manifest.get("events")
    require(isinstance(events, list) and len(events) == 1,
            "one selected CPU store requires exactly one four-plane event")
    event = events[0]
    require(isinstance(event, dict) and
            event.get("pc") == f"0F2D:{selected.target_pc:04X}" and
            event.get("next_pc") == f"0F2D:{selected.next_pc:04X}",
            "plane event does not match selected CPU store and successor")
    before, after = event.get("before"), event.get("after")
    require(isinstance(before, dict) and isinstance(after, dict),
            "paired before/after samples required")
    require(before.get("ram") != after.get("ram"),
            "before/after RAM files must be distinct")
    before_planes, after_planes = before.get("planes"), after.get("planes")
    require(isinstance(before_planes, list) and isinstance(after_planes, list) and
            len(before_planes) == len(after_planes) == 4 and
            all(isinstance(name, str) and name for name in
                before_planes + after_planes) and
            len(set(before_planes + after_planes)) == 8,
            "before/after samples need eight distinct plane files")
    require(before.get("gc_before") == after.get("gc_before"),
            "controller read-map baseline changed across one CPU store")
    changes = audit_planes(root, manifest)
    require(len(changes) == 1, "one paired plane event required")
    return EventGate(selected, changes[0])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--debugger", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True,
                        help="ignored private same-run directory")
    parser.add_argument("--frames", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    status, _ = debugger_features(args.debugger.read_bytes())
    require(status == "SOURCE_PREFLIGHT_READY_NO_NATIVE", status)
    frames_path = private_path(args.run, args.frames)
    manifest_path = private_path(args.run, args.manifest)
    gate = audit_bundle(args.run, json.loads(frames_path.read_text()),
                        json.loads(manifest_path.read_text()))
    print(f"selected=0F2D:{gate.selected.target_pc:04X} -> "
          f"0F2D:{gate.selected.next_pc:04X}; candidate_hits="
          f"{gate.selected.candidate_hits}")
    print(f"observed_witness_byte_changed_planes={len(gate.change.changed_planes)}; "
          f"observed_bit20_changed_planes={len(gate.change.changed_witness_bits)}")
    print("supplied capture authenticity, route completeness, first actual guest "
          "writer, page selection and host RGB timing unproved")


if __name__ == "__main__":
    main()
