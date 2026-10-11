#!/usr/bin/env python3
"""Bounded, address-only classification of the 673 overlay routine candidates (issue #26).

Reads the private JSON of overlay_inventory.py and callgraph.py (the latter supplies the
per-candidate fixup-confirmed far-call fan-in and is validated against the candidate set),
the public function ledger CSV for evidence grades, and GAME.OVR for a bounded linear decode
of every prologue candidate. It never claims a game-function denominator; it records measured byte
signals and assigns a class with a confidence from fixed rules:

  game_verified      the routine's behavior is byte-verified in docs/re (translated
                     functions, the ECL interpreter loop, dispatcher and its 49 handlers)
  game_evidenced     it pushes at least one printable Pascal string constant from its own
                     code segment, or touches a documented engine global, or calls a
                     game_verified routine (provisional: text or engine state is evidence of
                     application code, not proof of game logic)
  helper_candidate   no calls or only runtime (0xAF8) calls, no DS globals, no strings
                     (provisional: small utilities; could still be game arithmetic)
  game_or_other_library_unresolved  everything else

Decode audit: bytes of each unit linearly decoded between prologue candidates (this is not
reachable-code coverage), undecoded ranges, a heuristic flag for undecoded ranges that
decode as a self-contained return-terminated routine, and `jmp cs:[table]` dispatch sites.
The tool refuses to run unless the GAME.OVR file, the inventory and call graph JSON all
carry the audited v1.3 digests and the function ledger IDs equal the candidate set.
Outputs contain offsets and counts only.
"""
import argparse, collections, csv, hashlib, json, re

try:
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
except ImportError:
    Cs = None

RUNTIME_SEG = 0xAF8
# The audited DOS v1.3 build. The tool refuses any other file, inventory or call graph.
KNOWN_GAME_OVR_SHA256 = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
KNOWN_START_EXE_SHA256 = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
DOC_GLOBALS = {0x49ED: "ecl_pc", 0x49DE: "ecl_buf", 0x49E0: "ecl_buf", 0x6F7F: "ecl_op", 0x6F7E: "ecl_op",
               0x442E: "ecl_stop", 0x49FF: "ecl_stop", 0x6F70: "ecl_stack", 0x49D2: "win0", 0x49D4: "win0",
               0x49D6: "win1", 0x49D8: "win1", 0x49DA: "win2", 0x49DC: "win2", 0x5D92: "active_char",
               0x5D94: "active_char", 0x5D96: "party", 0x5D98: "party", 0x5E24: "window", 0x5E25: "window",
               0x6B84: "procvar", 0x6B86: "procvar", 0x84DF: "ecl_flag", 0x84E0: "ecl_flag", 0x84E2: "ecl_flag"}
for _i in range(1, 16):
    DOC_GLOBALS[0x7005 + _i] = "slot_tag"
    DOC_GLOBALS[0x7045 + _i] = "slot_hi"
    DOC_GLOBALS[0x7085 + _i] = "slot_lo"
for _i in range(6):
    DOC_GLOBALS[0x6F78 + _i] = "ecl_cmp"

# Byte-verified routines (unit, unit-relative offset): docs/re/FUNCTION_LEDGER.md and
# docs/re/ECL_CONTRACTS.md (interpreter loop, dispatcher, 49 distinct handlers).
# Unit 0x38 entries 8-10, 30 and 31: reviewed full translations in
# FUNCTION_LEDGER.md sections 1.9-1.13 (PRs #41, #44, #47, #50, #54).
# Sixteen more unit 0x38 entries: reviewed complete dispatch-entry translations
# in OVERLAY_UNIT38_EFFECT_ENTRIES.md (PR #63). These are exact prologue offsets.
UNIT38_EFFECT_ENTRIES = {
    (0x38, rel) for rel in (
        0x010D, 0x011D, 0x013B, 0x05D6, 0x064E, 0x06E4, 0x0A3D, 0x0BAC,
        0x0C26, 0x10C1, 0x1452, 0x171F, 0x2490, 0x25A2, 0x2A6D, 0x2B63,
    )
}
# Four more reviewed full dispatch-entry translations in
# OVERLAY_UNIT38_EFFECT_ENTRIES_NEXT.md (PR #69).
UNIT38_EFFECT_ENTRIES_NEXT = {
    (0x38, rel) for rel in (0x01C3, 0x24D9, 0x2700, 0x2A28)
}
# Two further reviewed full dispatch-entry translations in
# OVERLAY_UNIT38_WALK_ENTRIES.md (PR #85).
UNIT38_WALK_ENTRIES = {(0x38, 0x268A), (0x38, 0x2E37)}
# Reviewed full view-parameter setter translation in FUNCTION_LEDGER.md,
# section 1.14 (PR #65); the neighboring script initializer is a model.
VIEW_SETTER = (0x3BD, 0x0155)
ECL_HANDLERS = [0x0001, 0x00F4, 0x0111, 0x0126, 0x01A3, 0x0234, 0x0292, 0x02E0, 0x0450, 0x03B4, 0x0817, 0x0856,
                0x0951, 0x0994, 0x0A09, 0x0A9F, 0x0AED, 0x0F08, 0x0B52, 0x125D, 0x12C6, 0x1435, 0x0BD1, 0x0C74,
                0x1629, 0x16C9, 0x179B, 0x18F8, 0x19B0, 0x1DB1, 0x1EF8, 0x0E5E, 0x10D1, 0x25CB, 0x2CCF, 0x2775,
                0x0DEF, 0x2AA2, 0x266A, 0x2AEB, 0x2AC1, 0x0EBC, 0x2B7E, 0x2E10, 0x2B33, 0x271F, 0x2C03, 0x2E7D,
                0x2B14]
