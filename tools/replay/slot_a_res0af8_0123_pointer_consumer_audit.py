#!/usr/bin/env python3
"""Audit loaded 131C:0123 far-pointer consumer and both successor PCs."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
BODY_SHA = "90933d306e560eec30ef91dcdb77d5895c6f68a815df596d43335a70797651c5"
INPUT_SHA = "e546db32a2cc6329368ab7a74f1dfe0641fd39c0bfa0a5cd1c9ee36ea918a8a9"
TAIL_SHA = "404564f1a18d7f24cd9863f78f18d7db28cb08bd41d815f976c2c9317eca490b"
STORES_SHA = "751d75c900ef962f95f19f4111459c32cf118af5a3d3a4abd7853db1ebf0249d"
FALL_SHA = "72fb562c9d90dfcc008e5b0a391395fd2a244bb6bc231e2621a8ec6860d21fa3"
TAKEN_SHA = "05363f65c03f3827871b25a34a47bd5ddfc501c83e7513555d321949fb2779dc"
SOURCE = 0xAD99
LOADED = (0x131C << 4) + 0x0123
LENGTH = 10


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
    original_image = (0x0AF8 << 4) + 0x0123
    assert original_image == 0xB0A3 and LOADED == 0x132E3
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    body = start[SOURCE:SOURCE + LENGTH]
    assert len(body) == LENGTH and body == ram[LOADED:LOADED + LENGTH]
    assert sha256(body).hexdigest() == BODY_SHA
    assert [i for i in range(len(start) - LENGTH + 1)
            if start[i:i + LENGTH] == body] == [SOURCE]
    assert sha256(body[:4]).hexdigest() == INPUT_SHA
    assert sha256(body[4:]).hexdigest() == TAIL_SHA
    assert sha256(start[SOURCE - 8:SOURCE]).hexdigest() == STORES_SHA
    assert decode(START, SOURCE, SOURCE + LENGTH) == [
        (0, "les 0x4418,%bx"), (4, "mov %es,%ax"),
        (6, "or %bx,%ax"), (8, "je 0xadb6"),
    ]
    assert decode(RAM, LOADED, LOADED + LENGTH) == [
        (0, "les 0x4418,%bx"), (4, "mov %es,%ax"),
        (6, "or %bx,%ax"), (8, "je 0x13300"),
    ]
    assert LOADED + LENGTH == (0x131C << 4) + 0x012D
    fall = start[SOURCE + LENGTH:SOURCE + LENGTH + 2]
    taken = start[0xADB6:0xADB9]
    assert fall == ram[0x132ED:0x132EF]
    assert taken == ram[0x13300:0x13303]
    assert sha256(fall).hexdigest() == FALL_SHA
    assert sha256(taken).hexdigest() == TAKEN_SHA
    print(f"START.EXE bytes=65231 sha256={START_SHA}")
    print(f"loaded RAM bytes=655360 sha256={RAM_SHA}")
    print(f"consumer source [0x{SOURCE:05X},0x{SOURCE + LENGTH:05X}) "
          f"unique_hits=1 bytes=10 instructions=4 branches=1 sha256={BODY_SHA}")
    print(f"consumer loaded [0x{LOADED:05X},0x{LOADED + LENGTH:05X}) sha256={BODY_SHA}")
    print(f"LES input bytes=4 sha256={INPUT_SHA}; decision tail bytes=6 sha256={TAIL_SHA}")
    print("supplied far pointer DS:4418; null -> 131C:0140; nonnull -> 131C:012D")
    print(f"fall source sha256={FALL_SHA}; taken source sha256={TAKEN_SHA}")
    print("source-only; pointer, branch outcome, successor effects, and C parity unmeasured")


if __name__ == "__main__":
    main()
