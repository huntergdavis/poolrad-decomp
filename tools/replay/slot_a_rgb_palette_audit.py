#!/usr/bin/env python3
"""Check a C EGA page against directly observed slot-A palette and RGB frame."""

import argparse
from collections import Counter
import hashlib
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
PRIVATE = (ROOT / "resources").resolve()
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
TRACE_SHA = "578a577f4891dcb29d8e28eb84b12229727bba9a3989b8520baba3c0f83b5326"
SCREEN_SHA = "2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134"
PAGE_SHA = "09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a"
RGB_SHA = "ed2cc92edf4cf7ac11f9700f53fc16355259cdcaa748970d1159f6a5c6dd5d4e"
PLANE_SHA = (
    "68638729c3ac5a483f4fb023cf839dd4fea8432b51ec8e96d96d1fb3a8b35da1",
    "975cb7c40471ebb947734deae142c526bf37e0a756f3cb49c409492d432add35",
    "8c29a55dbd09cc924a3295263172b6336dd2d1c69caf3524f800af4929920404",
    "d1f05b349fd8db17bbf4739a7869448c4e8252f093db9c9592b597dfbb83c351",
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def checked(path: Path, expected: str) -> bytes:
    path = path.resolve(strict=True)
    if not path.is_relative_to(PRIVATE):
        raise ValueError("private input must be under resources/")
    data = path.read_bytes()
    if digest(data) != expected:
        raise ValueError(f"{path.name}: SHA-256 mismatch")
    return data


def palette_state(trace: str) -> tuple[tuple[int, ...], tuple[tuple[int, int, int], ...], int]:
    """Parse the same paused debugger's AC, DAC, and display-mode reports."""
    if not re.search(r"LOG: mode_control=09h horz-pel-pan=00h overscan-color=00h", trace):
        raise ValueError("EGA attribute mode or panning changed")
    if not re.search(r"LOG: color-plane-en=0fh color-select=00h", trace):
        raise ValueError("plane enable or color-select changed")
    if not re.search(r"LOG: mode=M_EGA .*", trace):
        raise ValueError("native display is not planar EGA")
    if not re.search(r"LOG: disp-start=0h real-start=0h .* scanlen=20 ", trace):
        raise ValueError("EGA display start or scanline stride changed")
    if not re.search(r"LOG: byte-skip=0 addr-shift=0 rd-mode=0", trace):
        raise ValueError("EGA address mapping changed")

    palettes = re.findall(r"LOG: palette=\{([^}]+)\}", trace)
    if not palettes:
        raise ValueError("missing native attribute palette")
    ac = tuple(int(x, 16) for x in re.findall(r"\b[0-9a-f]{2}\b", palettes[-1], re.I))
    if ac != (*range(8), *range(0x10, 0x18)):
        raise ValueError("saved slot-A attribute map changed")

    match = re.search(r"LOG: VGA DAC info: bits=(\d+) pel_mask=([0-9a-f]{2})h", trace, re.I)
    if match is None or int(match[1]) != 6:
        raise ValueError("missing or unsupported native DAC depth")
    mask = int(match[2], 16)
    if mask != 0xFF:
        raise ValueError("saved slot-A DAC pixel mask changed")
    try:
        rows = trace.split("LOG: VGA DAC palette (RGB):", 1)[1]
    except IndexError as exc:
        raise ValueError("missing native DAC palette dump") from exc
    dac = []
    for i, (address, values) in enumerate(re.findall(
            r"LOG: ([0-9a-f]{2}): ([^\r\n]*)", rows, re.I)[:32]):
        if int(address, 16) != 8 * i:
            raise ValueError("native DAC row order changed")
        entries = re.findall(r"[0-9a-f]{6}", values, re.I)
        if len(entries) != 8:
            raise ValueError("incomplete native DAC row")
        for entry in entries:
            rgb = tuple(int(entry[j:j + 2], 16) for j in (0, 2, 4))
            if any(channel > 63 for channel in rgb):
                raise ValueError("native DAC channel exceeds six bits")
            dac.append(rgb)
    if len(dac) != 256:
        raise ValueError("incomplete native DAC palette")
    return ac, tuple(dac), mask


def ega_rgb_frame(page: bytes, ac: tuple[int, ...],
                  dac: tuple[tuple[int, int, int], ...], mask: int,
                  width: int = 320, height: int = 200) -> tuple[bytes, Counter[int]]:
    """Decode four MSB-first planes, then AC -> DAC -> eight-bit RGB."""
    if width <= 0 or width % 8 or height <= 0 or len(page) != width * height // 2:
        raise ValueError("invalid four-plane frame layout")
    if len(ac) != 16 or len(dac) != 256 or not 0 <= mask <= 255:
        raise ValueError("incomplete palette state")
    if any(len(rgb) != 3 or any(not 0 <= c <= 63 for c in rgb) for rgb in dac):
        raise ValueError("DAC values must be six-bit RGB triples")
    size = width * height // 8
    colors = [bytes((channel << 2) | (channel >> 4) for channel in dac[index & mask])
              for index in ac]
    output = bytearray(width * height * 3)
    counts: Counter[int] = Counter()
    at = 0
    for byte in range(size):
        for bit in range(7, -1, -1):
            index = sum(((page[plane * size + byte] >> bit) & 1) << plane
                        for plane in range(4))
            output[at:at + 3] = colors[index]
            counts[index] += 1
            at += 3
    return bytes(output), counts


def doubled_rgb(frame: bytes, width: int = 320, height: int = 200) -> bytes:
    if len(frame) != width * height * 3:
        raise ValueError("incomplete RGB frame")
    output = bytearray()
    for row in range(height):
        source = frame[row * width * 3:(row + 1) * width * 3]
        doubled = b"".join(source[x:x + 3] * 2 for x in range(0, len(source), 3))
        output.extend(doubled)
        output.extend(doubled)
    return bytes(output)


def audit(args: argparse.Namespace) -> None:
    from PIL import Image

    run = args.run.resolve(strict=True)
    if not run.is_relative_to(PRIVATE):
        raise ValueError("private run must be under resources/")
    checked(args.save, SAVE_SHA)
    checked(args.emulator, EMULATOR_SHA)
    trace = checked(run / "debugger-palette.typescript", TRACE_SHA).decode("latin1")
    ac, dac, mask = palette_state(trace)
    planes = [checked(run / f"a000-plane{i}-stable.bin", PLANE_SHA[i])
              for i in range(4)]
    if any(len(plane) != 8000 for plane in planes):
        raise ValueError("incomplete same-run native EGA plane")
    native_page = b"".join(planes)
    c_page = checked(args.c_page, PAGE_SHA)
    if native_page != c_page:
        raise ValueError("same-run native page differs from the C page")

    rgb, counts = ega_rgb_frame(c_page, ac, dac, mask)
    if digest(rgb) != RGB_SHA:
        raise ValueError("C RGB frame hash changed")
    screen_path = run / "paused-first-view-private.png"
    checked(screen_path, SCREEN_SHA)
    with Image.open(screen_path) as image:
        if image.size != (1280, 1024):
            raise ValueError("native screenshot dimensions changed")
        crop = image.convert("RGB").crop((320, 312, 960, 712)).tobytes()
    expected_crop = doubled_rgb(rgb)
    if crop != expected_crop:
        differences = sum(crop[i:i + 3] != expected_crop[i:i + 3]
                          for i in range(0, len(crop), 3))
        raise ValueError(f"native/C RGB pixel mismatches: {differences}")
    if sum(counts.values()) != 64000 or len(counts) != 11:
        raise ValueError("saved first-view color coverage changed")
    output = run / "c-rgb-frame-private.bin"
    output.write_bytes(rgb)
    print("native AC/DAC: 16 attribute entries; 256 six-bit DAC entries; mask FF")
    print("same-state C/native A000 page: 32000/32000 bytes equal")
    print("same-state C/native RGB frame: 64000/64000 pixels equal; 11 used indices")
    print("C RGB SHA-256", RGB_SHA)
    print("native 2x RGB crop SHA-256", digest(crop))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("run", "save", "emulator", "c_page"):
        parser.add_argument("--" + name.replace("_", "-"), type=Path, required=True)
    audit(parser.parse_args())


if __name__ == "__main__":
    main()
