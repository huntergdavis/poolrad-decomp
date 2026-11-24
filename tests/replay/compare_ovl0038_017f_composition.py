#!/usr/bin/env python3
"""Compare the C composition output with the recorded post-body state of the native warm call (evidence only)."""
import argparse, json, sys
from pathlib import Path

VIDEO = 0xA0000

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--comp", required=True, type=Path, help="run dir warm/composition")
    ap.add_argument("--c-ram", required=True, type=Path)
    ap.add_argument("--c-regs", required=True, type=Path)
    ap.add_argument("--out", type=Path)
    a = ap.parse_args()
    native = (a.comp / "post-body-ram.bin").read_bytes()
    model = a.c_ram.read_bytes()
    if len(native) != VIDEO or len(model) != VIDEO:
        sys.exit("RAM images must both be 655360 bytes below A0000")
    diff = [i for i in range(VIDEO) if native[i] != model[i]]
    native_regs = (a.comp / "post-body-regs14.txt").read_text().split()
    model_regs = a.c_regs.read_text().split()
    reg_names = ("AX", "BX", "CX", "DX", "SI", "DI", "BP", "SP", "CS", "IP", "DS", "ES", "SS", "FLAGS")
    reg_diff = {n: (native_regs[k], model_regs[k]) for k, n in enumerate(reg_names) if native_regs[k].upper() != model_regs[k].upper()}
    result = {"ram_bytes_compared": VIDEO, "ram_diff_bytes": len(diff),
              "ram_diff_first": [hex(x) for x in diff[:8]],
              "register_fields": len(reg_names), "register_diff": reg_diff,
              "verdict": "EQUAL" if (not diff and not reg_diff) else "DIFFERS"}
    print(json.dumps(result, indent=1))
    if a.out:
        a.out.write_text(json.dumps(result, indent=1))
    return 0 if result["verdict"] == "EQUAL" else 1

if __name__ == "__main__":
    sys.exit(main())
