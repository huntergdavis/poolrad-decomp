#!/usr/bin/env python3
"""Check modeled first-flush EGA indices against one paired private DOS view."""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess

from PIL import Image
from slot_a_first_ega_indexed_byte_audit import (
    EGA_RGB, FLUSH_SHA, OVR_SHA, PRIVATE, START_SHA, check, pinned, word,
)

MOVE_SHA = "1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972"


RAM_HASHES = {
    "ram-flush-entry.bin": "226acf93f958f039e488692b28ec608eb8ef5d454f11ffa66779f1cbe9cce3a4",
    "ram-flush-return.bin": "144e32c8c8842a0e7f897f4c2e9bdc193fd0ec8a66f623227c77c01384246cbd",
    "ram-view-return.bin": "9277ad6edbd01972ad237bfb9672465c238998627ff1304f840f437eb528b34a",
    "view-return-private.png": "6f6b65475848ab63e35bc4fb55549c04a5957e8007cd6633fe1d4be377e61b7e",
    "gc-flush-entry.txt": "dbcd522682e32bc73ef8cb22d93140b9811f93aa6920d0d5feddafcb85585d73",
    "second-flush-dirty-flags-private.bin": "148adce341b0db5cf45c92f8e8c0f1fda11884eafe67b14c079b6a7b4fc6df28",
}
PLANE_HASHES = {
    ("a000", "pre"): (
        "36a26ecce01fa5b0397415be067d3d275359a48fce772eb50a916862e0bec1d6",
        "36a26ecce01fa5b0397415be067d3d275359a48fce772eb50a916862e0bec1d6",
        "6ffeaed1a1668a11e686a27690ceaee74fa0ce3a9d94431cacb6831b3be0a7d8",
        "661189324a78f2b972adbdde2fd6a293a8bb8731515a15b4d63eeb675d15bbf8",
    ),
    ("a000", "post"): (
        "d814a5546a0b639d0c938bf8329dedb24ec51fb58c9575ea8a77b40ef9dc542b",
        "70f07b00774fe17f46902e98dd97ace2e65fc3db2f646a7f787c8b31b17b80e4",
        "856d9c6df1956d1f341382133ecce008235a2ac9ad20410a4209cc9ec54574dd",
        "1bccbe447dfff5abd872cc3c3d84f05ef15ef941970879f6c5e8a2cf3daf10ac",
    ),
    ("a000", "view"): (
        "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
        "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37",
    ),
    ("a200", "pre"): (
        "36a26ecce01fa5b0397415be067d3d275359a48fce772eb50a916862e0bec1d6",
        "36a26ecce01fa5b0397415be067d3d275359a48fce772eb50a916862e0bec1d6",
        "36a26ecce01fa5b0397415be067d3d275359a48fce772eb50a916862e0bec1d6",
        "36a26ecce01fa5b0397415be067d3d275359a48fce772eb50a916862e0bec1d6",
    ),
    ("a200", "post"): (
        "d814a5546a0b639d0c938bf8329dedb24ec51fb58c9575ea8a77b40ef9dc542b",
        "70f07b00774fe17f46902e98dd97ace2e65fc3db2f646a7f787c8b31b17b80e4",
        "ed96517fd545cac2488dff458cde4e1bc522e0b4deb8671f0aaaf10e33c2922d",
        "92e52d64611a0e696cb0d6e238d8d70c2b8bcf18d8ec9a67e14b844fbfdf7ac8",
    ),
}


