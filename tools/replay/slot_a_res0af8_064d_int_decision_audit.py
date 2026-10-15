#!/usr/bin/env python3
"""Pin the source-only 131C:064D interrupt and DL decision."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "af608b02be4e02d7d07efe62a86600758e69bba2df4787400d3a7339f6864e82"
JNE_SHA = "33ce820402bdf76bc8ab0bdb085802782e50d60a6f2d2d3956663b516d222924"
SOURCE = 0xB2C3
LOADED = 0x1380D
SIZE = 19


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
    assert (0x0AF8 << 4) + 0x064D == 0xB5CD
    assert (0x131C << 4) + 0x064D == LOADED
    assert 0xB5CD - SOURCE == 0x30A
    assert LOADED - 0xB5CD == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "mov (%di),%bx"),
        (2, "mov $0x4400,%ax"),
        (5, "int $0x21"),
        (7, "mov $0x73f,%ax"),
        (10, "mov %ax,%cx"),
        (12, "mov %cs,%bx"),
        (14, "test $0x80,%dl"),
        (17, "jne 0xb2e7"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "mov (%di),%bx"),
        (2, "mov $0x4400,%ax"),
        (5, "int $0x21"),
        (7, "mov $0x73f,%ax"),
        (10, "mov %ax,%cx"),
        (12, "mov %cs,%bx"),
        (14, "test $0x80,%dl"),
        (17, "jne 0x13831"),
    ]
    assert sha256(start[SOURCE + 17:SOURCE + 19]).hexdigest() == JNE_SHA
    assert LOADED + SIZE == (0x131C << 4) + 0x0660
    assert 0x13831 - (0x131C << 4) == 0x0671
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+064D source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes=19 instructions=8 branches=1 interrupts=1 sha256={SPAN_SHA}")
    print(f"JNE sha256={JNE_SHA} taken=131C:0671 fallthrough=131C:0660")
    print("DS:DI, interrupt effect, DL value, native reach and path unmeasured")


if __name__ == "__main__":
    main()
