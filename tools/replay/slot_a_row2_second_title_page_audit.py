#!/usr/bin/env python3
"""Audit the second saved slot-A title call without publishing private bytes."""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess
import sys

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "overlay"))
from resident_loader_map import unpack  # noqa: E402


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
TITLE_PATH_SHA = "958bd3fa3dc6a45c35d3db3f48dcb3339491da159ca4581988c5a4a59a19bd8d"
CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
RUNTIME_CALL_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
RUNTIME_PRINTER_SHA = "8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164"
GLYPH_SHA = "8d71dd4c0162462f99aef0fbbd4cfee472702f330bca1e07b50f192ffd972aef"
RUNTIME_GLYPH_SHA = "05468fbf187f03220b52df161cea902636eb43d908da6ea9ae19543a2855b26e"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
SOURCE_SHA = "7039295c05fc9db0246086bb5dc3046e4ac8edffb2c6376a760c88f229dc7ef0"

RAM_SHA = {
    "ram-second-title-pre.bin": "a609efcedb0303dcc08794afad6df3a11c935cd87c28125abd60409a73a4e80a",
    "ram-second-title-post.bin": "7a9c3ea7a9d4406186d24bc57e447bd4766cee2a4286dfc6b87cf4b9971785f0",
    "ram-view-return.bin": "30fdfc4f425a56e1c1ece494dc69f464c6243788b5395b44a612896fa2c3695c",
}
FLUSH_SHA = (
    "5f88a817b8069d5c61544ddf086e59cb70912bcf465ee88b1c3588f6a3a18922",
    "5abd0baeb49a38e92ebbcc11b95452202420dc57b2dc53f90fe46657a8f64704",
    "c96df35307143185145de04add952f592c309116ad1e3b905eb2877689d20005",
    "d45e15b59947a4c70b8c18ae426268c989650ab2826d396db1319cb0270898e1",
)
PAGE_SHA = {
    "pre": (
        "9b12846eb28f3cce76980273504e392ab1e988774d77e1207ec736c9f3f3c33f",
        "2efa139e325407f1c69abca79b84875efd8f15176221fb6434df205d74a17539",
        "101edb2c68ec35620b03296cf51adc44498034576e53af325308455086789f56",
        "8cbfa1ccdead5394d2f99f9902132e40ca9050cbc1684bb65210e9085bc8370c",
    ),
    "post": (
        "6da28d474ae91d8ca2110bfcf16048ad4354f258d9cbff3589a5862579b28239",
        "7d77fac674f465342c7c2f33633abfeb74735a545eaf38d8c52c2dd7b092fbed",
        "f92bf011bb9524f832fe00eb1707125e5819bb68b5a52948160577870c7dfe78",
        "1c98f7a4ad6c81e47d171d65cc7530956e5575f1ec4bdaca766244dd0a0c6860",
    ),
    "view-return": (
        "bcf034e6c49ce4d3859914cae5bdaafaf2d8d6c5c8fcb06fbc9a9210ac3a7315",
        "53c95f18fee427ce09d95b952f61f680f3acc184dd46a5375554c87da4f7c022",
        "957c2f6840523e25aa37fd2c1cddc70be0349451289083194f80c3344e3e6c92",
        "6585c2ce577a7850d36459f1c20402b9ac20fbbb911bb7ad8063b77cf4becd12",
    ),
}


def require(condition: bool, label: str) -> None:
    if not condition:
        raise SystemExit(label)


