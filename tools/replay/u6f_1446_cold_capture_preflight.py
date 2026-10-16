#!/usr/bin/env python3
"""Offline capture recipe/consistency checks; no emulator or guest execution."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re

CPU_FIELDS = "CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS".split()
CUTS = [
    ("entry", 0x2a8e, 0x1446),
    ("cold", 0x08d0, 0x0025),
    ("handler", 0x12c6, 0x02e6),
    ("iret", 0x12c6, 0x0337),
    ("patched", 0x08d0, 0x0025),
    ("u9b", 0x2ccd, 0x0000),
    ("callee_return", 0x2a8e, 0x1579),
    ("outer_ret", 0x2a8e, 0x157c),
    ("outer_return", None, None),
]
INPUTS = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "saved_slot_a": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def plan(run_id):
    require(bool(re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{0,79}", run_id)),
            "invalid run ID")
    return {
        "schema_version": 1, "run_id": run_id, "state": "HELD_NO_LAUNCH",
        "input_sha256": INPUTS,
        "prerequisites": ["PM scope plus serialized native slot",
                          "new prepared run; natural title/menu/load-slot A",
                          "no register/RAM patches or forced service results",
                          "audit actual loaded identities before arming cuts"],
        "stops": [{"label": label, "ordinal": i, "run_id": run_id,
                   "breakpoint": (f"BP {cs:04X}:{ip:04X}" if cs is not None
                                  else "derive CS:IP from outer_ret live SS:SP"),
                   "cpu": {}, "ram_file": f"ram-{i:02d}-{label}.bin",
                   "ram_sha256": None}
                  for i, (label, cs, ip) in enumerate(CUTS)],
        "capture_commands": ["EV " + " ".join(CPU_FIELDS),
                             "MEMDUMPBIN 0000 0000 A0000"],
        "procedure": ["first gate natural slot-A caller at BP2A8E:19E3; verify live frame",
                      "remove only previous owned breakpoint; arm next; resume naturally",
                      "at each cut save raw16CPU and distinct RAM file before resuming",
                      "cold and patched are separate arrivals at0025; never force patch",
                      "at outer_ret derive successor and arm it before executing RETF4",
                      "stop at outer_return before next02AE instruction/call"],
        "ram_bytes": 655360,
        "service_transcript": "separate actual DOS/indirect/event record required",
        "claim_limits": ["cut consistency only; no C/native or whole-function parity",
                         "no upper384KiB/video aperture in640KiB dumps",
                         "no ordered transient-write or device-state proof",
                         "run provenance/natural UI require independent operator record"],
    }


def load_manifest(path):
    path = Path(path).resolve()
    data = json.loads(path.read_text())
    require(isinstance(data, dict), "manifest object")
    require(data.get("schema_version") == 1, "schema version")
    run_id = data.get("run_id")
    plan(run_id if isinstance(run_id, str) else "")
    require(data.get("input_sha256") == INPUTS, "input identity declarations")
    size = data.get("ram_bytes")
    require(type(size) is int and size in (655360, 1048576), "RAM coverage")
    stops = data.get("stops", [])
    require(isinstance(stops, list) and len(stops) == len(CUTS), "nine required cuts")
    snapshots = {}
    used = set()
    previous = -1
    for row, (label, cs, ip) in zip(stops, CUTS):
        require(isinstance(row, dict), "cut object")
        require(row.get("label") == label, "cut order/label")
        require(row.get("run_id") == run_id, "mixed run identifiers")
        ordinal = row.get("ordinal")
        require(type(ordinal) is int and ordinal > previous, "stop order")
        previous = ordinal
        cpu = row.get("cpu", {})
        require(isinstance(cpu, dict) and set(cpu) == set(CPU_FIELDS), "all16 CPU fields")
        require(all(type(v) is int and 0 <= v <= 65535 for v in cpu.values()), "CPU word range")
        if cs is not None:
            require((cpu["CS"], cpu["IP"]) == (cs, ip), f"{label}: loaded PC identity")
        name = row.get("ram_file")
        require(isinstance(name, str), "RAM filename")
        source = (path.parent / name).resolve()
        require(source.is_relative_to(path.parent) and source not in used, "distinct in-run RAM files")
        used.add(source)
        ram = source.read_bytes()
        require(len(ram) == size and sha256(ram).hexdigest() == row.get("ram_sha256"),
                f"{label}: RAM size/hash")
        snapshots[label] = (cpu, ram)
    return data, snapshots


def word(ram, seg, off):
    p = ((seg << 4) + (off & 0xffff)) & 0xfffff
    q = (p + 1) & 0xfffff
    require(p < len(ram) and q < len(ram), "required frame outside captured RAM")
    return ram[p] | (ram[q] << 8)


def at(ram, seg, off, count):
    p = ((seg << 4) + off) & 0xfffff
    require(p + count <= len(ram), "required identity outside captured RAM")
    return ram[p:p + count]


def check(path):
    data, s = load_manifest(path)
    entry, er = s["entry"]
    cold, cr = s["cold"]
    handler, hr = s["handler"]
    iret, ir = s["iret"]
    patched, pr = s["patched"]
    u9b, ur = s["u9b"]
    returned, rr = s["callee_return"]
    outer, ore = s["outer_ret"]
    successor, sr = s["outer_return"]
    require(at(cr, cold["CS"], cold["IP"], 4) == bytes((0xcd, 0x3f, 0, 0)),
            "stub must actually be cold with descriptor0")
    require((word(cr, 0, 0xfc), word(cr, 0, 0xfe)) == (handler["IP"], handler["CS"]),
            "live INT3F vector")
    require(handler["SS"] == cold["SS"] and handler["SP"] == (cold["SP"] - 6) & 0xffff,
            "interrupt stack delta")
    require(handler["FLAGS"] == cold["FLAGS"] & ~0x0300, "INT clears IF/TF")
    for field in set(CPU_FIELDS) - {"CS", "IP", "SP", "FLAGS"}:
        require(handler[field] == cold[field], "INT preserved field " + field)
    frame = lambda ram, cpu: tuple(word(ram, cpu["SS"], cpu["SP"] + n) for n in (0, 2, 4))
    require(frame(hr, handler) == (0x0027, cold["CS"], cold["FLAGS"]), "actual INT frame")
    require(iret["SS"] == cold["SS"] and iret["SP"] == handler["SP"], "pre-IRET stack")
    require(frame(ir, iret) == (patched["IP"], patched["CS"], patched["FLAGS"]), "live IRET frame")
    require(patched["SS"] == iret["SS"] and patched["SP"] == (iret["SP"] + 6) & 0xffff,
            "IRET stack cleanup")
    for field in set(CPU_FIELDS) - {"CS", "IP", "SP", "FLAGS"}:
        require(iret[field] == patched[field], "IRET preserved field " + field)
    require(cold == patched, "observed normal handler restoration")
    jump = at(pr, patched["CS"], patched["IP"], 5)
    require(jump[0] == 0xea and (int.from_bytes(jump[3:5], "little"),
                               int.from_bytes(jump[1:3], "little")) == (u9b["CS"], u9b["IP"]),
            "actual patched far jump")
    require(all(u9b[f] == patched[f] for f in CPU_FIELDS if f not in ("CS", "IP")),
            "jump preserved fields")
    require(pr == ur, "jump RAM identity")
    require((word(ur, u9b["SS"], u9b["SP"]), word(ur, u9b["SS"], u9b["SP"] + 2))
            == (returned["IP"], returned["CS"]), "live U9B return frame")
    require(returned["SS"] == u9b["SS"] and returned["SP"] == (u9b["SP"] + 8) & 0xffff,
            "U9B RETF4 cleanup")
    require(outer["SS"] == returned["SS"] and outer["SP"] == (returned["BP"] + 2) & 0xffff,
            "MOVSPBP/POPBP outer stack")
    require(outer["BP"] == word(rr, returned["SS"], returned["BP"]), "live restored BP")
    require(all(outer[f] == returned[f] for f in CPU_FIELDS if f not in ("SP", "BP", "IP")),
            "outer epilogue preserved fields")
    require(rr == ore == sr, "observed RAM identity across outer epilogue/return")
    target = (word(ore, outer["SS"], outer["SP"] + 2), word(ore, outer["SS"], outer["SP"]))
    require((successor["CS"], successor["IP"]) == target, "derived live outer successor")
    require(successor["SP"] == (outer["SP"] + 8) & 0xffff, "outer RETF4 cleanup")
    require(all(successor[f] == outer[f] for f in CPU_FIELDS if f not in ("CS", "IP", "SP")),
            "outer RETF4 preserved fields")
    require(successor["SS"] == entry["SS"] and successor["SP"] == (entry["SP"] + 8) & 0xffff,
            "complete ordinary outer-frame delta")
    return {"result": "bounded_capture_cut_consistency_pass", "run_id": data["run_id"],
            "cuts": len(CUTS), "cpu_fields": 16, "ram_bytes": data["ram_bytes"],
            "native_parity": False, "ordered_write_proof": False, "device_proof": False,
            "complete_service_proof": False,
            "provenance_and_complete_services_require_independent_review": True,
            "new_strict_credit": 0}


def index(plan_path, transcript_path):
    """Index existing files only; provenance is a declaration, not verified."""
    plan_path = Path(plan_path).resolve()
    data = json.loads(plan_path.read_text())
    require(isinstance(data, dict), "plan object")
    expected = plan(data.get("run_id", ""))
    require(data == expected, "index requires an unchanged generated plan")
    rows = Path(transcript_path).read_text().splitlines()
    require(bool(rows) and rows[0].split() == ["label"] + CPU_FIELDS, "raw EV header")
    require(len(rows) == len(CUTS) + 1, "raw EV nine ordered rows")
    for row, stop in zip(rows[1:], data["stops"]):
        fields = row.split()
        require(len(fields) == 17 and fields[0] == stop["label"], "raw EV label/columns")
        stop["cpu"] = {name: int(value, 16) for name, value in zip(CPU_FIELDS, fields[1:])}
        source = (plan_path.parent / stop["ram_file"]).resolve()
        require(source.is_relative_to(plan_path.parent), "RAM file outside run")
        ram = source.read_bytes()
        require(len(ram) == data["ram_bytes"], "indexed RAM size")
        stop["ram_sha256"] = sha256(ram).hexdigest()
    data["state"] = "ARTIFACTS_INDEXED_PROVENANCE_UNVERIFIED"
    data["raw_ev_sha256"] = sha256(Path(transcript_path).read_bytes()).hexdigest()
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("plan").add_argument("run_id")
    commands.add_parser("check").add_argument("manifest")
    indexing = commands.add_parser("index")
    indexing.add_argument("plan")
    indexing.add_argument("raw_ev")
    args = parser.parse_args()
    try:
        if args.command == "plan":
            result = plan(args.run_id)
        elif args.command == "index":
            result = index(args.plan, args.raw_ev)
        else:
            result = check(args.manifest)
    except (ValueError, OSError, TypeError, KeyError) as error:
        parser.exit(1, "capture preflight rejected: " + str(error) + "\n")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
