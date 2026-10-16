#!/usr/bin/env python3
"""Check the bounded U19 +02DD helper return against retained RAM.

Read only ignored original media and same-run RAM; print addresses and counts.
The 640-KiB RAM captures do not include EGA plane memory.
"""

import argparse
from hashlib import sha256
from pathlib import Path
from struct import unpack_from

from slot_a_printer_page_audit import unpack


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
HELPER_SHA = "1a9b687c96c69b4b791be00bd72730f6df05513ce522a18d53041f8846ce4d93"
CALL_SHA = "ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356"
PRE_SHA = "84191adc6aab0f6272fe0dc128b1cbff06668a01c9c40fd5fc1202d490b06699"
POST_SHA = "081901c405f2d67a3e5fa68a34bfa67a536f25b39c9641acfdcc7b5863d66f18"


def require(condition, explanation):
    if not condition:
        raise ValueError(explanation)


def checked(path, expected):
    data = path.read_bytes()
    require(sha256(data).hexdigest() == expected, f"{path.name} hash differs")
    return data


def audit(exe_path, ovr_path, run_path):
    exe = checked(exe_path, START_SHA)
    ovr = checked(ovr_path, OVR_SHA)
    image, *_ = unpack(exe)
    helper = image[0x8A94:0x8AB2]
    call = ovr[0x09AC + 0x02DD:0x09AC + 0x02E2]
    require(len(helper) == 30 and sha256(helper).hexdigest() == HELPER_SHA,
            "RES_0899:0104 original span differs")
    require(len(call) == 5 and sha256(call).hexdigest() == CALL_SHA and
            call[0] == 0x9A and unpack_from("<HH", call, 1) == (0x0104, 0x0899),
            "U19 +02DD far call differs")
    # Original +010A short jump skips the intervening body to +011E.
    require(helper[6] == 0xEB and
            0x010C + unpack_from("<b", helper, 7)[0] == 0x011E and
            helper[-1] == 0xCB, "reached helper skip/RETF differs")

    pre = checked(run_path / "at-u19-02dd.ram", PRE_SHA)
    post = checked(run_path / "at-u19-02e2.ram", POST_SHA)
    require(len(pre) == len(post) == 0xA0000, "RAM image length differs")
    ss_base = 0x1D16 << 4
    frame_at = ss_base + 0x3DB0
    changed = [i for i, (a, b) in enumerate(zip(pre, post)) if a != b]
    require(changed == list(range(frame_at, frame_at + 6)),
            "RAM delta is not exactly the retained six-byte stack frame")
    frame = unpack_from("<HHH", post, frame_at)
    require(frame == (0x3EF8, 0x02E2, 0x38D3),
            "saved BP / far return IP:CS differs")
    ds_at = 0x14A0 << 4
    for ram in (pre, post):
        require(tuple(unpack_from("<H", ram, ds_at + offset)[0]
                      for offset in (0x4A16, 0x4A18, 0x4A1A, 0x4A1E)) ==
                (0xA000, 0xA000, 0x0000, 0x0001),
                "display-page words differ")
    print("U19 +02DD -> RES_0899:0104 +010A skip -> U19 +02E2")
    print("RAM delta: 6 bytes, only SS:3DB0..3DB5 / physical 20F10..20F15")
    print("retained saved BP / return IP:CS: 3EF8 / 02E2:38D3")
    print("DS:4A16..4A1E display words unchanged; EGA planes not captured")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--ovr", required=True, type=Path)
    parser.add_argument("--run", required=True, type=Path)
    args = parser.parse_args()
    audit(args.exe, args.ovr, args.run)


if __name__ == "__main__":
    main()
