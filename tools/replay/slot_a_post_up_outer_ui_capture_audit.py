#!/usr/bin/env python3
"""Check bounded post-Up outer UI captures without launching an emulator."""

import argparse
import hashlib
import subprocess
from pathlib import Path


PINS = {
    "exe": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "game": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "save": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
    "outer_ram": "b8d808d1469791646d57003438b094f0d9fee0459442021c3747a5635025bc8c",
    "printer_entry_ram": "5c3aa481884391318a14a9cfe4e76b1446027b72b368065df606c2a37c5c0c23",
    "printer_return_ram": "24b7c6335e01ec918c823935a52342278ddb81c3150e06d67d07b10dcc63d1e8",
    "probe_input": "53fd15433e954797e4b2ef165d7eac15c0accea3db040d956bfe9dedf2042318",
    "entry_page": "b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44",
    "return_page": "a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac",
}


def locked(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise ValueError(f"hash mismatch: {path}: {actual}")
    return data


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--save", type=Path, required=True)
    parser.add_argument("--outer-run", type=Path, required=True)
    parser.add_argument("--printer-run", type=Path, required=True)
    parser.add_argument("--repeat-run", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    args = parser.parse_args()

    locked(args.exe, PINS["exe"])
    locked(args.game, PINS["game"])
    locked(args.save, PINS["save"])
    outer = locked(args.outer_run / "post-up-0dae-ram-private.bin", PINS["outer_ram"])
    locked(args.printer_run / "first-printer-entry-private.ram", PINS["printer_entry_ram"])
    locked(args.printer_run / "first-printer-return-private.ram", PINS["printer_return_ram"])
    probe = locked(args.printer_run / "first-printer-probe-input-private.bin", PINS["probe_input"])
    entry = b"".join((args.printer_run / f"first-printer-entry-plane{n}-private.bin").read_bytes()
                     for n in range(4))
    returned = b"".join((args.printer_run / f"first-printer-return-plane{n}-private.bin").read_bytes()
                        for n in range(4))
    if hashlib.sha256(entry).hexdigest() != PINS["entry_page"]:
        raise ValueError("entry page hash mismatch")
    if hashlib.sha256(returned).hexdigest() != PINS["return_page"]:
        raise ValueError("return page hash mismatch")
    for mode, expected in (("entry", entry), ("return", returned)):
        repeat = b"".join((args.repeat_run / f"first-printer-{mode}-plane{n}-private.bin").read_bytes()
                          for n in range(4))
        if repeat != expected:
            raise ValueError(f"repeat first-printer {mode} page differs")
    if len(entry) != 32000 or len(returned) != 32000 or len(outer) != 655360:
        raise ValueError("capture length mismatch")
    if probe[1675:] != entry:
        raise ValueError("probe input page differs from native entry")
    counts = [sum(a != b for a, b in zip(entry[n * 8000:(n + 1) * 8000],
                                         returned[n * 8000:(n + 1) * 8000]))
              for n in range(4)]
    if counts != [0, 105, 0, 105]:
        raise ValueError(f"unexpected plane deltas: {counts}")
    subprocess.run([str(args.fixture), str(args.printer_run / "first-printer-probe-input-private.bin"),
                    str(args.printer_run)], check=True)
    print("outer +0DAE RAM pinned; first printer page deltas=0,105,0,105; route join=unknown")


if __name__ == "__main__":
    main()
