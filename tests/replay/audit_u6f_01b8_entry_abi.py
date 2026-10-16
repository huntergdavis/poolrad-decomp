#!/usr/bin/env python3
"""Static original-span audit only; never execute original bytes."""
from hashlib import sha256
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: audit_u6f_01b8_entry_abi.py GAME.OVR")
    original = Path(sys.argv[1]).read_bytes()
    if len(original) != 206105 or sha256(original).hexdigest() != (
        "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
    ):
        raise SystemExit("original identity mismatch")
    code = original[0x1e19e:0x1e1a9]
    if sha256(code).hexdigest() != "1acbf172e8b73392aced0f987184f1fb31aee4dd64e1ff0c3858d594431e78be":
        raise SystemExit("entry span mismatch")
    # Temporary raw bytes remain in ignored resources, never public output.
    root = Path(__file__).resolve().parents[2] / "resources"
    root.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=root) as directory:
        span = Path(directory) / "private-span.bin"
        span.write_bytes(code)
        text = subprocess.check_output([
            "objdump", "-D", "-b", "binary", "-m", "i8086", "-M", "intel",
            "--no-show-raw-insn", "--adjust-vma=0x1b8", str(span)
        ], text=True)
    rows = [(int(m[1], 16), re.sub(r"\s+", " ", m[2].strip()))
            for m in re.finditer(r"^\s*([0-9a-f]+):\s+(.+)$", text, re.M)]
    expected = [(0x1b8, "push bp"), (0x1b9, "mov bp,sp"),
                (0x1bb, "sub sp,0x306"), (0x1bf, "push cs"),
                (0x1c0, "call 0x2ae")]
    if rows != expected:
        raise SystemExit("entry graph mismatch")
    print("static original audit PASS: 11 bytes / 5 starts; CALL01C0 ->02AE, return01C3")


if __name__ == "__main__":
    main()
