#!/usr/bin/env python3
"""Audit the saved first-wall offscreen-to-EGA transfer without emitting media."""

import argparse
from hashlib import sha256
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_16, Cs


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
FLUSH_SHA = "7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c"
WRAPPER_SHA = "1caa4f4b724b9f109077bbaa3afce253092779ecc08504cc3062c82d2da20675"
EXIT_SHA = "7dec4d4d92bb5fffbafdc4de698caef9e181d36f059162d329066df4f06867a3"

RUN_HASHES = {
    "ram-flush-entry.bin": "552f260119eeb651a33455a6ab512560c6a5c323d301e00900b31bc00b43b958",
    "ram-view-return.bin": "da6687e8d259188daf97ac9f9d5753ca173f9bfd737abadf1a531ae844350944",
    "vram-a000-plane2-pre.bin": "1bcc1bbf3316abe5c8b57be7dba5c7e7b1c7f9dc078913f6a1d367dd876b5849",
    "vram-a000-plane2-post.bin": "74c7e9e1450eb255e2b86a1289042fc6492b5db841d1fea6ab37c2494841344e",
    "vram-a200-plane2-pre.bin": "66687aadf862bd776c8fc18b8e9f8e20089714856ee233b3902a591d0d5f2925",
    "vram-a200-plane2-post.bin": "3c21f304946ef5c6b483a7e38bd4fe52a3f29ecca3e0ce191a6a05d69188400c",
}


def check(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def pinned(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    check(sha256(data).hexdigest() == expected, f"hash mismatch: {path}")
    return data


def word(data: bytes, offset: int) -> int:
    check(0 <= offset <= len(data) - 2, "word outside RAM")
    return data[offset] | data[offset + 1] << 8


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()

    exe = pinned(args.exe, START_SHA)
    ovr = pinned(args.ovr, OVR_SHA)
    spans = (
        ("RES_0464:1B52", exe, 0x61CB, 0x64D5, 0x1B52, FLUSH_SHA, 305),
        ("u3BD:+05F3", ovr, 0x2EEEA, 0x2EEF4, 0x05F3, WRAPPER_SHA, 5),
        ("u3BD:+0DAA", ovr, 0x2F6A1, 0x2F6AB, 0x0DAA, EXIT_SHA, 5),
    )
    instructions = {}
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    for name, image, start, end, ip, expected, count in spans:
        chunk = image[start:end]
        check(sha256(chunk).hexdigest() == expected, f"span hash mismatch: {name}")
        decoded = list(decoder.disasm(chunk, ip))
        check(len(decoded) == count and
              decoded[-1].address + decoded[-1].size == ip + len(chunk),
              f"unaligned instruction span: {name}")
        instructions[name] = {item.address: item for item in decoded}
        print(f"{name}: file [{start:#x},{end:#x}) {len(chunk)} bytes; "
              f"{count} instructions; sha256 {expected}")

    flush = instructions["RES_0464:1B52"]
    wrapper = instructions["u3BD:+05F3"]
    exit_path = instructions["u3BD:+0DAA"]
    for table, address, mnemonic, operand in (
        (wrapper, 0x05F6, "lcall", "0x464, 0x1b52"),
        (exit_path, 0x0DAB, "call", "0x5f3"),
        (flush, 0x1C37, "mov", "byte ptr es:[di], dl"),
        (flush, 0x1CC6, "mov", "byte ptr es:[di], dl"),
        (flush, 0x1DA0, "lcall", "0xaf8, 0x1685"),
    ):
        item = table.get(address)
        check(item is not None and item.mnemonic == mnemonic and
              item.op_str == operand, f"instruction mismatch at {address:#x}")

    captured = {name: pinned(args.run / name, expected)
                for name, expected in RUN_HASHES.items()}
    pre = captured["ram-flush-entry.bin"]
    post = captured["ram-view-return.bin"]
    check(len(pre) == len(post) == 0xA0000, "RAM image size")
    ss = 0x1D160
    check((word(pre, ss + 0x3E94), word(pre, ss + 0x3E96),
           word(pre, ss + 0x3E9A), word(pre, ss + 0x3E9C)) ==
          (0x05FB, 0x2D14, 0x0DAE, 0x2D14), "post-wall caller frame")

    ds = 0x14A00
    row = 56
    check(pre[ds + 0x4A1E] == 1, "EGA mode-one gate")
    check((pre[ds + 0x4A1A], pre[ds + 0x4A1B],
           word(pre, ds + 0x4A16), word(pre, ds + 0x4A18)) ==
          (0, 0, 0xA000, 0xA000), "active and draw page")
    check(pre[ds + 0x4A28 + row] == 1 and
          post[ds + 0x4A28 + row] == 0, "row-56 dirty gate/reset")
    first = word(pre, ds + 0x4C20 + 2 * row)
    last = word(pre, ds + 0x4D70 + 2 * row)
    low = word(pre, ds + 0x4AD0 + 2 * row)
    check((first, last, low) == (4712, 4755, 2), "row-56 extent")
    check((word(post, ds + 0x4C20 + 2 * row),
           word(post, ds + 0x4D70 + 2 * row)) ==
          (0xFFFF, 0), "row-56 extent reset")

    obj = word(pre, ds + 0x4A20) + 16 * word(pre, ds + 0x4A22)
    source_offset = 4734
    check(obj == 0x3DE48 and first <= source_offset <= last and
          (source_offset - first) % 4 == 2, "offscreen plane-two byte")
    check(pre[obj + 0x17 + source_offset] ==
          post[obj + 0x17 + source_offset] == 255,
          "offscreen source persists through transfer")
    check(pre[ds + 0x25F8] == 4, "plane-two sequencer map mask")

    intermediate_base = word(pre, ds + 0x5050 + 2 * (row + 8))
    intermediate = intermediate_base + low + 1 + (source_offset - first) // 4
    final_base = word(pre, ds + 0x5060) + 1 + row * 40 + low
    transfer_length = (last - first) // 4 + 1
    check(intermediate == 0x0A08 and final_base == 0x0A03 and
          final_base <= intermediate < final_base + transfer_length and
          transfer_length == 11, "A200-to-A000 transfer address")

    for segment in ("a200", "a000"):
        before = captured[f"vram-{segment}-plane2-pre.bin"]
        after = captured[f"vram-{segment}-plane2-post.bin"]
        check(len(before) == len(after) == 32 and
              (before[8], after[8]) == (0, 255),
              f"{segment} plane-two numeric transition")
        print(f"{segment.upper()} plane 2 +0x0A08: 0 -> 255; "
              f"changed bytes in 32-byte sample: "
              f"{sum(a != b for a, b in zip(before, after))}")
    print(f"row {row}: object data +{source_offset}, first/last "
          f"{first}/{last}, A200 +{intermediate:#x}, A000 copy "
          f"[{final_base:#x},{final_base + transfer_length:#x}); "
          "dirty 1 -> 0")


if __name__ == "__main__":
    main()
