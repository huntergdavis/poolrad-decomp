#!/usr/bin/env python3
"""Read-only hash-locked parity for every reached post-wall dirty-row span."""

import argparse
from pathlib import Path
import subprocess

from slot_a_ega_row56_transfer_audit import (
    FLUSH_SHA, MOVE_SHA, START_SHA, decoded, instruction, pinned, require, word,
)


CAPTURE_HASHES = {
    "gc-flush-entry.txt": "4a474c34200525498a5a223630e9a858db3902bb713d4191c70b7214d5bd6ce5",
    "ram-flush-entry.bin": "25ae4cdbceb249e04ec413b49e6cbfc6f14ff4210378bbdd34e19d37389266d1",
    "ram-flush-return.bin": "c8efd7433d584cf25303861be3b2e6aec9bcf8b7144631196e3e24ff0712e959",
    "vram-a000-plane0-post.bin": "ad9c8e641876f72754e3643a76cbc1eb641894fda15243cfa96997560eaba24f",
    "vram-a000-plane0-pre.bin": "8be02be5cf40bd0801f9460f69aaed960238ee80ee63aed35ccb63c116964a1f",
    "vram-a000-plane1-post.bin": "129fe74af8bcdaf28690ffd4311c652c7c587ef2f19772521abe33770ccb950c",
    "vram-a000-plane1-pre.bin": "8be02be5cf40bd0801f9460f69aaed960238ee80ee63aed35ccb63c116964a1f",
    "vram-a000-plane2-post.bin": "83229d02ce6d7c912181e7b2e0fc6bc2a86a0ef73473964a6ee5e8087ebc0b2d",
    "vram-a000-plane2-pre.bin": "778545edd4de4c13405747184597305bee2a688307f9093c51fdeff5db2c1912",
    "vram-a000-plane3-post.bin": "654962481e0aa57cf448e31752068c3fe3d58cf80702c90c6fac41a0fa5f51da",
    "vram-a000-plane3-pre.bin": "86532fad4fe6a7ab1d52de513ef00561fcfe23013d8ccd0c554b3876d9757134",
    "vram-a200-plane0-post.bin": "ad9c8e641876f72754e3643a76cbc1eb641894fda15243cfa96997560eaba24f",
    "vram-a200-plane0-pre.bin": "8be02be5cf40bd0801f9460f69aaed960238ee80ee63aed35ccb63c116964a1f",
    "vram-a200-plane1-post.bin": "129fe74af8bcdaf28690ffd4311c652c7c587ef2f19772521abe33770ccb950c",
    "vram-a200-plane1-pre.bin": "8be02be5cf40bd0801f9460f69aaed960238ee80ee63aed35ccb63c116964a1f",
    "vram-a200-plane2-post.bin": "ac0461dd29bc61ef21c6d213d6d4415320415d0a39e0390d8303172a9f7d66bc",
    "vram-a200-plane2-pre.bin": "8be02be5cf40bd0801f9460f69aaed960238ee80ee63aed35ccb63c116964a1f",
    "vram-a200-plane3-post.bin": "9b1c03828ca58947ba28656dd81283e18e9a85e2ff06d3c2e672010ca1b3de54",
    "vram-a200-plane3-pre.bin": "8be02be5cf40bd0801f9460f69aaed960238ee80ee63aed35ccb63c116964a1f",
}