def native_index(planes: tuple[bytes, ...], offset: int, x: int) -> int:
    bit = 7 - (x & 7)
    return sum(((plane[offset] >> bit) & 1) << lane
               for lane, plane in enumerate(planes))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--ovr", required=True, type=Path)
    parser.add_argument("--run", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    args = parser.parse_args()
    run = args.run.resolve(strict=True)
    probe = args.probe.resolve(strict=True)
    check(run.is_relative_to(PRIVATE) and probe.is_relative_to(PRIVATE),
          "private run/probe outside ignored resources")
    start = pinned(args.exe, START_SHA)
    pinned(args.ovr, OVR_SHA)
    check(sha256(start[0x61CB:0x64D5]).hexdigest() == FLUSH_SHA and
          sha256(start[0xC2FB:0xC31E]).hexdigest() == MOVE_SHA,
          "original flush/Move hashes")
    captured = {name: pinned(run / name, digest)
                for name, digest in RAM_HASHES.items()}
    pre, post, view = (captured[f"ram-{phase}.bin"]
                       for phase in ("flush-entry", "flush-return", "view-return"))
    check(len(pre) == len(post) == len(view) == 0xA0000, "RAM image lengths")
    ds, ss = 0x14A00, 0x1D160
    check((word(pre, ss + 0x3E94), word(pre, ss + 0x3E96),
           word(pre, ss + 0x3E9A), word(pre, ss + 0x3E9C)) ==
          (0x05FB, 0x2D14, 0x0DAE, 0x2D14), "post-wall flush caller frame")
    check(pre[ds + 0x4A1E] == post[ds + 0x4A1E] == view[ds + 0x4A1E] == 1,
          "video-state-1 route")
    check((pre[ds + 0x4A1A], pre[ds + 0x4A1B],
           word(pre, ds + 0x4A16), word(pre, ds + 0x4A18)) ==
          (0, 0, 0xA000, 0xA000), "active/draw page 0")
    check(tuple(pre[ds + 0x25F6:ds + 0x25FA]) == (1, 2, 4, 8),
          "native map-mask table")
    check(word(pre, ds + 0x4A20) +
          16 * word(pre, ds + 0x4A22) == 0x3DE48,
          "offscreen object pointer")
    second_flags = captured["second-flush-dirty-flags-private.bin"]
    check(len(second_flags) == 168 and
          [row for row, flag in enumerate(second_flags) if flag] ==
          list(range(16, 104)), "later flush-entry dirty flags")
    for row in range(168):
        dirty = 16 <= row <= 103
        check(bool(pre[ds + 0x4A28 + row]) == dirty,
              f"dirty flag row {row}")
        check(post[ds + 0x4A28 + row] == 0 and
              word(post, ds + 0x4C20 + 2 * row) == 0xFFFF and
              word(post, ds + 0x4D70 + 2 * row) == 0,
              f"postflush metadata row {row}")
        if not dirty:
            continue
        first = 1352 + 84 * (row - 16)
        low = word(pre, ds + 0x4AD0 + 2 * row)
        stage = word(pre, ds + 0x5050 + 2 * (row + 8)) + low + 1
        page = word(pre, ds + 0x5060) + 1 + row * 40 + low
        check((word(pre, ds + 0x4C20 + 2 * row),
               word(pre, ds + 0x4D70 + 2 * row), low,
               stage, page) ==
              (first, first + 43, 2, 0x03C3 + 40 * (row - 16),
               0x03C3 + 40 * (row - 16)),
              f"native extent/display mapping row {row}")

    gc = captured["gc-flush-entry.txt"].decode("ascii").splitlines()
    check(gc[:4] == [
        "source=normalized heavy debugger output observed in this run",
        "stop=0C88:1B52", "return=2D14:05FB", "ds=14A0",
    ] and gc[-1] == "OUTP 3CE 05" and len(gc) == 17,
          "private live GC record")
    values = []
    for i, register in enumerate((0, 1, 3, 4, 5, 8)):
        check(gc[4 + 2 * i] == f"OUTP 3CE {register:02X}" and
              gc[5 + 2 * i].startswith("INP 3CF -> "),
              f"GC register {register}")
        values.append(int(gc[5 + 2 * i].split()[-1], 16))
    check(tuple(values) == (0, 0, 0, 0, 0, 0xFF), "live GC values")

    planes = {}
    for (segment, phase), hashes in PLANE_HASHES.items():
        planes[segment, phase] = tuple(
            pinned(run / f"vram-{segment}-plane{lane}-{phase}.bin", digest)
            for lane, digest in enumerate(hashes))
        check(all(len(plane) == 88 * 40 for plane in planes[segment, phase]),
              f"{segment} {phase} captured region length")
    object_ptr = 0x3DE48
    source_stage = stage_page = 0
    dirty_offsets = set()
    for row in range(16, 104):
        first = 1352 + 84 * (row - 16)
        at = 3 + 40 * (row - 16)
        for column in range(11):
            offset = at + column
            dirty_offsets.add(offset)
            for lane in range(4):
                source_stage += (
                    pre[object_ptr + 0x17 + first + lane + 4 * column] ==
                    planes["a200", "post"][lane][offset])
                stage_page += (planes["a200", "post"][lane][offset] ==
                               planes["a000", "post"][lane][offset])
    check(len(dirty_offsets) == 968 and source_stage == stage_page == 3872,
          "first-flush object/stage/page dirty plane bytes")
    for segment in ("a200", "a000"):
        check(all(planes[segment, "pre"][lane][offset] ==
                  planes[segment, "post"][lane][offset]
                  for lane in range(4) for offset in range(88 * 40)
                  if offset not in dirty_offsets),
              f"{segment} clean plane bytes retained through first flush")
    later_changed = {offset for offset in range(88 * 40)
                     if any(planes["a000", "post"][lane][offset] !=
                            planes["a000", "view"][lane][offset]
                            for lane in range(4))}
    later_plane_bytes = sum(
        planes["a000", "post"][lane][offset] !=
        planes["a000", "view"][lane][offset]
        for lane in range(4) for offset in range(88 * 40))
    check(not later_changed.intersection(dirty_offsets) and
          later_plane_bytes == 1404,
          "intervening view changes confined to clean page bytes")
    inputs = [run / f"vram-{segment}-plane{lane}-pre.bin"
              for segment in ("a200", "a000") for lane in range(4)]
    result = subprocess.run(
        [str(probe), str(run / "ram-flush-entry.bin"), "14A0",
         *(f"{values[i]:02X}" for i in (0, 1, 2, 4, 5)),
         *(str(path) for path in inputs)],
        capture_output=True, check=False)
    check(result.returncode == 0 and not result.stderr and
          len(result.stdout) == 88 * 320, "C composition probe result")
    predicted = result.stdout
    check(all(index < 16 for index in predicted), "C EGA index range")

    image = Image.open(run / "view-return-private.png").convert("RGB")
    check(image.size == (640, 400), "native view size")
    palette = {rgb: index for index, rgb in enumerate(EGA_RGB)}
    totals = {group: {key: 0 for key in (
        "pixels", "matched", "mismatched", "unknown", "nonuniform",
        "pre_post_changed", "post_view_changed", "native_post_matched",
        "native_view_matched")}
              for group in ("dirty", "clean")}
    for y in range(24, 112):
        for x in range(320):
            group = totals["dirty" if 24 <= x < 112 else "clean"]
            group["pixels"] += 1
            byte_offset = (y - 24) * 40 + x // 8
            pixel_offset = (y - 24) * 320 + x
            native_pre = native_index(planes["a000", "pre"], byte_offset, x)
            native_post = native_index(planes["a000", "post"], byte_offset, x)
            native_view = native_index(planes["a000", "view"], byte_offset, x)
            group["native_post_matched"] += predicted[pixel_offset] == native_post
            group["pre_post_changed"] += native_pre != native_post
            group["post_view_changed"] += native_post != native_view
            color = image.getpixel((2 * x, 2 * y))
            if any(image.getpixel((2 * x + dx, 2 * y + dy)) != color
                   for dx in (0, 1) for dy in (0, 1)):
                group["unknown"] += 1
                group["nonuniform"] += 1
            elif color not in palette:
                group["unknown"] += 1
            else:
                screen = palette[color]
                group["matched"] += predicted[pixel_offset] == screen
                group["mismatched"] += predicted[pixel_offset] != screen
                group["native_view_matched"] += native_view == screen
    dirty, clean = totals["dirty"], totals["clean"]
    check(tuple(dirty[key] for key in (
        "pixels", "matched", "mismatched", "unknown", "nonuniform",
        "pre_post_changed", "post_view_changed", "native_post_matched",
        "native_view_matched")) ==
          (7744, 7744, 0, 0, 0, 7667, 0, 7744, 7744),
          "first-flush dirty indexed parity")
    check(tuple(clean[key] for key in (
        "pixels", "matched", "mismatched", "unknown", "nonuniform",
        "pre_post_changed", "post_view_changed", "native_post_matched",
        "native_view_matched")) ==
          (20416, 18554, 1862, 0, 0, 0, 1862, 20416, 20416),
          "retained clean input and later native change accounting")
    for name, group in totals.items():
        print(f"{name}: " + " ".join(f"{key}={value}"
              for key, value in group.items()))
    print("same-run first-view native page/screen 28160/28160; "
          "first-flush C page/native post 28160/28160; "
          "first-flush C dirty/screen 7744/7744; unknown=0")
    print("first-flush object/stage 3872/3872, stage/page 3872/3872; "
          "later flush-entry dirty rows 16..103")
    print("first-flush clean inputs stay unchanged at postflush; "
          "1404 clean plane bytes / 1862 clean pixels change before "
          "the first-view return")


if __name__ == "__main__":
    main()
