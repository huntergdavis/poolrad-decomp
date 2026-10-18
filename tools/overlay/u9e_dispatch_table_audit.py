#!/usr/bin/env python3
"""Decode DS:6828 installers; emit address/interface inventory, never credit."""
import argparse,csv,json,re
from collections import Counter
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import ROOT,EXE_SHA,OVR_SHA

# Explicit, unit-scoped interface inventory, audited at main d8ebbf6c plus the U38 +133F raw chain.
# A raw API is not a new independent verification or strict-credit decision.
GROUPS=[
 ('raw_cpu','overlay_u38_target_reset_133f_cpu.h','133F'),
 ('raw_cpu','overlay_u38_dispatch_nop_cpu.h','0647 1355 135C 319B'),
 ('scalar_or_callback','overlay_unit38_effects.h','010D 011D 013B 05D6 064E 06E4 0A3D 0BAC 0C26 10C1 1452 171F 2490 25A2 2A6D 2B63'),
 ('scalar_or_callback','overlay_unit38_effects_next.h','01C3 24D9 2700 2A28'),
 ('scalar_or_callback','overlay_unit38_walk.h','268A 2E37'),
 ('scalar_or_callback','ecl_helpers.h','0377 03AC 03E1 0C06 0C15'),
 ('raw_cpu','overlay_u38_helper_cluster_cpu.h','1490 29B6 29CB 29D9 29F0 2A11'),
 ('raw_cpu','overlay_u38_roll_gate_2915_cpu.h','2976 2984 2992'),
 ('raw_cpu','overlay_u38_pair_04ba.h','04BA'),
 ('raw_cpu','overlay_u38_sidecar_1472_cpu.h','1472'),
 ('raw_cpu','overlay_u38_roll_append_1730_cpu.h','1730'),
 ('raw_cpu','overlay_u38_lookup_append_2814_cpu.h','2814'),
 ('raw_cpu','overlay_u38_record_roll_2d38.h','2D38'),
 ('raw_cpu','overlay_u38_roll_clear_2de4_cpu.h','2DE4'),
 ('raw_cpu','overlay_u38_effect_match_12b2.h','12B2'),
 ('scalar_or_callback','overlay_u38_sidecar_flag_0918.h','0918'),
 ('scalar_or_callback','overlay_u38_target_mask81_25bc.h','25BC'),
 ('scalar_or_callback','overlay_u38_effect_clear_264b.h','264B'),
 ('scalar_or_callback','overlay_u38_effect_scale_clear_2d88.h','2D88'),
 ('scalar_or_callback','overlay_u38_target_index_gate_2a87.h','2A87'),
 ('scalar_or_callback','overlay_u38_target_gate_2ae6.h','2AE6'),
 ('scalar_or_callback','overlay_u38_target_reset_2b7d.h','2B7D'),
 ('scalar_or_callback','overlay_u38_link_message_2bc8.h','2BC8'),
 ('scalar_or_callback','overlay_u38_link_roll_2b20.h','2B20'),
 ('scalar_or_callback','overlay_u38_roll_dispatch_1699.h','1699'),
]

def check(value,message):
    if not value:raise ValueError(message)

