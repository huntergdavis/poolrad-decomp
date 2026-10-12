#!/usr/bin/env python3
"""Verify the saved u6F -> u9B runtime transfer without publishing RAM."""

import argparse
from hashlib import sha256
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

from u6f_u9b_transfer_audit import U9B_ZERO_SHA, audit as audit_original


SNAPSHOTS = {
    "pre-call-1574-ram.bin": "eb4341f1432fa965fde922a55d12e05ff56f6399ec4dfc8d1970a7dcb3279ca5",
    "handler-entry-02e6-ram.bin": "6c5eb4b2371d0d601081ea899629da575c16d9b0f10420283a7618b6a564d0a2",
    "pre-loader-0314-ram.bin": "225757953b6cc0467b2fbda2de973cccabb87d55921491e26f739bb97cb1a5bf",
    "pre-indirect-032a-ram.bin": "4917a4f5ef5032464b28f1dacc6e1440352bbbb009c6f676edf2001d1c9a1289",
    "u9b-entry-0000-ram.bin": "241080db30401c3c6ca757e556e632443d6dfbff19657254cd82bff99a9767d3",
    "u9b-after-0009-ram.bin": "6b13420aa124f52472214c7f830a9a6e20865c3a013065bb9cc137b334d53a24",
    "u9b-pre-0255-ram.bin": "0b50157ab03676d796b60f6ccda0b73fa17556a5775f49c91154f81faf0b8705",
    "u9b-return-01ac-ram.bin": "e08be366b1571cfa234c305fb94b23394b264889f7b7f00f370b12baef73c29c",
    "u6f-return-1579-ram.bin": "e08be366b1571cfa234c305fb94b23394b264889f7b7f00f370b12baef73c29c",
}


def check(condition, message):
    if not condition:
        raise ValueError(message)


def physical(segment, offset):
    return ((segment << 4) + offset) & 0xFFFFF


def word(memory, segment, offset):
    address = physical(segment, offset)
    return int.from_bytes(memory[address:address + 2], "little")


def first_instruction(memory, segment, offset):
    address = physical(segment, offset)
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
        memory[address:address + 8], offset, count=1))
    check(len(decoded) == 1, f"cannot decode {segment:04X}:{offset:04X}")
    return decoded[0]


def audit_native(directory):
    ram = {}
    for name, expected in SNAPSHOTS.items():
        data = (directory / name).read_bytes()
        check(len(data) == 0xA0000 and sha256(data).hexdigest() == expected,
              f"private snapshot changed: {name}")
        ram[name] = data

    before = ram["pre-call-1574-ram.bin"]
    handler = ram["handler-entry-02e6-ram.bin"]
    pre_loader = ram["pre-loader-0314-ram.bin"]
    dispatch = ram["pre-indirect-032a-ram.bin"]
    entry = ram["u9b-entry-0000-ram.bin"]
    first_write = ram["u9b-after-0009-ram.bin"]
    loop_exit = ram["u9b-pre-0255-ram.bin"]
    returned = ram["u9b-return-01ac-ram.bin"]
    caller = ram["u6f-return-1579-ram.bin"]

    check((word(before, 0x1D16, 0x379A), word(before, 0x1D16, 0x379C)) ==
          (0x0008, 0x449A), "pre-call far argument changed")
    check(tuple(word(handler, 0x1D16, offset) for offset in
                (0x3790, 0x3792, 0x3796, 0x3798)) ==
          (0x0027, 0x08D0, 0x1579, 0x2A8E),
          "INT 3Fh saved frame changed")
    check(word(pre_loader, 0x1D16, 0x3790) == 0x0025,
          "handler did not rewind saved stub IP")
    check((word(dispatch, 0x14A0, 0x43F0),
           word(dispatch, 0x14A0, 0x43F2)) == (0x02E5, 0x12C6),
          "DS:43F0 dispatch target changed")
    check(first_instruction(dispatch, 0x12C6, 0x02E5).mnemonic == "retf",
          "DS:43F0 target is not RETF")
    jump = first_instruction(dispatch, 0x08D0, 0x0025)
    check(jump.mnemonic == "ljmp" and jump.op_str == "0x2ccd:0",
          "patched overlay stub target changed")

    loaded_body = entry[physical(0x2CCD, 0):physical(0x2CCD, 0x1AF)]
    check(sha256(loaded_body).hexdigest() == U9B_ZERO_SHA,
          "loaded u9B entry body changed")
    check(tuple(word(entry, 0x1D16, offset) for offset in
                (0x3796, 0x3798, 0x379A, 0x379C)) ==
          (0x1579, 0x2A8E, 0x0008, 0x449A),
          "u9B far-call frame changed")
    record = physical(0x449A, 0x0008)
    check(entry[record + 0x2D] == 0x28 and
          first_write[record + 0x2D] == 0 and
          loop_exit[record + 0x2D] == 0x28,
          "first write or loop recomputation changed")
    check(entry[record + 0x96] == 0 and
          loop_exit[record + 0x96] == 0 and
          returned[record + 0x9C] == 0,
          "saved internal-call gates changed")
    check(entry[record:record + 0xC0] ==
          returned[record:record + 0xC0],
          "saved record has a net byte change")
    check(returned == caller and
          word(caller, 0x1D16, 0x3796) == 0x1579,
          "u9B return/caller snapshot changed")

    print(f"private native snapshots: {len(ram)} hash-locked 640 KiB captures")
    print("INT 3Fh: 12C6:02E6; DS:43F0 -> 12C6:02E5 RETF")
    print("stub 08D0:0025 patched to far jump 2CCD:0000")
    print("u9B +0009: record +2D cleared; +0191 reached; +01AC returns")
    print("saved +01AF and +0329 branch inputs zero; no net record change")


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=root / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path,
                        default=root / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--native-dir", type=Path,
                        help="directory of the ignored, exact native captures")
    args = parser.parse_args()
    audit_original(args.exe, args.ovr)
    if args.native_dir is not None:
        audit_native(args.native_dir)


if __name__ == "__main__":
    main()