VERIFIED = {(0x1c, 0x3171), (0x1c, 0x2F2D)} | {(0x1c, h) for h in ECL_HANDLERS} | {
    (0x25, 0x0000), (0x25, 0x06AF), (0x25, 0x06D3), (0x25, 0x06FB), (0x25, 0x0E0C), (0x25, 0x0C77),
    (0x25, 0x0F53), (0x25, 0x1153), (0x25, 0x12FB), (0x25, 0x1EA6), (0x36, 0x002D),
    (0x38, 0x0377), (0x38, 0x03AC), (0x38, 0x03E1),
    (0x38, 0x0C06), (0x38, 0x0C15), VIEW_SETTER} | UNIT38_EFFECT_ENTRIES | UNIT38_EFFECT_ENTRIES_NEXT | UNIT38_WALK_ENTRIES


def is_pascal_string(code, off, min_len=1):
    """True when code[off] is a length byte followed by that many printable ASCII bytes."""
    if off < 0 or off + 1 > len(code):
        return False
    n = code[off]
    if n < min_len or off + 1 + n > len(code):
        return False
    return all(32 <= b < 127 for b in code[off + 1:off + 1 + n])


def classify(signals):
    """Fixed rules over measured signals -> (class, confidence)."""
    if signals["is_verified"]:
        return "game_verified", "verified"
    if signals["strings"] or signals["doc_globals"] or signals["ver_calls"]:
        return "game_evidenced", "provisional"
    calls = signals["calls"]
    if signals["ds_globals"] == 0 and signals["strings"] == 0 and (calls == 0 or calls == signals["far_runtime"]):
        return "helper_candidate", "provisional"
    return "game_or_other_library_unresolved", "unresolved"


