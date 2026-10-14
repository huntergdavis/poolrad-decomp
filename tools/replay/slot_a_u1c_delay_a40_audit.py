#!/usr/bin/env python3
"""Hash-lock the U1C/0709/A40 delay chain using ignored DOS inputs.

Only addresses, hashes, and numeric values are printed. No game data is
written to the public tree.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import sys


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "overlay"))
from resident_loader_map import unpack  # noqa: E402


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
RAM_SHA = "107f1f0b4dbcecffc8fed3744b4d7f55b26e65186a2628401d586e354a19ab32"


def digest(data):
    return sha256(data).hexdigest()


def require_span(data, offset, size, expected, label):
    span = data[offset:offset + size]
    assert len(span) == size, f"{label}: truncated span"
    actual = digest(span)
    assert actual == expected, f"{label}: unexpected hash {actual}"
    print(f"{label}: offset={offset:#x} size={size} sha256={actual}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("start", type=Path, help="ignored DOS START.EXE")
    parser.add_argument("game", type=Path, help="ignored DOS GAME.OVR")
    parser.add_argument("ram", type=Path, help="ignored private 640 KiB RAM dump")
    parser.add_argument("save", type=Path, help="ignored slot-A SAVGAMA.DAT")
    args = parser.parse_args()

    start, game, ram, save = (p.read_bytes() for p in
                              (args.start, args.game, args.ram, args.save))
    for label, data, expected in (("START", start, START_SHA),
                                  ("GAME", game, GAME_SHA),
                                  ("RAM", ram, RAM_SHA),
                                  ("save", save, SAVE_SHA)):
        assert digest(data) == expected, f"{label}: unsupported hash"
        print(f"{label}: size={len(data)} sha256={expected}")
    assert len(ram) == 640 * 1024

    image, *_ = unpack(start)
    require_span(game, 0x3465, 14,
                 "aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1",
                 "U1C original +271F..+272D")
    require_span(image, 0x7E4E, 21,
                 "a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d",
                 "0709 original +0DBE..+0DD3")
    require_span(image, 0xA69C, 33,
                 "7ee3c156a50e06d1f23e4da738158d3cb48179223d3135c55b7976eae426e75e",
                 "A40 original +029C..+02BD")
    require_span(image, 0xA6BD, 12,
                 "d2df9bf05a361b9f27a4fe2591ea9938c31fb9d319ea1640d73bfbc52d93a2ea",
                 "A40 original +02BD..+02C9")
    require_span(ram, 0x3582 * 16 + 0x271F, 14,
                 "ecb6e7a981de47539c4809d0338832f0c107bf96a9e83d65e2847bf5a2f14ec2",
                 "U1C loaded +271F..+272D")
    require_span(ram, 0x0F2D * 16 + 0x0DBE, 21,
                 "7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be",
                 "0709 loaded +0DBE..+0DD3")
    require_span(ram, 0x1264 * 16 + 0x029C, 45,
                 "d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb",
                 "A40 loaded +029C..+02C9")

    ds = 0x14A0 * 16
    delay_factor = ram[ds + 0x49E2]
    calibration = int.from_bytes(ram[ds + 0x84FC:ds + 0x84FE], "little")
    bios_byte = ram[0x40 * 16]
    assert (delay_factor, calibration, bios_byte) == (4, 269, 248)
    print(f"separate PRINTCLEAR snapshot: factor={delay_factor} "
          f"units={delay_factor * 225} calibration={calibration} "
          f"bios_byte={bios_byte}")
    print("No U1C DELAY or A40 runtime reachability is asserted by this snapshot.")


if __name__ == "__main__":
    main()
