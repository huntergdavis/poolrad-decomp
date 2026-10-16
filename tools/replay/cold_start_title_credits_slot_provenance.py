#!/usr/bin/env python3
"""Check saved title/credits slot provenance without replaying the game."""

from __future__ import annotations

import argparse
from hashlib import md5, sha256
import json
from pathlib import Path
import re

from PIL import Image

GROUPS_SHA = "e3c2c3532f2ea068905b173860d701afef71f48fd1a2e561cd1b012ef695ef33"
CFR_SHA = "30c7d30caa8c7e6f8a33e9196c65c18777098a2cdb39b3cac953139b6c8f80d7"
PASSTHROUGH_SHA = "ef6db3cf4616fe6c57ce0cbbe22e7cab082a74ec77a54714fec69c58da837dff"
TITLE_PNG_SHA = "cea3d2c06f84c4c2dffaae493442649fe00bbe2bdb8e49324d46a883fda0bb99"
CREDITS_PNG_SHA = "5d434e35fdfcbdc62970ed61d365763497be64234ed98c491b3dcb72cd33f295"
TITLE_RGB_SHA = "3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8"
CREDITS_RGB_SHA = "624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698"
SKIPPED_PTS = (279, 280, 593, 962, 1582)
TITLE = (359, 433, 75, TITLE_RGB_SHA)
CREDITS = (443, 595, 153, CREDITS_RGB_SHA)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def pinned(path: Path, expected_sha: str) -> bytes:
    raw = path.read_bytes()
    require(sha256(raw).hexdigest() == expected_sha,
            f"saved artifact hash differs: {path.name}")
    return raw


def frame_records(raw: bytes, label: str) -> dict[int, str]:
    lines = raw.decode("ascii").splitlines()
    require("#tb 0: 1/15" in lines and
            "#dimensions 0: 640x400" in lines and
            "#hash: MD5" in lines, f"{label} metadata header differs")
    records = {}
    for line in lines:
        if not line or line.startswith("#"):
            continue
        fields = [part.strip() for part in line.split(",")]
        require(len(fields) == 6, f"{label} record shape differs")
        stream, dts, pts, duration, size = map(int, fields[:5])
        digest = fields[5]
        require(stream == 0 and dts == pts and duration == 1 and
                size == 640 * 400 * 3 and
                re.fullmatch(r"[0-9a-f]{32}", digest) is not None and
                pts not in records, f"{label} record differs")
        records[pts] = digest
    return records


def check_groups(raw: bytes) -> list[dict[str, object]]:
    groups = json.loads(raw)
    require(isinstance(groups, list) and len(groups) == 89,
            "saved frame-group count differs")
    next_slot = 0
    for group in groups:
        require(isinstance(group, dict) and
                set(group) == {"start", "end", "count", "sha256"},
                "frame-group shape differs")
        start, end, count, digest = (group[key] for key in
                                     ("start", "end", "count", "sha256"))
        require(type(start) is int and type(end) is int and
                type(count) is int and start == next_slot and
                0 < count == end - start + 1 and
                isinstance(digest, str) and
                re.fullmatch(r"[0-9a-f]{64}", digest) is not None,
                "frame-group continuity differs")
        next_slot = end + 1
    require(next_slot == 1830, "CFR group coverage differs")
    return groups


def saved_rgb(path: Path, png_sha: str, rgb_sha: str) -> str:
    pinned(path, png_sha)
    with Image.open(path) as image:
        require(image.mode == "RGB" and image.size == (640, 400),
                "saved stable PNG shape differs")
        pixels = image.tobytes()
    require(len(pixels) == 768000 and sha256(pixels).hexdigest() == rgb_sha,
            "saved stable PNG RGB hash differs")
    return md5(pixels).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True,
                        help="ignored native-cold-start-gap-004 run directory")
    run = parser.parse_args().run.resolve(strict=True)
    require(run.is_dir() and "resources" in run.parts,
            "saved run must stay under ignored resources/")

    groups = check_groups(pinned(run / "frame-groups.json", GROUPS_SHA))
    cfr = frame_records(pinned(run / "frame-md5-cfr.txt", CFR_SHA), "CFR")
    passthrough = frame_records(
        pinned(run / "frame-md5-passthrough.txt", PASSTHROUGH_SHA),
        "passthrough")
    require(set(cfr) == set(range(1830)) and len(passthrough) == 1825 and
            tuple(sorted(set(cfr) - set(passthrough))) == SKIPPED_PTS,
            "CFR/passthrough slot set differs")
    require(all(cfr[pts] == digest for pts, digest in passthrough.items()) and
            all(cfr[pts] == cfr[pts - 1] for pts in SKIPPED_PTS),
            "CFR duplicate or shared-PTS content differs")

    title_group = next((g for g in groups if g["start"] == TITLE[0]), None)
    credits_group = next((g for g in groups if g["start"] == CREDITS[0]), None)
    require(title_group is not None and credits_group is not None and
            tuple(title_group[key] for key in ("start", "end", "count", "sha256"))
            == TITLE and
            tuple(credits_group[key] for key in ("start", "end", "count", "sha256"))
            == CREDITS, "title/credits stable groups differ")
    gap = [g for g in groups if 434 <= g["start"] <= 442]
    require(len(gap) == 9 and
            [g["start"] for g in gap] == list(range(434, 443)) and
            all(g["end"] == g["start"] and g["count"] == 1 for g in gap) and
            len({g["sha256"] for g in gap}) == 9 and
            all(g["sha256"] not in (TITLE_RGB_SHA, CREDITS_RGB_SHA)
                for g in gap), "intervening singleton groups differ")
    require(len({cfr[pts] for pts in range(434, 443)}) == 9,
            "intervening frame-MD5 records differ")

    for name, group, png_sha, rgb_sha in (
        ("0359-0433.png", TITLE, TITLE_PNG_SHA, TITLE_RGB_SHA),
        ("0443-0595.png", CREDITS, CREDITS_PNG_SHA, CREDITS_RGB_SHA),
    ):
        digest = saved_rgb(run / "stable-frames" / name, png_sha, rgb_sha)
        require(all(cfr[slot] == digest for slot in range(group[0], group[1] + 1)),
                "saved PNG and stable-slot frame-MD5 differ")

    print("groups=89 cfr_slots=1830 passthrough_frames=1825 shared_pts_match=1825")
    print("title_slots=359..433 credits_slots=443..595 gap_singletons=9")
    print("skipped_pts=279,280,593,962,1582 duplicate_previous=5")
    print("saved_title_rgb_sha256=" + TITLE_RGB_SHA)
    print("saved_credits_rgb_sha256=" + CREDITS_RGB_SHA)


if __name__ == "__main__":
    main()
