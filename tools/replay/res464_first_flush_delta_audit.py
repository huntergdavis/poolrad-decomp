#!/usr/bin/env python3
"""Hash-locked saved-A first-flush plane delta; print addresses/counts only."""

import argparse
from hashlib import sha256
from pathlib import Path


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_HASHES = {
    "ram-flush-entry.bin": "48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7",
    "ram-flush-return.bin": "db0fc49dad03cc5a45ececba4017f2eddf51d608f1c5993608334d63f777085f",
}
PLANE_HASHES = {
    "a200-plane0-pre.bin": "9f1dcbc35c350d6027f98be0f5c8b43b42ca52b7604459c0c42be3aa88913d47",
    "a200-plane0-post.bin": "82250cc0d633b5433774d5bc0f599862f7e001ca1678c1c6160a8ce4bf349103",
    "a200-plane1-pre.bin": "9f1dcbc35c350d6027f98be0f5c8b43b42ca52b7604459c0c42be3aa88913d47",
    "a200-plane1-post.bin": "b02181363785115b0b6f83d1abf1fa5f31978d10f683f24c4732d3cd383c109f",
    "a200-plane2-pre.bin": "9f1dcbc35c350d6027f98be0f5c8b43b42ca52b7604459c0c42be3aa88913d47",
    "a200-plane2-post.bin": "1e1c9a9fcfeccbe9b79a33de8c00192a20c1868ee7dfd2b953d672b2b324e355",
    "a200-plane3-pre.bin": "9f1dcbc35c350d6027f98be0f5c8b43b42ca52b7604459c0c42be3aa88913d47",
    "a200-plane3-post.bin": "41c647bc7f2fb8b76278413779fa97cb516e378ec9a00a0cecb1381684a01a70",
    "a000-plane0-pre.bin": "1042758204ae5c5fa191406467b036d879cc24a50b594827e8ba4a0c24a06dbc",
    "a000-plane0-post.bin": "5f88a817b8069d5c61544ddf086e59cb70912bcf465ee88b1c3588f6a3a18922",
    "a000-plane1-pre.bin": "0f4283b7b82cb83134dd3267257eff1e6c7eabadf38d6086e42b967ad8fcb38e",
    "a000-plane1-post.bin": "5abd0baeb49a38e92ebbcc11b95452202420dc57b2dc53f90fe46657a8f64704",
    "a000-plane2-pre.bin": "4c2e233a292d8a9077f78b83ef3c50f72e326d17006b29872c5a8b5708634838",
    "a000-plane2-post.bin": "c96df35307143185145de04add952f592c309116ad1e3b905eb2877689d20005",
    "a000-plane3-pre.bin": "1414fdbc9b4069b31e4362d1dae60d1e1c5aa8cdbf2f6bfe42cce5f40354f071",
    "a000-plane3-post.bin": "d45e15b59947a4c70b8c18ae426268c989650ab2826d396db1319cb0270898e1",
}


def pinned(path: Path, expected: str, size: int) -> bytes:
    data = path.read_bytes()
    if len(data) != size or sha256(data).hexdigest() != expected:
        raise ValueError(f"private capture mismatch: {path.name}")
    return data


def word(data: bytes, at: int) -> int:
    return data[at] | data[at + 1] << 8


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    args = parser.parse_args()
    pinned(args.exe, START_SHA, args.exe.stat().st_size)
    ram = {name: pinned(args.run / name, digest, 0xA0000)
           for name, digest in RAM_HASHES.items()}
    entry = ram["ram-flush-entry.bin"]
    ds = 0x14A00
    if (entry[ds + 0x4A1E] != 1 or
            (word(entry, ds + 0x4A16), word(entry, ds + 0x4A18)) !=
            (0xA000, 0xA000)):
        raise ValueError("saved video/page state mismatch")
    if [row for row in range(168) if entry[ds + 0x4A28 + row]] != list(range(16, 104)):
        raise ValueError("dirty-row set mismatch")

    capture = {name: pinned(args.run / name, digest, 8192)
               for name, digest in PLANE_HASHES.items()}
    expected = {0x03C3 + 40 * row + col
                for row in range(88) for col in range(11)}
    if len(expected) != 968 or max(expected) >= 8000:
        raise ValueError("dirty page extents mismatch")
    for segment in ("a200", "a000"):
        changed = []
        for plane in range(4):
            before = capture[f"{segment}-plane{plane}-pre.bin"]
            after = capture[f"{segment}-plane{plane}-post.bin"]
            changed.append({i for i, (old, new) in enumerate(zip(before, after))
                            if old != new})
        if [len(part) for part in changed] != [772, 968, 768, 277]:
            raise ValueError(f"{segment} plane change counts mismatch")
        if [min(part) for part in changed] != [0x03C3, 0x03C3, 0x03EB, 0x03C3]:
            raise ValueError(f"{segment} first change offsets mismatch")
        if set().union(*changed) != expected:
            raise ValueError(f"{segment} changes outside dirty spans")
        print(f"{segment.upper()} first changed offsets by plane: "
              "03C3/03C3/03EB/03C3; changed bytes 772/968/768/277")

    stage = [capture[f"a200-plane{plane}-post.bin"] for plane in range(4)]
    before = [capture[f"a000-plane{plane}-pre.bin"] for plane in range(4)]
    after = [capture[f"a000-plane{plane}-post.bin"] for plane in range(4)]
    if any(stage[p][at] != after[p][at] for p in range(4) for at in expected):
        raise ValueError("post-stage/page dirty-byte mismatch")
    changed_pixels = 0
    first_row_pixels = 0
    first = None
    for row in range(88):
        for column in range(11):
            at = 0x03C3 + 40 * row + column
            for shift in range(7, -1, -1):
                before_index = sum(((before[p][at] >> shift) & 1) << p
                                   for p in range(4))
                after_index = sum(((after[p][at] >> shift) & 1) << p
                                  for p in range(4))
                if before_index != after_index:
                    changed_pixels += 1
                    if row == 0:
                        first_row_pixels += 1
                    if first is None:
                        first = (at, 24 + row, 24 + 8 * column + 7 - shift)
    if first != (0x03C3, 24, 24) or changed_pixels != 7667 or first_row_pixels != 88:
        raise ValueError("first indexed-pixel delta mismatch")
    print("A000 first changed logical pixel: byte 03C3, y=24, x=24")
    print("post-stage/page dirty plane-byte agreement: 3872/3872")
    print("first dirty row: 88/88 indices changed; all dirty spans: 7667/7744")
    print("RGB at this immediate flush remains unmeasured")


if __name__ == "__main__":
    main()