def inventory():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media hashes')
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr);by_unit={u['stub_para']:u for u in units}
    by_segment={u['memory_segment']:u for u in units};md=Cs(CS_ARCH_X86,CS_MODE_16)
    code=lambda u:ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    u38=by_unit[0x38];installer=code(u38)[0x31A2:0x38A9]
    check(sha256(installer).hexdigest()=='4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519','U38 installer hash')
    ins=list(md.disasm(installer,0x31A2));check(len(ins)==556,'installer extent')
    installed=[]
    for n in range(138):
        a,b,c,d=ins[2+4*n:6+4*n]
        check([i.mnemonic for i in (a,b,c,d)]==['mov']*4,'installer MOV quartet')
        off=int(a.op_str.split(', ')[1],0);seg=int(b.op_str.split(', ')[1],0)
        dest=int(re.search(r'\[(0x[0-9a-f]+)\]',c.op_str)[1],0)
        check(a.op_str.startswith('ax, ') and b.op_str.startswith('dx, ') and c.op_str==f'word ptr [{dest:#x}], ax' and d.op_str==f'word ptr [{dest+2:#x}], dx','store operands')
        check(b.address+1 in u38['fixups'],'installer segment fixup')
        installed.append(((dest-0x6828)//4,seg,off))
    check({r[0] for r in installed}==set(range(1,140))-{88},'installed types')
    u86=by_unit[0x86];body86=code(u86)[0x3413:0x37B0]
    check(sha256(body86).hexdigest()=='66ffc8c06e80199ee68e064db44ccb47bcb2e51d661786e0bcdc4f466080e5d5','U86 installer hash')
    suffix=list(md.disasm(code(u86)[0x37A1:0x37AE],0x37A1))
    check([(i.mnemonic,i.op_str) for i in suffix]==[('mov','ax, 0x15b'),('mov','dx, 0x96'),('mov','word ptr [0x6988], ax'),('mov','word ptr [0x698a], dx')],'U86 retained slot')
    check(0x37A5 in u86['fixups'],'U86 retained segment fixup');installed.append((88,0x96,0x15B))
    adjacent=[]
    all86=list(md.disasm(body86,0x3413))
    for n in range(len(all86)-3):
        a,b,c,d=all86[n:n+4]
        if a.mnemonic!='mov' or not a.op_str.startswith('ax, 0x') or b.mnemonic!='mov' or b.op_str!='dx, 0x96':continue
        match=re.fullmatch(r'word ptr \[(0x[0-9a-f]+)\], ax',c.op_str)
        if c.mnemonic!='mov' or not match:continue
        dest=int(match[1],0)
        if not 0x6B84<=dest<0x6C28:continue
        check(d.mnemonic=='mov' and d.op_str==f'word ptr [{dest+2:#x}], dx' and b.address+1 in u86['fixups'],'adjacent pointer store')
        off=int(a.op_str.split(', ')[1],0);entry=(off-0x20)//5
        adjacent.append(dict(slot=(dest-0x6828)//4,table_address=f'{dest:04X}',handler_id=f"OVL_0086_{u86['entries'][entry]:04X}",original_stub=f'0096:{off:04X}',public_entry=entry))
    check({r['slot'] for r in adjacent}==set(range(215,256)),'adjacent procvar overlap')
    strict_path=ROOT/'docs/re/strict_full_c_addresses.csv'
    strict={r['id']:r['evidence_doc'] for r in csv.DictReader(strict_path.open())}
    models={}
    for status,header,addresses in GROUPS:
        path=ROOT/'include/poolrad/recovered'/header;check(path.is_file(),f'model header {header}')
        for off in addresses.split():
            identity=f'OVL_0038_{off}';check(identity not in models,'duplicate model mapping')
            models[identity]=(status,str(path.relative_to(ROOT)))
    rows=[]
    for slot,seg,off in sorted(installed):
        unit=by_segment[seg];entry=(off-0x20)//5
        check(off>=0x20 and (off-0x20)%5==0 and entry<len(unit['entries']),'public stub entry')
        offset=unit['entries'][entry];identity=f"OVL_{unit['stub_para']:04X}_{offset:04X}"
        status,header=models.get(identity,('missing',''))
        # "missing" means no mapped standalone/grouped model located at this
        # snapshot; it is not a proof of absence from every historical branch.
        rows.append(dict(slot=slot,table_address=f'{0x6828+4*slot:04X}',handler_id=identity,original_stub=f'{seg:04X}:{off:04X}',public_entry=entry,canonical_strict_row=identity in strict,strict_evidence=strict.get(identity,''),interface=status,interface_header=header))
    unique={r['handler_id']:r for r in rows};missing=sorted(id for id,r in unique.items() if not r['canonical_strict_row'])
    check(len(rows)==139 and len(unique)==118,'inventory cardinality')
    summary=dict(bound_slots=139,distinct_handlers=118,canonical_strict_handlers=sum(r['canonical_strict_row'] for r in unique.values()),without_canonical_strict=len(missing),adjacent_alias_slots=len(adjacent),interfaces=dict(Counter(r['interface'] for r in unique.values())))
    return dict(schema_version=1,interface_review_base='d8ebbf6c + U38 133F raw chain',start_exe_sha256=EXE_SHA,game_ovr_sha256=OVR_SHA,strict_map_sha256=sha256(strict_path.read_bytes()).hexdigest(),dispatch_address_range='DS:6828..6C27 (256 possible slots)',initialized_address_range='DS:682C..6A57 (slots1..139)',unbound_slots=[0,*range(140,215)],adjacent_procvar_aliases=adjacent,summary=summary,slots=rows,without_canonical_strict=missing,limitations=['Only disjoint U86-then-U38 initialization is inventoried; later writes and invalid types require live-target validation. Types215..255 alias initialized U86 procedure pointers outside the intended effect table. A type1..139 invariant must be proven before treating 118 handlers as the complete closure.','raw_cpu describes a published interface, not verified composition through U9E. scalar_or_callback combines scalar models and raw shells with unresolved hooks.','missing means no model located by the explicit audited mapping at interface_review_base, not a global-history absence proof.','No accounting or strict-credit files are written.'])

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    report=inventory();a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary'],sort_keys=True))
if __name__=='__main__':main()
