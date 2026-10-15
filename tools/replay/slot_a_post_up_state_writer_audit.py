#!/usr/bin/env python3
"""Audit the saved post-Up U68 -> U7D state writer without publishing media."""

import argparse
from hashlib import sha256
from pathlib import Path
import struct


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
U68_LOADED_SHA = "04c0e7231a800b744bebd2d94ca11224d0d8d3e294fc6baf31d591a2f75cad7d"
U7D_LOADED_SHA = "7bd8c6d35073b0d7101c4a3fbe2154e916a1f72424636636fcf0b48718f148a4"

RAM_SHA = {
    "first-post-up-printer-return": "84619fbef1717b429a2ee8fb7a2bae828018161da86d9de43c0efe9c708a0b99",
    "u68-07e0-precall": "8dae80255e3157447e46abc7eb2990bb70d5e5690ece341505037363093b604b",
    "u7d-0462-idx3": "6d4fe6fbc100e90f93448b572716f76242dec1306f662a738fe0137121f26743",
    "u68-07e5-return": "7b54b0b3549b4aa3d76894f6e67565f785ef86c70123066bc7de19749f4f4057",
    "clear-entry": "b8fa241c1aed11cbcd1a6690e59fbf003d1719e7ce27f1a0e0fd9d641c270a45",
}
FRAME_SHA = {
    "u68-07e0-stack": "6521ee9c7fcfc49fbb364db83bf6bd8eb0b956e709f302434ecb20b82086f0e0",
    "u7d-03d5-entry-stack": "97a8f939d38bf7d01033620e31ee25db290d250efe4c9928092d1261ead725a3",
}
WORD_SHA = {
    "u7d-0462-idx2-window0": "d5798c18fd6aa2922958f40e404ce67c8a9aa61e048329353ad8246f04070881",
    "u7d-0462-idx3-window0": "405b80e106f9f5b9b2546c7de2be0850d5e0838cc3ce580a84cedfc28fda4d2b",
}
P9_SHA = {
    "first-return": "381d5bc520142c3c6db8d2712b309250b834121dcf7cf8a1f2dc0c6cd87c8774",
    "clear-entry": "54e1a3102c965595949559dc4d1bcf35334176c359cfd69b31a51d3f5c7b39af",
}

DS = 0x14A0
WINDOW0 = 0x3996
WINDOW1 = 0x3A16


def require(condition: bool, label: str) -> None:
    if not condition:
        raise ValueError(label)


def locked(path: Path, expected: str, size: int | None = None) -> bytes:
    data = path.read_bytes()
    require(size is None or len(data) == size, f"{path.name}: length {len(data)}")
    digest = sha256(data).hexdigest()
    require(digest == expected, f"{path.name}: SHA-256 {digest}")
    return data


def word(data: bytes, seg: int, off: int) -> int:
    return struct.unpack_from("<H", data, seg * 16 + off)[0]


def words(data: bytes) -> tuple[int, ...]:
    return tuple(word(data, WINDOW0, 0x18C + 2 * i) for i in range(7))


def party(data: bytes) -> tuple[int, ...]:
    return tuple(data[DS * 16 + 0x6AAD:DS * 16 + 0x6AB0])


def check_memory(label: str, data: bytes, expected_party: tuple[int, ...],
                 expected_words: tuple[int, ...]) -> None:
    require(party(data) == expected_party, f"{label}: party {party(data)}")
    require(words(data) == expected_words, f"{label}: window0 {words(data)}")
    require(struct.unpack_from("<HH", data, DS * 16 + 0x49D2) == (0, WINDOW0),
            f"{label}: window0 pointer")
    require(struct.unpack_from("<HH", data, DS * 16 + 0x49D6) == (0, WINDOW1),
            f"{label}: window1 pointer")
    require(word(data, WINDOW1, 0x594) & 1 == 1, f"{label}: window1 bit 0")
    require(word(data, DS, 0x363E) == 6, f"{label}: live limit[2]")
    require(data[DS * 16 + 0x273C] == 0xFF, f"{label}: facing-6 X step")
    require(data[DS * 16 + 0x2745] == 0, f"{label}: facing-6 Y step")


def check_loaded_body(data: bytes) -> None:
    for name, seg, off, length, expected in (
        ("U68", 0x2E60, 0x073B, 185, U68_LOADED_SHA),
        ("U7D", 0x2F6B, 0x03D5, 165, U7D_LOADED_SHA),
    ):
        actual = sha256(data[seg * 16 + off:seg * 16 + off + length]).hexdigest()
        require(actual == expected, f"{name}: loaded body SHA-256 {actual}")


