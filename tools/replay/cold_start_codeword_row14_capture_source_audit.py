#!/usr/bin/env python3
"""Pin DEBUGBOX command and memory-watch source for a future U19 capture.

Reads ignored debugger files only. It neither starts DOSBox-X nor collects
game RAM, EGA planes, or pixels.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess


SOURCE_COMMIT = "5a9c7c53481c2e933948039c33256776653f4faf"
DEBUG_SHA = "2762251108e54aeec3474b9f495222ce09bec9f246dd0af53a4d10fa7bb2a38d"
HEAVY_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
WINDOWS = (
    (682, 701, "669b8a12166f92d6be2164c1dd980a041a3bae2741a3b6fac651220c77abe4b8"),
    (729, 793, "c9cba40ea45854a6d3323e4c63c0261042d1f570fe2fa6446cf99133de4e9484"),
    (1994, 2000, "234edc0cdd3ce35aa7512598bef4dbd5d6f2080d68bbdbe4ae396c9874504351"),
    (2505, 2522, "acd91f4150f20159f134b787ab8751e1e43d25b2ffc833c2ce3fbfebc48e285e"),
    (3833, 3839, "8f9b1f26da533494a7514509bb7b206fe1467c06860957e179596de8cc6391ed"),
    (5794, 5811, "636ec61547ed2f23811922e8c8bc50005d4eebd09df050d4a08088b1deb36f52"),
)


def digest(data: bytes) -> str:
    return sha256(data).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def audit(source: Path, heavy: Path) -> None:
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    require(commit == SOURCE_COMMIT, "private debugger source revision differs")
    data = (source / "src" / "debug" / "debug.cpp").read_bytes()
    require(digest(data) == DEBUG_SHA, "private DEBUGBOX source differs")
    require(digest(heavy.read_bytes()) == HEAVY_SHA,
            "private heavy DEBUGBOX executable differs")
    lines = data.splitlines(keepends=True)
    for first, last, expected in WINDOWS:
        require(digest(b"".join(lines[first - 1:last])) == expected,
                f"DEBUGBOX source window {first}-{last} differs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--heavy", type=Path, required=True)
    args = parser.parse_args()
    audit(args.source, args.heavy)
    print("pinned DEBUGBOX: six source windows, one source revision, one executable")
    print("BPM compares one guest-readable byte; MEMDUMPBIN overwrites and zero-fills read failures")
    print("source only; no selected native store, four-plane change, page, or RGB claim")


if __name__ == "__main__":
    main()
