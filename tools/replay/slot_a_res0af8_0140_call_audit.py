#!/usr/bin/env python3
"""Audit loaded 131C:0140 through near CALL 131C:05C1, not its callee."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
BODY_SHA = "a41aff4f3ba30e8e833b6c51abd3543579617c9e091633e4ca341addeada47d1"
CALL_SHA = "4dccbdd62f52ff0c0b8d4012eba527eb3ece3258acc7697486231b8031e03498"
TARGET_SHA = "de4d9b12622b144b6dbcf41df6bf0af33507fcae98d15161a9ee6c2bdcb0ee1f"
SOURCE = 0xADB6
LOADED = (0x131C << 4) + 0x0140


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
    original_image = (0x0AF8 << 4) + 0x0140
    assert original_image == 0xB0C0 and LOADED == 0x13300
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    body = start[SOURCE:SOURCE + 9]
    assert len(body) == 9 and body == ram[LOADED:LOADED + 9]
    assert sha256(body).hexdigest() == BODY_SHA
    assert [i for i in range(len(start) - 8) if start[i:i + 9] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + 9) == [
        (0, "mov $0x8508,%ax"), (3, "push %ds"),
        (4, "push %ax"), (5, "push %cs"), (6, "call 0xb237"),
    ]
    assert decode(RAM, LOADED, LOADED + 9) == [
        (0, "mov $0x8508,%ax"), (3, "push %ds"),
        (4, "push %ax"), (5, "push %cs"), (6, "call 0x13781"),
    ]
    call = start[SOURCE + 6:SOURCE + 9]
    assert call == ram[LOADED + 6:LOADED + 9]
    assert sha256(call).hexdigest() == CALL_SHA
    original_target = (0x0AF8 << 4) + 0x05C1
    loaded_target = (0x131C << 4) + 0x05C1
    assert original_target == 0xB541 and loaded_target == 0x13781
    assert original_target - 0x30A == 0xB237
    assert start[0xB237:0xB23F] == ram[loaded_target:loaded_target + 8]
    assert sha256(start[0xB237:0xB23F]).hexdigest() == TARGET_SHA
    assert LOADED + 9 - (0x131C << 4) == 0x0149

    print(f"START.EXE bytes=65231 sha256={START_SHA}")
    print(f"loaded RAM bytes=655360 sha256={RAM_SHA}")
    print(f"body source [0x{SOURCE:05X},0x{SOURCE + 9:05X}) unique_hits=1 "
          f"bytes=9 instructions=5 calls=1 sha256={BODY_SHA}")
    print(f"body loaded [0x{LOADED:05X},0x{LOADED + 9:05X}) sha256={BODY_SHA}")
    print(f"near call original=0AF8:05C1 loaded=131C:05C1 "
          f"call_sha256={CALL_SHA} target_prefix_sha256={TARGET_SHA}")
    print("callee-entry stack top: return IP 0149, pushed CS, AX=8508, pushed DS")
    print("source-only; no callee execution, return, or role claim")


if __name__ == "__main__":
    main()
