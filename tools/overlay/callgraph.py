#!/usr/bin/env python3
"""Fixup-aware 16-bit call graph for the GAME.OVR overlay units.

Consumes the JSON written by overlay_inventory.py plus the two executables and resolves:
  * near calls (E8 rel16) inside a unit to the prologue function that contains the target,
  * far calls (9A off16 seg16) whose segment word is listed in the unit's fixup table.
    The stored segment is a paragraph relative to the resident program image, so the
    linear target is seg*16+off inside START.EXE's load image. If that linear address is
    the start of an entry stub of some unit, the edge is resolved to that unit's entry
    (and to its function in GAME.OVR). Otherwise the target is resident code; segment
    0xAF8 (the highest segment, the Turbo Pascal SYSTEM unit) is tagged "runtime".
  * far calls whose segment word is NOT in the fixup table are reported as "unrelocated"
    (the model does not apply to them).

Output: JSON with edges and a summary; optional Markdown summary. Addresses and counts only.

Usage:
  callgraph.py --inventory inv.json --exe START.EXE --ovr GAME.OVR --json cg.json --md cg.md
"""
import argparse, collections, json, re, struct

RUNTIME_SEG = 0xAF8


def func_for(unit, rel):
    """Return (function rel offset, exact) for a code offset inside a unit."""
    best = None
    for f in unit["functions"]:
        if f["rel"] <= rel:
            best = f["rel"]
        else:
            break
    return best, (best == rel)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--inventory", required=True)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--ovr", required=True)
    ap.add_argument("--json")
    ap.add_argument("--md")
    a = ap.parse_args()
    inv = json.load(open(a.inventory))
    exe = open(a.exe, "rb").read()
    ovr = open(a.ovr, "rb").read()
    img = exe[inv["mz"]["header_bytes"]:]
    units = inv["units"]
    by_para = {u["stub_para"]: u for u in units}
    stubs = {}
    for u in units:
        for i, e in enumerate(u["entries"]):
            stubs[u["stub_base"] + 5 * i] = (u["stub_para"], i, e)
    edges = []
    summ = collections.Counter()
    fanin = collections.Counter()
    seg_hist = collections.Counter()
    for u in units:
        code = ovr[u["ovr_offset"]: u["ovr_offset"] + u["code_size"]]
        fixset = set(u["fixups"])
        for m in re.finditer(rb"\xe8(..)", code):
            rel = int.from_bytes(m.group(1), "little", signed=True)
            tgt = m.start() + 3 + rel
            src_f, _ = func_for(u, m.start())
            if 0 <= tgt < len(code):
                f, exact = func_for(u, tgt)
                edges.append(dict(kind="near", unit=u["stub_para"], site=m.start(), src_func=src_f,
                                  target=tgt, target_func=f, exact_prologue=exact))
                summ["near"] += 1
                summ["near_exact_prologue"] += exact
                fanin[(u["stub_para"], f)] += 1
            else:
                edges.append(dict(kind="near_out_of_unit", unit=u["stub_para"], site=m.start(), target=tgt))
                summ["near_out_of_unit"] += 1
        for m in re.finditer(rb"\x9a(..)(..)", code):
            off = int.from_bytes(m.group(1), "little")
            seg = int.from_bytes(m.group(2), "little")
            site = m.start()
            src_f, _ = func_for(u, site)
            e = dict(kind="far", unit=u["stub_para"], site=site, src_func=src_f, seg=seg, off=off)
            if (site + 3) not in fixset:
                e["resolution"] = "unrelocated"
                summ["far_unrelocated"] += 1
            else:
                lin = seg * 16 + off
                seg_hist[seg] += 1
                if lin in stubs:
                    tp, idx, eoff = stubs[lin]
                    tu = by_para[tp]
                    f, exact = func_for(tu, eoff)
                    e.update(resolution="overlay", target_unit=tp, target_entry=idx,
                             target_rel=eoff, target_ovr=tu["ovr_offset"] + eoff, target_func=f,
                             exact_prologue=exact)
                    summ["far_overlay"] += 1
                    summ["far_overlay_exact_prologue"] += exact
                    fanin[(tp, f)] += 1
                else:
                    inside = any(s <= lin < s + 5 for s in stubs)
                    if inside:
                        e["resolution"] = "stub_misaligned"
                        summ["far_stub_misaligned"] += 1
                    elif seg == RUNTIME_SEG:
                        e["resolution"] = "runtime"
                        summ["far_runtime"] += 1
                    elif lin < len(img):
                        e["resolution"] = "resident"
                        summ["far_resident"] += 1
                    else:
                        e["resolution"] = "out_of_image"
                        summ["far_out_of_image"] += 1
                    e["linear"] = lin
            edges.append(e)
    top = fanin.most_common(12)
    xunit = collections.Counter((e["unit"], e["target_unit"]) for e in edges if e.get("resolution") == "overlay")
    out = dict(summary=dict(summ), far_segment_histogram={hex(k): v for k, v in seg_hist.most_common()},
               top_fan_in=[dict(unit=k[0], func_rel=k[1], callers=v) for k, v in top],
               cross_unit_edges=[dict(src=k[0], dst=k[1], n=v) for k, v in xunit.most_common()],
               edges=edges)
    if a.json:
        json.dump(out, open(a.json, "w"), indent=1)
    lines = ["# Fixup-aware call graph summary", "",
             "| measure | count |", "| --- | ---: |"]
    for k, v in sorted(summ.items()):
        lines.append(f"| {k} | {v} |")
    lines += ["", "Top far-call segments (relocated sites only): " +
              ", ".join(f"0x{k:x}: {v}" for k, v in seg_hist.most_common(10)), "",
              "| fan-in rank | unit | function rel | callers |", "| ---: | --- | --- | ---: |"]
    for i, (k, v) in enumerate(top, 1):
        lines.append(f"| {i} | 0x{k[0]:x} | 0x{k[1]:x} | {v} |")
    lines += ["", "| src unit | dst unit | far edges |", "| --- | --- | ---: |"]
    for k, v in xunit.most_common(20):
        lines.append(f"| 0x{k[0]:x} | 0x{k[1]:x} | {v} |")
    md = "\n".join(lines) + "\n"
    if a.md:
        open(a.md, "w").write(md)
    print(json.dumps(dict(summ), indent=1))


if __name__ == "__main__":
    main()
