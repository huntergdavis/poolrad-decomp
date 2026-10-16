#!/usr/bin/env python3
"""Pin source boundaries from host display latch to output dispatch, without native."""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess


SOURCE_COMMIT = "5a9c7c53481c2e933948039c33256776653f4faf"
HEAVY_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
FILES = {
    "src/hardware/vga_draw.cpp": (
        "f9b798311cffb633754145cfc95b559f180a01b7e6f9f111ef47b7f56e79518d",
        ((4089, 4148, "86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce"),
         (6190, 6220, "7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b")),
    ),
    "src/gui/render.cpp": (
        "2c0d47c3c7d73fe092009a7d883316a6e40cf5620723f952c8278e7d93bac1d6",
        ((472, 525, "c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3"),),
    ),
    "src/gui/sdlmain.cpp": (
        "312bf64551af8d939bf1aeefa994d3273030f9caa97017db8be7b379e5689f02",
        ((3187, 3270, "c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e"),),
    ),
}


def digest(data: bytes) -> str:
    return sha256(data).hexdigest()


def audit(source: Path, heavy: Path) -> None:
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    if commit != SOURCE_COMMIT:
        raise ValueError("private DOSBox-X revision differs")
    if digest(heavy.read_bytes()) != HEAVY_SHA:
        raise ValueError("private heavy DOSBox-X executable differs")
    for name, (expected_file, windows) in FILES.items():
        data = (source / name).read_bytes()
        if digest(data) != expected_file:
            raise ValueError(f"private source {name} differs")
        lines = data.splitlines(keepends=True)
        for first, last, expected_window in windows:
            if digest(b"".join(lines[first - 1:last])) != expected_window:
                raise ValueError(f"private source {name}:{first}-{last} differs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--heavy", type=Path, required=True)
    args = parser.parse_args()
    audit(args.source, args.heavy)
    print("source lock: three private files, four exact windows, one executable; PASS")
    print("pre-latch completion and renderer/backend output gates source-pinned")
    print("saved helper reach, latch event, host present and RGB chronology unmeasured")


if __name__ == "__main__":
    main()
