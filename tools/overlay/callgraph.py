#!/usr/bin/env python3
"""Fixup-aware 16-bit call graph for the GAME.OVR overlay units (candidate call sites).

IMPORTANT: near calls are found by scanning raw code bytes for E8 without
instruction-boundary validation, so every near site is a CANDIDATE. Confirmed
far calls are found by walking segment-word fixups and checking for a 9A three
bytes before each fixup. The relocation supports a real far-call operand;
near sites have no such confirmation and their counts are upper bounds.

Consumes the JSON written by overlay_inventory.py plus the two executables and resolves:
  * candidate near calls (E8 rel16) inside a unit to the prologue function containing the target,
  * fixup-confirmed far calls (9A off16 seg16) whose segment word is listed in the fixup table.
    The stored segment is the run-time paragraph of a code segment. For overlaid units the
    inventory supplies each unit's run-time stub segment (memory_segment, with the last unit
    explicitly marked as an inferred segment); entry j of that
    unit is addressed as memory_segment:(0x20 + 5*j). Sites that match resolve to the unit
    entry and its function in GAME.OVR. Other segments are resident code in START.EXE
    (file layout differs from run-time layout, so no file offset is claimed); segment 0xAF8
    (the highest, the Turbo Pascal SYSTEM unit) is tagged "runtime".
  * candidate far sites whose segment word is NOT in the fixup table are reported as
    "far_unrelocated_candidate" (probably not calls at all).

Output: JSON with edges and a summary; optional Markdown summary. Addresses and counts only.

Usage:
  callgraph.py --inventory inv.json --exe START.EXE --ovr GAME.OVR --json cg.json --md cg.md
"""
import argparse, collections, hashlib, json, re, struct

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
    ovr = open(a.ovr, "rb").read()
    exe = open(a.exe, "rb").read()
    inputs = dict(start_exe_sha256=hashlib.sha256(exe).hexdigest(), start_exe_size=len(exe),
                  game_ovr_sha256=hashlib.sha256(ovr).hexdigest(), game_ovr_size=len(ovr))
    if inputs["start_exe_sha256"] != inv["inputs"]["start_exe_sha256"] or inputs["game_ovr_sha256"] != inv["inputs"]["game_ovr_sha256"]:
        raise SystemExit("START.EXE/GAME.OVR digests differ from the inventory; refusing to build a call graph")
    units = inv["units"]
    by_memseg = {u["memory_segment"]: u for u in units if u.get("memory_segment") is not None}
    edges = []
    summ = collections.Counter()
    fanin = collections.Counter()
    seg_hist = collections.Counter()
    for u in units:
        code = ovr[u["ovr_offset"]: u["ovr_offset"] + u["code_size"]]
        fixset = set(u["fixups"])
        for m in re.finditer(rb"\xe8(..)", code, re.DOTALL):
            rel = int.from_bytes(m.group(1), "little", signed=True)
            tgt = m.start() + 3 + rel
            src_f, _ = func_for(u, m.start())
            if 0 <= tgt < len(code):
                f, exact = func_for(u, tgt)
                edges.append(dict(kind="near_candidate", unit=u["stub_para"], site=m.start(), src_func=src_f,
                                  target=tgt, target_func=f, exact_prologue=exact))
                summ["near_candidate"] += 1
                summ["near_candidate_exact_prologue"] += exact
                fanin[(u["stub_para"], f)] += 1
            else:
                edges.append(dict(kind="near_candidate_out_of_unit", unit=u["stub_para"], site=m.start(), target=tgt))
                summ["near_candidate_out_of_unit"] += 1
        for segment_word in sorted(fixset):
            site = segment_word - 3
            if site < 0 or site + 5 > len(code) or code[site] != 0x9a:
                continue
            off = int.from_bytes(code[site + 1:site + 3], "little")
            seg = int.from_bytes(code[site + 3:site + 5], "little")
            src_f, _ = func_for(u, site)
            e = dict(kind="far_fixup_confirmed", unit=u["stub_para"], site=site, src_func=src_f, seg=seg, off=off)
            summ["far_fixup_confirmed"] += 1
            seg_hist[seg] += 1
            tu = by_memseg.get(seg)
            if tu is not None and off >= 0x20 and (off - 0x20) % 5 == 0 and (off - 0x20) // 5 < tu["entry_count"]:
                idx = (off - 0x20) // 5
                eoff = tu["entries"][idx]
                f, exact = func_for(tu, eoff)
                e.update(resolution="overlay", target_unit=tu["stub_para"], target_entry=idx,
                         target_rel=eoff, target_ovr=tu["ovr_offset"] + eoff, target_func=f,
                         exact_prologue=exact)
                summ["far_overlay"] += 1
                summ["far_overlay_exact_prologue"] += exact
                fanin[(tu["stub_para"], f)] += 1
            elif tu is not None:
                e["resolution"] = "overlay_segment_bad_offset"
                summ["far_overlay_segment_bad_offset"] += 1
            elif seg == RUNTIME_SEG:
                e["resolution"] = "runtime"
                summ["far_runtime"] += 1
            else:
                e["resolution"] = "resident"
                summ["far_resident"] += 1
            edges.append(e)
        for m in re.finditer(rb"\x9a(..)(..)", code, re.DOTALL):
            site = m.start()
            if site + 3 in fixset:
                continue
            src_f, _ = func_for(u, site)
            edges.append(dict(kind="far_candidate", unit=u["stub_para"], site=site,
                              src_func=src_f, seg=int.from_bytes(m.group(2), "little"),
                              off=int.from_bytes(m.group(1), "little"),
                              resolution="unrelocated_candidate"))
            summ["far_unrelocated_candidate"] += 1
    top = fanin.most_common(12)
    xunit = collections.Counter((e["unit"], e["target_unit"]) for e in edges if e.get("resolution") == "overlay")
    out = dict(inputs=inputs, summary=dict(summ), far_segment_histogram={hex(k): v for k, v in seg_hist.most_common()},
               top_fan_in=[dict(unit=k[0], func_rel=k[1], callers=v) for k, v in top],
               cross_unit_edges=[dict(src=k[0], dst=k[1], n=v) for k, v in xunit.most_common()],
               edges=edges)
    if a.json:
        json.dump(out, open(a.json, "w"), indent=1)
    lines = ["# Fixup-aware call graph summary (candidate call sites)", "",
             "Confirmed far-call sites come from segment-word fixups with a 9A three bytes earlier. "
             "Unrelocated 9A and near E8 sites come from byte scans without instruction-boundary "
             "validation; their counts are candidates only.", "",
             "| measure | count |", "| --- | ---: |"]
    for k, v in sorted(summ.items()):
        lines.append(f"| {k} | {v} |")
    lines += ["", "Top far-call segments (fixup-confirmed sites only): " +
              ", ".join(f"0x{k:x}: {v}" for k, v in seg_hist.most_common(10)), "",
              "| fan-in rank | unit | function rel | candidate callers |", "| ---: | --- | --- | ---: |"]
    for i, (k, v) in enumerate(top, 1):
        lines.append(f"| {i} | 0x{k[0]:x} | 0x{k[1]:x} | {v} |")
    lines += ["", "| src unit | dst unit | fixup-confirmed far edges |", "| --- | --- | ---: |"]
    for k, v in xunit.most_common(20):
        lines.append(f"| 0x{k[0]:x} | 0x{k[1]:x} | {v} |")
    md = "\n".join(lines) + "\n"
    if a.md:
        open(a.md, "w").write(md)
    print(json.dumps(dict(summ), indent=1))


if __name__ == "__main__":
    main()
