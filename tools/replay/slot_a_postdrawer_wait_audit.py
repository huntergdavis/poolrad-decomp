#!/usr/bin/env python3
"""Audit the saved-slot-A prompt page through one reached no-key wait loop."""

import argparse
from hashlib import sha256
from pathlib import Path


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
PAGE_SHA = "09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a"
RAM_SHA = {
    "postdrawer-048e": "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f",
    "wait-0566": "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f",
    "wait2-0566": "4ee56e79a63b3c8991afeb5c83a90d768bb92c758939a1af4eb8da5cc784b6c8",
}
ORIGINAL_SPANS = {
    (0x048E, 0x0566): "1709d5b2bec86d104ce166c83b7b858828dd28b6794bbebe2ed95881c3c858ab",
    (0x0566, 0x0572): "cc4b28d4950373552e6630c9271f61a579ce77c835376ae4b43f5a247effb386",
    (0x06CF, 0x06D8): "c24571c84c5a3cb61e27a19efc23ad8695b66634c428cae065cb3bad1a954916",
}
KEYPOLL_SHA = "83af1ee6b1d5cebb5dee97b742228b8a4fb3cbb12ade84b39e5ce82275e7fc5e"


def checked(data: bytes, expected: str, label: str) -> bytes:
    actual = sha256(data).hexdigest()
    if actual != expected:
        raise ValueError(f"{label} SHA-256 changed: {actual}")
    return data


def word(data: bytes, address: int) -> int:
    return int.from_bytes(data[address:address + 2], "little")


def audit(args: argparse.Namespace) -> None:
    exe = checked(args.exe.read_bytes(), START_SHA, "START.EXE")
    game = checked(args.game.read_bytes(), GAME_SHA, "GAME.OVR")
    checked(args.save.read_bytes(), SAVE_SHA, "slot-A save")
    checked(args.emulator.read_bytes(), EMULATOR_SHA, "heavy DOSBox-X")

    ram = {}
    pages = {}
    for name, expected in RAM_SHA.items():
        image = checked((args.run / f"{name}-ram-private.bin").read_bytes(),
                        expected, f"{name} RAM")
        if len(image) != 655360:
            raise ValueError(f"{name} RAM has an unexpected length")
        ram[name] = image
        plane = [(args.run / f"{name}-plane{i}-private.bin").read_bytes()
                 for i in range(4)]
        if any(len(p) != 8000 for p in plane):
            raise ValueError(f"{name} EGA plane has an unexpected length")
        pages[name] = checked(b"".join(plane), PAGE_SHA, f"{name} EGA page")

    if len(set(pages.values())) != 1:
        raise ValueError("a page changed across the post-drawer no-key loop")
    if ram["postdrawer-048e"] != ram["wait-0566"]:
        raise ValueError("RAM changed before the first key poll")
    first = ram["wait-0566"]
    second = ram["wait2-0566"]
    changed_ram = [i for i, (a, b) in enumerate(zip(first, second)) if a != b]
    if changed_ram != list(range(0x20F5E, 0x20F68)):
        raise ValueError("the no-key loop RAM footprint changed")

    base = 182479  # audited GAME.OVR unit 0x3B0 file base
    loaded_base = 0x2116 << 4
    for (start, end), expected in ORIGINAL_SPANS.items():
        original = checked(game[base + start:base + end], expected,
                           f"original overlay +{start:04X}..+{end:04X}")
        loaded = first[loaded_base + start:loaded_base + end]
        relocated = bytearray(original)
        for offset in (0x04CF, 0x04EC, 0x04F3, 0x0528, 0x0543, 0x0569):
            if start <= offset and offset + 1 < end:
                at = offset - start
                relocated[at:at + 2] = ((word(original, at) + 0x824) & 0xFFFF).to_bytes(2, "little")
        if loaded != relocated:
            raise ValueError(f"loaded overlay +{start:04X}..+{end:04X} changed beyond relocations")

    keypoll = checked(exe[0xA3F6:0xA3F6 + 18], KEYPOLL_SHA,
                      "original resident key poll")
    if first[(0x1264 << 4) + 0x02FD:(0x1264 << 4) + 0x02FD + 18] != keypoll:
        raise ValueError("loaded resident key poll differs from START.EXE")

    ds = 0x14A0 << 4
    ss = 0x1D16 << 4
    for name, image in ram.items():
        window_off, window_seg = word(image, ds + 0x49D2), word(image, ds + 0x49D4)
        window = (window_seg << 4) + window_off
        actual = (
            (window_seg, window_off),
            word(image, window + 0x03FE),
            tuple(image[ds + a] for a in (0x6AAD, 0x6AAE, 0x6AAF)),
            tuple(word(image, window + a) for a in (0x018E, 0x0190, 0x0192)),
            image[ds + 0x84DC],
            image[ds + 0x6ABF], image[ds + 0x6ABE],
            word(image, ds + 0x6B2E), word(image, ds + 0x6B30),
            image[ss + 0x3E35],
        )
        expected = ((0x3996, 0), 3, (15, 4, 6), (0, 5, 10), 20, 0, 0, 0, 0, 0)
        if actual != expected:
            raise ValueError(f"{name} saved-slot frame or no-key branch input changed: {actual}")

    print("native stops: 2116:048E -> 0566 -> 056B AL=0 -> 06CF -> 048E -> 0566")
    print("EGA page: three 32000-byte snapshots equal; changed=0")
    print("RAM: first interval identical; full no-key loop changes 10 stack bytes")
    print("original/loaded code: far-call segment relocations +0824 only")
    print("page SHA-256", PAGE_SHA)
    print("no C page model or key-dispatch parity is claimed")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "game", "save", "emulator", "run"):
        parser.add_argument("--" + name, type=Path, required=True)
    audit(parser.parse_args())


if __name__ == "__main__":
    main()
