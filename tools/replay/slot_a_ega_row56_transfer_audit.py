#!/usr/bin/env python3
"""Hash-locked private row-56 EGA transfer parity; print no media bytes."""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess

from capstone import CS_ARCH_X86, CS_MODE_16, Cs


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
FLUSH_SHA = "7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c"
MOVE_SHA = "1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972"
GC_RECORD_SHA = "9cbcf4528ea3c3300faccfd2d40776d61f1d4a093a09d8a38464a7f5c2506ed3"
RAM_HASHES = {
    "ram-flush-entry.bin": "d102de8a62ef0895916263d20da1fd3b58e45ccc6ed15eedb22bb5514f292fc0",
    "ram-flush-return.bin": "87799350e1bec55a5e6b9b3c283f7c2cd11e8309ccd336f0f535fc64205d4466",
}
EMPTY_SHA = "71b6c1d53832f789a7f2435a7c629245fa3761ad8487775ebf4957330213a706"
POST_SHA = (
    "15300bd714efbc71d4830e661d7d6f7a052eaa0bd377a3332a3ed6dbbcee3f87",
    "15300bd714efbc71d4830e661d7d6f7a052eaa0bd377a3332a3ed6dbbcee3f87",
    "8135c933b3baa00efe955918860d969efcc4b84050e41025de82a2d4e2232876",
    EMPTY_SHA,
)


def require(ok: bool, message: str) -> None:
    if not ok:
        raise SystemExit(message)


def pinned(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    require(sha256(data).hexdigest() == expected, f"hash mismatch: {path}")
    return data


def word(data: bytes, at: int) -> int:
    require(0 <= at <= len(data) - 2, "word outside RAM")
    return data[at] | data[at + 1] << 8


def decoded(image: bytes, start: int, end: int, ip: int,
            expected_hash: str, expected_count: int):
    span = image[start:end]
    require(sha256(span).hexdigest() == expected_hash, "original span hash")
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ip))
    require(len(instructions) == expected_count and
            instructions[-1].address + instructions[-1].size == ip + len(span),
            "original instruction extent")
    return {item.address: item for item in instructions}


def instruction(items, ip: int, mnemonic: str, operand: str) -> None:
    item = items.get(ip)
    require(item is not None and item.mnemonic == mnemonic and
            item.op_str == operand, f"instruction mismatch at {ip:#x}")

