import csv, re
from pathlib import Path
from collections import defaultdict
ROOT=Path("/home/hunter/workspace/poolrad-decomp-opencode-ds41flash")
strict={r["id"] for r in csv.DictReader((ROOT/"docs/re/strict_full_c_addresses.csv").open())}
man={r["id"]:r["status"] for r in csv.DictReader((ROOT/"docs/re/partial_callee_models.csv").open())}
ledger={f"OVL_{int(r['unit'],16):04X}_{int(r['offset'],16):04X}" for r in csv.DictReader((ROOT/"docs/re/overlay_function_ledger.csv").open()) if r["id"].startswith("0x")}
resled={r["id"] for r in csv.DictReader((ROOT/"docs/re/resident_callable_ledger.csv").open()) if r["id"].startswith("RES_")}
cls={f"OVL_{int(c['unit'],16):04X}_{int(c['offset'],16):04X}": c["classification"] for c in csv.DictReader((ROOT/"docs/re/overlay_classification.csv").open())}
body_docs=[p for p in (ROOT/"docs/re").glob("*.md") if p.name.endswith("_CPU.md") or "CPU" in p.stem]
ids=defaultdict(set); noid=[]
for p in body_docs:
    text=p.read_text(errors="ignore")[:4000]
    got=set(re.findall(r"OVL_[0-9A-F]{4}_[0-9A-F]{4}", text)) | set(re.findall(r"RES_[0-9A-F]{4}_[0-9A-F]{4}", text))
    for m in re.finditer(r"U([0-9A-F]{1,4})\s*\+?\s*(?:0x)?([0-9A-F]{4})", text, re.I):
        got.add(f"OVL_{int(m.group(1),16):04X}_{int(m.group(2),16):04X}")
    for m in re.finditer(r"(?:unit|U)\s*0x([0-9A-F]{1,4})\s*\+?\s*0x([0-9A-F]{1,4})", text, re.I):
        got.add(f"OVL_{int(m.group(1),16):04X}_{int(m.group(2),16):04X}")
    if got: ids[p.name]=got
    else: noid.append(p.name)
allids=set().union(*ids.values()) if ids else set()
ovl={i for i in allids if i in ledger}; res={i for i in allids if i in resled}
uncred_ovl=sorted(ovl-strict); uncred_res=sorted(res-strict)
ident=[i for i in uncred_ovl if cls.get(i)!="game_verified"]
settled=[i for i in uncred_ovl if cls.get(i)=="game_verified"]
print(f"body docs scanned: {len(body_docs)} | yielded ids: {len(ids)} | no id: {len(noid)}")
print(f"ids: overlay-in-ledger {len(ovl)} | resident-in-ledger {len(res)} | total {len(ovl)+len(res)}")
print(f"credited {len([i for i in (ovl|res) if i in strict])} | UNCREDITED overlay {len(uncred_ovl)} + resident {len(uncred_res)} = {len(uncred_ovl)+len(uncred_res)}")
print(f"  overlay identity-blocked {len(ident)} | settled {len(settled)}")
print("  settled:", settled)
print("  identity-blocked:", ident[:20])
print("  docs with no derivable id:", noid[:8])
