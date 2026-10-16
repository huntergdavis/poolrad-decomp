#!/usr/bin/env python3
"""Pin U86's post-fourth-PUSH MOV AL,0 and following PUSH AX."""

from hashlib import sha256
from pathlib import Path
import sys

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from overlay_inventory import find_units, parse_mz  # noqa: E402

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
PRIOR_SHA = "5c62e091b8c0565f1bafad0dad5934276143ae2ccef7a5381e8ada5b1a8d26d2"
MOV_SHA = "46cc6a8cb5bcc0120f98b84c71699b2ac5b90cb34a781ae2a83305fa98cd8e67"


def main():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    unit = next(u for u in units if u["stub_para"] == 0x86)
    assert (unit["ovr_offset"], unit["code_size"], unit["entries"][1]) == (
        0x27018, 0x37B5, 0x8A)
    assert all(not (0x15D <= fx + byte < 0x161)
               for fx in unit["fixups"] for byte in (0, 1))
    prior = ovr[0x27175:0x27176]
    mov = ovr[0x27176:0x27178]
    successor = ovr[0x27178:0x27179]
    assert sha256(prior).hexdigest() == PRIOR_SHA
    assert sha256(mov).hexdigest() == MOV_SHA
    assert sha256(successor).hexdigest() == PRIOR_SHA
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    def decoded(data, address):
        return [(i.address, i.mnemonic, i.op_str, i.size)
                for i in decoder.disasm(data, address)]
    assert decoded(prior, 0x15D) == [(0x15D, "push", "ax", 1)]
    assert decoded(mov, 0x15E) == [(0x15E, "mov", "al, 0", 2)]
    assert decoded(successor, 0x160) == [(0x160, "push", "ax", 1)]
    print("U86 +015E original/loaded MOV AL,0 and +0160 PUSH AX PASS")


if __name__ == "__main__":
    main()
