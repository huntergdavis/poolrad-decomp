#!/usr/bin/env python3
"""Pin original U86 +019C LEA DI, +019F stop, and fixup exclusion."""

from hashlib import sha256
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from overlay_inventory import find_units, parse_mz  # noqa: E402

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LEA_SHA = "5a767b97c2cf4c7fa8157c8ba3f23d55e9e26b3da05163f8adcef919972d23dc"
STOP_SHA = "7cb7c4547cf2653590d7a9ace60cc623d25148adfbc88a89aeb0ef88da7839ba"


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: check_overlay_u86_entry1_postdc0_lea_di_019c_static.py START.EXE GAME.OVR")
    exe = Path(sys.argv[1]).read_bytes()
    game = Path(sys.argv[2]).read_bytes()
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    assert len(game) == 206105 and sha256(game).hexdigest() == OVR_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], game)
                if u["stub_para"] == 0x86)
    assert (unit["ovr_offset"], unit["code_size"], unit["entries"][1]) == (
        0x27018, 0x37B5, 0x008A)
    lea = game[0x271B4:0x271B7]
    stop = game[0x271B7:0x271B8]
    assert lea == bytes((0x8D, 0x7E, 0xCE))
    assert stop == bytes((0x16,))
    assert sha256(lea).hexdigest() == LEA_SHA
    assert sha256(stop).hexdigest() == STOP_SHA
    fixup_bytes = {fx + byte for fx in unit["fixups"] for byte in (0, 1)}
    assert set(range(0x019C, 0x01A0)).isdisjoint(fixup_bytes)
    print("U86 +019C original/loaded LEA DI and +019F PUSH SS stop: PASS")


if __name__ == "__main__":
    main()
