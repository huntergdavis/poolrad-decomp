#!/usr/bin/env python3
"""Hash-locked, numeric-only audit of a same-run post-Up two-page UI join.

Private RAM, EGA planes, font and text are read only from ignored resources/.
The C probe also writes its input and outputs there; no payload is printed.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import struct
import subprocess

from slot_a_printer_page_audit import check_hash, unpack

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
SOURCE_SHA = (
    "e767d5c8ef552436c5e411af72d8eca9581f3feacbb988c67c16bf2b701bc2fe",
    "f13e024bc34f688552e96d6cfb8be870b68d2ada58f4ae6fdea7fd7e583e9860",
)

PHASES = (
    "first-entry", "first-return", "clear-entry", "clear-return",
    "second-entry", "second-return",
)
RAM_SHA = {
    "first-entry": "26d4d41792fc6420c511863804f4ca24904373b19716cc2ce0e21f3a45eb210d",
    "first-return": "381d5bc520142c3c6db8d2712b309250b834121dcf7cf8a1f2dc0c6cd87c8774",
    "clear-entry": "54e1a3102c965595949559dc4d1bcf35334176c359cfd69b31a51d3f5c7b39af",
    "clear-return": "90b1a62f2f86e2de0668581640813ab47248b69b027d835e38275acfcb7b0f5b",
    "second-entry": "73630672480d345c7bf8ce41c4814cceeaa9d965c31a1e27aae66b5a97058d45",
    "second-return": "2776c4b7be6dfab5f25297214a68c87e756ef9b3e2dffcf0376cf53426230e52",
}
BASE_PLANES = (
    "625deb6ace7bb0c9386515828ec4f5cabbfceb88ad25940278f8ac0c6c7efb7e",
    "e9c47e65de1395c3be92b743b01a9c07c873d18f8f25032921948ab6c067bcf8",
    "41e9fd59f57279235fa656d70e114d23d2bba4309e23b6c0093216e9d65424df",
    "ac3c4008c06ef63653f9d214b52e2f0dd4d62c28fb38f220422b8c74ae98bc8f",
)
FIRST_PLANES = (
    BASE_PLANES[0],
    "b3b8d5a2823747c060e9b8c50fdcf2e82e503e5273e02033282593471b1c62ad",
    BASE_PLANES[2],
    "9a2f72c6e4a77de38371deacfbf3b3ed05a573f3becec6cd3b29284a29f581c6",
)
SECOND_PLANES = (
    BASE_PLANES[0],
    "9c386eb2e055b9b1030e59ed45dc8042a37fe15db3977e3ff84a421222b0e05d",
    BASE_PLANES[2],
    "558cb49ebe7333b97b7bdde0e9a3cb6355777e7968f1aa3dc1263bce488f9435",
)
PLANE_SHA = {
    "first-entry": BASE_PLANES,
    "first-return": FIRST_PLANES,
    "clear-entry": FIRST_PLANES,
    "clear-return": BASE_PLANES,
    "second-entry": BASE_PLANES,
    "second-return": SECOND_PLANES,
}
WINDOW_SHA = {
    "first-entry": "9946afbdbb1753236cee9398e04567ca2e93b22b564ee5ae301dfa04840ef9ca",
    "first-return": "7a67fbc0ce069067d4e2ac8cdff4dc5329a8bdf6a6479dc5b8ec42ba41d855d7",
    "clear-entry": "7a67fbc0ce069067d4e2ac8cdff4dc5329a8bdf6a6479dc5b8ec42ba41d855d7",
    "clear-return": "9946afbdbb1753236cee9398e04567ca2e93b22b564ee5ae301dfa04840ef9ca",
    "second-entry": "9946afbdbb1753236cee9398e04567ca2e93b22b564ee5ae301dfa04840ef9ca",
    "second-return": "41c5e54341b75a0ecb157343a2ab209df1ceb9388029164f3db74c08abb0d91a",
}
A000_SHA = {
    "first-entry": "b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44",
    "first-return": "a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac",
    "clear-entry": "a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac",
    "clear-return": "b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44",
    "second-entry": "b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44",
    "second-return": "1019a688b324afa165f731013c6491c0cb54232262403f7c70cbb7fb97543773",
}
A200_SHA = "f1d00351b9c1e79264fcb2b6804a32b26570c811aa50498185409205f07d416b"

PRINTER_CALL_IMAGE_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
PRINTER_CALL_LOADED_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
PRINTER_BODY_IMAGE_SHA = "b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed"
PRINTER_BODY_LOADED_SHA = "8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164"
PARENT_CALL_IMAGE_SHA = "395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f"
PARENT_CALL_LOADED_SHA = "2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850"
RECT_CALL_IMAGE_SHA = "2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c"
RECT_CALL_LOADED_SHA = "8d1b1dec8fbef9dea92ee3e851c55572850da6b639cd9fb3890c998e918cc8e2"
RECT_BODY_IMAGE_SHA = "f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5"
RECT_BODY_LOADED_SHA = "02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f"


def far_call(data: bytes, digest: str, target: tuple[int, int], label: str) -> None:
    check_hash(data, digest, label)
    if len(data) != 5 or data[0] != 0x9A or \
            struct.unpack_from("<HH", data, 1) != target:
        raise ValueError(f"{label} target changed")


def native_fields(memory: bytes) -> tuple[tuple[int, ...], tuple[int, ...]]:
    if len(memory) != 0xA0000:
        raise ValueError("RAM capture size changed")
    ds = 0x14A00
    if struct.unpack_from("<2H", memory, ds + 0x49D2) != (0, 0x3996):
        raise ValueError("window-zero far pointer changed")
    if struct.unpack_from("<2H", memory, ds + 0x4A16) != (0xA000, 0xA000) or \
            memory[ds + 0x4A1A] != 0 or memory[ds + 0x4A1E] != 1:
        raise ValueError("live EGA draw metadata changed")
    party = tuple(memory[ds + offset] for offset in (0x6AAD, 0x6AAE, 0x6AAF))
    words = struct.unpack_from("<3H", memory, 0x39960 + 0x18E)
    return party, words


def printer_call(memory: bytes, sp: int, digest: str) -> bytes:
    frame = struct.unpack_from("<7H", memory, 0x1D160 + sp)
    if frame[:2] != (0x2B5A, 0x08DE) or frame[3:] != (
            0x1D16, 0xA00A, 0xA00F, 0xA011):
        raise ValueError("post-Up printer caller frame changed")
    source_at = frame[3] * 16 + frame[2]
    length = memory[source_at]
    if length != 19:
        raise ValueError("post-Up printer source length changed")
    check_hash(memory[source_at:source_at + length + 1], digest,
               "private post-Up Pascal source")
    ds = 0x14A00
    if struct.unpack_from("<2H", memory, ds + 0x5E20) != (0, 0x3CB6):
        raise ValueError("live font pointer changed")
    font = memory[0x3CB60:0x3CB60 + 1416]
    check_hash(font, FONT_SHA, "live font block")
    segments = struct.unpack_from("<2H", memory, ds + 0x4A16)
    return (struct.pack("<2H4B", *segments, length, 17, 15, 10) +
            memory[source_at + 1:source_at + length + 1] + bytes(255 - length))


def validate_delta(before: bytes, after: bytes) -> tuple[int, int]:
    if len(before) != 4 * 16192 or len(after) != 4 * 16192:
        raise ValueError("two-page EGA window size changed")
    changed = {index for index, (a, b) in enumerate(zip(before, after))
               if a != b}
    allowed = {lane * 16192 + line * 40 + col
               for lane in (1, 3) for line in range(120, 128)
               for col in range(17, 36)}
    if len(changed) != 210 or not changed <= allowed:
        raise ValueError("post-Up page delta changed or reached A200")
    return len(changed), sum(index % 16192 >= 8192 for index in changed)


def audit(run: Path, emulator: Path, probe: Path) -> None:
    exe = (run / "POOLRAD/START.EXE").read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash((run / "POOLRAD/GAME.OVR").read_bytes(), GAME_SHA, "GAME.OVR")
    check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(), SAVE_SHA,
               "slot-A save")
    check_hash(emulator.read_bytes(), EMULATOR_SHA, "private DOSBox-X")
    image, *_ = unpack(exe)
    far_call(image[0x36F5:0x36FA], PRINTER_CALL_IMAGE_SHA,
             (0x0626, 0x0709), "original printer call")
    check_hash(image[0x76B6:0x7737], PRINTER_BODY_IMAGE_SHA,
               "original printer body")
    far_call(image[0x36E2:0x36E7], PARENT_CALL_IMAGE_SHA,
             (0x0137, 0x03F1), "original parent call")
    far_call(image[0x4101:0x4106], RECT_CALL_IMAGE_SHA,
             (0x00B7, 0x0709), "original rectangle call")
    check_hash(image[0x7147:0x7201], RECT_BODY_IMAGE_SHA,
               "original rectangle body")

    memory = {}
    windows = {}
    for phase in PHASES:
        memory[phase] = (run / f"{phase}-ram-private.bin").read_bytes()
        check_hash(memory[phase], RAM_SHA[phase], f"{phase} RAM")
        planes = []
        for lane, digest in enumerate(PLANE_SHA[phase]):
            data = (run / f"{phase}-plane{lane}-private.bin").read_bytes()
            if len(data) != 16192:
                raise ValueError(f"{phase} plane {lane} size changed")
            check_hash(data, digest, f"{phase} plane {lane}")
            planes.append(data)
        windows[phase] = b"".join(planes)
        check_hash(windows[phase], WINDOW_SHA[phase], f"{phase} full window")
        check_hash(b"".join(data[:8000] for data in planes), A000_SHA[phase],
                   f"{phase} A000 page")
        check_hash(b"".join(data[8192:16192] for data in planes), A200_SHA,
                   f"{phase} A200 page")
        expected = ((15, 4, 6), (0, 5, 10)) if phase.startswith("first-") \
            else ((14, 4, 6), (0, 0, 11))
        if native_fields(memory[phase]) != expected:
            raise ValueError(f"{phase} party/window boundary changed")

    for phase in ("first-entry", "second-entry"):
        loaded = memory[phase]
        far_call(loaded[0x08DE0 + 0x2B55:0x08DE0 + 0x2B5A],
                 PRINTER_CALL_LOADED_SHA, (0x0626, 0x0F2D),
                 "loaded printer call")
        check_hash(loaded[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7],
                   PRINTER_BODY_LOADED_SHA, "loaded printer body")
    loaded = memory["clear-entry"]
    far_call(loaded[0x08DE0 + 0x2B42:0x08DE0 + 0x2B47],
             PARENT_CALL_LOADED_SHA, (0x0137, 0x0C15), "loaded parent call")
    far_call(loaded[0x0C150 + 0x01F1:0x0C150 + 0x01F6],
             RECT_CALL_LOADED_SHA, (0x00B7, 0x0F2D),
             "loaded rectangle call")
    check_hash(loaded[0x0F2D0 + 0x00B7:0x0F2D0 + 0x0171],
               RECT_BODY_LOADED_SHA, "loaded rectangle body")
    if struct.unpack_from("<8H", loaded, 0x1D160 + 0x3B62) != (
            0x01F6, 0x0C15, 0, 0, 15, 38, 15, 17) or \
            struct.unpack_from("<2H", loaded, 0x1D160 + 0x3B80) != (
                0x2B47, 0x08DE):
        raise ValueError("row-15 rectangle frame or parent changed")

    for entry, returned in (("first-entry", "first-return"),
                            ("clear-entry", "clear-return"),
                            ("second-entry", "second-return")):
        if validate_delta(windows[entry], windows[returned]) != (210, 0):
            raise ValueError("A200 changed")
    if windows["first-return"] != windows["clear-entry"] or \
            windows["clear-return"] != windows["second-entry"]:
        raise ValueError("same-run intercall page bridge changed")

    first = memory["first-entry"]
    font = first[0x3CB60:0x3CB60 + 1416]
    payload = (font + printer_call(first, 0x3C6E, SOURCE_SHA[0]) +
               printer_call(memory["second-entry"], 0x3B7E, SOURCE_SHA[1]) +
               windows["first-entry"])
    if len(payload) != 1416 + 2 * 263 + 4 * 16192:
        raise ValueError("private C probe input size changed")
    input_path = run / "page-join-input-private.bin"
    outputs = [run / f"page-join-{stage}-private.bin"
               for stage in ("first", "clear", "second")]
    input_path.write_bytes(payload)
    result = subprocess.run([str(probe.resolve()), str(input_path),
                             *(str(path) for path in outputs)],
                            check=True, capture_output=True, text=True)
    if result.stdout.strip() != (
            "first pages=1 cells=19 stores=608 changed=210\n"
            "clear stores=704 changed=210 selected=A000 restored=A000\n"
            "second pages=1 cells=19 stores=608 changed=210"):
        raise ValueError("C page route report changed")
    for output, native in zip(outputs,
                              ("first-return", "clear-return", "second-return")):
        if output.read_bytes() != windows[native]:
            raise ValueError(f"persistent C page differs at {native}")
    print("same-run route: 08DE:2B55 -> 0C15:01F1 -> 08DE:2B55")
    print("native A000 changed: 210, 210, 210; A200 changed: 0, 0, 0")
    print("persistent C/native windows: 64768/64768 each at three returns")
    print("native party/window: 15/4/6;0/5/10 -> 14/4/6;0/0/11 before clear")
    print("first missing C state: intercall party/window transition before clear")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.run, args.emulator, args.probe)
