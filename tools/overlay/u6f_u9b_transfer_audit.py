#!/usr/bin/env python3
"""Verify the u6F insertion's actual u9B overlay entry without publishing bytes."""

import argparse
from hashlib import sha256
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

from resident_loader_map import HEADER_BYTES, find_units, unpack


EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
U6F_SHA = "370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245"
U9B_ZERO_SHA = "260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71"
STUB_25_SHA = "d67da1e87a1a5cac157296d1e710a6673b63b369f977f13bc9426973402e8103"
STUB_2A_SHA = "26e4a8aad0d6405d9d752df2d3725f4834cab36f96a1e47c81d7e08236276b3d"
HANDLER_SHA = "6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c"
LOADER_SHA = "58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991"


def check(condition, message):
    if not condition:
        raise ValueError(message)


def digest(data):
    return sha256(data).hexdigest()


def instructions(data, base):
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, base))
    check(decoded and decoded[-1].address + decoded[-1].size == base + len(data),
          f"incomplete instruction decode at {base:#x}")
    return {item.address: item for item in decoded}


def audit(exe_path, ovr_path):
    exe = exe_path.read_bytes()
    ovr = ovr_path.read_bytes()
    check(digest(exe) == EXE_SHA, "unsupported START.EXE")
    check(digest(ovr) == OVR_SHA, "unsupported GAME.OVR")
    image, origin, *_ = unpack(exe)

    units = [unit for unit in find_units(exe[HEADER_BYTES:], ovr)
             if unit["stub_para"] == 0x9B]
    check(len(units) == 1, "unit 0x9B descriptor is not unique")
    unit = units[0]
    check(unit["memory_segment"] == 0xAC and unit["ovr_offset"] == 0x2AA87,
          "unit 0x9B placement changed")
    check(unit["entries"] == [0x45E, 0x0000, 0x01AF, 0x0255, 0x0329],
          "unit 0x9B entry table changed")

    caller = ovr[0x1F42C:0x1F565]
    check(digest(caller) == U6F_SHA, "u6F insertion span changed")
    call = instructions(caller, 0x1446)[0x1574]
    check(call.mnemonic == "lcall" and call.op_str == "0xac, 0x25",
          "u6F transfer operand changed")

    for offset, expected_word, expected_sha in (
            (0x25, 0x0000, STUB_25_SHA), (0x2A, 0x01AF, STUB_2A_SHA)):
        start = (0xAC << 4) + offset
        stub = image[start:start + 5]
        check(digest(stub) == expected_sha, f"stub {offset:#x} changed")
        interrupt = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(stub[:2], offset))
        check(len(interrupt) == 1 and interrupt[0].mnemonic == "int" and
              interrupt[0].op_str == "0x3f" and
              int.from_bytes(stub[2:4], "little") == expected_word,
              f"stub {offset:#x} target changed")
        check(origin[start] is not None, f"stub {offset:#x} lacks source provenance")

    zero = ovr[unit["ovr_offset"]:unit["ovr_offset"] + 0x1AF]
    check(len(zero) == 431 and digest(zero) == U9B_ZERO_SHA,
          "u9B entry +0 body changed")
    body = instructions(zero, 0)
    check(len(body) == 159 and body[0].mnemonic == "push" and
          body[0x1AC].mnemonic == "retf", "u9B +0 extent changed")
    check([(address, body[address].mnemonic, body[address].op_str)
           for address in (0xEC, 0x191, 0x1A6)] == [
               (0xEC, "call", "0x1af"),
               (0x191, "call", "0x255"),
               (0x1A6, "call", "0x329")],
          "u9B +0 internal call edges changed")

    handler_base = (0xAA2 << 4) + 0x2E6
    handler = image[handler_base:handler_base + 0x52]
    loader = image[handler_base + 0x52:handler_base + 0x103]
    check(digest(handler) == HANDLER_SHA and digest(loader) == LOADER_SHA,
          "resident INT 3Fh dispatch changed")
    dispatch = instructions(handler, 0x2E6)
    check(dispatch[0x2F7].mnemonic == "les" and
          dispatch[0x2FA].mnemonic == "push" and
          dispatch[0x2FD].mnemonic == "sub" and
          dispatch[0x32A].mnemonic == "lcall" and
          dispatch[0x337].mnemonic == "iret",
          "INT 3Fh save/dispatch/return path changed")
    check(HEADER_BYTES + origin[handler_base] == 0xA9FC,
          "handler EXE source mapping changed")

    print(f"START.EXE {len(exe)} SHA-256 {EXE_SHA}")
    print(f"GAME.OVR {len(ovr)} SHA-256 {OVR_SHA}")
    print("u6F +1574: LCALL 00AC:0025; stub descriptor +0000")
    print("adjacent 00AC:002A: descriptor +01AF")
    print(f"u9B +0000..+01AF: {len(zero)} bytes, {len(body)} instructions, SHA-256 {U9B_ZERO_SHA}")
    print("u9B +0000 near calls: +00EC -> +01AF, +0191 -> +0255, +01A6 -> +0329")
    print("handler AA2:02E6..0338: 82 bytes, SHA-256 " + HANDLER_SHA)
    print("handler AA2:0338..03E9: 177 bytes, SHA-256 " + LOADER_SHA)


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=root / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path,
                        default=root / "resources/DOS/POOLRAD/GAME.OVR")
    args = parser.parse_args()
    audit(args.exe, args.ovr)


if __name__ == "__main__":
    main()
