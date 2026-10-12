#!/usr/bin/env python3
"""Hash-locked address-only audit of ECL RANDOM's SYSTEM conversion chain."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack

OVR_SHA256 = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA256 = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
HANDLER_SHA256 = "a634c69e97fe005c2041e9d18d0874d0eb8ede7f4a59833bfb642918b7c5d979"
SPANS = (
    ("real_encode_entry", 0x0C11A, 0x0C11E, 2,
     "e962b3c25deea98a57e98aac984c325f9cfc8dacebb4c508fdd5943756fad95a"),
    ("real_decode_entry", 0x0C11E, 0x0C126, 4,
     "072143d5fffbb6d2bdebaf88a86cbff0b593feaf31aaae45fbe5c312144926cc"),
    ("real_encode_body", 0x0C059, 0x0C098, 30,
     "64b69941b112ea462aea3848edd98e4f3ec85523f1b631a38fda67942f1c9251"),
    ("real_decode_body", 0x0C098, 0x0C0F4, 44,
     "7ccd6539f95a342fa34f081470130d1ed34749d7761369cb05dc02a5723c1234"),
    ("random", 0x0C140, 0x0C156, 10,
     "e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9"),
    ("seed", 0x0C18F, 0x0C1C5, 22,
     "f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242"),
)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def supported_real_roundtrip(value):
    """Restate the reached 10D9/1118 register path, including shift carry."""
    require(0 <= value <= 254, "outside handler's random range")
    if value == 0:
        # 10D9 jumps to RET with AX=BX=DX=0. 1118's large shift
        # count returns zero; the preceding unsigned CMP leaves CF=0.
        return 0, 0, 0
    ch = 0                      # +10DF: input DX=0, CH=DH
    bx = 0                      # +10F8 xchg BX,DX; BX was the value
    dx = value << 8             # +10FD xchg DL,DH (value is below 256)
    exponent = 0x88            # +10FB
    while dx < 0x8000:          # +1104..+110E normalization
        exponent -= 1
        bx = (bx * 2) & 0xFFFF
        dx = (dx * 2) & 0xFFFF
    dx &= 0x7FFF                # +1114 and DH,7F
    ax = exponent              # AX exponent; BX remains zero
    require(ch == 0 and bx == 0, "unexpected encode sign/mantissa")
    if value == 10:
        require(ax == 132 and dx == 8192,
                "native value-10 Real encoding checkpoint changed")

    bx, ax = ax, bx             # +1118 xchg BX,AX
    cl = 0x9F - (bx & 0xFF)     # +1119..+111D
    require(0 <= cl <= 0x1F, "unexpected decode overflow/underflow")
    cl += 1
    bh = dx >> 8                # +1126: sign snapshot
    dx |= 0x8000               # +1128: restore hidden leading bit
    require(cl >= 0x11, "unexpected short decode path")
    ax, dx = dx, 0             # +1130..+1132
    cl -= 0x10
    if cl >= 9:                 # +1137..+1144
        ax = (ax >> 8) & 0xFF
        cl -= 8
    carry = 0
    while cl:                   # +1147 SHR DX; RCR AX; DEC CL
        carry = dx & 1
        dx >>= 1
        next_carry = ax & 1
        ax = (ax >> 1) | (carry << 15)
        carry = next_carry
        cl -= 1
    require(carry == 0 and bh < 0x80,
            "unexpected rounding or negative-result branch")
    return ax, dx, 0            # +116B CLC, +11A5 RETF


def audit(exe, ovr):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16

    require(sha256(exe).hexdigest() == EXE_SHA256, "START hash changed")
    require(sha256(ovr).hexdigest() == OVR_SHA256, "OVR hash changed")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x1C)
    handler = ovr[unit["ovr_offset"] + 0x0234:
                  unit["ovr_offset"] + 0x0292]
    h = {i.address: i for i in decoder.disasm(handler, 0x0234)}
    require(unit["ovr_offset"] == 0x0D46 and
            len(handler) == 94 and len(h) == 35 and
            sum(i.size for i in h.values()) == len(handler) and
            sha256(handler).hexdigest() == HANDLER_SHA256,
            "handler extent changed")
    sites = {0x0269: ("xor", "ah, ah"),
             0x026C: ("lcall", "0xaf8, 0x11c0"),
             0x0271: ("xor", "dx, dx"),
             0x0273: ("lcall", "0xaf8, 0x119a"),
             0x0278: ("lcall", "0xaf8, 0x119e"),
             0x027D: ("mov", "byte ptr [bp - 2], al"),
             0x0286: ("xor", "ah, ah"),
             0x0289: ("lcall", "0x2b, 0x52")}
    require(all(h[a].mnemonic == op and h[a].op_str == args
                for a, (op, args) in sites.items()), "handler runtime/store flow changed")

    image, origin, _, _ = unpack(exe)
    spans = []
    decoded = {}
    for name, lo, hi, count, digest in SPANS:
        data = bytes(image[lo:hi])
        ins = list(decoder.disasm(data, lo))
        require(sha256(data).hexdigest() == digest and len(ins) == count and
                sum(i.size for i in ins) == hi - lo and
                all(origin[p] is not None and origin[p + 1] == origin[p] + 1
                    for p in range(lo, hi - 1)),
                f"SYSTEM span changed: {name}")
        decoded[name] = {i.address: i for i in ins}
        spans.append({"name": name, "image_start": lo,
                      "image_end_exclusive": hi, "file_start": origin[lo] + 0x200,
                      "file_end_exclusive": origin[hi - 1] + 0x201,
                      "bytes": hi - lo, "instructions": count, "sha256": digest})
    encode = decoded["real_encode_entry"]
    decode = decoded["real_decode_entry"]
    require(encode[0x0C11A].op_str == "0xc059" and
            encode[0x0C11D].mnemonic == "retf" and
            decode[0x0C11E].op_str == "ch, 0" and
            decode[0x0C120].op_str == "0xc098" and
            decode[0x0C123].op_str == "0xc12e" and
            decode[0x0C125].mnemonic == "retf",
            "Real wrapper entry/overflow path changed")
    require(decoded["random"][0x0C140].op_str == "0xc18f" and
            decoded["random"][0x0C14D].op_str == "0xc153" and
            decoded["random"][0x0C150].op_str == "bx" and
            decoded["seed"][0x0C18F].op_str == "ax, word ptr [0x4428]" and
            decoded["seed"][0x0C1C0].op_str == "word ptr [0x442a], dx" and
            int.from_bytes(image[0x0C1C5:0x0C1C7], "little") == 0x8405,
            "RNG seed, range, or multiplier changed")
    for value in range(255):
        require(supported_real_roundtrip(value) == (value, 0, 0),
                f"Real roundtrip changed at {value}")
    live_seed = (3140872353 * 0x08088405 + 1) & 0xFFFFFFFF
    require(live_seed == 3238563622 and (live_seed >> 16) % 14 == 10,
            "native seed/range checkpoint changed")
    return {"input_sha256": {"START.EXE": EXE_SHA256,
                             "GAME.OVR": OVR_SHA256},
            "handler": {"unit": "0x1C", "offset": 0x0234,
                        "end_exclusive": 0x0292, "file_start": 0x0F7A,
                        "file_end_exclusive": 0x0FD8, "bytes": 94,
                        "instructions": 35, "sha256": HANDLER_SHA256},
            "system": spans, "real_roundtrip_values": 255,
            "real_roundtrip_max": 254}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--ovr", required=True)
    args = parser.parse_args()
    print(json.dumps(audit(Path(args.exe).read_bytes(),
                           Path(args.ovr).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