def decode_unit(md, u, code, overlay_segs, slot_to_rel):
    """Bounded linear decode of each prologue candidate; returns rows and coverage facts."""
    pro = sorted(f["rel"] for f in u["functions"])
    cov = bytearray(len(code))
    rows = []
    jump_tables = 0
    for i, rel in enumerate(pro):
        end = pro[i + 1] if i + 1 < len(pro) else len(code)
        ins = list(md.disasm(code[rel:end], rel))
        strings, docg, ds = set(), set(), set()
        ver_calls = f_rt = f_ovl = f_res = near = ind = 0
        for k, x in enumerate(ins):
            for b in range(x.address, x.address + x.size):
                cov[b] = 1
            if x.mnemonic == "jmp" and "cs:[" in x.op_str:
                jump_tables += 1
            if (x.mnemonic == "mov" and x.op_str.startswith("di, 0x") and k + 2 < len(ins)
                    and ins[k + 1].mnemonic == "push" and ins[k + 1].op_str == "cs"
                    and ins[k + 2].mnemonic == "push" and ins[k + 2].op_str == "di"):
                off = int(x.op_str.split(", ")[1], 0)
                if is_pascal_string(code, off):
                    strings.add(off)
            if "bp" not in x.op_str:
                for g in re.findall(r"\[(0x[0-9a-f]{4})\]", x.op_str):
                    a = int(g, 16)
                    ds.add(a)
                    if a in DOC_GLOBALS:
                        docg.add(DOC_GLOBALS[a])
            if x.mnemonic == "lcall":
                if "[" in x.op_str:
                    ind += 1
                    continue
                seg, off = [int(t, 0) for t in x.op_str.split(", ")]
                if seg == RUNTIME_SEG:
                    f_rt += 1
                elif seg in overlay_segs:
                    f_ovl += 1
                    if slot_to_rel.get((seg, off)) in VERIFIED:
                        ver_calls += 1
                else:
                    f_res += 1
            elif x.mnemonic == "call":
                near += 1
                if x.op_str.startswith("0x") and (u["stub_para"], int(x.op_str, 0)) in VERIFIED:
                    ver_calls += 1
        sig = dict(strings=len(strings), doc_globals=";".join(sorted(docg)), ver_calls=ver_calls,
                   ds_globals=len(ds), calls=f_rt + f_ovl + f_res + near + ind, far_runtime=f_rt,
                   far_overlay=f_ovl, far_resident=f_res, near=near, indirect=ind,
                   is_verified=int((u["stub_para"], rel) in VERIFIED),
                   decoded=sum(x.size for x in ins), bound=end - rel)
        rows.append((rel, sig))
    gaps = []
    i = 0
    while i < len(code):
        if not cov[i]:
            j = i
            while j < len(code) and not cov[j]:
                j += 1
            ins = list(md.disasm(code[i:j], i))
            code_like = bool(ins) and sum(x.size for x in ins) == j - i and any(x.mnemonic.startswith("ret") for x in ins) \
                and ins[0].mnemonic in ("push", "mov", "sub", "xor", "cmp", "les", "jmp")
            gaps.append(dict(unit=u["stub_para"], offset=i, size=j - i, code_like=code_like,
                             before_first_prologue=(j <= pro[0]) if pro else True))
            i = j
        else:
            i += 1
    return rows, dict(covered=int(sum(cov)), gaps=gaps, jump_tables=jump_tables)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--inventory", required=True)
    ap.add_argument("--callgraph", required=True)
    ap.add_argument("--ledger-csv", required=True, help="docs/re/overlay_function_ledger.csv for evidence grades")
    ap.add_argument("--ovr", required=True)
    ap.add_argument("--csv", required=True)
    ap.add_argument("--md", required=True)
    a = ap.parse_args()
    if Cs is None:
        raise SystemExit("Capstone 5 is required")
    inv = json.load(open(a.inventory))
    cg = json.load(open(a.callgraph))
    ovr = open(a.ovr, "rb").read()
    if hashlib.sha256(ovr).hexdigest() != KNOWN_GAME_OVR_SHA256:
        raise SystemExit("GAME.OVR is not the audited v1.3 build; refusing to classify")
    for name, doc in (("inventory", inv), ("callgraph", cg)):
        ins = doc.get("inputs", {})
        if ins.get("game_ovr_sha256") != KNOWN_GAME_OVR_SHA256 or ins.get("start_exe_sha256") != KNOWN_START_EXE_SHA256:
            raise SystemExit(f"{name} JSON was not produced from the audited v1.3 build; refusing to classify")
    units = inv["units"]
    candidate_ids = {f"{u['stub_para']:#x}:{f['rel']:#06x}" for u in units for f in u["functions"]}
    ledger_rows = list(csv.DictReader(open(a.ledger_csv)))
    ledger_ids = {r["id"] for r in ledger_rows}
    if ledger_ids != candidate_ids:
        missing = sorted(candidate_ids - ledger_ids)[:5]
        extra = sorted(ledger_ids - candidate_ids)[:5]
        raise SystemExit(f"function ledger IDs do not match the inventory candidates (missing {len(candidate_ids - ledger_ids)} e.g. {missing}; extra {len(ledger_ids - candidate_ids)} e.g. {extra})")
    grades = {r["id"]: r["grade"] for r in ledger_rows}
    # Concrete use of the call graph: fixup-confirmed far calls into overlay entries give a
    # per-candidate fan-in; every such target must be a candidate (after +0x800 alias
    # normalization) or the inputs disagree.
    alias_raw = {}
    for u in units:
        starts = {f["rel"] for f in u["functions"]}
        for word in u["entries"]:
            if word not in starts and (word - 0x800) in starts:
                alias_raw[(u["stub_para"], word)] = word - 0x800
    far_in = collections.Counter()
    for e in cg["edges"]:
        if e.get("resolution") == "overlay":
            rel = alias_raw.get((e["target_unit"], e["target_rel"]), e["target_rel"])
            rid = f"{e['target_unit']:#x}:{rel:#06x}"
            if rid not in candidate_ids:
                raise SystemExit(f"call graph overlay target {rid} is not an inventory candidate")
            far_in[rid] += 1
    overlay_segs = {u["memory_segment"] for u in units if u.get("memory_segment") is not None}
    slot_to_rel = {}
    for u in units:
        if u.get("memory_segment") is None:
            continue
        for j, e in enumerate(u["entries"]):
            slot_to_rel[(u["memory_segment"], 0x20 + 5 * j)] = (u["stub_para"], e)
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    out_rows = []
    classes = collections.Counter()
    per_class_grade = collections.Counter()
    cov_total = code_total = 0
    gaps = []
    jump_tables = 0
    for u in units:
        code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
        rows, cov = decode_unit(md, u, code, overlay_segs, slot_to_rel)
        code_total += len(code)
        cov_total += cov["covered"]
        gaps += cov["gaps"]
        jump_tables += cov["jump_tables"]
        for rel, sig in rows:
            cls, conf = classify(sig)
            rid = f"{u['stub_para']:#x}:{rel:#06x}"
            classes[cls] += 1
            per_class_grade[(cls, grades.get(rid, "?"))] += 1
            out_rows.append(dict(id=rid, unit=f"{u['stub_para']:#x}", offset=f"{rel:#06x}",
                                 ovr_offset=u["ovr_offset"] + rel, grade=grades[rid],
                                 far_in=far_in.get(rid, 0), classification=cls, confidence=conf, **sig))
    with open(a.csv, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(out_rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(out_rows)
    small = sum(1 for g in gaps if g["size"] <= 8)
    pre = sum(g["size"] for g in gaps if g["before_first_prologue"])
    code_like = sum(1 for g in gaps if g["code_like"])
    lines = ["# Overlay classification summary (generated)", "",
             f"Inputs: GAME.OVR `{inv['inputs']['game_ovr_sha256']}`, START.EXE `{inv['inputs']['start_exe_sha256']}`; "
             f"{len(out_rows)} candidates; grades joined from the function ledger.", "",
             "| classification | confidence | count |", "| --- | --- | ---: |",
             f"| game_verified | verified | {classes['game_verified']} |",
             f"| game_evidenced | provisional | {classes['game_evidenced']} |",
             f"| helper_candidate | provisional | {classes['helper_candidate']} |",
             f"| game_or_other_library_unresolved | unresolved | {classes['game_or_other_library_unresolved']} |", "",
             "Signals (not mutually exclusive): " +
             f"string constants {sum(1 for r in out_rows if r['strings'])} routines ({sum(r['strings'] for r in out_rows)} references), "
             f"documented engine globals {sum(1 for r in out_rows if r['doc_globals'])}, calls into verified routines {sum(1 for r in out_rows if r['ver_calls'])}, "
             f"leaves {sum(1 for r in out_rows if r['calls'] == 0 and r['ds_globals'] == 0)}, runtime-only {sum(1 for r in out_rows if r['calls'] and r['calls'] == r['far_runtime'] and r['ds_globals'] == 0)}.", "",
             "| class | grade A | grade B | grade D |", "| --- | ---: | ---: | ---: |"]
    for cls in ("game_verified", "game_evidenced", "helper_candidate", "game_or_other_library_unresolved"):
        lines.append(f"| {cls} | {per_class_grade[(cls, 'A')]} | {per_class_grade[(cls, 'B')]} | {per_class_grade[(cls, 'D')]} |")
    lines += ["", f"Linear decode: {cov_total} of {code_total} unit code bytes ({100 * cov_total / code_total:.1f}%) linearly decoded between prologue candidates "
              f"(not reachable-code coverage); {len(gaps)} undecoded ranges ({small} of 8 bytes or less; {pre} bytes precede a unit's first prologue); "
              f"the gap heuristic (a range that decodes completely, starts with a plausible opcode and contains a return) flagged {code_like} ranges; "
              f"{jump_tables} `jmp cs:[table]` dispatch sites found by that one pattern; incoming fixup-confirmed far calls recorded per row as far_in ({sum(far_in.values())} total).",
              "", "Largest undecoded ranges (unit, offset, size): " + ", ".join(
                  f"{g['unit']:#x}:{g['offset']:#06x} {g['size']}" for g in sorted(gaps, key=lambda g: -g["size"])[:8]) + "."]
    open(a.md, "w").write("\n".join(lines) + "\n")
    print("\n".join(lines[4:10]))


if __name__ == "__main__":
    main()
