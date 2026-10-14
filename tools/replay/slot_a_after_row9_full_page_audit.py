#!/usr/bin/env python3
"""Hash-locked full-EGA-page audit of three slot-A calls after row nine.

Original media, source text, font, RAM, and EGA planes remain in ignored
resources/. Public output is limited to addresses, hashes, and counts.
"""

import argparse
from pathlib import Path
import struct
import subprocess

from slot_a_printer_page_audit import (
    FONT_SHA, PRINTER_IMAGE_SHA, PRINTER_RUNTIME_SHA, START_SHA,
    check_hash, unpack, word,
)

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
BEFORE_SHA = "c7ad8cf330e3c12800d6c72e68a274664efe6703031ac51ffe399ae5190096ce"
AFTER_SHA = "3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738"
ROW9_WINDOW_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"

BEFORE_PLANES = (
    "9aebca15b3451cc19ab87f687b708b36302135ed82b9d17285bce911009f9480",
    "f1cd6cf944848d4df7746d84f88559b759257cc7f88acb49c0c50b6f9443eef6",
    "10839de1de175a33b4797fbcb495540c81820120ff6eed05b9266a63b4d719c8",
    "417dffcddaa1c57adc85a76d21e28916be83fd9ca9a04ecc8e65cd9f5bbf6749",
)
AFTER_PLANES = (
    BEFORE_PLANES[0],
    "c19e3c79cba5f4524cb533fcd42033b953e2d8e242493ee1229b8e219fd1de9d",
    BEFORE_PLANES[2],
    "7828c5ee3141a02eebcdf919ff95081dc6d0e1034ab24c58d8046ff40595d2dd",
)
RAM_SHA = {
    "row9-anchor": "e21833acfa0d7a691b3ee6059b55dda6460dcb578afa97cc2b15b27224057a68",
    "call1-entry": "cd5c71d43ff073f799de1f4b12727cc7e8b6f1fa4a9ef1b34ee221321113ecfd",
    "call1-return": "6d87c15e06e3eb9a5afd0e328bd15f37047c5ceb2864d0a5e06524ebca88d19b",
    "call2-entry": "3459ed5c9a0a6edb2e9e11229c03195f3283afe6d0f9a696324e98a7169c172a",
    "call2-return": "9a95601aa0355dd804c92882e0796ac22f3919c857f6cb3f4028c9998f7f17f8",
    "call3-entry": "3d65fc60b278d77a81f0b52ef401a2fa31008d2c610f6ef4654b8c5a2bb020aa",
    "call3-return": "a64e1229cf6e6ad0f7cbda275a39e81ed574f853d8216e82d5588cd9aafe04c7",
    "first-view": "66772e9455ea24fcdc6040b912f20ae65defd2eaf0c513e81f911c446f8aeb7d",
}

# Site is the loaded RES_00BA offset; original expanded image offset is +0xBA0.
# Context spans are [site-16, site+16) in each corresponding address space.
CALLS = (
    (1, 0x2B55, 0x3C86,
     (0x2B5A, 0x08DE, 0x3E95, 0x1D16, 0xA00A, 0xA00F, 0xA011),
     12, "28208170efa836a86605cb5e33e47e95c8717d0263668fd6428fff6c1a07c856",
     "b65227edc0424b619b4d3dc4da629eabd7bf5464662226210627149bb952bcd1",
     "bb20ebcb0856a92678d7f14c88e7cfc162cb497048fd525dafb4c4fc81de0d8b",
     "cells=12 stores=384 changed=126"),
    (2, 0x079C, 0x3EB0,
     (0x07A1, 0x08DE, 0x3EC0, 0x1D16, 0xA00F, 0xA002, 0xA011),
     4, "4c1496b703656034f30aee34e197eb1f4abeff239d28dc9263af7a3df199090b",
     "605ed6ad6c8916c8f483d235a36f2c402714c969f84e4886f625ebec98a8ebe6",
     "27cc354dd8c656fd9d7836c54a1b4f03c855db30b253689e94ac6df1602611de",
     "cells=4 stores=128 changed=0"),
    (3, 0x07BA, 0x3EB0,
     (0x07BF, 0x08DE, 0x3EBE, 0x1D16, 0xA00F, 0xA002, 0xA021),
     6, "7039295c05fc9db0246086bb5dc3046e4ac8edffb2c6376a760c88f229dc7ef0",
     "6b8659d17825890f46a38ee4490df31c6ae55b31be5ddd562c31b16b3836c0a2",
     "737bdce626e86def418f7ba427d7d85373051a7023453edbff30e03e874d8723",
     "cells=6 stores=192 changed=0"),
)


def page(run: Path, phase: str) -> bytes:
    before = phase in ("row9-anchor", "call1-entry")
    hashes = BEFORE_PLANES if before else AFTER_PLANES
    planes = []
    for lane, digest in enumerate(hashes):
        data = (run / f"full-{phase}-plane{lane}.bin").read_bytes()
        if len(data) != 8000:
            raise ValueError(f"{phase} plane {lane} size changed")
        check_hash(data, digest, f"{phase} plane {lane}")
        planes.append(data)
    result = b"".join(planes)
    check_hash(result, BEFORE_SHA if before else AFTER_SHA, f"{phase} full page")
    return result


