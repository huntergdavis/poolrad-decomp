#!/usr/bin/env python3
"""Inventory of the Turbo Pascal overlay system in the DOS Pool of Radiance v1.3 build.

Reads START.EXE (resident loader) and GAME.OVR (FBOV overlay file) and emits:
  * the MZ and FBOV header facts,
  * every overlay unit descriptor found in the resident image
    (stub paragraph, file offset, code size, fixup-table size, entry count),
  * the entry stubs of each unit (entry index -> code offset inside the unit),
  * the decoded fixup table of each unit (offsets of segment words to relocate),
  * a prologue-based function list per unit (55 89 E5 = push bp; mov bp,sp).

Only offsets, sizes, and counts are written. No game bytes are copied to the output.

Descriptor layout (observed on this build, consistent with Turbo Pascal 5.x overlays):
  +0  CD 3F          int 3Fh trap
  +2  u16 = 0        "not loaded" marker
  +4  u32            file offset of the unit's code in GAME.OVR
  +8  u16            code size
  +10 u16            fixup table size (bytes; table follows the code in GAME.OVR)
  +12 u16            entry count
  +14 .. +21         loader bookkeeping (not needed here)
  +22 or +23        entry stubs, 5 bytes each: CD 3F, u16 entry offset, 00 (header length varies)

Usage:
  overlay_inventory.py --exe START.EXE --ovr GAME.OVR --json out.json --md out.md
"""
import argparse, hashlib, json, re, struct, sys

PROLOGUE = b"\x55\x89\xe5"


def sha256(b):
    return hashlib.sha256(b).hexdigest()


def parse_mz(exe):
    f = struct.unpack_from("<HHHHHHHHHHHHHH", exe, 0)
    if exe[:2] != b"MZ":
        raise SystemExit("START.EXE: not an MZ executable")
    hdr = f[4] * 16
    size = (f[2] - 1) * 512 + f[1] if f[1] else f[2] * 512
    return dict(header_bytes=hdr, relocations=f[3], image_size=size - hdr, declared_size=size,
                cs=f[11], ip=f[10], ss=f[7], sp=f[8])


def parse_fbov(ovr):
    if ovr[:4] != b"FBOV":
        raise SystemExit("GAME.OVR: missing FBOV magic")
    return dict(magic="FBOV", payload_size=struct.unpack_from("<I", ovr, 4)[0], file_size=len(ovr))


def find_units(img, ovr):
    units = []
    for m in re.finditer(rb"\xcd\x3f\x00\x00", img):
        d = m.start()
        off, code, fix, ent = struct.unpack_from("<IHHH", img, d + 4)
        if not (0 < off < len(ovr) and code and off + code + fix <= len(ovr) and ent <= 512):
            continue
        entries, stub_base = None, None
        for base in (22, 23, 24):          # header length varies between units on this build
            cand = []
            for i in range(ent):
                s = d + base + 5 * i
                if img[s:s + 2] != b"\xcd\x3f":
                    cand = None
                    break
                cand.append(struct.unpack_from("<H", img, s + 2)[0])
            if cand is not None:
                entries, stub_base = cand, d + base
                break
        if entries is None:
            continue
        fix_tab = ovr[off + code: off + code + fix]
        fixups = [struct.unpack_from("<H", fix_tab, i)[0] for i in range(0, len(fix_tab) - 1, 2)]
        codeb = ovr[off: off + code]
        kinds = {"lcall_segment": 0, "mov_ax_imm16": 0, "other": 0}
        for fx in fixups:
            if fx >= 3 and codeb[fx - 3] == 0x9A:
                kinds["lcall_segment"] += 1
            elif fx >= 1 and codeb[fx - 1] == 0xB8:
                kinds["mov_ax_imm16"] += 1
            else:
                kinds["other"] += 1
        pro = [p.start() for p in re.finditer(re.escape(PROLOGUE), codeb)]
        funcs = []
        for i, p in enumerate(pro):
            end = pro[i + 1] if i + 1 < len(pro) else code
            funcs.append(dict(rel=p, ovr=off + p, size_upper_bound=end - p))
        units.append(dict(
            stub_para=d >> 4, descriptor_offset=d, stub_base=stub_base, stub_header_len=stub_base - d,
            ovr_offset=off, code_size=code, fixup_size=fix, entry_count=ent,
            entries=entries, fixups=fixups, fixup_kinds=kinds,
            functions=funcs, code_sha256=sha256(codeb)))
    units.sort(key=lambda u: u["ovr_offset"])
    return units


