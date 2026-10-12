#!/usr/bin/env python3
"""Read only numeric run010 states; no private RAM or code is emitted."""
import hashlib
from pathlib import Path
import sys

SAMPLES = (
    ("bp-026c-after-seed-ram.bin", "3ee6a79b9a53a7401d2679fd803635ef2df144a0d3f17f5c9b29cbd376fd108c", 0x9B40, 0x00, 0x08, 3140872353, 16),
    ("bp-028e-after-store-ram.bin", "d33e3eaf1968a4f656f6df1304a4348eae620bbf0a2655bbec39178efb95a62b", 0x9B40, 0x00, 0x08, 3238563622, 10),
    ("bp-script-00-3582-2f2d-ram.bin", "26ea01908c6c4b9e512687ff5d9a2dfc3876d9834c27480ae7c1805b9c7ab5e8", 0x9B40, 0x08, 0x03, 3238563622, 10),
    ("bp-script-01-3582-2f2d-ram.bin", "9c3a223bd89941cabe4bc2410f29dc21cfced283a13a3eb8a0ee6c2cb21a9bb7", 0x9B46, 0x03, 0x16, 3238563622, 10),
    ("bp-script-02-3582-2f2d-ram.bin", "9e7cb4f44ee120a836bbd2c5d1ce6c5c4f4c7f4ed0f7c2378b744e87d5ea7672", 0x9B50, 0x04, 0x03, 3238563622, 10),
    ("bp-script-03-3582-2f2d-ram.bin", "c5cf5066193216df0cd480dad0de485aed18bbff448b8545c10fd40e0503d926", 0x9B56, 0x03, 0x16, 3238563622, 10),
    ("bp-script-04-3582-2f2d-ram.bin", "b931382d4e68b25233ec64d23412f12e8b3ca49f09e74697707f7a98d529adff", 0x9B60, 0x04, 0x03, 3238563622, 10),
    ("bp-script-05-3582-2f2d-ram.bin", "a208851898a27a6d1034d5c795166e6c10c1a557fddcbddf49f84872161738c7", 0x9B66, 0x03, 0x1A, 3238563622, 10),
    ("bp-script-06-3582-2f2d-ram.bin", "bae33746a473a4b2c28aa40097095f67615cf1552641b0a6002bf02bbe4500a8", 0x9B67, 0x1A, 0x00, 3238563622, 10),
)
SPANS = (
    (0x0F7A, 0x0FD8, "a634c69e97fe005c2041e9d18d0874d0eb8ede7f4a59833bfb642918b7c5d979"),
    (0x0E6C, 0x0EE9, "330e74c38244df80bd2af04209126a207b5ebdf5dcf97c4781d4b960e2a52404"),
    (0x1898, 0x190E, "11830c28e2ca43d40e80db72b6374a46e73b4e0cc0522ba9728e7026e3718061"),
)
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
DS = 0x14A0 << 4


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def check(ok: bool, label: str) -> None:
    if not ok:
        raise AssertionError(label)


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: audit_slot_a_random_exit_chain.py IGNORED_RUN010_DIR IGNORED_GAME.OVR")
    directory, game_path = map(Path, sys.argv[1:])
    game = game_path.read_bytes()
    check(len(game) == 206105 and sha(game) == GAME_SHA, "GAME.OVR hash/size")
    for start, end, expected in SPANS:
        check(sha(game[start:end]) == expected, f"source span {start:05X}..{end - 1:05X}")
    for name, expected_hash, pc, previous, opcode, seed, w2_value in SAMPLES:
        ram = (directory / name).read_bytes()
        check(len(ram) == 655360 and sha(ram) == expected_hash, name + " hash/size")
        byte = lambda off: ram[DS + off]
        word = lambda off: int.from_bytes(ram[DS + off:DS + off + 2], "little")
        check(word(0x49ED) == pc and byte(0x6F7E) == previous and byte(0x6F7F) == opcode,
              name + " dispatcher state")
        check(int.from_bytes(ram[DS + 0x4428:DS + 0x442C], "little") == seed, name + " seed")
        check(tuple(ram[DS + 0x6F78:DS + 0x6F7E]) == (0, 1, 1, 0, 1, 0), name + " flags")
        check((word(0x49D2), word(0x49D4)) == (0, 0x3996), name + " window-0 pointer")
        check((word(0x49D6), word(0x49D8)) == (0, 0x3A16), name + " window-1 pointer")
        check((word(0x49DA), word(0x49DC)) == (0, 0x3A96), name + " window-2 pointer")
        def wword(segment: int, offset: int) -> int:
            physical = (segment << 4) + offset
            return int.from_bytes(ram[physical:physical + 2], "little")
        check((wword(0x3996, 0x18E), wword(0x3996, 0x190), wword(0x3996, 0x192),
               wword(0x3996, 0x1CC), wword(0x3996, 0x216)) == (1, 5, 10, 1, 0),
              name + " window-0 words")
        check(wword(0x3A16, 0x594) == 0 and wword(0x3A96, 0x21E) == w2_value,
              name + " window-1/2 words")
        check(byte(0x84E2) == 0 and byte(0x4430) == 0 and
              word(0x6F70) == 0 and word(0x6F72) == 0, name + " EXIT gates")
        check(byte(0x442E) == 0 and byte(0x49FF) == 0 and
              (byte(0x5E24), byte(0x5E25)) == (1, 17), name + " run state")
        print(f"{name}: sha256={expected_hash}, PC={pc:04X}, latch={previous:02X}/{opcode:02X}")
    print("nine native pre-RANDOM/store/dispatcher states and three handler spans match")


if __name__ == "__main__":
    main()
