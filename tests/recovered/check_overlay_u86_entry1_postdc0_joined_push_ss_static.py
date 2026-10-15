#!/usr/bin/env python3
"""Pin U86's joined PUSH SS and following PUSH DI boundary."""

from hashlib import sha256
from pathlib import Path
import sys

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from overlay_inventory import find_units, parse_mz  # noqa: E402

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
PRIOR_SHA = "5a767b97c2cf4c7fa8157c8ba3f23d55e9e26b3da05163f8adcef919972d23dc"
PUSH_SHA = "7cb7c4547cf2653590d7a9ace60cc623d25148adfbc88a89aeb0ef88da7839ba"
NEXT_SHA = "fcb5f40df9be6bae66c1d77a6c15968866a9e6cbd7314ca432b019d17392f6f4"


def main():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    unit = next(u for u in units if u["stub_para"] == 0x86)
    assert (unit["ovr_offset"], unit["code_size"], unit["entries"][1]) == (
        0x27018, 0x37B5, 0x8A)
    assert all(not (0x19C <= fx + byte < 0x1A1)
               for fx in unit["fixups"] for byte in (0, 1))
    prior = ovr[0x271B4:0x271B7]
    push = ovr[0x271B7:0x271B8]
    successor = ovr[0x271B8:0x271B9]
    assert sha256(prior).hexdigest() == PRIOR_SHA
    assert sha256(push).hexdigest() == PUSH_SHA
    assert sha256(successor).hexdigest() == NEXT_SHA
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    def decoded(data, address):
        return [(i.address, i.mnemonic, i.op_str, i.size)
                for i in decoder.disasm(data, address)]
    assert decoded(prior, 0x19C) == [
        (0x19C, "lea", "di, [bp - 0x32]", 3)]
    assert decoded(push, 0x19F) == [(0x19F, "push", "ss", 1)]
    assert decoded(successor, 0x1A0) == [(0x1A0, "push", "di", 1)]
    print("U86 +019F original/loaded PUSH SS and +01A0 PUSH DI PASS")


if __name__ == "__main__":
    main()
