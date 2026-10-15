#!/usr/bin/env python3
"""Audit saved slot-A mode-set palette transition and native one-Up RGB frame.

All source media and captures are read from ignored resources/. Public output
contains only addresses, hashes, counts, and comparison results.
"""

import argparse
from dataclasses import dataclass
import hashlib
from pathlib import Path
import re
import sys

from slot_a_rgb_palette_audit import doubled_rgb, ega_rgb_frame


ROOT = Path(__file__).resolve().parents[2]
PRIVATE = (ROOT / "resources").resolve()
sys.path.insert(0, str(ROOT / "tools" / "overlay"))
from resident_loader_map import unpack  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
SPAN_SHA = "1291e63d47f0d18e3c3708159546250c155647778751c8e8a2123f8bcc44afb3"
TRACE_SHA = "adb097bd6a16c49acacd6776c09af3383c7a1b15379742d15779acb08f700500"
MODE_WORD_SHA = "f566cc6fccc657365c0197accf3a7d6f80f85209ff666ff774f4dcbc524aa842"
FIRST_SCREEN_SHA = "2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134"
POST_SCREEN_SHA = "ecb89bbc83b4cafa8a50500b3a16f9cd286f0b41191d05c75c4e77db9b17e3c7"
POST_RAM_SHA = "2d08287fa4185124c182c0ec74215d0dad2ac8d0304db1e02a888ea24cb6ea47"
C_FIRST_PAGE_SHA = "09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a"
POST_PAGE_SHA = "f963d2bcdb1d0f0cf1a0fdc32e285e5299993dd7f9a7b474cfbbd6978b4cbbe1"
POST_RGB_SHA = "8912d397e356eee080b90be2e836c9bcc4d3dfb4d769f5774a384d7c0d8897b3"
POST_CROP_SHA = "27001077963c89a498e8e872c710411954578095c02024bbfc6e3c17ff3cff56"
PLANE_SHA = (
    "334d8dce215b43db059b13746aeb005551a0eefe0e3f56034641fecf390e8a58",
    "bf420fb4c558119a97bbf6580f15480d0db058760de933d2aa32ae6de8373091",
    "b59db1ee961ca5fbcb9cbf8c6429e64175aee41f43ded143ed579e98443aeaed",
    "54a0c23217d11cc1d0e64de83994872aa8599eb898b257f80341848356acee02",
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def checked(path: Path, expected: str) -> bytes:
    path = path.resolve(strict=True)
    if not path.is_relative_to(PRIVATE):
        raise ValueError("original input must remain under ignored resources/")
    data = path.read_bytes()
    if digest(data) != expected:
        raise ValueError(f"{path.name}: SHA-256 mismatch")
    return data


@dataclass(frozen=True)
class Hardware:
    mode: str
    control: int
    pan: int
    ac: tuple[int, ...]
    dac: tuple[tuple[int, int, int], ...]
    mask: int


def hardware_reports(trace: str) -> tuple[Hardware, ...]:
    """Read complete, ordered AC/DAC/mode reports, rejecting partial rows."""
    blocks = trace.split("LOG: VGA Attribute Controller info:")[1:]
    reports = []
    for block in blocks:
        control = re.search(r"LOG: mode_control=([0-9a-f]{2})h horz-pel-pan=([0-9a-f]{2})h", block, re.I)
        palette = re.search(r"LOG: palette=\{([^}]+)\}", block)
        dac_info = re.search(r"LOG: VGA DAC info: bits=(\d+) pel_mask=([0-9a-f]{2})h", block, re.I)
        mode = re.search(r"LOG: mode=(M_[A-Z]+)\b", block)
        if not (control and palette and dac_info and mode):
            raise ValueError("incomplete native hardware report")
        ac = tuple(int(x, 16) for x in re.findall(r"\b[0-9a-f]{2}\b", palette[1], re.I))
        if len(ac) != 16 or int(dac_info[1]) != 6:
            raise ValueError("unsupported AC or DAC layout")
        rows = re.findall(r"^LOG: ([0-9a-f]{2}): ([^\r\n]+)", block, re.I | re.M)
        if len(rows) != 32:
            raise ValueError("incomplete native DAC table")
        dac = []
        for i, (address, values) in enumerate(rows):
            entries = re.findall(r"\b[0-9a-f]{6}\b", values, re.I)
            if int(address, 16) != i * 8 or len(entries) != 8:
                raise ValueError("invalid native DAC row")
            for entry in entries:
                triple = tuple(int(entry[j:j + 2], 16) for j in (0, 2, 4))
                if any(value > 63 for value in triple):
                    raise ValueError("DAC channel exceeds six bits")
                dac.append(triple)
        reports.append(Hardware(mode[1], int(control[1], 16),
                                int(control[2], 16), ac, tuple(dac),
                                int(dac_info[2], 16)))
    return tuple(reports)


def changed_channels(before: Hardware, after: Hardware) -> tuple[int, int, int]:
    ac = sum(a != b for a, b in zip(before.ac, after.ac))
    entries = sum(a != b for a, b in zip(before.dac, after.dac))
    channels = sum(a != b for left, right in zip(before.dac, after.dac)
                   for a, b in zip(left, right))
    return ac, entries, channels


def pixel_differences(a: bytes, b: bytes) -> int:
    if len(a) != len(b) or len(a) % 3:
        raise ValueError("RGB frame lengths differ")
    return sum(a[i:i + 3] != b[i:i + 3] for i in range(0, len(a), 3))


def require_stop_order(trace: str) -> None:
    pre = re.search(r"LOG: EV of 'CS' is:\s*LOG: c88\s*LOG: EV of 'IP' is:\s*LOG: ee", trace)
    post = re.search(r"LOG: EV of 'CS' is:\s*LOG: c88\s*LOG: EV of 'IP' is:\s*LOG: f3", trace)
    reports = [match.start() for match in
               re.finditer(r"LOG: VGA Attribute Controller info:", trace)]
    if not (pre and post and len(reports) == 4 and
            pre.end() < reports[0] < post.start() < reports[1] < reports[2] < reports[3]):
        raise ValueError("native mode-call stop/report ordering differs")


def audit(args: argparse.Namespace) -> None:
    from PIL import Image

    run = args.run.resolve(strict=True)
    if not run.is_relative_to(PRIVATE):
        raise ValueError("run must remain under ignored resources/")
    exe = checked(args.exe, START_SHA)
    checked(args.save, SAVE_SHA)
    checked(args.emulator, EMULATOR_SHA)
    image, _, _, _ = unpack(exe)
    if digest(image[0x0470F:0x0473F]) != SPAN_SHA:
        raise ValueError("original RES_0464:00CF..00FE span changed")
    if checked(run / "mode-word-pre-private.bin", MODE_WORD_SHA) != bytes((0x0D, 0)):
        raise ValueError("pre-call BIOS AX is not 000D")
    trace = checked(run / "debugger-mode.typescript", TRACE_SHA).decode("latin1")
    require_stop_order(trace)
    pre, post, first, up = hardware_reports(trace)
    if (pre.mode, post.mode, first.mode, up.mode) != ("M_TEXT", "M_EGA", "M_EGA", "M_EGA"):
        raise ValueError("native video mode route differs")
    if (pre.control, pre.pan, post.control, post.pan) != (12, 8, 9, 0):
        raise ValueError("AC mode/pan transition differs")
    if any(report.mask != 255 for report in (pre, post, first, up)):
        raise ValueError("DAC pixel mask differs")
    if changed_channels(pre, post) != (9, 49, 68):
        raise ValueError("mode-set AC/DAC transition differs")
    if not (post == first == up):
        raise ValueError("saved view or Up palette differs from post-mode state")

    checked(run / "paused-first-view-private.png", FIRST_SCREEN_SHA)
    checked(run / "post-up-stable-private.png", POST_SCREEN_SHA)
    checked(run / "post-up-paused-private.png", POST_SCREEN_SHA)
    checked(run / "post-up-ram-private.bin", POST_RAM_SHA)
    planes = tuple(checked(run / f"post-up-plane{i}-private.bin", PLANE_SHA[i])
                   for i in range(4))
    if any(len(plane) != 8000 for plane in planes):
        raise ValueError("incomplete native post-Up plane")
    page = b"".join(planes)
    if digest(page) != POST_PAGE_SHA:
        raise ValueError("post-Up A000 page changed")
    rgb, counts = ega_rgb_frame(page, up.ac, up.dac, up.mask)
    if digest(rgb) != POST_RGB_SHA or sum(counts.values()) != 64000 or len(counts) != 11:
        raise ValueError("post-Up native RGB frame changed")
    with Image.open(run / "post-up-paused-private.png") as screen:
        if screen.size != (1280, 1024):
            raise ValueError("post-Up native screenshot geometry changed")
        crop = screen.convert("RGB").crop((320, 312, 960, 712)).tobytes()
    if digest(crop) != POST_CROP_SHA or crop != doubled_rgb(rgb):
        raise ValueError("post-Up native plane/palette to screenshot RGB mismatch")

    c_first = checked(args.c_first_page, C_FIRST_PAGE_SHA)
    if len(c_first) != 32000:
        raise ValueError("incomplete first-view C page")
    byte_delta = sum(a != b for a, b in zip(c_first, page))
    plane_delta = tuple(sum(a != b for a, b in zip(c_first[i * 8000:(i + 1) * 8000], planes[i]))
                        for i in range(4))
    c_first_rgb, _ = ega_rgb_frame(c_first, up.ac, up.dac, up.mask)
    pixel_delta = pixel_differences(c_first_rgb, rgb)
    if (byte_delta, plane_delta, pixel_delta) != (725, (174, 106, 308, 137), 1979):
        raise ValueError("first-view C page versus post-Up native delta changed")
    print("mode-set boundary: native 0C88:00EE to 0C88:00F3; AX=000D")
    print("hardware transition: AC entries 9/16; DAC entries 49/256; channels 68/768")
    print("post-mode, first-view and post-Up palette snapshots: equal")
    print("post-Up native planes/palette versus screenshot: 64000/64000 RGB pixels equal")
    print("first-view C page versus post-Up native: 725/32000 plane bytes; 1979/64000 RGB pixels differ")
    print("C post-Up full-frame parity: blocked at native key dispatch")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("run", "exe", "save", "emulator", "c_first_page"):
        parser.add_argument("--" + name.replace("_", "-"), required=True, type=Path)
    audit(parser.parse_args())


if __name__ == "__main__":
    main()
