#!/usr/bin/env python3
"""Audit loaded 1264:0143 conditional break/drain source extent."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
BODY_SHA = "13a55513f0a200a1881c7a6390aa09771c86074b92c70ca5511ff9794e79b9ce"
PREFIX_SHA = "33829488593660a57d88cc10534e3e439fd5ce6c75742aedfec89696001df063"
CALL_SHA = "5a71529d8f099022e64f13de73e379c00ccebccc5939da4f1ced9e93fec85b13"
SOURCE = 0xA23C
LOADED = (0x1264 << 4) + 0x0143
LENGTH = 40


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
    original_image = (0x0A40 << 4) + 0x0143
    assert original_image == 0xA543 and LOADED == 0x12783
    assert original_image - SOURCE == 0x307
    assert LOADED - original_image == 0x8240
    body = start[SOURCE:SOURCE + LENGTH]
    assert len(body) == LENGTH and body == ram[LOADED:LOADED + LENGTH]
    assert sha256(body).hexdigest() == BODY_SHA
    assert [i for i in range(len(start) - LENGTH + 1)
            if start[i:i + LENGTH] == body] == [SOURCE]
    assert sha256(body[:8]).hexdigest() == PREFIX_SHA
    source_ins = decode(START, SOURCE, SOURCE + LENGTH)
    loaded_ins = decode(RAM, LOADED, LOADED + LENGTH)
    assert len(source_ins) == len(loaded_ins) == 16
    assert [offset for offset, _ in source_ins] == [offset for offset, _ in loaded_ins]
    assert source_ins == [
        (0, "cmpb $0x0,-0x7afe"), (5, "jne 0xa244"),
        (7, "ret"), (8, "movb $0x0,-0x7afe"),
        (13, "mov $0x1,%ah"), (15, "int $0x16"),
        (17, "je 0xa255"), (19, "mov $0x0,%ah"),
        (21, "int $0x16"), (23, "jmp 0xa249"),
        (25, "mov $0x5e,%al"), (27, "call 0xa577"),
        (30, "mov $0x43,%al"), (32, "call 0xa577"),
        (35, "call 0xa570"), (38, "int $0x23"),
    ]
    assert loaded_ins == [
        (0, "cmpb $0x0,-0x7afe"), (5, "jne 0x1278b"),
        (7, "ret"), (8, "movb $0x0,-0x7afe"),
        (13, "mov $0x1,%ah"), (15, "int $0x16"),
        (17, "je 0x1279c"), (19, "mov $0x0,%ah"),
        (21, "int $0x16"), (23, "jmp 0x12790"),
        (25, "mov $0x5e,%al"), (27, "call 0x12abe"),
        (30, "mov $0x43,%al"), (32, "call 0x12abe"),
        (35, "call 0x12ab7"), (38, "int $0x23"),
    ]
    caller = start[0xA426:0xA429]
    assert caller == ram[0x1296D:0x12970]
    assert sha256(caller).hexdigest() == CALL_SHA
    assert decode(START, 0xA426, 0xA429) == [(0, "call 0xa23c")]
    assert decode(RAM, 0x1296D, 0x12970) == [(0, "call 0x12783")]
    assert LOADED + LENGTH == (0x1264 << 4) + 0x016B
    print(f"START.EXE bytes=65231 sha256={START_SHA}")
    print(f"loaded RAM bytes=655360 sha256={RAM_SHA}")
    print(f"body source [0x{SOURCE:05X},0x{SOURCE + LENGTH:05X}) "
          f"unique_hits=1 bytes=40 instructions=16 branches=3 "
          f"near_calls=3 interrupts=3 sha256={BODY_SHA}")
    print(f"body loaded [0x{LOADED:05X},0x{LOADED + LENGTH:05X}) sha256={BODY_SHA}")
    print(f"first decision prefix bytes=8 sha256={PREFIX_SHA}")
    print(f"030F near CALL source=0x0A426 loaded=0x1296D sha256={CALL_SHA}")
    print("typed boundaries: DS:8502; INT16 AH1/AH0; helper 1264:047E/0477; INT23")
    print("source-only; no interrupt, queue, visible text, caller return, or C parity claim")


if __name__ == "__main__":
    main()
