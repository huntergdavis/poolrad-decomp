#!/usr/bin/env python3
"""Address/hash/count audit of the post-Up view flush into persistent EGA pages."""

import argparse
import hashlib
from pathlib import Path
import subprocess

from capstone import CS_ARCH_X86, CS_MODE_16, Cs


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
CALL_SHA = "e0055a6b9ab9d331162e677d95400c8459a2dd025aa4ffa8f4f2ba92f7968411"
LIVE_CALL_SHA = "1f137b76909275734e41077c1dc7471b9effc1a81029c67294f96cdf8496b477"
FLUSH_SHA = "7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c"
OUTPUT_SHA = "86b0c234c0958a9b841529e5df8a61630c2d0dc40eca4ce7a5fc9f1c7a17fd22"
RAM_HASHES = {
    "view-entry": "04c001fa62397b12e6944bb737fec9beba9bc64c636d11a02de7bb92db61b5b3",
    "first-tile-entry": "ce46d9cb61cbd8bb58187e113bcd20369ee55d5eea99544102c4edeb612e5d35",
    "flush-entry": "0a52ffd104d9277534e57940de97560b50ceeafe46a37583894fb5184b215061",
    "flush-return": "9d3ff617ab35da40cd5b03f0e38c53b4aaa051feaa56e392370ccb2279c15038",
}
PRE_HASHES = (
    "625deb6ace7bb0c9386515828ec4f5cabbfceb88ad25940278f8ac0c6c7efb7e",
    "9c386eb2e055b9b1030e59ed45dc8042a37fe15db3977e3ff84a421222b0e05d",
    "41e9fd59f57279235fa656d70e114d23d2bba4309e23b6c0093216e9d65424df",
    "558cb49ebe7333b97b7bdde0e9a3cb6355777e7968f1aa3dc1263bce488f9435",
)
POST_HASHES = (
    "1cc993dcf9fa7f47ed1bc27bcb32b1376382f21aba89033f6cbf187172bac70f",
    "38f7ba9fc9c900545a5c59157826bf33a7ff1278484084a5d82d776874593a19",
    "350012034c279d2bbf0131aa9843a1d9faf433e5b95b8b3277e56c0ab0847b99",
    "7101f831f127d638507ce3f0367683ef306b57883f746baa1735533f1252bf0a",
)


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise AssertionError(reason)