def audit(run: Path, emulator: Path, p9_run: Path | None) -> None:
    locked(run / "POOLRAD/START.EXE", START_SHA)
    locked(run / "POOLRAD/GAME.OVR", GAME_SHA)
    locked(run / "POOLRAD/SAVGAMA.DAT", SAVE_SHA, 13137)
    locked(emulator, EMULATOR_SHA)

    ram = {name: locked(run / f"{name}-ram-private.bin", digest, 0xA0000)
           for name, digest in RAM_SHA.items()}
    before = (0, 0, 5, 10, 0, 0, 0)
    after = (0, 0, 0, 11, 0, 0, 0)
    check_memory("printer return", ram["first-post-up-printer-return"],
                 (15, 4, 6), before)
    check_memory("U68 pre-call", ram["u68-07e0-precall"], (14, 4, 6), before)
    for name in ("u7d-0462-idx3", "u68-07e5-return", "clear-entry"):
        check_memory(name, ram[name], (14, 4, 6), after)
    check_loaded_body(ram["clear-entry"])
    u68_code = ram["u68-07e0-precall"][0x2E600 + 0x07DA:0x2E600 + 0x07E5]
    require(u68_code == bytes.fromhex("b0 01 50 b0 02 50 9a 25 00 b0 08"),
            "U68 bit-set push/call bytes")
    u7d_store = ram["clear-entry"][0x2F6B0 + 0x045D:0x2F6B0 + 0x0462]
    require(u7d_store == bytes.fromhex("26 89 95 00 6e"),
            "U7D ES:[DI+6E00] word store bytes")
    require(((0x9390 + 0x6E00) & 0xFFFF,
             (0x9392 + 0x6E00) & 0xFFFF) == (0x0190, 0x0192),
            "U7D observed DI destinations")

    stack = {name: locked(run / f"{name}-private.bin", digest, 48)
             for name, digest in FRAME_SHA.items()}
    require(stack["u68-07e0-stack"][:4] == bytes.fromhex("02 00 01 00"),
            "U68 pre-call idx=2,n=1")
    require(stack["u68-07e0-stack"][6:10] == bytes.fromhex("31 10 60 2e"),
            "U68 caller return 2E60:1031")
    require(stack["u7d-03d5-entry-stack"][:8] ==
            bytes.fromhex("e5 07 60 2e 02 00 01 00"),
            "U7D return 2E60:07E5,idx=2,n=1")

    saved = {name: locked(run / f"{name}-private.bin", digest, 14)
             for name, digest in WORD_SHA.items()}
    at_minute = struct.unpack("<7H", saved["u7d-0462-idx2-window0"])
    at_hour = struct.unpack("<7H", saved["u7d-0462-idx3-window0"])
    require(at_minute == (0, 0, 0, 10, 0, 0, 0), "U7D +190 store")
    require(at_hour == after, "U7D +192 store")
    incremented = before[2] + 1
    carry, normalized = divmod(incremented, word(ram["u68-07e0-precall"], DS, 0x363E))
    require((normalized, before[3] + carry) == (at_hour[2], at_hour[3]),
            "U7D limit[2] carry")

    if p9_run is not None:
        p9 = {name: locked(p9_run / f"{name}-ram-private.bin", digest, 0xA0000)
              for name, digest in P9_SHA.items()}
        check_memory("p9 first return", p9["first-return"], (15, 4, 6), before)
        check_memory("p9 clear entry", p9["clear-entry"], (14, 4, 6), after)
        check_loaded_body(p9["clear-entry"])
        for plane in range(4):
            first = (p9_run / f"first-return-plane{plane}-private.bin").read_bytes()
            clear = (p9_run / f"clear-entry-plane{plane}-private.bin").read_bytes()
            require(len(first) == len(clear) == 0x3F40, f"p9 plane {plane} length")
            require(first == clear, f"p9 plane {plane}: unexpected EGA change")
        print("p9 endpoint fields and all four EGA windows: match")

    print("native U68 -> U7D frame and 5 RAM checkpoints: match")
    print("U7D +190 then +192 stores and limit[2] carry: match")
    print("full joined CPU/RAM parity: not asserted")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", required=True, type=Path)
    parser.add_argument("--emulator", required=True, type=Path)
    parser.add_argument("--p9-run", type=Path)
    args = parser.parse_args()
    audit(args.run, args.emulator, args.p9_run)


if __name__ == "__main__":
    main()
