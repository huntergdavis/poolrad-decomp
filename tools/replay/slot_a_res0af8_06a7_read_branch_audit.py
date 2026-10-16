#!/usr/bin/env python3
"""Pin loaded 131C:06A7 through its DOS-read carry join."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "99ae350d67e58e7c14024fa77cbde4d5e5ce1289d2b34bc5103bc7caa5dc0b02"
JAE_SHA = "0105a3904ec8a03179d0ac37e167c66f513b69a99429cd8311826a5aba05a220"
SOURCE = 0xB31D
LOADED = 0x13867
SIZE = 28


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
    assert (0x0AF8 << 4) + 0x06A7 == 0xB627
    assert (0x131C << 4) + 0x06A7 == LOADED
    assert 0xB627 - SOURCE == 0x30A
    assert LOADED - 0xB627 == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "mov %dx,%cx"),
        (2, "mov %ax,%dx"),
        (4, "mov (%di),%bx"),
        (6, "mov $0x4200,%ax"),
        (9, "int $0x21"),
        (11, "lea 0x80(%di),%dx"),
        (15, "mov $0x80,%cx"),
        (18, "mov (%di),%bx"),
        (20, "mov $0x3f,%ah"),
        (22, "int $0x21"),
        (24, "jae 0xb339"),
        (26, "xor %ax,%ax"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "mov %dx,%cx"),
        (2, "mov %ax,%dx"),
        (4, "mov (%di),%bx"),
        (6, "mov $0x4200,%ax"),
        (9, "int $0x21"),
        (11, "lea 0x80(%di),%dx"),
        (15, "mov $0x80,%cx"),
        (18, "mov (%di),%bx"),
        (20, "mov $0x3f,%ah"),
        (22, "int $0x21"),
        (24, "jae 0x13883"),
        (26, "xor %ax,%ax"),
    ]
    assert sha256(start[SOURCE + 24:SOURCE + 26]).hexdigest() == JAE_SHA
    assert LOADED + SIZE == (0x131C << 4) + 0x06C3
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+06A7 source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes=28 instructions=12 branches=1 interrupts=2 sha256={SPAN_SHA}")
    print(f"JAE sha256={JAE_SHA} taken=131C:06C3 fallthrough=131C:06C1 "
          "join=131C:06C3")
    print("DS:DI, interrupt results, native reach and path unmeasured")


if __name__ == "__main__":
    main()
