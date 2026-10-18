#!/usr/bin/env python3
"""Measure +2EDA's dependency boundary, not full-function parity or credit."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
from u9e_dispatch_table_audit import ROOT, EXE_SHA, OVR_SHA, inventory, check

ENTRY, END = 0x2EDA, 0x2F4B
BODY_SHA = 'e75c9f7ebfcd4235a1a5a11b6d77ae544c0bd64a3c3f8ade8c5fac454757ecd3'
DISPATCH_SHA = '1572342d9be429f7a7b7f6b015b2a609726b4d5580e34235fda31eb2a43b9217'
SLOTS = [128,129,130,133,134,136,138,139]


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'owner media hashes')
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr)
    by_unit={u['stub_para']:u for u in units}
    by_segment={u['memory_segment']:u for u in units}
    u38,u9e=by_unit[0x38],by_unit[0x9E]
    body=ovr[u38['ovr_offset']+ENTRY:u38['ovr_offset']+END]
    dispatch=ovr[u9e['ovr_offset']:u9e['ovr_offset']+0x26]
    check(sha256(body).hexdigest()==BODY_SHA and sha256(dispatch).hexdigest()==DISPATCH_SHA,'body hashes')
    md=Cs(CS_ARCH_X86,CS_MODE_16)
    ins=list(md.disasm(body,ENTRY));prefix=list(md.disasm(dispatch,0))
    check(len(ins)==47 and sum(i.size for i in ins)==113,'caller extent')
    check(u38['entries'][111]==ENTRY and 0x20+111*5==0x24B,'public entry')
    calls=[]
    for i in ins:
        if i.mnemonic!='lcall':continue
        seg,off=(int(v.strip(),0) for v in i.op_str.split(','))
        target=by_segment[seg];entry=(off-0x20)//5
        check(i.address+3 in u38['fixups'] and off>=0x20 and (off-0x20)%5==0,'call fixup/stub')
        calls.append(dict(site=f'{i.address:04X}',stub=f'{seg:04X}:{off:04X}',
                          target=f"OVL_{target['stub_para']:04X}_{target['entries'][entry]:04X}"))
    check(calls==[
        dict(site='2F06',stub='00B0:002A',target='OVL_009E_0026'),
        dict(site='2F24',stub='00B0:0052',target='OVL_009E_0E52'),
        dict(site='2F40',stub='00B0:0025',target='OVL_009E_0000')],'resolved calls')
    by_ip={i.address:i for i in ins}
    check([(by_ip[ip].mnemonic,by_ip[ip].op_str) for ip in (0x2EEC,0x2EF0,0x2F2C,0x2F30)]==[
        ('cmp','byte ptr [bp + 6], 0'),('je','0x2f0d'),
        ('mov','al, byte ptr es:[di + 0x3d]'),('push','ax')],'branch/type provenance')
    check([(i.mnemonic,i.op_str) for i in prefix if i.address in (0x13,0x16,0x18,0x1A,0x1C,0x1E)]==[
        ('mov','al, byte ptr [bp + 0x10]'),('xor','ah, ah'),('mov','di, ax'),
        ('shl','di, 1'),('shl','di, 1'),('lcall','[di + 0x6828]')],'dispatch indexing')
    table=inventory()
    rows=[r for r in table['slots'] if r['handler_id']=='OVL_0038_2EDA']
    check([r['slot'] for r in rows]==SLOTS,'eight handler bindings')
    metadata=dict(target='OVL_0038_2EDA',bytes=113,instructions=47,sha256=BODY_SHA,
                  file_start=u38['ovr_offset']+ENTRY,file_end=u38['ovr_offset']+END,
                  public_entry=111,original_stub='0041:024B',slots=SLOTS,calls=calls,
                  dispatcher_sha256=DISPATCH_SHA,verdict='dynamic_boundary_not_currently_composable')
    return body,dispatch,table,metadata


def probe(body,dispatch,table):
    """Original continuation only: append and selected handlers are not run."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP,UC_X86_REG_SS,UC_X86_REG_SP,UC_X86_REG_BP,UC_X86_REG_DS,UC_X86_REG_AX,UC_X86_REG_DI
    cs,callee,ds,ss=0x8000,0x7000,0x1800,0x2800
    rows={r['slot']:r for r in table['slots']+table['adjacent_procvar_aliases']}
    samples=[];visited=set()
    for kind in range(257):
        value=kind if kind<256 else 54
        uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,1<<20)
        uc.mem_write((cs<<4)+ENTRY,body);uc.mem_write(callee<<4,dispatch)
        # Resolve the one reached public stub to its body for this synthetic continuation.
        uc.mem_write((cs<<4)+0x2F41,struct.pack('<HH',0,callee))
        def put(seg,off,*words):uc.mem_write((seg<<4)+off,struct.pack('<'+'H'*len(words),*words))
        for slot,row in rows.items():
            seg,off=(int(v,16) for v in row['original_stub'].split(':'))
            put(ds,0x6828+4*slot,off,seg)
        # Post-append state is supplied; no assumptions about the append's effects.
        put(ss,0x7FFC,0x100,0x5000);put(ss,0x800C,0x300,0x5000)
        uc.mem_write(0x5013D,bytes([value]))
        if kind==256:put(ds,0x6828+4*value,0xBEEF,0xCAFE)
        for reg,v in ((UC_X86_REG_CS,cs),(UC_X86_REG_IP,0x2F29),(UC_X86_REG_SS,ss),
                      (UC_X86_REG_SP,0x7FFC),(UC_X86_REG_BP,0x8000),(UC_X86_REG_DS,ds),(UC_X86_REG_AX,0xA500)):
            uc.reg_write(reg,v)
        stopped=[]
        def hook(em,address,size,user):
            del address,size,user
            here=(em.reg_read(UC_X86_REG_CS),em.reg_read(UC_X86_REG_IP))
            if here==(callee,0x1E):stopped.append(here);em.emu_stop()
            else:visited.add(here)
        uc.hook_add(UC_HOOK_CODE,hook)
        uc.emu_start((cs<<4)+0x2F29,1<<20,count=100)
        check(stopped==[(callee,0x1E)],f'dispatch stop {value}')
        check(uc.reg_read(UC_X86_REG_DI)==4*value,'full unsigned byte index')
        check(uc.reg_read(UC_X86_REG_SP)==0x7FE0,'prepared handler frame')
        check(struct.unpack('<5H',uc.mem_read((ss<<4)+0x7FE0,10))==(0,0,0,0x300,0x5000),'handler arguments')
        actual=struct.unpack('<HH',uc.mem_read((ds<<4)+0x6828+4*value,4))
        if kind==256:
            check(actual==(0xBEEF,0xCAFE),'live target mutation');continue
        row=rows.get(value)
        expected=(0,0) if row is None else tuple(reversed([int(v,16) for v in row['original_stub'].split(':')]))
        check(actual==expected,'installed table target')
        if value in (0,4,54,128,139,140,214,215,255):
            samples.append(dict(type=value,table_address=f'{0x6828+4*value:04X}',
                                target=row['handler_id'] if row else 'uninitialized_by_installers'))
    return dict(continuation_cases=257,byte_values=256,live_target_mutations=1,
                starts_before_boundary=len(visited),stop='U9E:001E before indirect LCALL',
                examples=samples,
                limitation='Starts at U38:2F29 with synthetic post-append state; neither append, removal nor any selected handler executes. No full-function parity or natural-game reachability claim.')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--unicorn',action='store_true');p.add_argument('--output',type=Path)
    args=p.parse_args();body,dispatch,table,report=source()
    if args.unicorn:report['probe']=probe(body,dispatch,table)
    if args.output:args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
