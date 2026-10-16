#!/usr/bin/env python3
"""Pin U19 plane-capture source and existing inputs without starting native."""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess


SOURCE_COMMIT = "5a9c7c53481c2e933948039c33256776653f4faf"
HEAVY_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
FILES = {
    "src/debug/debug.cpp": (
        "2762251108e54aeec3474b9f495222ce09bec9f246dd0af53a4d10fa7bb2a38d",
        ((3327, 3336, "4a42b9a847ec186d71443ea530daf6a1f0d8287a8c8a17e875e8ada21c8b25f8"),
         (3375, 3384, "153e4a23edb61b14d841d5cc03731f4619d9371f842e48da91310fb0d0e684c6"),
         (3812, 3841, "ae2eee9af3e0f384feb141792e79c53ab8058d9af94b97d91df6e50960183182"),
         (5794, 5811, "636ec61547ed2f23811922e8c8bc50005d4eebd09df050d4a08088b1deb36f52")),
    ),
    "src/hardware/vga_gfx.cpp": (
        "7e29520e6118ddb635d7245d4dc192365bc6ebde9aba7c56c1f2a6f4d55a3520",
        ((98, 115, "652932b3092b8f732445fb14c9d548814a0c9694a85c8951d67631627f17d33f"),
         (218, 258, "1d48c0c3e302fce19f9e1cea6ac4d7ce76f4b2a6868d81d20dc1116088706ba9")),
    ),
    "src/hardware/vga_seq.cpp": (
        "72746cc41c882cb66bd78e82162fce907efed1f8a79761b52727b87d3d79c395",
        ((172, 181, "ef3fe3d90eacc0940cbaf783e92789fb5ea585215fcf90ea618f78523410a13e"),
         (240, 277, "38878099b7a9c110cf448d88f6fa893318310be48cc1c01ba72676974c558ceb")),
    ),
    "src/hardware/vga_memory.cpp": (
        "f506017948503fa22ce40715205293c987473a938247cbd09ba409ffbb652c14",
        ((145, 183, "e8afbb45e2e12a44b0b7c29d9ee6d7ce6349f24a9ffbe0f0241011471fdece35"),
         (282, 330, "96f25917754d8fa9ceaf3a994c89f7eb565e69c6ee3d62473747faa5310856eb"),
         (580, 594, "a5d1c8b980152a9381add18663001f28ce6f72cf23c7aaa82bfbf87ce0281795")),
    ),
    "include/dosbox.h": (
        "7439a8acca85151ec593745362fc1ca92ae6688e3922df6602ddc0dbe3d64bee",
        ((200, 207, "149782e5be9fc82ba826059a316c183b6e5d116cf570e9714e668dd51ddb9f61"),),
    ),
}
GAME_SHA = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
}
RETAINED_RAM = {
    "at-u19-02dd.ram": "84191adc6aab0f6272fe0dc128b1cbff06668a01c9c40fd5fc1202d490b06699",
    "at-u19-02e2.ram": "081901c405f2d67a3e5fa68a34bfa67a536f25b39c9641acfdcc7b5863d66f18",
    "at-u19-02fd.ram": "b4dc553331795b63fb6e8fb6678df59b0a72799f4c386ccdc21ce81b1ec892a3",
    "at-u19-0302.ram": "c35e6bf7920805fe864d5f31aa59b65fa1a321c9e2a6efadc29d1d931f40eeb5",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def digest(data: bytes) -> str:
    return sha256(data).hexdigest()


def audit(source: Path, heavy: Path, game: Path, run: Path) -> tuple[str, ...]:
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    require(commit == SOURCE_COMMIT, "private DOSBox-X revision differs")
    require(digest(heavy.read_bytes()) == HEAVY_SHA, "private DOSBox-X executable differs")
    for name, (full_sha, windows) in FILES.items():
        data = (source / name).read_bytes()
        require(digest(data) == full_sha, f"private DOSBox-X {name} differs")
        lines = data.splitlines(keepends=True)
        for first, last, expected in windows:
            require(digest(b"".join(lines[first - 1:last])) == expected,
                    f"private DOSBox-X {name}:{first}-{last} differs")
    for name, expected in GAME_SHA.items():
        require(digest((game / name).read_bytes()) == expected,
                f"ignored game input {name} differs")
    for name, expected in RETAINED_RAM.items():
        data = (run / name).read_bytes()
        require(len(data) == 655360 and digest(data) == expected,
                f"retained U19 checkpoint {name} differs")
    return tuple(name for name in ("route.json", "cpu-frames.json", "plane-events.json")
                 if not (run / name).is_file())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--heavy", type=Path, required=True)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    missing = audit(args.source, args.heavy, args.game, args.run)
    print("source lock: five files, twelve windows, one executable; PASS")
    print("ignored inputs: two game files, four retained post-call RAM checkpoints; PASS")
    print("same-run route/CPU/plane manifests absent=" + str(len(missing)))
    print("absent names=" + ",".join(missing) if missing else "manifest names present; provenance unproved")
    print("native writer, plane values, controller restoration and RGB timing unproved")


if __name__ == "__main__":
    main()
