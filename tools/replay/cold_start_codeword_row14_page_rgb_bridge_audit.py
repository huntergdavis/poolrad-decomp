#!/usr/bin/env python3
"""Pin the conditional INT 10h page-to-EGA-line host source bridge.

This reads ignored DOSBox-X source and executable only. It does not launch
the emulator or establish that a saved U19 helper reached the host branch.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess


SOURCE_COMMIT = "5a9c7c53481c2e933948039c33256776653f4faf"
HEAVY_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
FILES = {
    "ints/int10.cpp": (
        "d53de126371cffcaa385e567218880096062d1e7130d7c8e1cb73d98a377a6a9",
        ((143, 170, "090e5fd80c31a0ccd0112e45faff0bfc620a6ffaa7b62d8ccd65a737069f1b19"),),
    ),
    "ints/int10_char.cpp": (
        "6c33d9715cfc6d2b2c3ffbe53809083c33b28319bec857d2df332ede7ad1942b",
        ((1184, 1213, "17d105217bf5a6d88f4416a3d38b8f22ac61c8d0bcf564fb4874040c0ad1906e"),),
    ),
    "hardware/vga_crtc.cpp": (
        "5318ee32824bf89621217b5d4af1b06a4e7e9f5784d950ef08d8283542e73d1a",
        ((247, 259, "0f6f4cd6fb0781a3f7fd86776596dfcd8b809e0c03b22e8b7a918f3250b33274"),),
    ),
    "hardware/vga_draw.cpp": (
        "f9b798311cffb633754145cfc95b559f180a01b7e6f9f111ef47b7f56e79518d",
        ((3956, 4018, "8f26cd690da5ad9853d53d9ed06dc81382efd428c7e4b7242e7ad75b44974438"),
         (4119, 4148, "bffc6f2bee77aff833ea09a015384a2700ae3f5b266a3673de1f761e773c4000"),
         (6123, 6156, "0e74faacbb721cf65ff7a4534f6d7a32a837aadbe7fa50a971785440ad231fb8"),
         (6259, 6290, "888d758087aa016fa34a920fd9913b6393723e920c22478c128ad5f3e3e9b9a9"),
         (6814, 6830, "0a2692162240ae0cd1e7d7fca508fcf7f76102a4ac0a51567b5a34675b4a9e4c")),
    ),
}


def digest(data: bytes) -> str:
    return sha256(data).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def audit(source: Path, heavy: Path) -> None:
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    require(commit == SOURCE_COMMIT, "private DOSBox-X source revision differs")
    require(digest(heavy.read_bytes()) == HEAVY_SHA,
            "private heavy DOSBox-X executable differs")
    for name, (full_sha, windows) in FILES.items():
        data = (source / "src" / name).read_bytes()
        require(digest(data) == full_sha, f"DOSBox-X {name} differs")
        lines = data.splitlines(keepends=True)
        for first, last, expected in windows:
            require(digest(b"".join(lines[first - 1:last])) == expected,
                    f"DOSBox-X {name}:{first}-{last} differs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--heavy", type=Path, required=True)
    args = parser.parse_args()
    audit(args.source, args.heavy)
    print("pinned conditional host page bridge: four files, eight windows")
    print("INT10 page -> CRTC display_start -> EGA retrace real_start -> line render")
    print("saved U19 helper reach, mode branch, plane change, page and RGB timing unproved")


if __name__ == "__main__":
    main()
