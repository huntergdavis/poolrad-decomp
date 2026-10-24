#!/usr/bin/env python3
"""Where does the INDEX byte of the unit-0x9E +0000 effect dispatcher come from? (docs/re/OVL_009E_0000_INDEX_REACHABILITY.md)

Light, read-only static scans over the original GAME.OVR / START.EXE; no emulation, no heavy slot.

  sites                  the 11 far (00B0:0025) and 5 near (call 0000) call sites of the dispatcher and what supplies the index byte
  callers STUB K NEAR    every far call to 00B0:STUB (and every near call to unit-9E +NEAR) with the instruction that supplies the K-th push from the end
  writers DISP           every store to a [reg+DISP] operand in any unit's code (record bytes +3Dh/+3Eh)
  tables                 the three DS tables that supply an index (DS:0113h+i, DS:0C13h+i, DS:0C23h+i), read from the expanded START.EXE image, and a scan for code that could write them
  inputs                 SHA-256 of every media file the scans read (the scan inputs, by hash)
  items                  effect bytes +3Ch/+3Dh/+3Eh of every 63-byte item record in the shipped .ITM files and MON*ITM.DAX blocks

An index is "supplied" by the instruction that last loaded AL before the PUSH AX of the argument: MOV AL,imm (a constant), MOV AL,[mem] (data), or a parameter of the enclosing function.
"""
import argparse
import re
import struct
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from capstone import Cs, CS_ARCH_X86, CS_MODE_16  # noqa: E402
from overlay_inventory import find_units, parse_mz  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
DS_PARAGRAPH = 0x0C7C                    # the program data segment in the expanded START.EXE image (docs/re/FUNCTION_CEILING_CENSUS.md)
MD = Cs(CS_ARCH_X86, CS_MODE_16)


def load():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    code = {n: ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]] for n, u in units.items()}
    return units, code


def decode_to(code, at):
    """the instructions that end exactly with the call at `at` (resynchronised, widest clean window first: the original has no symbols)"""
    size = 5 if code[at] == 0x9A else 3
    for st in range(max(0, at - 160), at - 4):
        ins = list(MD.disasm(code[st:at + size], st))
        if ins and ins[-1].address == at and ins[-1].address + ins[-1].size == at + size:
            return ins
    return None


def kth_push_source(ins, k):
    """what supplied the k-th push counting back from the call (k=1 is the last push before it; 'push cs' of a near call is skipped)"""
    body = list(ins[:-1])
    if body and body[-1].mnemonic == "push" and body[-1].op_str == "cs":
        body = body[:-1]
    pushes = [n for n, i in enumerate(body) if i.mnemonic == "push"]
    if len(pushes) < k:
        return "no-push"
    n = pushes[-k]
    if body[n].op_str != "ax":
        return f"push {body[n].op_str}"
    j = n - 1
    while j >= 0 and not (body[j].mnemonic in ("mov", "xor", "add", "sub", "inc", "dec", "and", "or") and body[j].op_str.startswith(("al", "ax"))):
        j -= 1
    return f"{body[j].mnemonic} {body[j].op_str}" if j >= 0 else "ax=?"


def callers(units, code, stub, k, near):
    out = []
    for n, c in code.items():
        for at in range(len(c) - 4):
            far = (stub is not None and c[at] == 0x9A and (at + 3) in units[n]["fixups"] and (c[at + 3] | c[at + 4] << 8) == 0xB0 and (c[at + 1] | c[at + 2] << 8) == stub)
            nr = False
            if n == 0x9E and near is not None and c[at] == 0xE8:
                rel = c[at + 1] | c[at + 2] << 8
                rel = rel - 0x10000 if rel >= 0x8000 else rel
                nr = (at + 3 + rel) & 0xFFFF == near
            if far or nr:
                ins = decode_to(c, at)
                out.append((n, at, "near" if nr else "far", kth_push_source(ins, k) if ins else "no-decode"))
    return out


def immediate(src):
    m = re.search(r"mov al, (0x[0-9a-f]+|\d+)$", src)
    return int(m.group(1), 0) if m else None


def cmd_sites(units, code):
    # the dispatcher's own six pushes: the INDEX is the FIRST push, i.e. the 6th from the call
    rows = callers(units, code, 0x25, 6, 0x0000)
    for n, at, kind, src in rows:
        v = immediate(src)
        print(f"{n:04X}+{at:04X} {kind:4} index <- {src}" + (f"   = {v} (in 1..139: {1 <= v <= 139})" if v is not None else ""))
    print(f"{sum(1 for r in rows if r[2] == 'far')} far sites, {sum(1 for r in rows if r[2] == 'near')} near sites")


def cmd_callers(units, code, stub, k, near):
    rows = callers(units, code, stub, k, near)
    vals = [immediate(r[3]) for r in rows]
    for (n, at, kind, src), v in zip(rows, vals):
        print(f"{n:04X}+{at:04X} {kind:4} <- {src}" + (f" = {v}" if v is not None else ""))
    imm = [v for v in vals if v is not None]
    print(f"{len(rows)} sites: {len(imm)} immediates (min {min(imm) if imm else '-'}, max {max(imm) if imm else '-'}, outside 1..139: {sorted(v for v in set(imm) if not 1 <= v <= 139)}), {len(rows) - len(imm)} not an immediate")


