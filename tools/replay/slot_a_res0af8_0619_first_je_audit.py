#!/usr/bin/env python3
"""Audit loaded 131C:0619 through its first JE control boundary."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
BODY_SHA = "1dd96e233790b8fdbac81475a618a39b2f5f639e855bc5b6bce2a80b9df15b82"
JE_SHA = "b2a2028749af082a496044f7e1e70ceb4d21b57aedc24eacb85fc7373caa7b04"
TAKEN_SHA = "9585a4fad7065049202d052c78cd9efc9e59b068e8fada1c3d4c7b6a2d925560"
FALLTHROUGH_SHA = "944e63bf90ea3bdaf95ae2c35787a51f132205f9e8240d635413a3aa2c3026a9"
SOURCE = 0xB28F
LOADED = (0x131C << 4) + 0x0619


def decode(path: Path, begin: int, end: int) -> list[tuple[int, str]]:
    result = subprocess.run(
        ["objdump", "-D", "-b", "binary", "-m", "i8086",
         "--no-show-raw-insn", f"--start-address={begin}",
         f"--stop-address={end}", str(path)],
        capture_output=True, text=True, check=True,
    )
    return [(int(m[1], 16) - begin, re.sub(r"\s+", " ", m[2].strip()))
            for line in result.stdout.splitlines()
            if (m := re.match(r"\s*([0-9a-f]+):\s+(.+)", line))]


def main() -> None:
    start = START.read_bytes()
    ram = RAM.read_bytes()
    assert len(start) == 65231 and sha256(start).hexdigest() == START_SHA
    assert len(ram) == 655360 and sha256(ram).hexdigest() == RAM_SHA
    original_image = (0x0AF8 << 4) + 0x0619
    assert original_image == 0xB599 and LOADED == 0x137D9
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    body = start[SOURCE:SOURCE + 10]
    assert len(body) == 10 and body == ram[LOADED:LOADED + 10]
    assert sha256(body).hexdigest() == BODY_SHA
    assert [i for i in range(len(start) - 9) if start[i:i + 10] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + 10) == [
        (0, "mov $0x3d00,%ax"),
        (3, "cmpw $0xd7b1,0x2(%di)"),
        (8, "je 0xb2a6"),
    ]
    assert decode(RAM, LOADED, LOADED + 10) == [
        (0, "mov $0x3d00,%ax"),
        (3, "cmpw $0xd7b1,0x2(%di)"),
        (8, "je 0x137f0"),
    ]
    je = start[SOURCE + 8:SOURCE + 10]
    assert je == ram[LOADED + 8:LOADED + 10]
    assert sha256(je).hexdigest() == JE_SHA
    taken_source = 0xB2A6
    taken_loaded = (0x131C << 4) + 0x0630
    taken = start[taken_source:taken_source + 4]
    assert taken == ram[taken_loaded:taken_loaded + 4]
    assert sha256(taken).hexdigest() == TAKEN_SHA
    assert decode(START, taken_source, taken_source + 4) == [
        (0, "cmpb $0x0,0x30(%di)"),
    ]
    fallthrough_source = SOURCE + 10
    fallthrough_loaded = LOADED + 10
    fallthrough = start[fallthrough_source:fallthrough_source + 2]
    assert fallthrough == ram[fallthrough_loaded:fallthrough_loaded + 2]
    assert sha256(fallthrough).hexdigest() == FALLTHROUGH_SHA
    assert decode(START, fallthrough_source, fallthrough_source + 2) == [
        (0, "mov $0x2,%al"),
    ]
    assert fallthrough_loaded - (0x131C << 4) == 0x0623
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"body source [0x{SOURCE:05X},0x{SOURCE + 10:05X}) unique_hits=1 "
          f"bytes=10 instructions=3 branches=1 sha256={BODY_SHA}")
    print(f"body loaded [0x{LOADED:05X},0x{LOADED + 10:05X}) sha256={BODY_SHA}")
    print(f"JE sha256={JE_SHA} taken=131C:0630 target_sha256={TAKEN_SHA} "
          f"fallthrough=131C:0623 prefix_sha256={FALLTHROUGH_SHA}")
    print("DS:[DI+2] and native branch outcome unmeasured; successor effects excluded")


if __name__ == "__main__":
    main()
