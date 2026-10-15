#!/usr/bin/env python3
"""Audit private bounded u6A reach attempts; never infer a leaf frame from a stub.

Only addresses, hashes, ranges and counts leave this audit. Original media,
terminal logs, CPU records, images and RAM remain under ignored resources/.
"""
import argparse
from hashlib import sha256
import json
import math
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
FIELDS = "ax bx cx dx si di bp sp cs ip ds es ss flags".split()
EV_ORDER = "cs ip ds es ss sp bp ax bx cx dx si di flags".split()
INPUTS = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "SAVGAMA.DAT": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
}
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
LOCKS = (
    {
        "events.json": "820743a700c5bb76f4bbf5d141aa69a6bce63e68016116a1ad09c14ac09aa859",
        "debugger.log": "1a906c223c4ed688fb1aa4266557f52c6d47b41af9154e5c8054344954318055",
        "summary.json": "ce6e9ed7f81ab5b357ad4a120102f46bdb61dbca4c57a139c2ffef0270fd7fea",
        "pre-encamp.png": "fbff0e5d9303df4460cb118c1c3c6830b1470e77dd5a1abe95dbf4cdefa15b2a",
        "memorize.png": "81edbd476a4f51ea0f781639b2ef487ea7d71d4979edd9433e733a170b38df95",
    },
    {
        "events.json": "a9ca74537abfd1e1ccfc1a395af502ef855c488dcffde27743d5101c5c3ab88d",
        "debugger.log": "43184b16962ed0eb5b21f1fa1d0769bc7a16e5b8e84d2e6090c72919488b0621",
        "summary.json": "e9a20f1f0948adcae4d0c00c811f08c02d6d52012d93ffc537ea31fa3f54cfc1",
        "captures.json": "24bf9f9cd4b57280ae3fcacf6765c19e86c0aca08d4bbce66e61923852a23cee",
        "u6a-stub-0020.bin": "12954e898df4ab54730e7397e9ee160df8013131814d7fa4b5f66da4367042a5",
        "stopped-12c6-02e6.bin": "9d974e81ae3c7032ea8ae3ed905dffda21fa48912db66bedd80b64ed6f58bd66",
        "loader-stop-complete.bin": "9f1b205450f4becac725d7d29df0f7d97bb5b76a5d41c0e796d05452a681f4f4",
        "drive.py": "c579566cd46edb58449fbd8015b9884da9bdeaa5ac921c7c6cd3b8e47f826fac",
    },
)


def require(ok, message):
    if not ok:
        raise ValueError(message)


def read_locked(path, digest, size=None):
    data = path.read_bytes()
    require(size is None or len(data) == size, f"{path.name}: size mismatch")
    require(sha256(data).hexdigest() == digest, f"{path.name}: hash mismatch")
    return data


def clean_terminal(log):
    # Curses redraws older messages. Repeated records are not additional hits.
    return re.sub(re.escape(bytes([27]) + b"[") + rb"[0-?]*[ -/]*[@-~]", b"", log)


def parse_cpu(log):
    ws = b"[ " + bytes([13, 10, 9]) + b"]"
    pattern = (b"EV of 'CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS' is:" +
               ws + b"*(?:LOG:" + ws + b"*)?((?:[0-9a-fA-F]{1,8}" +
               ws + b"+){13}[0-9a-fA-F]{1,8})(?=" + ws + b"|$)")
    rows = []
    for match in re.finditer(pattern, clean_terminal(log)):
        values = [int(v, 16) for v in match.group(1).split()]
        require(all(v <= 0xffff for v in values), "CPU word exceeds 16 bits")
        rows.append(dict(zip(EV_ORDER, values)))
    require(rows, "no complete EV snapshot")
    return rows


def cpu_bytes(cpu):
    require(set(cpu) == set(FIELDS) and
            all(type(v) is int and 0 <= v <= 65535 for v in cpu.values()),
            "invalid CPU record")
    return struct.pack("<14H", *(cpu[name] for name in FIELDS))


def physical(cpu):
    return ((cpu["cs"] << 4) + cpu["ip"]) & 0xfffff


def nonhit_window(events, cpus, log, target=(0x0899, 0x0025)):
    times = [row["seconds"] for row in events]
    require(all(type(t) in (int, float) and math.isfinite(t) and t >= 0 for t in times)
            and times == sorted(times), "invalid event timeline")
    commands = [row for row in events if row["kind"] == "debug-command"]
    runs = [row for row in commands if row["command"] == "RUN"]
    require(runs, "no resume command")
    command = f"BP {target[0]:04X}:{target[1]:04X}"
    require(any(row["command"] == command and row["seconds"] < runs[0]["seconds"]
                for row in commands), "target BP not armed before resume")
    require((f"Set breakpoint at {target[0]:04X}:{target[1]:04X}").encode()
            in clean_terminal(log), "missing debugger BP confirmation")
    require(not any(row["command"].startswith("BPDEL") for row in commands),
            "BP deletion requires a narrower boundary")
    address = ((target[0] << 4) + target[1]) & 0xfffff
    require(all(physical(cpu) != address for cpu in cpus), "target BP stop observed")
    return runs[0]["seconds"]