def cmd_writers(units, code, disp):
    pat = re.compile(r"\[[a-z ]*\+ 0x%x\]" % disp)
    for n, c in code.items():
        starts = [i for i in range(len(c) - 3) if c[i:i + 3] == bytes([0x55, 0x89, 0xE5])]
        seen = set()
        for a, b in zip(starts, starts[1:] + [len(c)]):
            for i in MD.disasm(c[a:b], a):
                if i.address in seen:
                    continue
                seen.add(i.address)
                if pat.search(i.op_str.split(",")[0]) and i.mnemonic in ("mov", "add", "sub", "inc", "dec", "and", "or", "xor", "not", "neg", "shl", "shr"):
                    print(f"{n:04X}+{i.address:04X}: {i.mnemonic} {i.op_str}")


def image():
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import u9e_dispatch_0000_audit as audit
    return bytes(audit.source()["image"])


def cmd_tables(units, code):
    img = image()
    ds = DS_PARAGRAPH * 16
    for name, base, n in (("DS:0113h+i (unit 0x1E +03B1, i = 1..6)", 0x113, 6), ("DS:0C13h+i (unit 0x9E +1032, i = 1..16)", 0xC13, 16), ("DS:0C23h+i (unit 0x9E +1069, i = 1..2)", 0xC23, 2)):
        vals = [img[ds + base + i] for i in range(1, n + 1)]
        print(f"{name}: {vals}  all in 1..139: {all(1 <= v <= 139 for v in vals)}")
    # writers: absolute-address stores into those ranges in the resident image's code and in every overlay unit (DS-relative, not ES-prefixed)
    ranges = [(0x114, 0x119), (0xC14, 0xC24)]
    pat = re.compile(r"\[(?:[a-z]{2} \+ )?0x([0-9a-f]+)\]")
    blobs = [("resident", img[:ds])] + [(f"{n:04X}", c) for n, c in code.items()]
    hits = []
    for label, c in blobs:
        starts = [i for i in range(len(c) - 3) if c[i:i + 3] == bytes([0x55, 0x89, 0xE5])]
        seen = set()
        for a, b in zip(starts, starts[1:] + [len(c)]):
            for i in MD.disasm(c[a:b], a):
                if i.address in seen:
                    continue
                seen.add(i.address)
                dst = i.op_str.split(",")[0]
                m = pat.search(dst)
                if m and i.mnemonic in ("mov", "add", "sub", "inc", "dec", "and", "or", "xor", "not", "neg", "shl", "shr", "stosb", "movsb"):
                    v = int(m.group(1), 16)
                    if any(lo <= v <= hi for lo, hi in ranges):
                        hits.append((label, i.address, i.mnemonic, i.op_str, "es:" in dst))
    ds_hits = [h for h in hits if not h[4]]
    print(f"stores into those ranges: {len(hits)} found, {len(ds_hits)} DS-relative (the rest carry an ES override: character-record fields +114h..+119h, not these tables)")
    for h in ds_hits:
        print("  ", h[:4])


def cmd_items(units, code):
    import glob
    import os
    import ecl_walk
    d = ROOT / "resources/DOS/POOLRAD"
    counts = {o: Counter() for o in (0x3C, 0x3D, 0x3E)}
    n = 0
    for f in sorted(glob.glob(str(d / "*.ITM"))):
        data = open(f, "rb").read()
        assert len(data) % 63 == 0, f
        for k in range(len(data) // 63):
            n += 1
            for o in counts:
                counts[o][data[k * 63 + o]] += 1
    ni = n
    for f in sorted(glob.glob(str(d / "MON*ITM.DAX"))):
        for _bid, data in ecl_walk.read_dax(f):
            assert len(data) % 63 == 0, os.path.basename(f)
            for k in range(len(data) // 63):
                n += 1
                for o in counts:
                    counts[o][data[k * 63 + o]] += 1
    print(f"{ni} items in the shipped .ITM files, {n - ni} in MON*ITM.DAX, {n} in all (63-byte records)")
    for o, c in counts.items():
        print(f"+{o:02X}: {sorted(c.items())}")
    print("+3E values with the high bit set (the only ones the unit-0x77 sites dispatch):", sorted(v for v in counts[0x3E] if v > 0x7F))


def cmd_inputs():
    import glob
    import hashlib
    d = ROOT / "resources/DOS/POOLRAD"
    names = ["START.EXE", "GAME.OVR"] + sorted(Path(f).name for f in glob.glob(str(d / "*.ITM"))) + sorted(Path(f).name for f in glob.glob(str(d / "MON*ITM.DAX")))
    for n in names:
        print(f"{hashlib.sha256((d / n).read_bytes()).hexdigest()}  {n}")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("sites")
    c = sub.add_parser("callers"); c.add_argument("stub", type=lambda s: int(s, 16)); c.add_argument("k", type=int); c.add_argument("near", type=lambda s: int(s, 16))
    w = sub.add_parser("writers"); w.add_argument("disp", type=lambda s: int(s, 16))
    sub.add_parser("tables"); sub.add_parser("items"); sub.add_parser("inputs")
    a = p.parse_args()
    if a.cmd == "inputs":
        cmd_inputs()
        return
    units, code = load()
    if a.cmd == "sites":
        cmd_sites(units, code)
    elif a.cmd == "callers":
        cmd_callers(units, code, a.stub, a.k, a.near)
    elif a.cmd == "writers":
        cmd_writers(units, code, a.disp)
    elif a.cmd == "tables":
        cmd_tables(units, code)
    elif a.cmd == "items":
        cmd_items(units, code)


if __name__ == "__main__":
    main()
