#!/usr/bin/env python3
"""Hash-locked saved-A second-flush parity, without publishing game bytes."""

import argparse
from hashlib import sha256
from pathlib import Path
import re
import subprocess

from slot_a_ega_row56_transfer_audit import (
    FLUSH_SHA, MOVE_SHA, START_SHA, decoded, instruction, pinned, require,
    word,
)

TRANSCRIPT_SHA = "e2e37a39dc8bd2feef3e57c8446d4452602c1b5c99e3eabb2f0f1b0c0bd4da30"
LOOP_RAM_SHA = "c023d5e38f31a6a23faf0ceb278730b67464d66394b955cbbb2f9a7d2cc437d2"
RAM_HASHES = {
    "ram-flush2-entry.bin": "bb6b86553477f8185a12153d2901ebafc3b907b32c436c6b01f1cb51e3b12324",
    "ram-flush2-return.bin": "227c649d491add1df356e405c5c6c0c635541f9ad91dad4a7c695fc76fecd95a",
}
PLANE_HASHES = {
    "a200": (
        "ad9c8e641876f72754e3643a76cbc1eb641894fda15243cfa96997560eaba24f",
        "129fe74af8bcdaf28690ffd4311c652c7c587ef2f19772521abe33770ccb950c",
        "ac0461dd29bc61ef21c6d213d6d4415320415d0a39e0390d8303172a9f7d66bc",
        "9b1c03828ca58947ba28656dd81283e18e9a85e2ff06d3c2e672010ca1b3de54",
    ),
    "a000": (
        "a6a54c38fd257fa38a9171752346724dc87f24faeb9c2e5089338cdb5841c9eb",
        "343503de98879ca9dbcfd38de62d7e35d8007c8768d79b0db59c9dc3363bc417",
        "f18d65b18514e32aebbbbab9e9b5b00b0bd59c767eb29183ed09d4255e37df0d",
        "a08fb3ca1c19b261d03ac82b3aaa36832b083fe44b8ab47b6681298ccf6aeb02",
    ),
}