def audit(first, second, emulator):
    require(sha256(emulator.read_bytes()).hexdigest() == EMULATOR_SHA,
            "emulator identity mismatch")
    products = []
    for directory, locks, limit in zip((first, second), LOCKS, (420, 120)):
        require(directory.resolve().is_relative_to((ROOT / "resources/oracle-runs").resolve()),
                "capture must remain under ignored resources/oracle-runs")
        for name, digest in INPUTS.items():
            read_locked(directory / "POOLRAD" / name, digest)
        data = {name: read_locked(directory / name, digest) for name, digest in locks.items()}
        events, summary = json.loads(data["events.json"]), json.loads(data["summary.json"])
        require(summary["bound_seconds"] == limit and
                0 < summary["elapsed_seconds"] <= limit + 5, "wall bound mismatch")
        require(not summary["control_reached"] and summary["runtime_segment"] is None,
                "unexpected reached-frame claim")
        cpus = parse_cpu(data["debugger.log"])
        resume = nonhit_window(events, cpus, data["debugger.log"])
        products.append((data, events, cpus, resume))
    first_data, first_events, first_cpus, resume = products[0]
    require({(c["cs"], c["ip"]) for c in first_cpus} ==
            {(0x1796, 0x12), (0x2116, 0x539)}, "first-pass stop boundary changed")
    pause = next(row["seconds"] for row in first_events
                 if row["kind"] == "ui-input" and row["keys"] == ["Alt_L+Pause"])
    print(f"first pass: BP 0899:0025 nonhit; resume {resume:.3f}s -> manual pause {pause:.3f}s")
    print("first pass: observed CS:IP 1796:0012 and 2116:0539; no saved-prompt control stop")

    data, events, cpus, resume = products[1]
    rows = json.loads(data["captures.json"])
    require(len(rows) == 2, "unexpected capture count")
    expected_cpu = (
        "897ab17f7243d52cd8458a275e9c90127ceeb52729f80e75dad3ca9c550d60e3",
        "5aa4c97faef0e4977ec62d1ec581e37940b4aab48f6493f5a40f3838ec5feeee",
    )
    for row, digest in zip(rows, expected_cpu):
        require(sha256(cpu_bytes(row["cpu"])).hexdigest() == digest and row["cpu"] in cpus,
                "captured CPU does not match hashed debugger transcript")
    stub = data["u6a-stub-0020.bin"]
    complete, partial = data["loader-stop-complete.bin"], data["stopped-12c6-02e6.bin"]
    require(len(stub) == len(complete) == 655360 and len(partial) == 462848 and
            complete.startswith(partial), "measured RAM range/prefix mismatch")
    exe = (second / "POOLRAD/START.EXE").read_bytes()
    header = struct.unpack_from("<H", exe, 8)[0] * 16
    # Original descriptor table -> runtime slot formula from overlay_inventory.py.
    source_stubs = exe[header + 1725:header + 1740]
    require(len(source_stubs) == 15 and stub[0x89b0:0x89bf] == source_stubs and
            complete[0x89b0:0x89bf] == source_stubs, "native u6A stub identity mismatch")
    def word(memory, at):
        return struct.unpack_from("<H", memory, at)[0]
    require(word(stub, 0x1d16*16 + 0x3ffc) == 0x00ab and
            word(stub, 0x1d16*16 + 0x3ffe) == 0x0824, "startup far-return frame mismatch")
    interrupt = 0x1d16*16 + 0x3ff6
    require(tuple(word(complete, interrupt + i*2) for i in range(3)) ==
            (0x0022, 0x0899, 0x7246), "loader interrupt frame mismatch")
    ovr = (second / "POOLRAD/GAME.OVR").read_bytes()
    body = ovr[0x18055:0x180f0]
    require(sha256(body).hexdigest() ==
            "c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7",
            "original leaf identity mismatch")
    require(body not in stub and body not in complete, "leaf body unexpectedly loaded in measured RAM")
    print("second pass: native u6A slot-0 stop 0899:0020; far return 0824:00AB")
    print("second pass: watch stop 12C6:02E6; interrupt return 0899:0022")
    print("RAM: two complete [00000,A0000) captures; partial [00000,71000) is a verified prefix")
    print("u6A resident slots 0/1/2 match original hashes; +0824 absent from measured RAM")
    print("leaf BP/frame/parity UNMEASURED; startup stub is not a leaf hit; no strict credit")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first", type=Path)
    parser.add_argument("second", type=Path)
    parser.add_argument("--emulator", type=Path, default=Path("/tmp/poolrad-heavy-dosbox-bin/dosbox-x"))
    args = parser.parse_args()
    audit(args.first, args.second, args.emulator)


if __name__ == "__main__":
    main()