def coverage(units, ovr_len):
    gaps = []
    for a, b in zip(units, units[1:]):
        end = a["ovr_offset"] + a["code_size"] + a["fixup_size"]
        if end != b["ovr_offset"]:
            gaps.append(dict(after_unit=hex(a["stub_para"]), end=end, next=b["ovr_offset"]))
    last = units[-1]
    return dict(first_unit_offset=units[0]["ovr_offset"],
                last_unit_end=last["ovr_offset"] + last["code_size"] + last["fixup_size"],
                file_size=ovr_len, gaps=gaps,
                code_total=sum(u["code_size"] for u in units),
                fixup_total=sum(u["fixup_size"] for u in units),
                entry_total=sum(u["entry_count"] for u in units),
                function_total=sum(len(u["functions"]) for u in units))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--ovr", required=True)
    ap.add_argument("--json")
    ap.add_argument("--md")
    a = ap.parse_args()
    exe = open(a.exe, "rb").read()
    ovr = open(a.ovr, "rb").read()
    mz = parse_mz(exe)
    img = exe[mz["header_bytes"]:]
    fb = parse_fbov(ovr)
    units = find_units(img, ovr)
    if not units:
        raise SystemExit("no overlay descriptors found")
    cov = coverage(units, len(ovr))
    out = dict(inputs=dict(start_exe_sha256=sha256(exe), start_exe_size=len(exe),
                           game_ovr_sha256=sha256(ovr), game_ovr_size=len(ovr)),
               mz=mz, fbov=fb, coverage=cov, units=units,
               total_prologues_in_ovr=len(re.findall(re.escape(PROLOGUE), ovr)))
    if a.json:
        json.dump(out, open(a.json, "w"), indent=1)
    lines = ["# Overlay inventory", "",
             f"START.EXE sha256 `{out['inputs']['start_exe_sha256']}` ({len(exe)} bytes), "
             f"MZ relocations {mz['relocations']}, header {mz['header_bytes']} bytes, "
             f"resident image {mz['image_size']} bytes.", "",
             f"GAME.OVR sha256 `{out['inputs']['game_ovr_sha256']}` ({len(ovr)} bytes), "
             f"FBOV payload {fb['payload_size']} bytes.", "",
             f"Units {len(units)}, entry points {cov['entry_total']}, code {cov['code_total']} bytes, "
             f"fixup tables {cov['fixup_total']} bytes, prologue functions {cov['function_total']} "
             f"(prologues anywhere in file: {out['total_prologues_in_ovr']}). "
             f"Units tile the file from {cov['first_unit_offset']} to {cov['last_unit_end']} of "
             f"{len(ovr)}; gaps: {len(cov['gaps'])}.", "",
             "| stub para | ovr offset | code | fixups | entries | functions | fixup kinds (lcall/mov ax/other) |",
             "| --- | ---: | ---: | ---: | ---: | ---: | --- |"]
    for u in sorted(units, key=lambda u: -u["code_size"]):
        k = u["fixup_kinds"]
        lines.append(f"| 0x{u['stub_para']:x} | {u['ovr_offset']} | {u['code_size']} | {len(u['fixups'])} | "
                     f"{u['entry_count']} | {len(u['functions'])} | {k['lcall_segment']}/{k['mov_ax_imm16']}/{k['other']} |")
    md = "\n".join(lines) + "\n"
    if a.md:
        open(a.md, "w").write(md)
    print(md if not a.md else lines[6])


if __name__ == "__main__":
    main()
