#!/usr/bin/env python3
"""Pinned complete U6C +4235 caller and raw U9E/SYSTEM RNG parity."""

import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
from overlay_inventory import find_units, parse_mz
from overlay_u9e_entry8_roll_audit import audit as dice_source_audit
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END = 0x4235, 0x42ED
BODY_SHA = "fdb467838a6d2067178154c2b0bf3d60f0acff37060f337977e3d66d24391d60"
DICE_SHA = "7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba"
RANDOM_SHA = "e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9"
SEED_SHA = "f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242"
INSTRUCTION_BOUND = 50000


class OriginalBoundExceeded(Exception):
    """Original x86 did not reach a return within the declared audit bound."""


EXTENTS = {"caller":(ENTRY,END,73,BODY_SHA),"dice":(0xDE3,0xE2E,29,DICE_SHA),
           "random":(0x11C0,0x11D6,10,RANDOM_SHA),"seed":(0x120F,0x1245,22,SEED_SHA)}


class Layout(ctypes.Structure):
    _fields_ = [("dice_cs",ctypes.c_uint16),("system_cs",ctypes.c_uint16)]


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(exe) != 65231 or sha256(exe).hexdigest() != EXE_SHA or len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("input provenance differs")
    dice_source_audit(exe,ovr) # Reuse the merged byte ABI/seed and file-origin pins.
    units = {u["stub_para"]:u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],ovr)}
    u = units[0x6C]
    if u["ovr_offset"] != 104831 or u["memory_segment"] != 0x78:
        raise ValueError("U6C placement differs")
    code = ovr[u["ovr_offset"]:u["ovr_offset"]+u["code_size"]]
    d = units[0x9E]
    dice = ovr[d["ovr_offset"]+0xDE3:d["ovr_offset"]+0xE2E]
    image,origin,_,_ = unpack(exe)
    bodies = {"caller":code[ENTRY:END],"dice":dice,
              "random":bytes(image[0xC140:0xC156]),"seed":bytes(image[0xC18F:0xC1C5])}
    decoded = {}
    for name,(start,end,count,digest) in EXTENTS.items():
        ins = list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(bodies[name],start))
        if sha256(bodies[name]).hexdigest() != digest or len(ins) != count or sum(i.size for i in ins) != end-start:
            raise ValueError(f"{name} extent differs")
        decoded[name] = {i.address:i for i in ins}
    if [p for p in u["fixups"] if ENTRY <= p < END] != [0x4283]:
        raise ValueError("caller relocation differs")
    if decoded["caller"][0x4280].op_str != "0xb0, 0x48" or decoded["dice"][0xE0E].op_str != "0xaf8, 0x11c0":
        raise ValueError("reached far call differs")
    if bytes(image[0xC1C5:0xC1C7]) != b"\x05\x84":
        raise ValueError("seed multiplier differs")
    starts = sorted(f["rel"] for f in u["functions"])
    for start,site in ((0x056C,0x1D3B),(0x29AE,0x2FB1)):
        end = next((v for v in starts if v > start),len(code))
        caller = {i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[start:end],start)}
        if caller[site].op_str != "0x4235" or caller[site].mnemonic != "call" or caller[site-1].op_str != "cs":
            raise ValueError("aligned incoming caller differs")
    raw_calls = [p for p in range(len(code)-2) if code[p] == 0xE8 and
                 (p+3+int.from_bytes(code[p+1:p+3],"little",signed=True)) & 0xFFFF == ENTRY]
    if raw_calls != [0x1D3B,0x2FB1]: raise ValueError("raw near caller set differs")
    return bodies,decoded,dict(caller_prefix=code[0x2FA4:0x2FB4],multiplier=b"\x05\x84",
        file_extents={"caller":[u["ovr_offset"]+ENTRY,u["ovr_offset"]+END],
                      "dice":[d["ovr_offset"]+0xDE3,d["ovr_offset"]+0xE2E],
                      "random":[origin[0xC140]+0x200,origin[0xC155]+0x201],
                      "seed":[origin[0xC18F]+0x200,origin[0xC1C4]+0x201]})