def pinned(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    require(hashlib.sha256(data).hexdigest() == expected,
            f"hash mismatch: {path}")
    return data


def word(data: bytes, at: int) -> int:
    return data[at] | (data[at + 1] << 8)


def instruction(span: bytes, start: int, mnemonic: str, operands: str,
                count: int) -> None:
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, start))
    require(len(insns) == count and sum(i.size for i in insns) == len(span),
            f"aligned instruction count at {start:04X}")
    if count == 1:
        require((insns[0].mnemonic, insns[0].op_str) ==
                (mnemonic, operands), f"instruction at {start:04X}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--start", type=Path, required=True)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    start = pinned(args.start, START_SHA)
    game = pinned(args.game, GAME_SHA)
    pinned(args.run / "POOLRAD/SAVGAMA.DAT", SAVE_SHA)
    call = game[0x41D8:0x41DD]
    require(hashlib.sha256(call).hexdigest() == CALL_SHA,
            "original U1C view call hash")
    instruction(call, 0x3492, "lcall", "0x3ca, 0x25", 1)
    flush = start[0x61CB:0x64D5]
    require(len(flush) == 778 and
            hashlib.sha256(flush).hexdigest() == FLUSH_SHA,
            "resident flush original span")
    instruction(flush, 0x1B52, "", "", 305)

    ram = {name: pinned(args.run / f"{name}-ram-private.bin", sha)
           for name, sha in RAM_HASHES.items()}
    require(all(len(buf) == 0xA0000 for buf in ram.values()),
            "complete DOS RAM captures")
    view, entry, returned = (ram[name] for name in
                             ("view-entry", "flush-entry", "flush-return"))
    loaded = view[0x3582 * 16 + 0x3492:0x3582 * 16 + 0x3497]
    require(hashlib.sha256(loaded).hexdigest() == LIVE_CALL_SHA,
            "loaded U1C call hash")
    instruction(loaded, 0x3492, "lcall", "0xbee, 0x25", 1)
    ss, ds = 0x1D16 * 16, 0x14A0 * 16
    require((word(entry, ss + 0x3EBE), word(entry, ss + 0x3EC0)) ==
            (0x05FB, 0x2D14), "live flush return frame")
    require((entry[ds + 0x4A1A], entry[ds + 0x4A1E],
             word(entry, ds + 0x4A16), word(entry, ds + 0x4A18)) ==
            (0, 1, 0xA000, 0xA000), "reached video state/page")
    for name in ("view-entry", "flush-entry", "flush-return"):
        frame = ram[name]
        window = word(frame, ds + 0x49D2) + 16 * word(frame, ds + 0x49D4)
        require(tuple(frame[ds + 0x6AAD:ds + 0x6AB0]) == (14, 4, 6)
                and window == 0x39960 and
                tuple(word(frame, window + offset)
                      for offset in (0x18E, 0x190, 0x192)) == (0, 0, 11),
                f"post-Up party/window frame at {name}")
    require(tuple(entry[ds + 0x25F6:ds + 0x25FA]) == (1, 2, 4, 8),
            "four map masks")
    ptr = word(entry, ds + 0x4A20) + 16 * word(entry, ds + 0x4A22)
    require(ptr == 0x3DE48, "live far offscreen pointer")
    require(sum(bool(x) for x in view[ds + 0x4A28:ds + 0x4AD0]) == 0,
            "clean rows at view entry")
    rows = [y for y in range(168) if entry[ds + 0x4A28 + y]]
    require(rows == list(range(16, 104)), "88 reached dirty rows")
    for y in rows:
        first = word(entry, ds + 0x4C20 + 2 * y)
        last = word(entry, ds + 0x4D70 + 2 * y)
        low = word(entry, ds + 0x4AD0 + 2 * y)
        stage = word(entry, ds + 0x5050 + 2 * (y + 8)) + low + 1
        page = word(entry, ds + 0x5060) + 1 + y * 40 + low
        require((first, last, low, stage, page) ==
                (1352 + 84 * (y - 16), 1395 + 84 * (y - 16), 2,
                 0x03C3 + 40 * (y - 16), 0x03C3 + 40 * (y - 16)),
                f"reached row {y} extent")
    source_end = 0x17 + 8704
    source_changes = sum(a != b for a, b in zip(
        view[ptr:ptr + source_end], entry[ptr:ptr + source_end]))
    require(source_changes == 697, "upstream offscreen value changes")

    phases = ("view-entry", "first-tile-entry", "flush-entry",
              "flush-return", "view-return")
    captures = {}
    for phase in phases:
        hashes = PRE_HASHES if phase in phases[:3] else POST_HASHES
        captures[phase] = [pinned(args.run /
                                  f"{phase}-plane{plane}-private.bin", sha)
                           for plane, sha in enumerate(hashes)]
        require(all(len(p) == 0x3F40 for p in captures[phase]),
                f"four complete page windows at {phase}")
    changed = sum(a != b for before, after in zip(
        captures["flush-entry"], captures["flush-return"])
        for a, b in zip(before, after))
    require(changed == 1394, "native flush page-value delta")

    process = subprocess.run(
        [str(args.probe), str(args.run / "flush-entry-ram-private.bin"),
         "14A0", "00", "00", "00", "00", "FF",
         *(str(args.run / f"flush-entry-plane{p}-private.bin")
           for p in range(4))],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    require(process.returncode == 0,
            f"C probe failed: {process.stderr.decode(errors='replace')}")
    output = process.stdout
    require(len(output) == 4 * 0x3F40 + 5 * 168 and
            hashlib.sha256(output).hexdigest() == OUTPUT_SHA,
            "C full-page output shape/hash")
    page_matches = 0
    for plane in range(4):
        prediction = output[plane * 0x3F40:(plane + 1) * 0x3F40]
        native = captures["flush-return"][plane]
        page_matches += sum(a == b for a, b in zip(prediction, native))
    metadata_matches = 0
    for y in range(168):
        native = (bytes([returned[ds + 0x4A28 + y]]) +
                  returned[ds + 0x4C20 + 2 * y:ds + 0x4C22 + 2 * y] +
                  returned[ds + 0x4D70 + 2 * y:ds + 0x4D72 + 2 * y])
        prediction = output[4 * 0x3F40 + 5 * y:4 * 0x3F40 + 5 * (y + 1)]
        metadata_matches += native == prediction
    require(page_matches == 4 * 0x3F40 and metadata_matches == 168,
            "persistent C/native page and metadata parity")
    print(f"U1C:+3492 original 5 bytes/1 instruction SHA-256 {CALL_SHA}; "
          f"loaded 3582:3492 SHA-256 {LIVE_CALL_SHA}")
    print(f"RES_0464:1B52..1E5B original 778 bytes/305 instructions "
          f"SHA-256 {FLUSH_SHA}; return 2D14:05FB")
    print(f"view entry clean rows 0; flush entry dirty rows 16..103 (88); "
          f"offscreen pointer 3DE4:0008, bounded source changes "
          f"{source_changes}")
    print(f"native page-value changes only across observed flush boundary "
          f"{changed}; C page {page_matches}/{4 * 0x3F40}, "
          f"metadata {metadata_matches}/168")
    print("upstream offscreen construction and all same-value/transient "
          "EGA writes remain outside this bounded parity gate")


if __name__ == "__main__":
    main()
