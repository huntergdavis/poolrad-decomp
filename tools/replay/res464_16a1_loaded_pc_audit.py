#!/usr/bin/env python3
"""Derive first-page Move loaded CS from a pinned ignored caller snapshot."""

from __future__ import annotations

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[2]
RESOURCES = (ROOT / "resources").resolve()
CALLER_RAM = RESOURCES / "oracle-runs/slot-a-u40c-all-dirty-p5-001/ram-flush-entry.bin"
CALLER_SHA = "25ae4cdbceb249e04ec413b49e6cbfc6f14ff4210378bbdd34e19d37389266d1"
CALLER_ADDRESS = (0x0C88 << 4) + 0x1DA0


def ignored(path: Path) -> Path:
    resolved = path.resolve(strict=True)
    if not resolved.is_relative_to(RESOURCES):
        raise ValueError("private input must be under ignored resources/")
    return resolved


def loaded_cs(image: bytes) -> int:
    if len(image) != 0xA0000 or sha256(image).hexdigest() != CALLER_SHA:
        raise ValueError("loaded caller RAM size/hash differs")
    call = image[CALLER_ADDRESS:CALLER_ADDRESS + 5]
    if len(call) != 5 or call[0] != 0x9A or int.from_bytes(call[1:3], "little") != 0x1685:
        raise ValueError("loaded caller opcode/offset differs")
    segment = int.from_bytes(call[3:5], "little")
    if segment == 0:
        raise ValueError("loaded caller has null segment")
    return segment


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bridge", type=Path, help="strict C bridge executable in ignored resources/")
    parser.add_argument("--record", type=Path, help="private native stop record in ignored resources/")
    args = parser.parse_args()
    if (args.bridge is None) != (args.record is None):
        parser.error("--bridge and --record must be supplied together")
    segment = loaded_cs(ignored(CALLER_RAM).read_bytes())
    print(f"hash-locked loaded far-call target {segment:04X}:1685; expected stop {segment:04X}:16A1")
    if args.bridge is not None:
        subprocess.run([str(ignored(args.bridge)), "--record", str(ignored(args.record)),
                        "--loaded-cs", f"{segment:04X}"], check=True)
    else:
        print("STOP: no native +16A1 hardware/plane record supplied")


if __name__ == "__main__":
    main()
