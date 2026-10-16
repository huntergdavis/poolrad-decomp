#!/usr/bin/env python3
"""Pin original U86 +01ED JZ, two immediate stops, and fixup exclusion."""

from hashlib import sha256
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from overlay_inventory import find_units, parse_mz  # noqa: E402

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
JZ_SHA = "1494e884fa55e9a6a34261f4b652305a2bd9fd2085830ab6a6977abbc44a5234"
TARGET_SHA = "5a767b97c2cf4c7fa8157c8ba3f23d55e9e26b3da05163f8adcef919972d23dc"
FALL_SHA = "f9b2f007fe9d534e5759e0fa95b981e1017c25c8ac31719c77d5345e5134d0cc"


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: check_overlay_u86_entry1_postdc0_jz_01ed_static.py START.EXE GAME.OVR")
    exe = Path(sys.argv[1]).read_bytes()
    game = Path(sys.argv[2]).read_bytes()
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    assert len(game) == 206105 and sha256(game).hexdigest() == OVR_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], game)
                if u["stub_para"] == 0x86)
    assert (unit["ovr_offset"], unit["code_size"], unit["entries"][1]) == (
        0x27018, 0x37B5, 0x008A)
    assert game[0x27200:0x27205] == bytes((0x9A, 0xC0, 0x0D, 0xF8, 0x0A))
    branch = game[0x27205:0x27207]
    target = game[0x271B4:0x271B7]
    fallthrough = game[0x27207:0x2720A]
    assert branch == bytes((0x74, 0xAD))
    assert target == bytes((0x8D, 0x7E, 0xCE))
    assert fallthrough == bytes((0xA1, 0x66, 0x47))
    assert sha256(branch).hexdigest() == JZ_SHA
    assert sha256(target).hexdigest() == TARGET_SHA
    assert sha256(fallthrough).hexdigest() == FALL_SHA
    displacement = int.from_bytes(branch[1:], "little", signed=True)
    assert (0x01EF + displacement) & 0xFFFF == 0x019C
    sites = (*range(0x019C, 0x019F), *range(0x01ED, 0x01F2))
    fixup_bytes = {fx + byte for fx in unit["fixups"] for byte in (0, 1)}
    assert set(sites).isdisjoint(fixup_bytes)
    print("U86 +01ED original JZ and +019C/+01EF stops: PASS")


if __name__ == "__main__":
    main()
