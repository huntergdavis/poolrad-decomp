#!/usr/bin/env python3
"""Per-address overlay routine ledger with evidence grades (issue #26).

Inputs: the private JSON of overlay_inventory.py and callgraph.py, optionally the JSON of
validate_near_calls.py, and GAME.OVR for Capstone-based features. Output: a CSV with one
row per Pascal-prologue candidate (stable `unit:offset` id) and a Markdown summary with
measured counts. Only offsets, counts and hashes are written; no game bytes.

Evidence columns (1 = present):
  entry        a public entry slot points exactly at this prologue
  entry_alias  a public entry slot word equals this prologue offset + 0x800 (raw word kept)
  far_called   a fixup-confirmed far call targets the entry slot of this prologue
  near_called  a direct near call targets this prologue; counted from the raw exact-prologue
               scan, which is promoted to decoded evidence only when the validate_near_calls.py
               summary (given with --near-calls) proves the raw and decoded sets identical
  far_called / procptr target slots that are +0x800 aliases are resolved to the prologue first
  All given input files must match the inventory's SHA-256 digests or no output is written.
  procptr      a `mov dx,seg / mov ax,offset` far-pointer constant names this prologue's
               entry slot (procedure variables and dispatch tables)
Grade: A = entry or far_called; B = near_called only; C = procptr only; D = entry_alias
only; U = none.

Feature columns come from a bounded linear decode from the prologue to the next prologue
(except hash-checked executable-span corrections documented in QUEUE_METADATA_REPAIR.md)
(Capstone 5, 16-bit): decoded bytes, instruction count, far calls by segment class
(runtime 0xAF8, overlay stub segments, other resident), direct near calls, indirect calls
through memory, distinct DS-relative global operands, ES-relative operands, return form.
Measured behavior class (not a semantic judgement):
  state    touches at least one DS global
  glue     no DS global but makes calls (sub-class runtime-only when all calls go to 0xAF8)
  leaf     no calls, no DS globals, no ES operands
"""
import argparse, collections, csv, hashlib, json, re, struct

try:
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
except ImportError:
    Cs = None

RUNTIME_SEG = 0xAF8
ALIAS_DELTA = 0x800

# Reviewed executable ends, not a heuristic to stop at the first RETF: other
# routines can have live branch targets after an early return. Hashes bind
# these exceptions to the original bodies; no trailing string data is decoded.
EXECUTABLE_SPANS = {
    (0x1C, 0x271F): (0x272D, "aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1"),
    (0x38, 0x1363): (0x1382, "4850fb98f51666178d3b6e8fc612f3a2afa80e355a7fa13cafb56073481a58c2"),
    (0x38, 0x1730): (0x175E, "402e83b7c5036596f9af8cb3da21ea77519e58cca195e5385fc88d593e0e2f25"),
    (0x38, 0x25BC): (0x2605, "4921365d3d512d870ac24085a63d408b38ab60e6f98e4136f583a7c5c78fe753"),
    (0x38, 0x2EDA): (0x2F4B, "e75c9f7ebfcd4235a1a5a11b6d77ae544c0bd64a3c3f8ade8c5fac454757ecd3"),
    (0x86, 0x3092): (0x3097, "fc1e97b8fe4158fdf9a46de4d3a1b4a04602fb787f6793778bc6d2b7a758d780"),
    (0x6A, 0x0824): (0x08BF, "c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7"),
    (0x6A, 0x0E37): (0x0EBE, "5f63f1823287894ede57fcef122307f3e296293a633e530a1836f2f36d389234"),
    (0x7D, 0x047A): (0x0563, "471d0464b3c216b69f1c3188b7a8aa2cc69ca1f81109de5217e3f4dd574da8b7"),
    (0x7D, 0x05F0): (0x06D8, "ac3eb0a915fa30d77b06b262429879f2400d3b8584b6e97ba413dde924709c7d"),
    (0x9E, 0x115C): (0x1287, "e122e18cccee13655b750e38c4d5e6eb664fca16b125cc72bcd724580af6fb8b"),
}


