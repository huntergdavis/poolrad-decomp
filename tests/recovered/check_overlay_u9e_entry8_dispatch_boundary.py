#!/usr/bin/env python3
"""Pin the original U7F call and U9E entry-8 stub; execute no overlay."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "overlay"))
from overlay_inventory import find_units, parse_mz  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
STUB_SHA = "16e87d451726931b6e084c2a73e82f385b41e3886c5d7a9806ec0fd814f991f2"
CALL_SHA = "e4551a2bc99d5c8ac9cc97784911c182c2470f56668e8cf8bdb3a895ea4d9115"
ENTRY_SHA = "7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba"


def checked(data, expected, name):
    digest = sha256(data).hexdigest()
    if digest != expected:
        raise ValueError(f"{name} SHA-256 differs: {digest}")


def audit(exe, ovr):
    checked(exe, EXE_SHA, "START.EXE")
    checked(ovr, OVR_SHA, "GAME.OVR")
    header = parse_mz(exe)["header_bytes"]
    units = {u["stub_para"]: u for u in find_units(exe[header:], ovr)}
    caller, callee = units[0x7F], units[0x9E]
    if (callee["ovr_offset"] != 0x2AEEA or
            callee["memory_segment"] != 0xB0 or
            callee["entries"][8:10] != [0x0DE3, 0x0E2E]):
        raise ValueError("U9E descriptor, live stub segment, or entry bounds differ")

    packed_at = callee["stub_base"] + 5 * 8
    packed = exe[header + packed_at:header + packed_at + 5]
    checked(packed, STUB_SHA, "packed B0:0048 stub")
    if packed != bytes.fromhex("cd3fe30d00"):
        raise ValueError("packed stub is not INT 3Fh followed by +0DE3")

    expanded, origin, _, _ = unpack(exe)
    live_at = (0xB0 << 4) + 0x48
    live = bytes(expanded[live_at:live_at + 5])
    checked(live, STUB_SHA, "expanded B0:0048 stub")
    if live != packed or origin[live_at:live_at + 5] != list(range(packed_at, packed_at + 5)):
        raise ValueError("expanded stub differs from packed descriptor")

    caller_at = caller["ovr_offset"] + 0x0FDE
    call = ovr[caller_at:caller_at + 5]
    checked(call, CALL_SHA, "U7F +0FDE far call")
    if call != bytes.fromhex("9a4800b000") or 0x0FE1 not in caller["fixups"]:
        raise ValueError("U7F call or segment fixup differs")

    entry_at = callee["ovr_offset"] + 0x0DE3
    entry = ovr[entry_at:entry_at + 75]
    checked(entry, ENTRY_SHA, "U9E entry8 body")
    return {
        "u7f_call_file": caller_at,
        "u7f_call_sha256": CALL_SHA,
        "u9e_stub_file": header + packed_at,
        "u9e_expanded_image": live_at,
        "u9e_stub_sha256": STUB_SHA,
        "u9e_entry_file": entry_at,
        "u9e_entry_bytes": len(entry),
        "u9e_entry_sha256": ENTRY_SHA,
        "dispatch_executed": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.exe.read_bytes(), args.ovr.read_bytes()), indent=2))


if __name__ == "__main__":
    main()
