#!/usr/bin/env python3
"""Pin read-only host display-state commands for a future U19 same-run gate."""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess


SOURCE_COMMIT = "5a9c7c53481c2e933948039c33256776653f4faf"
HEAVY_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
FILES = {
    "src/debug/debug.cpp": (
        "2762251108e54aeec3474b9f495222ce09bec9f246dd0af53a4d10fa7bb2a38d",
        ((3215, 3240, "49371ae314c4b5293c2e8d4d26d74ec9e28649c619e6bda5d8f303c45cfa8727"),
         (3307, 3326, "da5ad1cfb9718557b1689eba5b428b6615d0be45584e24b976eb74e99ce449e2"),
         (3405, 3433, "ad1df338666e5c9f25005391b01308b0f3ac5572f7131e9527718bfa5d41d328"),
         (3434, 3468, "8e641ca88669a77bed8b4ce3ed124ba6cbe6c87356bdaff5281c4c0d33fc9efa")),
    ),
    "src/hardware/vga_draw.cpp": (
        "f9b798311cffb633754145cfc95b559f180a01b7e6f9f111ef47b7f56e79518d",
        ((4119, 4148, "bffc6f2bee77aff833ea09a015384a2700ae3f5b266a3673de1f761e773c4000"),
         (6123, 6156, "0e74faacbb721cf65ff7a4534f6d7a32a837aadbe7fa50a971785440ad231fb8"),
         (6814, 6830, "0a2692162240ae0cd1e7d7fca508fcf7f76102a4ac0a51567b5a34675b4a9e4c")),
    ),
}
RETAINED_RGB_FILES = {
    "at-u19-02dd.png": (3044, "b317f05727cb0700e3d660c254583b96bf499afd85bae757e74c9fa4b8dd8c78"),
    "at-u19-02e2.png": (3044, "b317f05727cb0700e3d660c254583b96bf499afd85bae757e74c9fa4b8dd8c78"),
    "at-u19-02fd.png": (3044, "b317f05727cb0700e3d660c254583b96bf499afd85bae757e74c9fa4b8dd8c78"),
    "at-u19-0302.png": (3271, "0a21f179e0e5530b79d2331692fa13eb047358dde4232a4502296907c5e348ce"),
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def digest(data: bytes) -> str:
    return sha256(data).hexdigest()


def audit(source: Path, heavy: Path, run: Path) -> None:
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    require(commit == SOURCE_COMMIT, "private DOSBox-X revision differs")
    require(digest(heavy.read_bytes()) == HEAVY_SHA, "private DOSBox-X executable differs")
    for name, (full_sha, windows) in FILES.items():
        data = (source / name).read_bytes()
        require(digest(data) == full_sha, f"private DOSBox-X {name} differs")
        lines = data.splitlines(keepends=True)
        for first, last, expected in windows:
            require(digest(b"".join(lines[first - 1:last])) == expected,
                    f"private DOSBox-X {name}:{first}-{last} differs")
    for name, (size, expected) in RETAINED_RGB_FILES.items():
        data = (run / name).read_bytes()
        require(len(data) == size and digest(data) == expected,
                f"retained U19 RGB checkpoint {name} differs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--heavy", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    audit(args.source, args.heavy, args.run)
    print("source lock: two files, seven windows, one executable; PASS")
    print("retained RGB files: four size/hash checks; PASS")
    print("VGA CRTC/MODE/DRAW observation source pinned; VRD redraw changes latch/timer")
    print("saved helper reach, real retrace event, selected page and RGB timing unproved")


if __name__ == "__main__":
    main()
