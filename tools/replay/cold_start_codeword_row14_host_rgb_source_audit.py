#!/usr/bin/env python3
"""Pin source-only U19 row-14 guest-to-host display boundaries.

The ignored DOSBox-X source is a candidate pipeline, not a same-run trace.
This audit launches neither the game nor the emulator.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess

from cold_start_codeword_row14_boundary_audit import digest, far_target, require
from cold_start_codeword_row14_page_selector_audit import audit as audit_page
from cold_start_codeword_effect_source_audit import U19_BASE
from slot_a_printer_page_audit import unpack


SOURCE_COMMIT = "5a9c7c53481c2e933948039c33256776653f4faf"
HEAVY_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
DRAWER_SHA = "3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4"
HOST_FILES = {
    "hardware/vga_memory.cpp": (
        "f506017948503fa22ce40715205293c987473a938247cbd09ba409ffbb652c14",
        ((331, 385, "bdeed38164872c251e714d1a3b778a5f6c3a836e932f78e72a0209fb07e15efc"),),
    ),
    "hardware/vga_draw.cpp": (
        "f9b798311cffb633754145cfc95b559f180a01b7e6f9f111ef47b7f56e79518d",
        ((1092, 1190, "9215ac88953466efce8bc1492d13f6febde354f284491e0b264d6574d245e23e"),
         (1192, 1245, "f6203fe131910eedf3a293e34cafdb4b67d40720594375931ad92baa9ed9a383"),
         (1246, 1259, "b1b9667c74dbdd420cb73393e95af2597b135fd5dd7f4e21427d0283bf4c3e92"),
         (1266, 1333, "b2e8f6552d5640af6bc44ee24c6cae13a42ae7c78b32065a47d50310e93003e4"),
         (3956, 4024, "e46ec21e3f8c0b44ed02f9affbf52c751c895ea5f52080229abc87c03cf3ba57"),
         (7740, 7762, "72093599dcdffeed48fa0c14a95eb20395533200ba187274d50a585254735652")),
    ),
    "gui/render.cpp": (
        "2c0d47c3c7d73fe092009a7d883316a6e40cf5620723f952c8278e7d93bac1d6",
        ((320, 362, "8ab422d816f710e46b60a2e5157c77ef349456faf36ee00ebaae73a4a3a95fe2"),
         (400, 440, "d2db7e45f8636f620418fb63293065f3bc083bf3cc3a3b1f02fd7187036dc799"),
         (472, 525, "c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3")),
    ),
}


def audit_host_source(source: Path, heavy: Path) -> None:
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    require(commit == SOURCE_COMMIT, "private DOSBox-X source commit differs")
    require(digest(heavy.read_bytes()) == HEAVY_SHA,
            "pinned heavy DOSBox-X executable differs")
    for name, (full_sha, windows) in HOST_FILES.items():
        data = (source / "src" / name).read_bytes()
        require(digest(data) == full_sha, f"DOSBox-X {name} source differs")
        lines = data.splitlines(keepends=True)
        for first, last, expected in windows:
            part = b"".join(lines[first - 1:last])
            require(sha256(part).hexdigest() == expected,
                    f"DOSBox-X {name}:{first}-{last} window differs")


def audit(exe: bytes, ovr: bytes, run: Path,
          source: Path, heavy: Path) -> None:
    audit_page(exe, ovr, run)
    image, *_ = unpack(exe)
    require(digest(image[0x7417:0x7501]) == DRAWER_SHA,
            "resident glyph drawer window differs")
    require(far_target(ovr, U19_BASE + 0x02D8) == (0x0626, 0x0709) and
            far_target(ovr, U19_BASE + 0x02DD) == (0x0104, 0x0899) and
            far_target(ovr, U19_BASE + 0x02FD) == (0x09E3, 0x0709),
            "U19 printer and return-interval far calls differ")
    audit_host_source(source, heavy)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--host-source", type=Path, required=True)
    parser.add_argument("--heavy", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe.read_bytes(), args.ovr.read_bytes(), args.run,
          args.host_source, args.heavy)
    print("U19 guest windows and retained 477/477/477/0 row mismatch checked")
    print("host candidate: planar write -> EGA scanline -> render cache -> GFX update")
    print("source only; no reached handler, page flip, plane change, or RGB timing")


if __name__ == "__main__":
    main()
