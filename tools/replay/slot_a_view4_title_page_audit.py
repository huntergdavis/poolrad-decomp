#!/usr/bin/env python3
"""Hash-locked, address-only audit of the first title page effect after view four.

The DOS binaries, saved game, RAM, EGA planes and derived text stay in ignored
resources/. This script prints only addresses, hashes, counts and parity totals.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess
import sys

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "overlay"))
from resident_loader_map import unpack  # noqa: E402


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
FLUSH_SHA = "7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c"
TITLE_SHA = "515cd8d55ed7ed32f3e0a1d710f972030cbc50816e13e2dfde8534684d345e43"
CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
PRINTER_SHA = "b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed"
GLYPH_SHA = "8d71dd4c0162462f99aef0fbbd4cfee472702f330bca1e07b50f192ffd972aef"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
TEXT_SHA = "4c1496b703656034f30aee34e197eb1f4abeff239d28dc9263af7a3df199090b"
RUNTIME_CALL_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
RUNTIME_PRINTER_SHA = "8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164"
RUNTIME_GLYPH_SHA = "05468fbf187f03220b52df161cea902636eb43d908da6ea9ae19543a2855b26e"

TRANSFER_SHA = {
    "ram-flush-entry.bin": "c2b0022f860eac293c0975caea879dcbec32d72feaf10217898566f177773602",
    "ram-flush-return.bin": "221ad81821cd63ea63d4ce7c3dc09d5dff57a52f498a7d64c3dc6d15910305b2",
    "ram-view-return.bin": "579eb0d232dd1c17d64211847078dc3e25e2c186b39ffda61b2cb83c6b182d16",
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
    "page-plane0-after-flush.bin": "5f88a817b8069d5c61544ddf086e59cb70912bcf465ee88b1c3588f6a3a18922",
    "page-plane0-view-return.bin": "bcf034e6c49ce4d3859914cae5bdaafaf2d8d6c5c8fcb06fbc9a9210ac3a7315",
    "page-plane1-after-flush.bin": "5abd0baeb49a38e92ebbcc11b95452202420dc57b2dc53f90fe46657a8f64704",
    "page-plane1-view-return.bin": "53c95f18fee427ce09d95b952f61f680f3acc184dd46a5375554c87da4f7c022",
    "page-plane2-after-flush.bin": "c96df35307143185145de04add952f592c309116ad1e3b905eb2877689d20005",
    "page-plane2-view-return.bin": "957c2f6840523e25aa37fd2c1cddc70be0349451289083194f80c3344e3e6c92",
    "page-plane3-after-flush.bin": "d45e15b59947a4c70b8c18ae426268c989650ab2826d396db1319cb0270898e1",
    "page-plane3-view-return.bin": "6585c2ce577a7850d36459f1c20402b9ac20fbbb911bb7ad8063b77cf4becd12",
}
TITLE_CAPTURE_SHA = {
    "ram-printer-pre.bin": "aa6052aef3d2bbd70a29f2d1cbb73e8cddbe02e3418277e33ac21a4cecc56027",
    "ram-printer-post.bin": "f4b2ee642772907ca63083f70f55e450946cb3050a1a28377b5e1f82b19ce988",
    "page-plane0-pre.bin": "5f88a817b8069d5c61544ddf086e59cb70912bcf465ee88b1c3588f6a3a18922",
    "page-plane0-post.bin": "9b12846eb28f3cce76980273504e392ab1e988774d77e1207ec736c9f3f3c33f",
    "page-plane1-pre.bin": "5abd0baeb49a38e92ebbcc11b95452202420dc57b2dc53f90fe46657a8f64704",
    "page-plane1-post.bin": "2efa139e325407f1c69abca79b84875efd8f15176221fb6434df205d74a17539",
    "page-plane2-pre.bin": "c96df35307143185145de04add952f592c309116ad1e3b905eb2877689d20005",
    "page-plane2-post.bin": "101edb2c68ec35620b03296cf51adc44498034576e53af325308455086789f56",
    "page-plane3-pre.bin": "d45e15b59947a4c70b8c18ae426268c989650ab2826d396db1319cb0270898e1",
    "page-plane3-post.bin": "8cbfa1ccdead5394d2f99f9902132e40ca9050cbc1684bb65210e9085bc8370c",
}
WATCH_SHA = "b82dafd31eed6cce9ab612a0fd2a80885efba66cb25ddc37ff91cc12b509e68c"


def require(ok: bool, label: str) -> None:
    if not ok:
        raise SystemExit(label)


def pinned(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    require(sha256(data).hexdigest() == expected, f"hash mismatch: {path}")
    return data


def word(data: bytes, at: int) -> int:
    require(0 <= at <= len(data) - 2, "word outside RAM")
    return data[at] | data[at + 1] << 8


def checked_code(image: bytes, lo: int, hi: int, ip: int,
                 expected: str, count: int | None = None):
    data = image[lo:hi]
    require(sha256(data).hexdigest() == expected, f"code hash {lo:#x}")
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, ip))
    if count is not None:
        require(len(instructions) == count and
                instructions[-1].address + instructions[-1].size == ip + len(data),
                f"code boundary {lo:#x}")
    return {instruction.address: instruction for instruction in instructions}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--ovr", required=True, type=Path)
    parser.add_argument("--transfer-run", required=True, type=Path)
    parser.add_argument("--title-run", required=True, type=Path)
    parser.add_argument("--watch-run", required=True, type=Path)
    parser.add_argument("--transfer-probe", required=True, type=Path)
    parser.add_argument("--title-probe", required=True, type=Path)
    args = parser.parse_args()

    exe = pinned(args.exe, START_SHA)
    pinned(args.ovr, OVR_SHA)
    image, *_ = unpack(exe)
    require(sha256(exe[0x61CB:0x64D5]).hexdigest() == FLUSH_SHA,
            "transfer code hash")
    title_code = checked_code(image, 0x130D, 0x135F, 0x76D,
                              TITLE_SHA, 34)
    checked_code(image, 0x133C, 0x1341, 0x79C, CALL_SHA, 1)
    checked_code(image, 0x76B6, 0x7737, 0x626, PRINTER_SHA, 51)
    glyph_code = checked_code(image, 0x726F, 0x767A, 0x1DF, GLYPH_SHA)
    require(title_code[0x76D].mnemonic == "cmp" and
            title_code[0x772].mnemonic == "jne" and
            title_code[0x77A].op_str == "byte ptr [bp - 5], 0x11" and
            title_code[0x77E].op_str == "byte ptr [bp - 6], 2" and
            title_code[0x79C].mnemonic == "lcall" and
            glyph_code[0x412].mnemonic == "mov" and
            glyph_code[0x412].op_str == "byte ptr es:[di], dl",
            "title gate / printer / glyph-writer instructions")

    transfer = {name: pinned(args.transfer_run / name, expected)
                for name, expected in TRANSFER_SHA.items()}
    title = {name: pinned(args.title_run / name, expected)
             for name, expected in TITLE_CAPTURE_SHA.items()}
    watch = pinned(args.watch_run / "ram-page0291-watch.bin", WATCH_SHA)
    ds, ss = 0x14A00, 0x1D160
    entry = transfer["ram-flush-entry.bin"]
    flush_return = transfer["ram-flush-return.bin"]
    pre = title["ram-printer-pre.bin"]
    post = title["ram-printer-post.bin"]
    require(all(len(ram) == 0xA0000 for ram in
                (entry, flush_return, transfer["ram-view-return.bin"],
                 pre, post, watch)), "RAM capture lengths")
    require((word(entry, ss + 0x3E94), word(entry, ss + 0x3E96),
             word(entry, ss + 0x3E9A), word(entry, ss + 0x3E9C)) ==
            (0x05FB, 0x2D14, 0x0DAE, 0x2D14) and
            entry[ds + 0x4A1E] == 1, "view-four downstream flush frame")
    rows = [y for y in range(168) if entry[ds + 0x4A28 + y]]
    require(rows == list(range(16, 104)), "dirty rows 16..103")
    spans = []
    for y in rows:
        first = word(entry, ds + 0x4C20 + 2 * y)
        last = word(entry, ds + 0x4D70 + 2 * y)
        low = word(entry, ds + 0x4AD0 + 2 * y)
        at = word(entry, ds + 0x5060) + 1 + 40 * y + low
        require(first <= last and first % 4 == 0 and low == 2 and
                (last - first) % 4 == 3, f"dirty extent row {y}")
        spans.append((at, (last - first) // 4 + 1))
    origin = min(at for at, _ in spans)
    end = max(at + length for at, length in spans)
    require((origin, end) == (0x03C3, 0x1166), "transfer union")
    size = end - origin
    transfer_inputs = [args.transfer_run /
                       f"vram-{seg}-plane{plane}-pre.bin"
                       for seg in ("a200", "a000") for plane in range(4)]
    process = subprocess.run(
        [str(args.transfer_probe.resolve()),
         str(args.transfer_run / "ram-flush-entry.bin"),
         "14A0", "00", "00", "00", "00", "FF",
         *(str(path) for path in transfer_inputs)],
        capture_output=True, check=False)
    require(process.returncode == 0 and
            len(process.stdout) == 8 * size + 5 * 168,
            "all-dirty C probe")
    for seg_index, seg in enumerate(("a200", "a000")):
        for plane in range(4):
            expected = transfer[f"vram-{seg}-plane{plane}-post.bin"]
            got = process.stdout[(seg_index * 4 + plane) * size:
                                 (seg_index * 4 + plane + 1) * size]
            require(len(expected) == size and got == expected,
                    f"transfer plane {seg}/{plane}")
    metadata = process.stdout[8 * size:]
    for y in range(168):
        native = (bytes([flush_return[ds + 0x4A28 + y]]) +
                  flush_return[ds + 0x4C20 + 2 * y:ds + 0x4C22 + 2 * y] +
                  flush_return[ds + 0x4D70 + 2 * y:ds + 0x4D72 + 2 * y])
        require(metadata[5 * y:5 * y + 5] == native,
                f"transfer metadata row {y}")

    first_later = []
    for plane in range(4):
        after_flush = transfer[f"page-plane{plane}-after-flush.bin"]
        later = transfer[f"page-plane{plane}-view-return.bin"]
        title_pre = title[f"page-plane{plane}-pre.bin"]
        require(len(after_flush) == len(later) == len(title_pre) == 8192,
                "full page window")
        require(after_flush[origin:end] ==
                transfer[f"vram-a000-plane{plane}-post.bin"],
                "full page vs transfer-window capture")
        require(after_flush == title_pre,
                "independent replay page state at title entry")
        changes = [i for i, (a, b) in enumerate(zip(after_flush, later))
                   if a != b]
        require(changes and changes[0] == 0x291 and
                all(i in changes for i in range(0x291, 0x295)),
                f"lowest later page range, plane {plane}")
        first_later.append(changes[0])

    require(pre[ds + 0x49F3] != 0 and pre[ds + 0x4A1E] == 1 and
            word(pre, ds + 0x4A16) == word(pre, ds + 0x4A18) == 0xA000,
            "saved title branch and EGA page")
    require(sha256(pre[0x08DE0 + 0x79C:0x08DE0 + 0x7A1]).hexdigest() ==
            RUNTIME_CALL_SHA and
            sha256(pre[0x0F2D0 + 0x626:0x0F2D0 + 0x6A7]).hexdigest() ==
            RUNTIME_PRINTER_SHA and
            sha256(watch[0x0F2D0 + 0x1DF:0x0F2D0 + 0x5EA]).hexdigest() ==
            RUNTIME_GLYPH_SHA, "relocated runtime call / printer / glyph")
    source_off, source_seg, color, row, col = (
        word(pre, ss + 0x3EAA + 2 * i) for i in range(5))
    require((color & 0xFF, row & 0xFF, col & 0xFF) == (15, 2, 17) and
            (color >> 8, row >> 8, col >> 8) == (1, 1, 1),
            "reached first title printer ABI")
    source_at = source_off + 16 * source_seg
    length = pre[source_at]
    require(length == 4 and
            sha256(pre[source_at:source_at + length + 1]).hexdigest() ==
            TEXT_SHA, "private title length and hash")
    font_at = word(pre, ds + 0x5E20) + 16 * word(pre, ds + 0x5E22)
    font = pre[font_at:font_at + 1416]
    require(len(font) == 1416 and sha256(font).hexdigest() == FONT_SHA,
            "live font")
    require((word(watch, ss + 0x3D8E + 2),
             word(watch, ss + 0x3D8E + 4),
             word(watch, ss + 0x3EA4 + 2),
             word(watch, ss + 0x3EA4 + 4)) ==
            (0x0694, 0x0F2D, 0x07A1, 0x08DE),
            "watch glyph/printer caller frames")
    require((word(post, ss + 0x3EA6), word(post, ss + 0x3EA8)) ==
            (0x07A1, 0x08DE), "immediate printer return frame")

    title_before = [title[f"page-plane{plane}-pre.bin"]
                    for plane in range(4)]
    title_after = [title[f"page-plane{plane}-post.bin"]
                   for plane in range(4)]
    changes = {(plane, i) for plane in range(4)
               for i, (a, b) in enumerate(zip(title_before[plane],
                                               title_after[plane])) if a != b}
    targets = {(plane, 40 * y + x) for plane in range(4)
               for y in range(16, 24) for x in range(17, 21)}
    require(len(changes) == 112 and changes <= targets and
            len(targets) == 128, "native four-cell page effect")
    private_input = (font + bytes((length, col & 0xFF, row & 0xFF,
                                   color & 0xFF)) +
                     pre[source_at + 1:source_at + 1 + length] +
                     bytes(255 - length) + b"".join(title_before))
    require(len(private_input) == 1416 + 4 + 255 + 4 * 8192,
            "private title probe input")
    input_path = args.title_run / "title-probe-input-private.bin"
    output_path = args.title_run / "title-probe-output-private.bin"
    input_path.write_bytes(private_input)
    result = subprocess.run([str(args.title_probe.resolve()),
                             str(input_path), str(output_path)],
                            capture_output=True, text=True, check=False)
    require(result.returncode == 0 and
            result.stdout.strip() == "cells=4 stores=128 changed=112" and
            output_path.read_bytes() == b"".join(title_after),
            "C/native first title page parity")
    print(f"original START {START_SHA}; first-title caller [0x130D,0x135F) "
          f"82 bytes/34 instructions SHA {TITLE_SHA}")
    print(f"glyph writer 0F2D:0412; first title call 08DE:079C; "
          f"native watch A000:0291; immediate return 08DE:07A1")
    print(f"view-four downstream flush 0C88:1B52: 88 rows, "
          f"8/8 planes {8 * size}/{8 * size} bytes, metadata 168/168")
    print(f"lowest changed page range after flush [0x0291,0x0295), "
          f"row 16 columns 17..20, on 4/4 planes")
    print(f"reached row-2 title: 4 cells, 128 stores, 112 native changes; "
          f"C/native page {4 * 8192}/{4 * 8192} bytes")


if __name__ == "__main__":
    main()
