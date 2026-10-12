#!/usr/bin/env python3
"""Hash-lock ignored u6f source and compare numeric saved/live fields."""
import hashlib
from pathlib import Path
import sys

SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "0e4be4f81f729cb89cadda3cecdc6bc16084a666cc4fbee1e1e4a61e23a9fed8"
RAM_SHA = "4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def check(ok: bool, label: str) -> None:
    if not ok:
        raise AssertionError(label)


def main() -> None:
    if len(sys.argv) != 4:
        raise SystemExit("usage: audit_slot_a_post_transfer.py IGNORED_SAVE IGNORED_GAME.OVR IGNORED_PRECALL_RAM")
    save, game, ram = (Path(p).read_bytes() for p in sys.argv[1:])
    check(len(save) == 13137 and sha(save) == SAVE_SHA, "save hash/size")
    check(len(game) == 206105 and sha(game) == GAME_SHA, "GAME.OVR hash/size")
    span = game[0x1F8D0:0x1F904]
    check(len(span) == 52 and sha(span) == SPAN_SHA, "u6f post-transfer span hash/size")
    check(len(ram) == 655360 and sha(ram) == RAM_SHA, "pre-call RAM hash/size")
    w0 = save[0x0001:0x0801]
    w1 = save[0x0801:0x1001]
    word = int.from_bytes(w0[0x1FE:0x200], "little")
    check(word == 3 and w0[0x1F8] == 4 and int.from_bytes(w1[0x67C:0x67E], "little") == 6,
          "saved source fields")
    ds = 0x14A0 << 4
    check((ram[ds + 0x49F6], ram[ds + 0x49F5], ram[ds + 0x49E2]) ==
          ((word >> 1) & 0xFF, word & 1, w0[0x1F8]), "native post-load DS bytes")
    print("u6f +18EA..+191D: 52-byte source and save/live DS 1/1/4 match; later party count remains separate")


if __name__ == "__main__":
    main()