def gc_observation(path: Path) -> tuple[int, ...]:
    lines = pinned(path, GC_RECORD_SHA).decode("ascii").splitlines()
    require(lines[:4] == [
        "source=interactive debugger tool output; transcribed after run",
        "stop=0C88:1B52", "return=2D14:05FB", "ds=14A0",
    ] and len(lines) == 17 and lines[-1] == "OUTP 3CE 05",
            "GC observation stop/provenance")
    values = []
    for index, register in enumerate((0, 1, 3, 4, 5, 8)):
        require(lines[4 + 2 * index] == f"OUTP 3CE {register:02X}" and
                lines[5 + 2 * index].startswith("INP 3CF -> "),
                f"GC register {register} command")
        values.append(int(lines[5 + 2 * index].split()[-1], 16))
    require(tuple(values) == (0, 0, 0, 0, 0, 0xFF),
            "GC register values")
    return tuple(values)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--run", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    args = parser.parse_args()

    exe = pinned(args.exe, START_SHA)
    flush = decoded(exe, 0x61CB, 0x64D5, 0x1B52, FLUSH_SHA, 305)
    move = decoded(exe, 0xC2FB, 0xC31E, 0x1685, MOVE_SHA, 16)
    for ip, op in ((0x1C37, "byte ptr es:[di], dl"),
                   (0x1C7E, "byte ptr es:[di], dl"),
                   (0x1CC6, "byte ptr es:[di], dl"),
                   (0x1D0F, "byte ptr es:[di], dl")):
        instruction(flush, ip, "mov", op)
    for ip, op in ((0x1C09, "al, byte ptr [0x25f6]"),
                   (0x1C4F, "al, byte ptr [0x25f7]"),
                   (0x1C96, "al, byte ptr [0x25f8]"),
                   (0x1CDE, "al, byte ptr [0x25f9]")):
        instruction(flush, ip, "mov", op)
    instruction(flush, 0x1D31, "mov", "al, 0xf")
    instruction(flush, 0x1D3E, "mov", "al, 5")
    instruction(flush, 0x1D44, "mov", "al, 1")
    instruction(flush, 0x1DA0, "lcall", "0xaf8, 0x1685")
    instruction(flush, 0x1DB0, "mov", "al, 0")
    for ip in (0x1E31, 0x1E42, 0x1E53):
        instruction(flush, ip, "lcall", "0xaf8, 0x16a8")
    instruction(move, 0x1695, "cld", "")
    instruction(move, 0x1696, "cmp", "si, di")
    instruction(move, 0x1698, "jae", "0x16a1")
    instruction(move, 0x16A1, "rep movsb",
                "byte ptr es:[di], byte ptr [si]")
    instruction(move, 0x16A5, "retf", "0xa")

    pre = pinned(args.run / "ram-flush-entry.bin",
                 RAM_HASHES["ram-flush-entry.bin"])
    post = pinned(args.run / "ram-flush-return.bin",
                  RAM_HASHES["ram-flush-return.bin"])
    require(len(pre) == len(post) == 0xA0000, "RAM size")
    ss, ds, row = 0x1D160, 0x14A00, 56
    require((word(pre, ss + 0x3E94), word(pre, ss + 0x3E96),
             word(pre, ss + 0x3E9A), word(pre, ss + 0x3E9C)) ==
            (0x05FB, 0x2D14, 0x0DAE, 0x2D14), "post-wall flush frame")
    require(pre[ds + 0x4A1E] == 1 and
            (pre[ds + 0x4A1A], pre[ds + 0x4A1B],
             word(pre, ds + 0x4A16), word(pre, ds + 0x4A18)) ==
            (0, 0, 0xA000, 0xA000), "video-state/page gate")
    first = word(pre, ds + 0x4C20 + 2 * row)
    last = word(pre, ds + 0x4D70 + 2 * row)
    low = word(pre, ds + 0x4AD0 + 2 * row)
    row_base = word(pre, ds + 0x5050 + 2 * (row + 8))
    page_base = word(pre, ds + 0x5060)
    require((first, last, low, row_base, page_base) ==
            (4712, 4755, 2, 0x0A00, 0x0140), "row-56 extent/base")
    require(tuple(pre[ds + 0x25F6:ds + 0x25FA]) == (1, 2, 4, 8),
            "sequencer map-mask table")
    require(pre[ds + 0x4A28 + row] == 1 and
            post[ds + 0x4A28 + row] == 0 and
            (word(post, ds + 0x4C20 + 2 * row),
             word(post, ds + 0x4D70 + 2 * row)) == (0xFFFF, 0),
            "row-56 dirty metadata reset")
    offset = row_base + low + 1
    require(offset == page_base + 1 + row * 40 + low == 0x0A03 and
            ((last - first) >> 2) + 1 == 11, "staging/page Move range")
    ptr = word(pre, ds + 0x4A20) + 16 * word(pre, ds + 0x4A22)
    require(ptr == 0x3DE48, "offscreen object pointer")
    gc = gc_observation(args.run / "gc-flush-entry.txt")

    inputs, native_post = [], []
    source_matches = page_matches = 0
    for segment in ("a200", "a000"):
        for plane in range(4):
            name = f"vram-{segment}-plane{plane}-pre.bin"
            inputs.append(args.run / name)
            before = pinned(args.run / name, EMPTY_SHA)
            after = pinned(args.run / f"vram-{segment}-plane{plane}-post.bin",
                           POST_SHA[plane])
            require(len(before) == len(after) == 11, "plane sample length")
            native_post.append(after)
            if segment == "a200":
                source = bytes(pre[ptr + 0x17 + first + plane + 4 * j]
                               for j in range(11))
                source_matches += sum(a == b for a, b in zip(source, after))
            else:
                page_matches += sum(a == b for a, b in
                                    zip(native_post[plane], after))

    process = subprocess.run(
        [str(args.probe), str(args.run / "ram-flush-entry.bin"), "14A0",
         *(f"{gc[index]:02X}" for index in (0, 1, 2, 4, 5)),
         *(str(path) for path in inputs)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    require(process.returncode == 0,
            f"C model probe failed: {process.stderr.decode(errors='replace')}")
    require(len(process.stdout) == 88, "C model output size")
    stage_output = process.stdout[:44]
    page_output = process.stdout[44:]
    stage_native = b"".join(native_post[:4])
    page_native = b"".join(native_post[4:])
    stage_matches = sum(a == b for a, b in
                        zip(stage_output, stage_native))
    final_matches = sum(a == b for a, b in zip(page_output, page_native))
    require((source_matches, page_matches, stage_matches, final_matches) ==
            (44, 44, 44, 44), "row-56 source/stage/page parity")
    print("RES_0464:1B52..1E5B file [0x61CB,0x64D5): "
          f"778 bytes, 305 instructions, sha256 {FLUSH_SHA}")
    print("runtime Move 0AF8:1685 file [0xC2FB,0xC31E): "
          f"35 bytes, 16 instructions, sha256 {MOVE_SHA}")
    print("pre-flush GC numeric record at 0C88:1B52: "
          f"sha256 {GC_RECORD_SHA}; registers 0/1/3/4/5/8 = "
          "00/00/00/00/00/FF (interactive output transcription)")
    print("row56 A200/A000 [0x0A03,0x0A0E): "
          f"source-stage {source_matches}/44, stage-page {page_matches}/44, "
          f"C-stage {stage_matches}/44, C-page {final_matches}/44; "
          "dirty 1->0, first/last reset FFFF/0000")


if __name__ == "__main__":
    main()
