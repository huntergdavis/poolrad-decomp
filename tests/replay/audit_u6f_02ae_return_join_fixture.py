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
        raise SystemExit("usage: audit_u6f_02ae_return_join_fixture.py GAME.OVR")
    original = Path(sys.argv[1]).read_bytes()
    if len(original) != 206105 or sha256(original).hexdigest() != (
        "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
    ):
        raise SystemExit("original identity mismatch")
    code = original[0x1e311:0x1e315]
    if sha256(code).hexdigest() != "d938592002da1e8f84eae35df938d5b693d827276f6e045d142340640128831a":
        raise SystemExit("entry span mismatch")
    # Temporary raw bytes remain in ignored resources, never public output.
    root = Path(__file__).resolve().parents[2] / "resources"
    root.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=root) as directory:
        span = Path(directory) / "private-span.bin"
        span.write_bytes(code)
        text = subprocess.check_output([
            "objdump", "-D", "-b", "binary", "-m", "i8086", "-M", "intel",
            "--no-show-raw-insn", "--adjust-vma=0x32b", str(span)
        ], text=True)
    rows = [(int(m[1], 16), re.sub(r"\s+", " ", m[2].strip()))
            for m in re.finditer(r"^\s*([0-9a-f]+):\s+(.+)$", text, re.M)]
    expected = [(0x32b, "mov sp,bp"), (0x32d, "pop bp"), (0x32e, "retf")]
    if rows != expected:
        raise SystemExit("entry graph mismatch")
    print("static original audit PASS: 4 bytes / 3 starts; actual MOVSPBP/POPBP/RETF, no stores")


if __name__ == "__main__":
    main()
