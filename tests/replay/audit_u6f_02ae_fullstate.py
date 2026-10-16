#!/usr/bin/env python3
"""Read private original inputs; check source identity, never execute them."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from resident_loader_map import unpack


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: audit_u6f_02ae_fullstate.py GAME.OVR START.EXE")
    paths = [Path(p).resolve(strict=True) for p in sys.argv[1:]]
    if any("resources" not in p.parts for p in paths):
        raise SystemExit("private inputs must remain under resources/")
    game, exe = [p.read_bytes() for p in paths]
    assert (len(game), digest(game)) == (
        206105, "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614")
    assert (len(exe), digest(exe)) == (
        65231, "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd")
    body = game[0x1E294:0x1E315]
    error = game[0x1E2CF:0x1E305]
    assert digest(body) == "ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539"
    assert digest(error) == "7975be64c15f8771caa46217d7242d4c001d79a2c61845864eeeef43be8b8c02"
    original, *_ = unpack(exe)
    assert digest(original[0x7C85:0x7CCF]) == (
        "4a3f940caae95794f257a25f5241bcf3a04c2749b212f5b8d9def5befd63a4cc")
    private = ROOT / "resources/build-u6f-02ae-fullstate-static"
    private.mkdir(parents=True, exist_ok=True)
    image = private / "caller.bin"
    image.write_bytes(body)
    disassembly = subprocess.check_output([
        "objdump", "-D", "-b", "binary", "-m", "i8086", "-M", "intel",
        "--no-show-raw-insn", "--adjust-vma=686", str(image)], text=True)
    starts = {int(s, 16) for s in re.findall(r"^\s*([0-9a-f]+):\s", disassembly, re.M)}
    old = (ROOT / "src/replay/u6f_02ae_abi.c").read_text()
    table = old.split("static const instruction caller[] = {", 1)[1].split("};", 1)[0]
    entry = {int(s, 16) for s in re.findall(r"I\(0x([0-9a-f]+),", table)}
    new = (ROOT / "src/replay/u6f_02ae_fullstate.c").read_text()
    missing = {int(s, 16) for s in re.findall(r"case 0x([0-9a-f]+):", new)}
    assert len(starts) == 51 and len(entry) == 30 and len(missing) == 21
    assert starts == entry | missing and not entry & missing
    callers = []
    for pc in (0x0024, 0x19E7):
        b = game[0x1DFE6+pc-1:0x1DFE6+pc+3]
        assert b[0] == 0x0E and b[1] == 0xE8
        target = (pc+3+int.from_bytes(b[2:4], "little", signed=True)) & 65535
        assert target == 0x02AE
        callers.append({"call": f"{pc:04X}", "return": f"{pc+3:04X}",
                        "push_cs_call_sha256": digest(b)})
    print(json.dumps({"bytes": len(body), "instructions": len(starts),
                      "new_bytes": len(error), "new_instructions": len(missing),
                      "callers": callers, "original_execution": False,
                      "boundary_audit": "PASS"}, sort_keys=True))


if __name__ == "__main__":
    main()