def audit(exe_path: Path, emulator_path: Path, run: Path, probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash(emulator_path.read_bytes(), EMULATOR_SHA, "heavy DOSBox-X")
    check_hash((run / "POOLRAD/GAME.OVR").read_bytes(), GAME_SHA, "GAME.OVR")
    check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(), SAVE_SHA, "slot-A save")
    image, *_ = unpack(exe)
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA,
               "original RES_0709:0626 printer body")
    ram = {}
    pages = {}
    for phase, digest in RAM_SHA.items():
        data = (run / f"ram-{phase}-private.bin").read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{phase} RAM size changed")
        check_hash(data, digest, f"{phase} RAM")
        ram[phase] = data
        pages[phase] = page(run, phase)
    check_hash(b"".join(pages["row9-anchor"][lane * 8000 + 0x3C0:
                       lane * 8000 + 0x1180] for lane in range(4)),
               ROW9_WINDOW_SHA, "row-nine preceding page window")
    if struct.unpack_from("<8H", ram["row9-anchor"], 0x1D160 + 0x3EA4) != (
            0x3EC2, 0x08BA, 0x08DE, 0, 9, 38, 8, 0x4529):
        raise ValueError("row-nine anchor frame changed")
    for a, b in (("row9-anchor", "call1-entry"),
                 ("call1-return", "call2-entry"),
                 ("call2-return", "call3-entry"),
                 ("call3-return", "first-view")):
        if pages[a] != pages[b]:
            raise ValueError(f"page changed between {a} and {b}")
    changed = {i for i, (a, b) in enumerate(zip(pages["call1-entry"],
                                                 pages["call1-return"])) if a != b}
    possible = {lane * 8000 + scanline * 40 + col
                for lane in (1, 3) for scanline in range(120, 128)
                for col in range(17, 29)}
    if len(changed) != 126 or not changed <= possible:
        raise ValueError("row-15 full-page delta changed")
    if pages["call2-entry"] != pages["call2-return"] or \
            pages["call3-entry"] != pages["call3-return"]:
        raise ValueError("row-two calls changed full-page values")

    working = pages["row9-anchor"]
    for number, site, sp, frame, length, source_sha, orig_sha, live_sha, report in CALLS:
        phase = f"call{number}"
        entry = ram[f"{phase}-entry"]
        original_at = site + 0xBA0
        check_hash(image[original_at - 16:original_at + 16], orig_sha,
                   f"original {site:04X} caller context")
        loaded_at = 0x08DE0 + site
        check_hash(entry[loaded_at - 16:loaded_at + 16], live_sha,
                   f"loaded {site:04X} caller context")
        original_call = image[original_at:original_at + 5]
        loaded_call = entry[loaded_at:loaded_at + 5]
        if (original_call[0] != 0x9A or
            struct.unpack_from("<HH", original_call, 1) != (0x0626, 0x0709) or
            loaded_call[0] != 0x9A or
            struct.unpack_from("<HH", loaded_call, 1) != (0x0626, 0x0F2D)):
            raise ValueError(f"{site:04X} far-call target changed")
        check_hash(entry[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7],
                   PRINTER_RUNTIME_SHA, f"loaded printer body at {phase}")
        if struct.unpack_from("<7H", entry, 0x1D160 + sp) != frame:
            raise ValueError(f"{phase} printer frame changed")
        if struct.unpack_from("<2H", entry, 0x1D160 + 0x3EC8) != (
                0x2894, 0x08DE):
            raise ValueError(f"{phase} upper caller return changed")
        _, _, src_off, src_seg, color, row, col = frame
        source_at = src_seg * 16 + src_off
        if entry[source_at] != length:
            raise ValueError(f"{phase} Pascal source length changed")
        check_hash(entry[source_at:source_at + length + 1], source_sha,
                   f"{phase} private source")
        ds = 0x14A00
        font_at = word(entry, ds + 0x5E20) + 16 * word(entry, ds + 0x5E22)
        font = entry[font_at:font_at + 1416]
        check_hash(font, FONT_SHA, f"{phase} live font")
        if (word(entry, ds + 0x4A16), word(entry, ds + 0x4A18),
                entry[ds + 0x4A1E]) != (0xA000, 0xA000, 1):
            raise ValueError(f"{phase} EGA page mode changed")
        if working != pages[f"{phase}-entry"]:
            raise ValueError(f"{phase} C input differs from native entry")
        private_input = run / f"{phase}-audit-input-private.bin"
        private_output = run / f"{phase}-audit-output-private.bin"
        private_input.write_bytes(font + bytes((length, col & 255, row & 255,
                                                color & 255)) +
                                  entry[source_at + 1:source_at + length + 1] +
                                  bytes(255 - length) + working)
        result = subprocess.run([str(probe.resolve()), str(private_input),
                                 str(private_output)], check=True,
                                capture_output=True, text=True)
        if result.stdout.strip() != report:
            raise ValueError(f"{phase} C report changed: {result.stdout!r}")
        working = private_output.read_bytes()
        if working != pages[f"{phase}-return"]:
            raise ValueError(f"{phase} C output differs from native return")
    if working != pages["first-view"]:
        raise ValueError("sequential C page differs from same-run first view")
    print("post-row-nine calls: 08DE:2B5A -> 07A1 -> 07BF")
    print("row/col/length/color: 15/17/12/10, 2/17/4/15, 2/33/6/15")
    print("22 cells; 704 modeled plane stores; 126 net changed full-page bytes")
    print("initial 32000-byte page SHA-256", BEFORE_SHA)
    print("third-return and first-view page SHA-256", AFTER_SHA)
    print("C sequence: 3/3 native return pages each 32000/32000 bytes")
    print("C final: 32000/32000 same-run first-view page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.emulator, args.run, args.probe)