def library(directory):
    path = directory / "u6c_4235.so"
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Wpedantic","-Werror","-fPIC","-shared",
        "-I",str(ROOT / "include"),str(ROOT / "src/recovered/overlay_u6c_record_roll_sum_4235.c"),
        "-o",str(path)],check=True,capture_output=True,text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u6c_record_roll_sum_4235.argtypes = [ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,
        ctypes.POINTER(Cpu),ctypes.POINTER(Layout)]
    lib.pr_u6c_record_roll_sum_4235.restype = ctypes.c_int
    return lib


def install(memory,bodies,caller_cs,layout):
    segments = dict(caller=caller_cs,dice=layout.dice_cs,random=layout.system_cs,seed=layout.system_cs)
    for name,(start,_,_,_) in EXTENTS.items():
        body = bytearray(bodies[name])
        if name == "caller":body[0x4283-ENTRY:0x4285-ENTRY] = layout.dice_cs.to_bytes(2,"little")
        if name == "dice":body[0xE11-0xDE3:0xE13-0xDE3] = layout.system_cs.to_bytes(2,"little")
        at = physical(segments[name],start)
        memory[at:at+len(body)] = body
    at = physical(layout.dice_cs,0x48)
    memory[at:at+3] = b"\xE9\x98\x0D" # Transparent loaded entry-8 stub, not a loader model.
    put16(memory,layout.system_cs,0x1245,0x8405)


def make_case(bodies,*,gates=None,counts=None,ranges=None,bits=None,mask=255,seed=0xBB36E7A1,
              record=(0x4000,0x1000),ss=0x2000,sp=0x8000,ds=0x1000,flags=0x0202,background=None):
    memory = bytearray([0xA5])*SIZE if background is None else bytearray(background)
    cpu = Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,0xF000,ENTRY,ds,0x1111,ss,flags)
    layout = Layout(0xC000,0xD000)
    segment,offset = record
    for i,value in enumerate([1]*8 if gates is None else gates):
        memory[physical(segment,(offset+0x96+i)&0xFFFF)] = value
    for base,values in ((0x05DE,[1<<i for i in range(8)] if bits is None else bits),
                        (0x08B6,[1]*8 if counts is None else counts),
                        (0x08BE,[6]*8 if ranges is None else ranges)):
        for i,value in enumerate(values):memory[physical(ds,base+i)] = value
    put16(memory,ds,0x4428,seed & 0xFFFF)
    put16(memory,ds,0x442A,seed >> 16)
    # Far return and far argument each use contiguous physical words.
    at = physical(ss,sp)
    for i,b in enumerate((0x12,7,cpu.cs & 255,cpu.cs >> 8)):memory[(at+i)&(SIZE-1)] = b
    put16(memory,ss,(sp+4)&0xFFFF,mask)
    at = physical(ss,(sp+6)&0xFFFF)
    for i,b in enumerate((offset & 255,offset >> 8,segment & 255,segment >> 8)):
        memory[(at+i)&(SIZE-1)] = b
    install(memory,bodies,cpu.cs,layout)
    return memory,cpu,layout


