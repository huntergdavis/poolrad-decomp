#!/usr/bin/env python3
"""Private byte-parity audit of the reached RES_0709:00B7 rectangle calls.

Original media, RAM, EGA pages, and generated probe inputs stay in ignored
resources/. Public output contains only addresses, hashes, counts, and status.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import re
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "overlay"))
from resident_loader_map import unpack

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
ORIGINAL_BODY_SHA = "f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5"
LOADED_BODY_SHA = "02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f"
RESET_SHA = "5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5"
PAGE_SELECTOR_SHA = "6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d"
SYSTEM_FILL_SHA = "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"
PRINT_ORIGINAL_CALL_SHA = "505ad54ab9e5390450c330a9decab8bab5ee9f07d44dc475f2414fa8afc6f9ea"
PRINT_LOADED_CALL_SHA = "9d5cce1a3faf38548317d311f935802eab5da2a7485708f6afe43e48d1da1b07"
PRINT_CLEAR_CALL_SHA = "00a6fe385950a2f1ebfdcf643d224c90385570cd16696eefa51e9b11ccfa81e5"
RECT_CALL_SHA = (
    "22a02fb9bd68d98657e6e8c09df5ba1fe39c89511380a1c7cf4737f70b8960e6",
    "808e7dd970fcb77fa9495f410ea96a7eb011b8bd207fe400a5868792e958bf8b",
)
RAM_SHA = {
    "preprint": "107f1f0b4dbcecffc8fed3744b4d7f55b26e65186a2628401d586e354a19ab32",
    "rect1-entry": "8f472b6045aca6b7e712da3b74a53241a17a1f6b9115adf0350d0b4b4de7c31d",
    "rect1-return": "acb4bcb609ba237546b0f04b1ac2efeba9b9415fa1f60fc373b00e56524962da",
    "rect2-entry": "2c64228d7ea3d8a09095286549cc1c01399ac4442ee4a54dbbf8f636cce25c29",
    "rect2-return": "94288ca7d078634bff196d10cb972f3f1b8ecddebca8ae7d8f30438d3e333a73",
    "postprint": "91bcc91e10734e2f3466c21520ba7544c7a30f4e800c91ed054f31ffd1b6294f",
}
WINDOW_SHA = "ebef6f9b1c56c45b7a81d985602fc186abde3bafbd7cff0d602462cca778f31d"
PAGE_SHA = (
    "f963d2bcdb1d0f0cf1a0fdc32e285e5299993dd7f9a7b474cfbbd6978b4cbbe1",
    "7af052afad72a3c8938f567d2bd57fe9914947d2a70e519fd7bace6e4d75fc90",
)
ROW5_PRE_SHA = "3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738"
ROW5_POST_SHA = "570ad46e976182f1227b76bdb367fa0303bd789c42bfa095770a74cb5f7aa52f"


def require(condition: bool, label: str) -> None:
    if not condition:
        raise ValueError(label)


def locked(data: bytes, expected: str, label: str) -> None:
    actual = sha256(data).hexdigest()
    require(actual == expected, f"{label} hash mismatch: {actual}")


def window(run: Path, tag: str) -> bytes:
    data = b"".join((run / f"{tag}-plane{plane}-private.bin").read_bytes()
                    for plane in range(4))
    require(len(data) == 4 * 0x3F40, f"{tag} EGA window length")
    locked(data, WINDOW_SHA, f"{tag} EGA window")
    for page in (0, 1):
        part = b"".join(data[plane * 0x3F40 + page * 0x2000:
                             plane * 0x3F40 + page * 0x2000 + 8000]
                        for plane in range(4))
        locked(part, PAGE_SHA[page], f"{tag} page {page}")
    return data


def probe_once(probe: Path, run: Path, label: str, before: bytes,
               expected: bytes, args: tuple[int, ...],
               expected_stores: int, expected_changed: int,
               selected: int, restored: int) -> None:
    source = run / f"{label}-audit-input-private.bin"
    result = run / f"{label}-audit-output-private.bin"
    source.write_bytes(before)
    command = [str(probe.resolve()), str(source), str(result)]
    command.extend(str(arg) for arg in args)
    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    match = re.fullmatch(r"stores=(\d+) changed=(\d+) selected=([0-9A-F]{4}) "
                         r"restored=([0-9A-F]{4})\n", completed.stdout)
    require(match is not None, f"{label} probe output")
    measured = (int(match[1]), int(match[2]), int(match[3], 16), int(match[4], 16))
    require(measured == (expected_stores, expected_changed, selected, restored),
            f"{label} probe report {measured}")
    require(result.read_bytes() == expected, f"{label} C/native planar mismatch")
    print(f"{label}: {len(expected)}/{len(expected)} plane bytes, "
          f"native changes {expected_changed}, C targets {expected_stores}")


def audit(options) -> None:
    exe = options.exe.read_bytes()
    game = options.ovr.read_bytes()
    locked(exe, START_SHA, "START.EXE")
    locked(game, GAME_SHA, "GAME.OVR")
    locked(options.save.read_bytes(), SAVE_SHA, "SAVGAMA.DAT")
    locked(options.emulator.read_bytes(), EMULATOR_SHA, "heavy DOSBox-X")
    image, *_ = unpack(exe)
    locked(image[0x7147:0x7201], ORIGINAL_BODY_SHA, "original rectangle body")
    locked(image[0x7090:0x7101], RESET_SHA, "original EGA reset callee")
    locked(image[0x7130:0x7147], PAGE_SELECTOR_SHA, "original page selector")
    locked(game[0x17D7:0x17DC], PRINT_ORIGINAL_CALL_SHA, "U1C print far call")
    locked(image[0x7090 + 0x07E1:0x7090 + 0x07E4], PRINT_CLEAR_CALL_SHA,
           "print to clear near call")
    for number, offset in enumerate((0x0E8E, 0x0EA8)):
        locked(image[0x7090 + offset:0x7090 + offset + 3],
               RECT_CALL_SHA[number], f"original rectangle call {number + 1}")

    ram = {}
    pages = {}
    for tag, expected in RAM_SHA.items():
        data = (options.run / f"{tag}-ram-private.bin").read_bytes()
        require(len(data) == 0xA0000, f"{tag} RAM length")
        locked(data, expected, f"{tag} RAM")
        locked(data[0x0F2D0 + 0x00B7:0x0F2D0 + 0x0171],
               LOADED_BODY_SHA, f"{tag} loaded rectangle")
        locked(data[0x0F2D0:0x0F2D0 + 0x0071],
               RESET_SHA, f"{tag} loaded EGA reset")
        locked(data[0x0F2D0 + 0x00A0:0x0F2D0 + 0x00B7],
               PAGE_SELECTOR_SHA, f"{tag} loaded page selector")
        locked(data[0x131C0 + 0x16A8:0x131C0 + 0x16BC],
               SYSTEM_FILL_SHA, f"{tag} loaded SYSTEM fill")
        locked(data[0x35820 + 0x0A91:0x35820 + 0x0A96],
               PRINT_LOADED_CALL_SHA, f"{tag} loaded U1C call")
        locked(data[0x0F2D0 + 0x07E1:0x0F2D0 + 0x07E4],
               PRINT_CLEAR_CALL_SHA, f"{tag} loaded clear call")
        require(data[0x14A00 + 0x4A1E] == 1, f"{tag} EGA mode")
        require(data[0x14A00 + 0x4A1A] == 0, f"{tag} active page")
        require(struct.unpack_from("<H", data, 0x14A00 + 0x4A18)[0] == 0xA000,
                f"{tag} restored draw segment")
        ram[tag] = data
        pages[tag] = window(options.run, tag)
    for number, return_ip, page in ((1, 0x0E91, 0), (2, 0x0EAB, 1)):
        entry = ram[f"rect{number}-entry"]
        words = struct.unpack_from("<8H", entry, 0x1D160 + 0x3C7C)
        require(words[:2] == (return_ip, 0x0F2D), f"rectangle {number} return frame")
        require(tuple(word & 255 for word in words[2:]) ==
                (0, page, 22, 38, 17, 1), f"rectangle {number} arguments")
        site = (0x0E8E, 0x0EA8)[number - 1]
        locked(entry[0x0F2D0 + site:0x0F2D0 + site + 3],
               RECT_CALL_SHA[number - 1], f"loaded rectangle call {number}")
        probe_once(options.probe, options.run, f"rect{number}",
                   pages[f"rect{number}-entry"], pages[f"rect{number}-return"],
                   (0, page, 22, 38, 17, 1, 0), 7296, 0,
                   0xA000 + page * 0x200, 0xA000)
    require(pages["preprint"] == pages["postprint"], "outer print page boundary")

    before = b"".join((options.row5_run /
                       f"page-call027-return-plane{plane}.bin").read_bytes()
                      for plane in range(4))
    after = b"".join((options.row5_run /
                      f"page-call028-entry-plane{plane}.bin").read_bytes()
                     for plane in range(4))
    require(len(before) == len(after) == 32000, "row-five page length")
    locked(before, ROW5_PRE_SHA, "row-five pre-clear page")
    locked(after, ROW5_POST_SHA, "row-five post-clear page")
    require(sum(a != b for a, b in zip(before, after)) == 154,
            "row-five native value changes")
    probe_once(options.probe, options.row5_run, "row5", before, after,
               (0, 0, 5, 38, 5, 17, 0), 704, 154, 0xA000, 0xA000)
    print("bounded rectangle parity: 3/3 calls, 161536/161536 plane bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--ovr", required=True, type=Path)
    parser.add_argument("--save", required=True, type=Path)
    parser.add_argument("--emulator", required=True, type=Path)
    parser.add_argument("--run", required=True, type=Path)
    parser.add_argument("--row5-run", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    audit(parser.parse_args())
