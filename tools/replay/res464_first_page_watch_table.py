#!/usr/bin/env python3
"""Check address-only first-row A200-to-A000 watch targets from old captures."""

from __future__ import annotations

import argparse
import csv
from hashlib import sha256
from io import StringIO
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-view4-last6-compose-p5-001"
EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
MAP = ROOT / "docs/re/res464_first_page_watch_addresses.csv"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
POST_STAGE_SHA = "83acb4eddfdaa0e6323c621a26c691bcef55e6ed7412e3cc8c3a678fbaf77f9a"
MOVE_SHA = "1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972"
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


def pinned(path: Path, digest: str, size: int | None = None) -> bytes:
    data = path.read_bytes()
    if (size is not None and len(data) != size) or sha256(data).hexdigest() != digest:
        raise ValueError(f"private input changed: {path.name}")
    return data


def word(data: bytes, at: int) -> int:
    return int.from_bytes(data[at:at + 2], "little")


def render() -> tuple[str, tuple[int, ...]]:
    exe = pinned(EXE, START_SHA)
    if (sha256(exe[0x63A0:0x642E]).hexdigest() != POST_STAGE_SHA or
            sha256(exe[0xC2FB:0xC31E]).hexdigest() != MOVE_SHA):
        raise ValueError("original post-stage or runtime Move span changed")
    entry = pinned(RUN / "ram-flush-entry.bin", RAM_HASHES["ram-flush-entry.bin"],
                   0xA0000)
    pinned(RUN / "ram-flush-return.bin", RAM_HASHES["ram-flush-return.bin"],
           0xA0000)
    ds = 0x14A00
    row = 16
    if (entry[ds + 0x4A1E] != 1 or
            (word(entry, ds + 0x4A16), word(entry, ds + 0x4A18)) !=
            (0xA000, 0xA000) or
            any(entry[ds + 0x4A28 + i] for i in range(row)) or
            entry[ds + 0x4A28 + row] == 0):
        raise ValueError("first dirty row or video state changed")
    first = word(entry, ds + 0x4C20 + 2 * row)
    last = word(entry, ds + 0x4D70 + 2 * row)
    low = word(entry, ds + 0x4AD0 + 2 * row)
    stage = word(entry, ds + 0x5050 + 2 * (row + 8)) + low + 1
    page = word(entry, ds + 0x5060) + 1 + row * 40 + low
    count = ((last - first) >> 2) + 1
    if (first, last, low, stage, page, count) != (
            1352, 1395, 2, 0x03C3, 0x03C3, 11):
        raise ValueError("first dirty-row source/page extent changed")

    planes = {name: pinned(RUN / name, digest, 8192)
              for name, digest in PLANE_HASHES.items()}
    output = StringIO()
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(("dirty_row", "byte_index", "plane", "source",
                     "destination", "stage_changed_by_return",
                     "page_changed_by_return"))
    changed = [0, 0, 0, 0]
    for index in range(count):
        at = stage + index
        for plane in range(4):
            source_pre = planes[f"a200-plane{plane}-pre.bin"][at]
            source_post = planes[f"a200-plane{plane}-post.bin"][at]
            page_pre = planes[f"a000-plane{plane}-pre.bin"][at]
            page_post = planes[f"a000-plane{plane}-post.bin"][at]
            if source_post != page_post:
                raise ValueError("post-staging/page first-row mismatch")
            stage_changed = int(source_pre != source_post)
            page_changed = int(page_pre != page_post)
            if stage_changed != page_changed:
                raise ValueError("first-row stage/page change flags differ")
            changed[plane] += page_changed
            writer.writerow((row, index, plane, f"A200:{at:04X}",
                             f"A000:{page + index:04X}",
                             stage_changed, page_changed))
    if tuple(changed) != (11, 11, 0, 11):
        raise ValueError("first-row plane change counts differ")
    return output.getvalue(), tuple(changed)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emit-csv", action="store_true")
    args = parser.parse_args()
    csv_text, changed = render()
    if args.emit_csv:
        print(csv_text, end="")
        return
    if MAP.read_text(encoding="ascii") != csv_text:
        raise ValueError("checked-in address table differs")
    print("START spans, two RAM stops, sixteen plane captures hash-locked")
    print("first dirty row: 11 offsets x 4 planes; stage/page addresses paired")
    print("entry-to-return net changes by plane:", "/".join(map(str, changed)))
    print("address-only CSV SHA-256", sha256(csv_text.encode("ascii")).hexdigest())


if __name__ == "__main__":
    main()