def pinned(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    require(sha256(data).hexdigest() == expected, f"hash mismatch: {path}")
    return data


def word(data: bytes, at: int) -> int:
    require(0 <= at <= len(data) - 2, "word outside RAM")
    return int.from_bytes(data[at:at + 2], "little")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--run", required=True, type=Path)
    parser.add_argument("--transfer-run", required=True, type=Path)
    parser.add_argument("--first-title-run", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    args = parser.parse_args()

    image, *_ = unpack(pinned(args.exe, START_SHA))
    path = image[0x1341:0x135F]
    require(sha256(path).hexdigest() == TITLE_PATH_SHA and
            sha256(image[0x135A:0x135F]).hexdigest() == CALL_SHA,
            "original title path/call")
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(path, 0x7A1))
    require(len(instructions) == 14 and
            instructions[-1].address + instructions[-1].size == 0x7BF and
            all(not item.mnemonic.startswith("j") for item in instructions) and
            instructions[0].mnemonic == "mov" and
            instructions[0].op_str == "al, 0x21" and
            instructions[-1].mnemonic == "lcall" and
            instructions[-1].op_str == "0x709, 0x626",
            "aligned, unconditional second title path")
    glyph = image[0x726F:0x767A]
    require(sha256(glyph).hexdigest() == GLYPH_SHA and
            any(item.address == 0x412 and item.mnemonic == "mov" and
                item.op_str == "byte ptr es:[di], dl"
                for item in Cs(CS_ARCH_X86, CS_MODE_16).disasm(glyph, 0x1DF)),
            "original glyph page writer")

    ram = {name: pinned(args.run / name, digest)
           for name, digest in RAM_SHA.items()}
    require(all(len(data) == 0xA0000 for data in ram.values()), "RAM sizes")
    pre = ram["ram-second-title-pre.bin"]
    post = ram["ram-second-title-post.bin"]
    ds, ss = 0x14A00, 0x1D160
    require(sha256(pre[0x08DE0 + 0x7BA:0x08DE0 + 0x7BF]).hexdigest() ==
            RUNTIME_CALL_SHA and
            sha256(pre[0x0F2D0 + 0x626:0x0F2D0 + 0x6A7]).hexdigest() ==
            RUNTIME_PRINTER_SHA and
            sha256(pre[0x0F2D0 + 0x1DF:0x0F2D0 + 0x5EA]).hexdigest() ==
            RUNTIME_GLYPH_SHA, "relocated caller, printer and glyph writer")
    require(pre[ds + 0x4A1E] == 1 and
            word(pre, ds + 0x4A16) == word(pre, ds + 0x4A18) == 0xA000,
            "saved EGA output page")
    source_off, source_seg, color, row, col = (
        word(pre, ss + 0x3EAA + 2 * index) for index in range(5))
    require((color & 0xFF, row & 0xFF, col & 0xFF) == (15, 2, 33),
            "reached second title arguments")
    source_at = 16 * source_seg + source_off
    length = pre[source_at]
    require(length == 6 and
            sha256(pre[source_at:source_at + length + 1]).hexdigest() ==
            SOURCE_SHA, "private source length/hash")
    require((word(post, ss + 0x3EA6), word(post, ss + 0x3EA8),
             word(post, ss + 0x3EB2) & 0xFF) == (0x7BF, 0x08DE, 39),
            "completed six-cell printer return")
    font_at = word(pre, ds + 0x5E20) + 16 * word(pre, ds + 0x5E22)
    font = pre[font_at:font_at + 1416]
    require(len(font) == 1416 and sha256(font).hexdigest() == FONT_SHA,
            "private live font")

    pages = {stage: [pinned(args.run / f"page-plane{plane}-{stage}.bin",
                            PAGE_SHA[stage][plane]) for plane in range(4)]
             for stage in PAGE_SHA}
    flush = [pinned(args.transfer_run /
                    f"page-plane{plane}-after-flush.bin", FLUSH_SHA[plane])
             for plane in range(4)]
    prior = [pinned(args.first_title_run / f"page-plane{plane}-post.bin",
                    PAGE_SHA["pre"][plane]) for plane in range(4)]
    first_targets = {(plane, 40 * y + x) for plane in range(4)
                     for y in range(16, 24) for x in range(17, 21)}
    second_targets = {(plane, 40 * y + x) for plane in range(4)
                      for y in range(16, 24) for x in range(33, 39)}
    changes = {(plane, at) for plane in range(4) for at in range(8192)
               if pages["pre"][plane][at] != pages["post"][plane][at]}
    require(len(changes) == 112 and changes <= second_targets and
            all((plane, 0x2A1) in changes and (plane, 0x2A2) in changes
                for plane in range(4)), "second title's lowest range/effect")
    for plane in range(4):
        require(len(pages["pre"][plane]) == len(pages["post"][plane]) ==
                len(pages["view-return"][plane]) == 8192 and
                prior[plane] == pages["pre"][plane] and
                pages["post"][plane][:1280] ==
                pages["view-return"][plane][:1280], "title handoff/top persistence")
    top_delta = {(plane, at) for plane in range(4) for at in range(1280)
                 if flush[plane][at] != pages["view-return"][plane][at]}
    require(len(top_delta) == 224 and
            top_delta <= first_targets | second_targets and
            all(pages["post"][plane][at] ==
                pages["view-return"][plane][at] for plane, at in changes),
            "both titles account for net top-four-row changes")

    private_input = (font + bytes((length, col & 0xFF, row & 0xFF,
                                   color & 0xFF)) +
                     pre[source_at + 1:source_at + 1 + length] +
                     bytes(255 - length) + b"".join(pages["pre"]))
    require(len(private_input) == 1416 + 4 + 255 + 4 * 8192,
            "private full-page probe input")
    input_path = args.run / "second-title-probe-input-private.bin"
    output_path = args.run / "second-title-probe-output-private.bin"
    input_path.write_bytes(private_input)
    result = subprocess.run([str(args.probe.resolve()), str(input_path),
                             str(output_path)], capture_output=True,
                            text=True, check=False)
    require(result.returncode == 0 and
            result.stdout.strip() == "cells=6 stores=192 changed=112" and
            output_path.read_bytes() == b"".join(pages["post"]),
            "C/native full-page parity")
    print(f"original START {START_SHA}; title path [0x1341,0x135F) "
          f"30 bytes/14 instructions SHA {TITLE_PATH_SHA}")
    print("saved second title 08DE:07BA -> 0F2D:0626 / glyph store "
          "0F2D:0412 -> 08DE:07BF; "
          "row 2, columns 33..38, color 15, six private cells")
    print("lowest remaining page range [0x02A1,0x02A3) on 4/4 planes; "
          "192 stores, 112 native changes; C/native 32768/32768 page bytes")
    print("top four text rows: 224 net changed plane bytes from flush to "
          "first-view return, all in the two reached title target rectangles")


if __name__ == "__main__":
    main()
