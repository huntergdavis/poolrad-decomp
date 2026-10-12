#!/usr/bin/env python3
"""Read-only, address-only audit of ignored run010 EXIT snapshots."""
import hashlib
from pathlib import Path
import sys

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "9f2483f23b51d3dbc54a83592241cbd6f5bc27e1173e62f17c54c3b6672ca493"
RELOCATED_SHA = "c09b0f2dd45cb0ccad3b25afa54ea0e885dff87873679d630319ee47310d70ff"
SAMPLES = (
    ("bp-script-06-3582-2f2d-ram.bin", "bae33746a473a4b2c28aa40097095f67615cf1552641b0a6002bf02bbe4500a8", 0x9B67, 0),
    ("bp-script-07-3582-0001-ram.bin", "7d9303e803eaf54ecf8d30db5aa339a2107546543c1678debf1eee5189196091", 0x9B67, 0),
    ("bp-script-08-3582-31aa-ram.bin", "12a715b7ee9ca93844ed2edd0127a10353e4df38f0f389d245d985f32264557a", 0x9B68, 1),
    ("bp-script-09-3582-31b0-ram.bin", "d43b75633114880a24480a73a64bc623bbadb736729d39d78d2612e94f2c2d12", 0x9B68, 0),
)
DS = 0x14A0 << 4


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def byte(ram: bytes, offset: int) -> int:
    return ram[DS + offset]


def word(ram: bytes, offset: int) -> int:
    return int.from_bytes(ram[DS + offset:DS + offset + 2], "little")


def check(condition: bool, label: str) -> None:
    if not condition:
        raise AssertionError(label)


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: audit_slot_a_exit_snapshots.py IGNORED_SNAPSHOT_DIR IGNORED_GAME.OVR")
    directory, game_path = map(Path, sys.argv[1:])
    game = game_path.read_bytes()
    check(len(game) == 206105 and sha(game) == GAME_SHA, "GAME.OVR hash/size")
    span = game[0x0D47:0x0E3A]
    check(len(span) == 243 and sha(span) == SPAN_SHA, "EXIT original span hash/size")
    for name, expected_hash, expected_pc, expected_stop in SAMPLES:
        ram = (directory / name).read_bytes()
        check(len(ram) == 655360 and sha(ram) == expected_hash, name + " hash/size")
        if name.startswith("bp-script-07-"):
            relocated = ram[(0x3582 << 4) + 1:(0x3582 << 4) + 0x00F4]
            check(len(relocated) == 243 and sha(relocated) == RELOCATED_SHA,
                  "relocated EXIT span hash/size")
        check(word(ram, 0x49ED) == expected_pc, name + " PC")
        check(byte(ram, 0x442E) == expected_stop, name + " stop")
        check(byte(ram, 0x84E2) == 0, name + " box gate")
        check(byte(ram, 0x4430) == 0, name + " character gate")
        check(word(ram, 0x6F70) == 0 and word(ram, 0x6F72) == 0, name + " nil list")
        check(byte(ram, 0x6F7E) == 0x1A and byte(ram, 0x6F7F) == 0, name + " latch")
        check(word(ram, 0x49D2) == 0 and word(ram, 0x49D4) == 0x3996, name + " window pointer")
        w0 = (word(ram, 0x49D4) << 4) + word(ram, 0x49D2)
        w0word = lambda offset: int.from_bytes(ram[w0 + offset:w0 + offset + 2], "little")
        check(w0word(0x1CC) == 1 and w0word(0x18E) == 1 and
              w0word(0x190) == 5 and w0word(0x192) == 10, name + " window words")
        check(byte(ram, 0x49FF) == 0 and byte(ram, 0x5E24) == 1 and
              byte(ram, 0x5E25) == 0x11, name + " run state")
        check(all(byte(ram, off) == 0 for off in (0x84C8, 0x84C9, 0x84E0, 0x84DE)),
              name + " cleared bytes")
        print(f"{name}: sha256={expected_hash}, PC={expected_pc:04X}, stop={expected_stop}")
    print("four hash-locked native EXIT states and 243-byte original span match")


if __name__ == "__main__":
    main()
