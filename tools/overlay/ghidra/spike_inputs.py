#!/usr/bin/env python3
"""Prepare Ghidra spike inputs: the resident image, one overlay unit's code, its fixup list,
and the entry-stub map. Writes only into the given scratch directory."""
import argparse, json, os, re, struct, sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import overlay_inventory as oi

ap = argparse.ArgumentParser()
ap.add_argument("--exe", required=True); ap.add_argument("--ovr", required=True)
ap.add_argument("--unit-ovr-offset", type=int, required=True); ap.add_argument("--out", required=True)
a = ap.parse_args()
exe = open(a.exe, "rb").read(); ovr = open(a.ovr, "rb").read()
img = exe[oi.parse_mz(exe)["header_bytes"]:]
units = oi.find_units(img, ovr)
u = [x for x in units if x["ovr_offset"] == a.unit_ovr_offset][0]
os.makedirs(a.out, exist_ok=True)
open(os.path.join(a.out, "resident.bin"), "wb").write(img)
open(os.path.join(a.out, "unit1c_code.bin"), "wb").write(ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]])
with open(os.path.join(a.out, "fixups.txt"), "w") as f:
    f.writelines(f"{x}\n" for x in u["fixups"])
with open(os.path.join(a.out, "stubs.txt"), "w") as f:
    for x in units:
        for i, e in enumerate(x["entries"]):
            f.write(f"{hex(x['stub_base'] + 5 * i)} {hex(x['stub_para'])} {i} {x['ovr_offset'] + e}\n")
print("spike inputs written:", a.out, "unit stub para", hex(u["stub_para"]), "code", u["code_size"], "fixups", len(u["fixups"]))
