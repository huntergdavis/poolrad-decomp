#!/usr/bin/env python3
"""Pin loaded 131C:0660 predecessor arm through the +0676 join."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "7b5b1c35a0e1b0bfb687fd426aec8f2bd67ae0cbfb795ffcbf884db880d1d749"
CALL_SHA = "f4cee570c6b2dd23d00ed873768e45ed3b24f03ceeee06fb133ef5559ba8c073"
SOURCE = 0xB2D6
LOADED = 0x13820
SIZE = 22


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
    assert (0x0AF8 << 4) + 0x0660 == 0xB5E0
    assert (0x131C << 4) + 0x0660 == LOADED
    assert 0xB5E0 - SOURCE == 0x30A
    assert LOADED - 0xB5E0 == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "cmpw $0xd7b3,0x2(%di)"),
        (5, "jne 0xb2e0"),
        (7, "call 0xb306"),
        (10, "mov $0x71a,%ax"),
        (13, "xor %cx,%cx"),
        (15, "mov %cx,%bx"),
        (17, "movw $0xd7b2,0x2(%di)"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "cmpw $0xd7b3,0x2(%di)"),
        (5, "jne 0x1382a"),
        (7, "call 0x13850"),
        (10, "mov $0x71a,%ax"),
        (13, "xor %cx,%cx"),
        (15, "mov %cx,%bx"),
        (17, "movw $0xd7b2,0x2(%di)"),
    ]
    assert sha256(start[SOURCE + 7:SOURCE + 10]).hexdigest() == CALL_SHA
    assert LOADED + SIZE == (0x131C << 4) + 0x0676
    assert 0x13850 - (0x131C << 4) == 0x0690
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+0660 source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes=22 instructions=7 branches=1 calls=1 stores=1 sha256={SPAN_SHA}")
    print(f"call_sha256={CALL_SHA} call=131C:0690 JNE=131C:066A join=131C:0676")
    print("DS:DI, callee effects, native reach and path unmeasured")


if __name__ == "__main__":
    main()