def compare(label,memory,cpu,layout,decoded,lib):
    original = Uc(UC_ARCH_X86,UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing))
    original.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):original.reg_write(reg,getattr(cpu,name))
    caller_cs = cpu.cs
    segments = {"caller":caller_cs,"dice":layout.dice_cs,
                "random":layout.system_cs,"seed":layout.system_cs}
    address_sites = {physical(segments[name],ip):(name,ip)
                     for name,ins in decoded.items() for ip in ins}
    stub_address = physical(layout.dice_cs,0x48)
    snapshots = []
    for name,(start,end,_,_) in EXTENTS.items():
        segment = caller_cs if name == "caller" else layout.dice_cs if name == "dice" else layout.system_cs
        at = physical(segment,start)
        snapshots.append((at,bytes(backing[at:at+end-start])))
    at = physical(layout.system_cs,0x1245)
    snapshots.append((at,bytes(backing[at:at+2])))
    visited,edges,previous,pending = set(),set(),[None],[None]
    stop,undefined = [None],[0]
    rng_calls,thresholds = [0],[0]

    def on_instruction(uc,address,size,user):
        del size,user
        if pending[0] is not None:
            cs,ip = uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)
            expected = pending[0]
            pending[0] = None
            if expected == "outer":stop[0]=0;uc.emu_stop();return
            if (cs,ip) != expected:stop[0]=2;uc.emu_stop();return
        address &= SIZE-1
        if address == stub_address:return
        key = address_sites.get(address)
        if key is None:
            cs,ip = uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)
            raise ValueError(f"unmodeled reached dependency {label}: {cs:04X}:{ip:04X}")
        name,ip = key
        if previous[0] is not None:
            pn,pi = previous[0]
            ins = decoded[pn][pi]
            if ins.mnemonic.startswith("j") and ins.mnemonic != "jmp":edges.add((pn,pi,ip))
        previous[0] = key
        visited.add(key)
        if name == "seed" and ip == 0x1244:pending[0]=(layout.system_cs,0x11C3);undefined[0]=0
        if name == "random" and ip == 0x11D3:pending[0]=(layout.dice_cs,0xE13)
        if name == "dice" and ip == 0xE2B:pending[0]=(caller_cs,0x4285);undefined[0]=0
        if name == "caller" and ip == 0x42EA:pending[0]="outer";undefined[0]=0
        if name == "random" and ip == 0x11CB:undefined[0]=0x10
        if name == "random" and ip == 0x11D0:undefined[0]=0x8D5
        if name == "dice" and ip == 0xE0E:rng_calls[0]+=1
        if name == "caller" and ip in (0x42AB,0x42CA):thresholds[0]+=1

    original.hook_add(UC_HOOK_CODE,on_instruction)
    original.emu_start(physical(caller_cs,ENTRY),2*SIZE,count=INSTRUCTION_BOUND)
    if stop[0] is None:raise OriginalBoundExceeded(label)
    for at,data in snapshots:
        if bytes(backing[at:at+len(data)]) != data:raise ValueError(f"immutable code/constant changed: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = lib.pr_u6c_record_roll_sum_4235(ram,SIZE,ctypes.byref(cpu),ctypes.byref(layout))
    if status != stop[0]:raise ValueError(f"status differs {label}: {status}/{stop[0]}")
    if bytes(ram) != bytes(backing):
        diffs = [i for i,(a,b) in enumerate(zip(ram,backing)) if a != b]
        raise ValueError(f"RAM differs {label}: {diffs[:8]}")
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        mask = 0xFFFF ^ undefined[0] if name == "flags" else 0xFFFF
        if (getattr(cpu,name) ^ expected) & mask:
            raise ValueError(f"{name} differs {label}: {getattr(cpu,name):04X}/{expected:04X}")
    return visited,edges,status,rng_calls[0],thresholds[0],undefined[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick",action="store_true")
    parser.add_argument("--output",type=Path)
    parser.add_argument("--progress",action="store_true")
    args = parser.parse_args()
    bodies,decoded,meta = source()
    visited,edges,count,transfers,rng_calls,thresholds = set(),set(),0,0,0,0
    bounded_original_cases = []
    transfer_cases = []
    with tempfile.TemporaryDirectory(prefix="poolrad-u6c-4235-") as tmp:
        lib = library(Path(tmp))
        def run(label,**options):
            nonlocal visited,edges,count,transfers,rng_calls,thresholds
            if args.progress:print(f"case {count}: {label}",file=sys.stderr,flush=True)
            memory,cpu,layout = make_case(bodies,**options)
            try:
                sites,branches,status,rolls,divs,undefined = compare(label,memory,cpu,layout,decoded,lib)
            except OriginalBoundExceeded:
                bounded_original_cases.append(label)
                return
            visited |= sites;edges |= branches;count+=1;transfers+=status==2;rng_calls+=rolls;thresholds+=divs
            if status == 2:
                transfer_cases.append(dict(case=label,cs=cpu.cs,ip=cpu.ip,sp=cpu.sp,undefined_flags_mask=undefined))
        for mask in range(256):run(f"mask-{mask}",mask=mask)
        for value in range(256):run(f"signed-gate-{value}",gates=[value]*8)
        for range_byte in range(0,256,16 if args.quick else 1):
            for gate in (1,2):run(f"range-{range_byte}-{gate}",ranges=[range_byte]*8,gates=[gate]*8)
        for count_byte in (0,1,2,3,8,127,128,255):
            for range_byte in (0,1,6,255):
                run(f"count-{count_byte}-{range_byte}",counts=[count_byte]+[0]*7,ranges=[range_byte]*8)
        for seed in (0,1,0xFFFFFFFF,0xFFFF,0x80000000,0xBB36E7A1,0x31415926,0x5733,0xFFFF5733):run(f"seed-{seed:08X}",seed=seed)
        for offset in range(0x7F40,0x800B,8 if args.quick else 1):run(f"record-frame-{offset:04X}",record=(0x2000,offset))
        for sp in range(0x441E,0x4450,2 if args.quick else 1):run(f"seed-frame-{sp:04X}",ss=0x1000,sp=sp)
        for base in (0x05DE,0x08B6,0x08BE):
            for sp in range(base-4,base+30,4 if args.quick else 1):run(f"table-frame-{base:X}-{sp:X}",ss=0x1000,sp=sp)
        for segment in (0,0x4000,0xFFFF):
            for offset in (0,1,0xFF69,0xFF6A,0xFFFF):run(f"record-wrap-{segment:X}-{offset:X}",record=(segment,offset))
        for ss in (0,0x2000,0xFFFF):
            for sp in (0,1,2,3,4,5,6,7,8,9,10,11,0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):
                run(f"frame-wrap-{ss:X}-{sp:X}",ss=ss,sp=sp)
        rng = random.Random(0x6C4235)
        for index in range(8 if args.quick else 128):
            run(f"seeded-{index}",gates=[rng.randrange(256) for _ in range(8)],counts=[rng.randrange(5) for _ in range(8)],
                ranges=[rng.randrange(256) for _ in range(8)],bits=[rng.randrange(256) for _ in range(8)],
                seed=rng.randrange(1<<32),mask=rng.randrange(1<<16),ds=rng.choice((0,0x1000,0xFFFF)),
                flags=rng.choice((0x0202,0x0ED7,0x0047,0x0A93)),background=rng.randbytes(SIZE))
    required_sites = {(name,ip) for name,ins in decoded.items() for ip in ins}
    required_edges = {(name,ip,target) for name,ins in decoded.items() for ip,i in ins.items()
        if i.mnemonic.startswith("j") and i.mnemonic != "jmp" for target in (int(i.op_str,16),ip+i.size)}
    infeasible = {("dice",0xDFE,0xE1F)} # Bound just copied from the nonzero count; AL=1.
    if visited != required_sites or edges != required_edges-infeasible:
        raise ValueError(f"coverage incomplete: sites {required_sites-visited}, edges {required_edges-infeasible-edges}")
    report = dict(target="OVL_006C_4235",source_sha256=BODY_SHA,dependency_sha256={k:v[3] for k,v in EXTENTS.items() if k!="caller"},
        cases=count,original_instruction_bound=INSTRUCTION_BOUND,
        bounded_original_cases_without_C_parity=bounded_original_cases,returns=count-transfers,internal_transfers=transfers,transfer_cases=transfer_cases,rng_calls=rng_calls,threshold_divisions=thresholds,
        covered_sites=len(visited),source_sites=len(required_sites),conditional_edges=len(edges),
        infeasible_edges=["U9E:0DFE->0E1F (nonzero bound snapshot, AL=1)"],compared_ram_bytes=SIZE,compared_cpu_words=14,
        normal_return_flags_masked=0,transfer_flags_masked_only_when_undefined=True,
        aligned_near_callers=["056C:+1D3B","29AE:+2FB1"],natural_game_route_measured=False,**meta)
    report.pop("caller_prefix");report.pop("multiplier")
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":main()
