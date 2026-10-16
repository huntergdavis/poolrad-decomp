#!/usr/bin/env python3
"""Pin loaded 131C:0690 near-callee prefix through its +06A7 join."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "da932359fa8a1900f9b3bb57f8b2d1425da8c9a1fdb55f0155a728f6845f57b5"
JAE_SHA = "a16acfba5d54a1f2edb21cabe3f51fbc5b61a01f265791c48affc9a9476d1601"
SOURCE = 0xB306
LOADED = 0x13850
SIZE = 23


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
    assert (0x0AF8 << 4) + 0x0690 == 0xB610
    assert (0x131C << 4) + 0x0690 == LOADED
    assert 0xB610 - SOURCE == 0x30A
    assert LOADED - 0xB610 == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "xor %dx,%dx"),
        (2, "xor %cx,%cx"),
        (4, "mov (%di),%bx"),
        (6, "mov $0x4202,%ax"),
        (9, "int $0x21"),
        (11, "sub $0x80,%ax"),
        (14, "sbb $0x0,%dx"),
        (17, "jae 0xb31d"),
        (19, "xor %ax,%ax"),
        (21, "xor %dx,%dx"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "xor %dx,%dx"),
        (2, "xor %cx,%cx"),
        (4, "mov (%di),%bx"),
        (6, "mov $0x4202,%ax"),
        (9, "int $0x21"),
        (11, "sub $0x80,%ax"),
        (14, "sbb $0x0,%dx"),
        (17, "jae 0x13867"),
        (19, "xor %ax,%ax"),
        (21, "xor %dx,%dx"),
    ]
    assert sha256(start[SOURCE + 17:SOURCE + 19]).hexdigest() == JAE_SHA
    assert LOADED + SIZE == (0x131C << 4) + 0x06A7
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+0690 source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes=23 instructions=10 branches=1 interrupts=1 sha256={SPAN_SHA}")
    print(f"JAE sha256={JAE_SHA} taken=131C:06A7 fallthrough=131C:06A3 "
          "join=131C:06A7")
    print("DS:DI, interrupt result, native reach and path unmeasured")


if __name__ == "__main__":
    main()