def trace_frame(run: Path) -> None:
    transcript = pinned(run / "debugger-interactive.typescript",
                        TRANSCRIPT_SHA).decode("utf-8", errors="replace")
    events = re.findall(
        r"LOG: EV of '([A-Z]+)' is:\s*LOG: ([0-9a-fA-F]+)", transcript)
    starts = [i for i in range(len(events) - 1)
              if events[i][0] == "CS" and events[i + 1][0] == "IP"]
    frames = []
    for ordinal, start in enumerate(starts):
        stop = starts[ordinal + 1] if ordinal + 1 < len(starts) else len(events)
        frames.append({name: int(value, 16) for name, value in
                       events[start:stop]})
    require([(frame["CS"], frame["IP"]) for frame in frames] ==
            [(0x0C88, 0x1B52), (0x0C88, 0x1B52),
             (0x2D14, 0x05FB), (0x2D14, 0x05FB)],
            "first/second flush and immediate-return stop order")
    require([frame["SP"] for frame in frames] ==
            [0x3E94, 0x3EBE, 0x3EC2, 0x3EC2] and
            all(frame["DS"] == 0x14A0 and frame["SS"] == 0x1D16
                for frame in frames), "saved first-view stack/data frames")
    gc = [int(value, 16) for value in
          re.findall(r"LOG: Result: ([0-9a-fA-F]+)", transcript)]
    require(gc == [0, 0, 0, 0, 0, 0xFF],
            "second-flush entry graphics-controller inputs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--run", required=True, type=Path)
    parser.add_argument("--loop-run", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    args = parser.parse_args()

    image = pinned(args.exe, START_SHA)
    flush = decoded(image, 0x61CB, 0x64D5, 0x1B52, FLUSH_SHA, 305)
    move = decoded(image, 0xC2FB, 0xC31E, 0x1685, MOVE_SHA, 16)
    for ip in (0x1C37, 0x1C7E, 0x1CC6, 0x1D0F):
        instruction(flush, ip, "mov", "byte ptr es:[di], dl")
    instruction(flush, 0x1DA0, "lcall", "0xaf8, 0x1685")
    instruction(move, 0x16A1, "rep movsb",
                "byte ptr es:[di], byte ptr [si]")
    trace_frame(args.run)

    pre = pinned(args.run / "ram-flush2-entry.bin",
                 RAM_HASHES["ram-flush2-entry.bin"])
    post = pinned(args.run / "ram-flush2-return.bin",
                  RAM_HASHES["ram-flush2-return.bin"])
    loop = pinned(args.loop_run / "ram-c3d-next.bin", LOOP_RAM_SHA)
    require(len(pre) == len(post) == len(loop) == 0xA0000, "RAM extent")
    ss, ds = 0x1D160, 0x14A00
    require((word(pre, ss + 0x3EBE), word(pre, ss + 0x3EC0),
             word(pre, ss + 0x3EC4), word(pre, ss + 0x3EC6)) ==
            (0x05FB, 0x2D14, 0x0DAE, 0x2D14),
            "second flush return/outer caller frame")
    require(pre[ds + 0x4A1E] == 1 and
            (pre[ds + 0x4A1A], pre[ds + 0x4A1B],
             word(pre, ds + 0x4A16), word(pre, ds + 0x4A18)) ==
            (0, 0, 0xA000, 0xA000), "video-state and selected page")
    require(tuple(pre[ds + 0x25F6:ds + 0x25FA]) == (1, 2, 4, 8),
            "native four-plane map masks")
    object_at = word(pre, ds + 0x4A20) + 16 * word(pre, ds + 0x4A22)
    require(object_at == 0x3DE48 and
            sha256(pre[object_at:object_at + 14135]).hexdigest() ==
            "489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1" and
            pre[object_at:object_at + 14135] ==
            post[object_at:object_at + 14135], "second-flush offscreen object")
    writer_at = object_at + 0x17 + 0x127E
    require(writer_at == 0x3F0DD and loop[writer_at] == pre[writer_at],
            "previous run's measured writer byte remains at flush entry")

    rows = []
    for y in range(168):
        if pre[ds + 0x4A28 + y]:
            first = word(pre, ds + 0x4C20 + 2 * y)
            last = word(pre, ds + 0x4D70 + 2 * y)
            low = word(pre, ds + 0x4AD0 + 2 * y)
            stage = word(pre, ds + 0x5050 + 2 * (y + 8)) + low + 1
            page = word(pre, ds + 0x5060) + 1 + 40 * y + low
            require(first <= last and first % 4 == 0 and
                    (last - first) % 4 == 3 and low == 2 and stage == page,
                    f"dirty row {y} bounds")
            rows.append((y, first, last, stage, (last - first) // 4 + 1))
        require(post[ds + 0x4A28 + y] == 0 and
                word(post, ds + 0x4C20 + 2 * y) == 0xFFFF and
                word(post, ds + 0x4D70 + 2 * y) == 0,
                f"reset metadata row {y}")
    require([row[0] for row in rows] == list(range(16, 104)) and
            all(row[4] == 11 for row in rows) and
            (rows[0][3], rows[-1][3] + rows[-1][4]) ==
            (0x03C3, 0x1166), "88 dirty rows and captured union")
    y, first, _, stage, _ = rows[56 - 16]
    target = stage + (0x127E - first - 2) // 4
    require(y == 56 and first == 4712 and target == 0x0A08,
            "measured writer byte's plane-2 display index")

    size = 0x0DA3
    captures = {}
    for segment in ("a200", "a000"):
        for plane, digest in enumerate(PLANE_HASHES[segment]):
            for phase in ("pre", "post"):
                name = f"vram-{segment}-plane{plane}-{phase}.bin"
                captures[name] = pinned(args.run / name, digest)
                require(len(captures[name]) == size, "EGA window extent")
    target_index = target - 0x03C3
    require(all(captures[f"vram-{segment}-plane2-{phase}.bin"][target_index]
                == pre[writer_at]
                for segment in ("a200", "a000")
                for phase in ("pre", "post")),
            "writer byte already present in staging/page at entry")

    paths = [args.run / f"vram-{segment}-plane{plane}-pre.bin"
             for segment in ("a200", "a000") for plane in range(4)]
    result = subprocess.run(
        [str(args.probe), str(args.run / "ram-flush2-entry.bin"),
         "14A0", "00", "00", "00", "00", "FF",
         *(str(path) for path in paths)],
        capture_output=True, check=False)
    require(result.returncode == 0, "bounded C probe failed")
    require(len(result.stdout) == 8 * size + 5 * 168,
            "bounded C probe output extent")
    predicted = native = 0
    source_stage = stage_page = 0
    for segment_index, segment in enumerate(("a200", "a000")):
        for plane in range(4):
            before = captures[f"vram-{segment}-plane{plane}-pre.bin"]
            after = captures[f"vram-{segment}-plane{plane}-post.bin"]
            part = result.stdout[(segment_index * 4 + plane) * size:
                                 (segment_index * 4 + plane + 1) * size]
            predicted += sum(a == b for a, b in zip(part, after))
            native += sum(a != b for a, b in zip(before, after))
            for _, first, _, offset, count in rows:
                for column in range(count):
                    index = offset - 0x03C3 + column
                    if segment == "a200":
                        source_stage += (
                            pre[object_at + 0x17 + first + plane + 4 * column]
                            == after[index])
                    else:
                        stage_after = captures[f"vram-a200-plane{plane}-post.bin"]
                        stage_page += stage_after[index] == after[index]
    metadata = result.stdout[8 * size:]
    metadata_matches = sum(
        metadata[5 * y:5 * y + 5] ==
        bytes([post[ds + 0x4A28 + y]]) +
        post[ds + 0x4C20 + 2 * y:ds + 0x4C22 + 2 * y] +
        post[ds + 0x4D70 + 2 * y:ds + 0x4D72 + 2 * y]
        for y in range(168))
    require((predicted, native, source_stage, stage_page,
             metadata_matches) == (8 * size, 0, 3872, 3872, 168),
            "second-flush C/native plane and metadata parity")
    print("RES_0464:1B52 second saved-A flush: 88 dirty rows, "
          "0 changed plane bytes in [0x03C3,0x1166); "
          "C/native 27928/27928 plane bytes, 168/168 metadata rows")
    print("Prior +0C3D writer byte maps to plane 2 at A000:0A08 "
          "and is already present at flush entry; no full-frame claim")


if __name__ == "__main__":
    main()
