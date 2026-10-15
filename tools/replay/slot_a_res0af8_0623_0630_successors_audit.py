#!/usr/bin/env python3
"""Audit two source-only successors of loaded 131C:0621."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
FALLTHROUGH_SHA = "96fd30f7a96321cb7b3dbad93350291f05eb5e14e8a2d88d99e45ee824856584"
FALLTHROUGH_JE_SHA = "0b358362552792220751c58c4cca9c4e54407d2339649a0b96182880f0504c1a"
GAP_SHA = "0b83e1d7c4f0763abe7a196d07183643d6247f3a7b23dc59999abe7d81e4875e"
JOIN_SHA = "fb657a460f00ab1a148c477ec43c70204228d6d97ca8723fcec965d5992bb56e"
JOIN_JE_SHA = "e6332f7e34686e19d5ad393559009f37d77126d1221407bfdee68bec60234a29"
SECOND_FALLTHROUGH_SHA = "3d97f896cfd1b838f6c23379f12ae715d99301fcf7b03af0fd2e40e5b0be530a"
SECOND_TAKEN_SHA = "4481c2d28fa875cebc722f5bc6b172762750d5ee3161465ad0cfcafdaa861675"
FALLTHROUGH_SOURCE = 0xB299
FALLTHROUGH_LOADED = (0x131C << 4) + 0x0623
JOIN_SOURCE = 0xB2A6
JOIN_LOADED = (0x131C << 4) + 0x0630


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


def pinned_span(start: bytes, ram: bytes, source: int, loaded: int,
                size: int, expected_sha: str) -> bytes:
    body = start[source:source + size]
    assert len(body) == size and body == ram[loaded:loaded + size]
    assert sha256(body).hexdigest() == expected_sha
    return body


def main() -> None:
    start = START.read_bytes()
    ram = RAM.read_bytes()
    assert len(start) == 65231 and sha256(start).hexdigest() == START_SHA
    assert len(ram) == 655360 and sha256(ram).hexdigest() == RAM_SHA
    assert (0x0AF8 << 4) + 0x0623 == 0xB5A3
    assert (0x0AF8 << 4) + 0x0630 == 0xB5B0
    assert FALLTHROUGH_LOADED == 0x137E3 and JOIN_LOADED == 0x137F0
    assert 0xB5A3 - FALLTHROUGH_SOURCE == 0x30A
    assert 0xB5B0 - JOIN_SOURCE == 0x30A
    assert FALLTHROUGH_LOADED - 0xB5A3 == 0x8240
    assert JOIN_LOADED - 0xB5B0 == 0x8240

    fallthrough = pinned_span(start, ram, FALLTHROUGH_SOURCE,
                              FALLTHROUGH_LOADED, 11, FALLTHROUGH_SHA)
    assert [i for i in range(len(start) - 10)
            if start[i:i + 11] == fallthrough] == [FALLTHROUGH_SOURCE]
    assert decode(START, FALLTHROUGH_SOURCE, FALLTHROUGH_SOURCE + 11) == [
        (0, "mov $0x2,%al"),
        (2, "incw (%di)"),
        (4, "cmpw $0xd7b3,0x2(%di)"),
        (9, "je 0xb2a6"),
    ]
    assert decode(RAM, FALLTHROUGH_LOADED, FALLTHROUGH_LOADED + 11) == [
        (0, "mov $0x2,%al"),
        (2, "incw (%di)"),
        (4, "cmpw $0xd7b3,0x2(%di)"),
        (9, "je 0x137f0"),
    ]
    pinned_span(start, ram, 0xB2A2, 0x137EC, 2, FALLTHROUGH_JE_SHA)
    pinned_span(start, ram, 0xB2A4, 0x137EE, 2, GAP_SHA)
    assert decode(START, 0xB2A4, 0xB2A6) == [(0, "mov $0x3c,%ah")]

    join = pinned_span(start, ram, JOIN_SOURCE, JOIN_LOADED, 6, JOIN_SHA)
    assert [i for i in range(len(start) - 5)
            if start[i:i + 6] == join] == [JOIN_SOURCE]
    assert decode(START, JOIN_SOURCE, JOIN_SOURCE + 6) == [
        (0, "cmpb $0x0,0x30(%di)"),
        (4, "je 0xb2b5"),
    ]
    assert decode(RAM, JOIN_LOADED, JOIN_LOADED + 6) == [
        (0, "cmpb $0x0,0x30(%di)"),
        (4, "je 0x137ff"),
    ]
    pinned_span(start, ram, 0xB2AA, 0x137F4, 2, JOIN_JE_SHA)
    pinned_span(start, ram, 0xB2AC, 0x137F6, 3, SECOND_FALLTHROUGH_SHA)
    pinned_span(start, ram, 0xB2B5, 0x137FF, 3, SECOND_TAKEN_SHA)
    assert FALLTHROUGH_LOADED + 11 - (0x131C << 4) == 0x062E
    assert JOIN_LOADED + 6 - (0x131C << 4) == 0x0636

    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+0623 source [0x{FALLTHROUGH_SOURCE:05X},0x{FALLTHROUGH_SOURCE + 11:05X}) "
          f"unique_hits=1 bytes=11 instructions=4 branches=1 sha256={FALLTHROUGH_SHA}")
    print(f"+0630 source [0x{JOIN_SOURCE:05X},0x{JOIN_SOURCE + 6:05X}) "
          f"unique_hits=1 bytes=6 instructions=2 branches=1 sha256={JOIN_SHA}")
    print(f"+0623 JE sha256={FALLTHROUGH_JE_SHA} taken=131C:0630 "
          f"fallthrough=131C:062E gap_sha256={GAP_SHA}")
    print(f"+0630 JE sha256={JOIN_JE_SHA} taken=131C:063F "
          f"fallthrough=131C:0636")
    print("DS:DI values, native branch outcomes, and successor effects unmeasured")


if __name__ == "__main__":
    main()