def executable_end(unit, start, proposed_end, code):
    correction = EXECUTABLE_SPANS.get((unit, start))
    if correction is None:
        return proposed_end
    end, digest = correction
    if not start < end <= proposed_end <= len(code):
        raise ValueError(f"reviewed span outside candidate bounds: {unit:04X}:{start:04X}")
    if hashlib.sha256(code[start:end]).hexdigest() != digest:
        raise ValueError(f"reviewed executable span hash differs: {unit:04X}:{start:04X}")
    return end


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--inventory", required=True)
    ap.add_argument("--callgraph", required=True)
    ap.add_argument("--near-calls", help="JSON from validate_near_calls.py (optional)")
    ap.add_argument("--ovr", help="GAME.OVR for Capstone features (optional)")
    ap.add_argument("--exe", help="START.EXE for stub-byte checks (optional)")
    ap.add_argument("--csv", required=True)
    ap.add_argument("--md", required=True)
    a = ap.parse_args()
    inv = json.load(open(a.inventory))
    cg = json.load(open(a.callgraph))
    expected = inv["inputs"]
    ovr = open(a.ovr, "rb").read() if a.ovr else None
    exe = open(a.exe, "rb").read() if a.exe else None
    if ovr is not None and hashlib.sha256(ovr).hexdigest() != expected["game_ovr_sha256"]:
        raise SystemExit("GAME.OVR digest differs from the inventory; refusing to emit a ledger")
    if exe is not None and hashlib.sha256(exe).hexdigest() != expected["start_exe_sha256"]:
        raise SystemExit("START.EXE digest differs from the inventory; refusing to emit a ledger")
    nc = None
    if a.near_calls:
        nc = json.load(open(a.near_calls))
        nc_inputs = nc.get("inputs", {})
        for key in ("start_exe_sha256", "game_ovr_sha256"):
            if key not in nc_inputs:
                raise SystemExit(f"near-call validation JSON lacks input digest {key}")
            if nc_inputs[key] != expected[key]:
                raise SystemExit(f"near-call validation input {key} differs from the inventory")
        raw_exact_sites = sum(1 for e in cg["edges"] if e["kind"] == "near_candidate" and e.get("exact_prologue"))
        if nc.get("raw_near_call_candidates_to_prologues") != raw_exact_sites:
            raise SystemExit("near-call validation raw exact-prologue site total differs from this call graph")
        invariants = (nc.get("invalid_seed_paths") == 0 and nc.get("truncated_seed_paths") == 0
                      and nc.get("raw_exact_not_decoded") == 0 and nc.get("decoded_exact_not_raw") == 0
                      and nc.get("decoded_near_calls_to_prologues") == nc.get("raw_near_call_candidates_to_prologues"))
        if not invariants:
            raise SystemExit("near-call validation invariants not met; raw exact-prologue edges cannot be promoted")
    units = inv["units"]
    by_para = {u["stub_para"]: u for u in units}
    by_seg = {u["memory_segment"]: u for u in units if u.get("memory_segment") is not None}
    overlay_segs = set(by_seg)
    prologues = {(u["stub_para"], f["rel"]) for u in units for f in u["functions"]}

    entry_of = {}
    aliases = []
    for u in units:
        starts = {f["rel"] for f in u["functions"]}
        for j, word in enumerate(u["entries"]):
            if word in starts:
                entry_of[(u["stub_para"], word)] = j
            elif (word - ALIAS_DELTA) in starts:
                aliases.append(dict(unit=u["stub_para"], entry=j, raw_word=word,
                                    normalized=word - ALIAS_DELTA, in_code=word < u["code_size"]))
    alias_of = {(x["unit"], x["normalized"]): x for x in aliases}

    alias_raw = {(x["unit"], x["raw_word"]): x["normalized"] for x in aliases}

    def normalize(unit, rel):
        return (unit, alias_raw.get((unit, rel), rel))

    far_called = collections.Counter()
    far_called_via_alias = 0
    for e in cg["edges"]:
        if e.get("resolution") == "overlay":
            key = normalize(e["target_unit"], e["target_rel"])
            far_called[key] += 1
            far_called_via_alias += (key[1] != e["target_rel"])
    # Raw exact-prologue near edges are promoted to validated evidence only when the
    # validation summary proved them identical to the Capstone-decoded call set above.
    near_called = collections.Counter()
    for e in cg["edges"]:
        if e["kind"] == "near_candidate" and e.get("exact_prologue"):
            near_called[(e["unit"], e["target"])] += 1
    near_source = "decoded (validate_near_calls.py invariants hold)" if nc else "raw byte scan, unvalidated"

    md = Cs(CS_ARCH_X86, CS_MODE_16) if (ovr is not None and Cs is not None) else None
    procptr = collections.Counter()
    procptr_unresolved = []
    features = {}
    if md is not None:
        for u in units:
            code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
            pro = sorted(f["rel"] for f in u["functions"])
            fixset = {f for f in u["fixups"] if not (f >= 3 and code[f - 3] == 0x9A)}
            for i, rel in enumerate(pro):
                end = pro[i + 1] if i + 1 < len(pro) else len(code)
                end = executable_end(u["stub_para"], rel, end, code)
                ins = list(md.disasm(code[rel:end], rel))
                f_rt = f_ovl = f_res = ind = near = 0
                for k, x in enumerate(ins):
                    if x.mnemonic == "lcall":
                        if "[" in x.op_str:
                            ind += 1
                            continue
                        seg = int(x.op_str.split(", ")[0], 0)
                        if seg == RUNTIME_SEG:
                            f_rt += 1
                        elif seg in overlay_segs:
                            f_ovl += 1
                        else:
                            f_res += 1
                    elif x.mnemonic == "call":
                        if "[" in x.op_str:
                            ind += 1
                        else:
                            near += 1
                    if x.mnemonic == "mov" and x.op_str.startswith("dx, 0x") and (x.address + 1) in fixset:
                        seg = int(x.op_str.split(", ")[1], 0)
                        off = None
                        for y in ins[max(0, k - 3):k + 4]:
                            if y.mnemonic == "mov" and y.op_str.startswith("ax, "):
                                v = y.op_str.split(", ")[1]
                                if v.startswith("0x") or v.isdigit():
                                    off = int(v, 0)
                        tu = by_seg.get(seg)
                        if tu is not None and off is not None and off >= 0x20 and (off - 0x20) % 5 == 0 \
                                and (off - 0x20) // 5 < tu["entry_count"]:
                            procptr[normalize(tu["stub_para"], tu["entries"][(off - 0x20) // 5])] += 1
                        else:
                            procptr_unresolved.append(dict(unit=u["stub_para"], site=x.address, seg=seg, off=off))
                ops = " ".join(x.op_str for x in ins)
                ds_globals = len(set(re.findall(r"(?<!bp )\[(0x[0-9a-f]{4})\]", ops)))
                es_refs = sum(1 for x in ins if "es:" in x.op_str)
                rets = [x for x in ins if x.mnemonic.startswith("ret")]
                features[(u["stub_para"], rel)] = dict(
                    bound=end - rel, decoded=sum(x.size for x in ins), insns=len(ins),
                    far_runtime=f_rt, far_overlay=f_ovl, far_resident=f_res, near=near,
                    indirect=ind, ds_globals=ds_globals, es_refs=es_refs,
                    ret=(rets[-1].mnemonic + " " + rets[-1].op_str).strip() if rets else "none")

    rows = []
    grades = collections.Counter()
    classes = collections.Counter()
    for u in units:
        for f in sorted(u["functions"], key=lambda f: f["rel"]):
            key = (u["stub_para"], f["rel"])
            ev = dict(entry=int(key in entry_of), entry_alias=int(key in alias_of),
                      far_called=far_called.get(key, 0), near_called=near_called.get(key, 0),
                      procptr=procptr.get(key, 0))
            if ev["entry"] or ev["far_called"]:
                grade = "A"
            elif ev["near_called"]:
                grade = "B"
            elif ev["procptr"]:
                grade = "C"
            elif ev["entry_alias"]:
                grade = "D"
            else:
                grade = "U"
            ft = features.get(key, {})
            cls = ""
            if ft:
                calls = ft["far_runtime"] + ft["far_overlay"] + ft["far_resident"] + ft["near"] + ft["indirect"]
                if ft["ds_globals"] > 0:
                    cls = "state"
                elif calls > 0:
                    cls = "glue-runtime-only" if calls == ft["far_runtime"] else "glue"
                elif ft["es_refs"] == 0:
                    cls = "leaf"
                else:
                    cls = "leaf-es"
            grades[grade] += 1
            classes[cls] += 1
            rows.append(dict(id=f"{u['stub_para']:#x}:{f['rel']:#06x}", unit=f"{u['stub_para']:#x}",
                             offset=f"{f['rel']:#06x}", ovr_offset=u["ovr_offset"] + f["rel"],
                             memory_segment=(f"{u['memory_segment']:#x}" if u.get("memory_segment") is not None else ""),
                             entry_slot=entry_of.get(key, ""), raw_alias_word=(f"{alias_of[key]['raw_word']:#x}" if key in alias_of else ""),
                             grade=grade, **ev, cls=cls, **ft))
    with open(a.csv, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)

    # stub fifth-byte check (metadata only)
    fifth = collections.Counter()
    odd_stubs = []
    if exe is not None:
        img = exe[inv["mz"]["header_bytes"]:]
        for u in units:
            for j in range(u["entry_count"]):
                s = u["stub_base"] + 5 * j
                b = img[s + 4]
                fifth[b] += 1
                if b != 0:
                    odd_stubs.append((f"{u['stub_para']:#x}", j, u["entry_count"]))

    lines = ["# Overlay routine ledger summary (generated)", "",
             f"Inputs: START.EXE `{inv['inputs']['start_exe_sha256']}`, GAME.OVR `{inv['inputs']['game_ovr_sha256']}`.",
             f"Rows: {len(rows)} prologue candidates in {len(units)} units; features via Capstone: {'yes' if features else 'no'};"
             f" near-call evidence: {near_source}. Input digests verified against the inventory for every file given.", "",
             "| grade | meaning | count |", "| --- | --- | ---: |",
             f"| A | public entry slot on the prologue, or fixup-confirmed far call to it (alias slots resolved to their prologue: {far_called_via_alias} such calls) | {grades['A']} |",
             f"| B | direct near-call target only ({near_source}) | {grades['B']} |",
             f"| C | procedure-pointer constant target only | {grades['C']} |",
             f"| D | public entry alias (slot word = offset + 0x800) only | {grades['D']} |",
             f"| U | no listed evidence | {grades['U']} |", "",
             f"Overlapping evidence: entries on prologues {sum(1 for r in rows if r['entry'])}, far-called {sum(1 for r in rows if r['far_called'])}, "
             f"near-called {sum(1 for r in rows if r['near_called'])}, procedure-pointer targets {sum(1 for r in rows if r['procptr'])} "
             f"({sum(procptr.values())} constant sites; {len(procptr_unresolved)} unresolved), aliases {len(aliases)}.", "",
             "| public entry aliases | unit | slot | raw word | normalized offset | raw word inside code? |", "| --- | --- | ---: | --- | --- | --- |"]
    for x in aliases:
        lines.append(f"| | {x['unit']:#x} | {x['entry']} | {x['raw_word']:#x} | {x['normalized']:#x} | {'yes' if x['in_code'] else 'no'} |")
    lines += ["", "| measured class | count |", "| --- | ---: |"]
    for k, v in sorted(classes.items()):
        lines.append(f"| {k or 'n/a'} | {v} |")
    if features:
        lt = sum(1 for r in rows if r.get("decoded", 0) < r.get("bound", 0))
        indirect = sum(1 for r in rows if r.get("indirect", 0))
        lines += ["", f"Functions whose linear decode stops before the next prologue: {lt}; functions with indirect calls through memory: {indirect}."]
    if a.exe:
        lines += ["", f"Entry stub fifth byte: {fifth.get(0, 0)} zero, {sum(v for k, v in fifth.items() if k)} non-zero"
                  + (f" (unit, slot, entries): {odd_stubs}" if odd_stubs else "") + "."]
    open(a.md, "w").write("\n".join(lines) + "\n")
    print("\n".join(lines[5:16]))


if __name__ == "__main__":
    main()