def gc_values(run: Path) -> tuple[int, ...]:
    lines = pinned(run / "gc-flush-entry.txt",
                   CAPTURE_HASHES["gc-flush-entry.txt"]).decode("ascii").splitlines()
    require(lines[:4] == [
        "source=heavy debugger output captured in this run",
        "stop=0C88:1B52", "return=2D14:05FB", "ds=14A0",
    ] and len(lines) == 17 and lines[-1] == "OUTP 3CE 05",
            "live GC observation provenance/frame")
    values = []
    for index, register in enumerate((0, 1, 3, 4, 5, 8)):
        require(lines[4 + 2 * index] == f"OUTP 3CE {register:02X}" and
                lines[5 + 2 * index].startswith("INP 3CF -> "),
                f"live GC register {register} command")
        values.append(int(lines[5 + 2 * index].split()[-1], 16))
    require(tuple(values) == (0, 0, 0, 0, 0, 0xFF),
            "live GC register values")
    return tuple(values)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--run", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    args = parser.parse_args()
    image = pinned(args.exe, START_SHA)
    flush = decoded(image, 0x61CB, 0x64D5, 0x1B52, FLUSH_SHA, 305)
    move = decoded(image, 0xC2FB, 0xC31E, 0x1685, MOVE_SHA, 16)
    for ip in (0x1C37, 0x1C7E, 0x1CC6, 0x1D0F):
        instruction(flush, ip, "mov", "byte ptr es:[di], dl")
    instruction(flush, 0x1D31, "mov", "al, 0xf")
    instruction(flush, 0x1D44, "mov", "al, 1")
    instruction(flush, 0x1DA0, "lcall", "0xaf8, 0x1685")
    instruction(move, 0x1695, "cld", "")
    instruction(move, 0x1696, "cmp", "si, di")
    instruction(move, 0x1698, "jae", "0x16a1")
    instruction(move, 0x16A1, "rep movsb",
                "byte ptr es:[di], byte ptr [si]")

    captured = {name: pinned(args.run / name, expected)
                for name, expected in CAPTURE_HASHES.items()
                if name != "gc-flush-entry.txt"}
    pre = captured["ram-flush-entry.bin"]
    post = captured["ram-flush-return.bin"]
    require(len(pre) == len(post) == 0xA0000, "RAM image length")
    gc = gc_values(args.run)
    ss, ds = 0x1D160, 0x14A00
    require((word(pre, ss + 0x3E94), word(pre, ss + 0x3E96),
             word(pre, ss + 0x3E9A), word(pre, ss + 0x3E9C)) ==
            (0x05FB, 0x2D14, 0x0DAE, 0x2D14), "post-wall caller frame")
    require(pre[ds + 0x4A1E] == 1 and
            (pre[ds + 0x4A1A], pre[ds + 0x4A1B],
             word(pre, ds + 0x4A16), word(pre, ds + 0x4A18)) ==
            (0, 0, 0xA000, 0xA000), "video-state/page gate")
    require(tuple(pre[ds + 0x25F6:ds + 0x25FA]) == (1, 2, 4, 8),
            "live map-mask table")
    ptr = word(pre, ds + 0x4A20) + 16 * word(pre, ds + 0x4A22)
    require(ptr == 0x3DE48, "offscreen object pointer")

    rows = []
    for y in range(168):
        if not pre[ds + 0x4A28 + y]:
            continue
        first = word(pre, ds + 0x4C20 + 2 * y)
        last = word(pre, ds + 0x4D70 + 2 * y)
        low = word(pre, ds + 0x4AD0 + 2 * y)
        stage = word(pre, ds + 0x5050 + 2 * (y + 8)) + low + 1
        page = word(pre, ds + 0x5060) + 1 + y * 40 + low
        require(first <= last and first % 4 == 0 and low == 2 and
                (last - first) % 4 == 3 and
                stage == page, f"unsupported reached row {y}")
        rows.append((y, first, last, stage, page, (last - first) // 4 + 1))
    require(len(rows) == 88 and rows[0][0] == 16 and
            rows[-1][0] == 103, "reached dirty-row set")
    require(all(b[0] == a[0] + 1 and b[1] == a[1] + 84 and
                b[2] == a[2] + 84 and b[3] == a[3] + 40 and
                a[2] - a[1] == 43 and a[5] == 11
                for a, b in zip(rows, rows[1:])) and
            rows[-1][2] - rows[-1][1] == 43 and rows[-1][5] == 11,
            "measured extent series for every dirty row")
    origin = min(row[3] for row in rows)
    end = max(row[3] + row[5] for row in rows)
    size = end - origin
    require((origin, end, size) == (0x03C3, 0x1166, 0x0DA3),
            "captured union range")
    dirty = {offset - origin + j
             for _, _, _, offset, _, length in rows
             for j in range(length)}
    require(len(dirty) == 968, "dirty byte-offset union")
    for y in range(168):
        require(post[ds + 0x4A28 + y] == 0 and
                word(post, ds + 0x4C20 + 2 * y) == 0xFFFF and
                word(post, ds + 0x4D70 + 2 * y) == 0,
                f"native metadata reset row {y}")

    inputs = [args.run / f"vram-{segment}-plane{plane}-pre.bin"
              for segment in ("a200", "a000") for plane in range(4)]
    process = subprocess.run(
        [str(args.probe), str(args.run / "ram-flush-entry.bin"), "14A0",
         *(f"{gc[index]:02X}" for index in (0, 1, 2, 4, 5)),
         *(str(path) for path in inputs)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    require(process.returncode == 0,
            f"C probe failed: {process.stderr.decode(errors='replace')}")
    require(len(process.stdout) == 8 * size + 5 * 168,
            "C probe output length")

    source_stage = stage_page = predicted_dirty = predicted_full = 0
    outside_changes = native_changes = 0
    for segment_index, segment in enumerate(("a200", "a000")):
        for plane in range(4):
            prefix = f"vram-{segment}-plane{plane}-"
            before = captured[prefix + "pre.bin"]
            after = captured[prefix + "post.bin"]
            predicted = process.stdout[
                (segment_index * 4 + plane) * size:
                (segment_index * 4 + plane + 1) * size]
            require(len(before) == len(after) == size,
                    "native plane sample length")
            predicted_dirty += sum(predicted[i] == after[i] for i in dirty)
            predicted_full += sum(a == b for a, b in zip(predicted, after))
            native_changes += sum(before[i] != after[i] for i in dirty)
            outside_changes += sum(
                before[i] != after[i] for i in range(size) if i not in dirty)
            if segment == "a200":
                source_stage += sum(
                    pre[ptr + 0x17 + first + plane + 4 * j] ==
                    after[offset - origin + j]
                    for _, first, _, offset, _, length in rows
                    for j in range(length))
            else:
                stage_native = captured[f"vram-a200-plane{plane}-post.bin"]
                stage_page += sum(
                    stage_native[i] == after[i] for i in dirty)
    metadata = process.stdout[8 * size:]
    metadata_matches = 0
    for y in range(168):
        native = (
            bytes([post[ds + 0x4A28 + y]]) +
            post[ds + 0x4C20 + 2 * y:ds + 0x4C22 + 2 * y] +
            post[ds + 0x4D70 + 2 * y:ds + 0x4D72 + 2 * y])
        metadata_matches += metadata[5 * y:5 * y + 5] == native
    require((source_stage, stage_page, predicted_dirty, predicted_full,
             outside_changes, metadata_matches) ==
            (3872, 3872, 7744, 8 * size, 0, 168),
            "all-dirty-row plane/metadata parity")
    print("RES_0464:1B52..1E5B 778 bytes/305 instructions "
          f"sha256 {FLUSH_SHA}; runtime Move 35 bytes/16 instructions "
          f"sha256 {MOVE_SHA}")
    print("same-run RAM entry/return sha256 "
          f"{CAPTURE_HASHES['ram-flush-entry.bin']} / "
          f"{CAPTURE_HASHES['ram-flush-return.bin']}; "
          f"native GC record sha256 {CAPTURE_HASHES['gc-flush-entry.txt']}")
    print("video-state 1 dirty rows 16..103: rows=88, "
          "byte spans=968, plane-byte spans=3872 per segment; "
          "union [0x03C3,0x1166)")
    print(f"source-stage {source_stage}/3872, stage-page {stage_page}/3872, "
          f"C dirty planes {predicted_dirty}/7744, "
          f"C full union {predicted_full}/{8 * size}; "
          f"native changed plane bytes {native_changes}, "
          f"outside-dirty changes {outside_changes}, "
          f"metadata rows {metadata_matches}/168")
    print("reached mode-1 rows unsupported=0; other video modes "
          "and invalid DOS pointers remain outside this bounded model")


if __name__ == "__main__":
    main()
