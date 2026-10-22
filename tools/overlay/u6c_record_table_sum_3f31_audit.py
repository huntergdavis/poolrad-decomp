#!/usr/bin/env python3
"""Pinned U6C +3F31 source/callers and complete raw-state parity audit."""

import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import UC_X86_REG_IP

from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END = 0x3F31, 0x3FB3
BODY_SHA = "9f192cb8fe704324facbbec368d1e24602558b878aa3f7e0031f644862e09767"
BRANCHES = (0x3F5C,0x3F78,0x3F82,0x3F8F,0x3F99,0x3FA2)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(exe) != 65231 or sha256(exe).hexdigest() != EXE_SHA:
        raise ValueError("START.EXE provenance differs")
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("GAME.OVR provenance differs")
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],ovr) if u["stub_para"] == 0x6C)
    if unit["ovr_offset"] != 104831 or unit["memory_segment"] != 0x78:
        raise ValueError("U6C placement differs")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"]+unit["code_size"]]
    body = code[ENTRY:END]
    dec = list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,ENTRY))
    if len(body) != 130 or sha256(body).hexdigest() != BODY_SHA or len(dec) != 44 or sum(i.size for i in dec) != 130:
        raise ValueError("U6C extent differs")
    decoded = {i.address:i for i in dec}
    if dec[-1].address != 0x3FB0 or dec[-1].mnemonic != "retf" or dec[-1].op_str != "4":
        raise ValueError("leaf return differs")
    if body[0x3F50-ENTRY] != 0x98 or decoded[0x3F50].size != 1:
        raise ValueError("16-bit CBW differs")
    if any(ENTRY <= p < END for p in unit["fixups"]):
        raise ValueError("leaf gained a relocation")
    for i in dec:
        if "call" in i.mnemonic or i.mnemonic.startswith("int"):
            raise ValueError("leaf gained an external dependency")
        if i.mnemonic.startswith("j") and int(i.op_str,16) not in decoded:
            raise ValueError("branch leaves leaf")
    starts = sorted(f["rel"] for f in unit["functions"])
    for start,site in ((0x056C,0x1D5C),(0x29AE,0x2FE8),(0x3FB3,0x4018),(0x408C,0x41C1)):
        end = next((v for v in starts if v > start),len(code))
        caller = {i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[start:end],start)}
        if caller[site].mnemonic != "call" or caller[site].op_str != "0x3f31" or caller[site-1].mnemonic != "push" or caller[site-1].op_str != "cs":
            raise ValueError("aligned near caller differs")
        if start == 0x29AE and (caller[0x2FDF].op_str != "word ptr [0x5d94]" or caller[0x2FE3].op_str != "word ptr [0x5d92]"):
            raise ValueError("active-record caller argument differs")
    return body,decoded,dict(caller_suffix=code[0x2FDF:0x2FEB])


CHECKER_ROWS = ('OVL_006C_3F31',)


def library(directory):
    path = directory / "u6c_3f31.so"
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Wpedantic","-Werror",
        "-fPIC","-shared","-I",str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u6c_record_table_sum_3f31.c"),"-o",str(path)],
        check=True,capture_output=True,text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u6c_record_table_sum_3f31.argtypes = [ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    lib.pr_u6c_record_table_sum_3f31.restype = ctypes.c_int
    return lib


def make_case(body,type_byte=18,kind=2,table=3,gates=None,*,record=(0x4000,0x1000),
              ss=0x2000,sp=0x8000,ds=0x1000,flags=0x0202,background=None):
    memory = bytearray([0xA5])*SIZE if background is None else bytearray(background)
    cpu = Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
              0x7777,sp,0xF000,ENTRY,ds,0x1111,ss,flags)
    at = physical(cpu.cs,ENTRY)
    memory[at:at+len(body)] = body
    segment,offset = record
    values = gates if gates is not None else [1]*8
    for i,value in enumerate(values):
        memory[physical(segment,(offset+0x96+i) & 0xFFFF)] = value
    memory[physical(segment,(offset+0x14) & 0xFFFF)] = type_byte
    memory[physical(segment,(offset+0x2F) & 0xFFFF)] = kind
    memory[physical(ds,0x408E+type_byte)] = table
    at = physical(ss,sp)
    for i,b in enumerate((0x12,7,cpu.cs & 255,cpu.cs >> 8)):
        memory[(at+i) & (SIZE-1)] = b
    put16(memory,ss,(sp+4) & 0xFFFF,offset)
    at = (physical(ss,(sp+4) & 0xFFFF)+2) & (SIZE-1)
    memory[at],memory[(at+1) & (SIZE-1)] = segment & 255,segment >> 8
    return memory,cpu


def compare(label,memory,cpu,decoded,lib):
    original = Uc(UC_ARCH_X86,UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing))
    original.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        original.reg_write(reg,getattr(cpu,name))
    entry = physical(cpu.cs,ENTRY)
    expected_code = memory[entry:entry+END-ENTRY]
    visited,edges,previous,at_return = set(),set(),[None],[False]

    def on_instruction(uc,address,size,user):
        del size,user
        ip = uc.reg_read(UC_X86_REG_IP)
        if not entry <= address < entry+END-ENTRY or ip not in decoded:
            raise ValueError(f"unexpected transfer: {label}")
        if previous[0] in BRANCHES:
            edges.add((previous[0],ip))
        previous[0] = ip
        visited.add(ip)
        if ip == 0x3FB0:
            at_return[0] = True
            uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE,on_instruction)
    original.emu_start(entry,2*SIZE,count=1000)
    if not at_return[0]:
        raise ValueError(f"original did not return within bound: {label}")
    original.hook_del(hook)
    original.emu_start(physical(cpu.cs,0x3FB0),2*SIZE,count=1)
    if bytes(backing[entry:entry+END-ENTRY]) != expected_code:
        raise ValueError(f"immutable code changed: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u6c_record_table_sum_3f31(ram,SIZE,ctypes.byref(cpu)):
        raise ValueError(f"C rejected input: {label}")
    actual,expected = bytes(ram),bytes(backing)
    if actual != expected:
        diffs = [i for i,(a,b) in enumerate(zip(actual,expected)) if a != b]
        raise ValueError(f"RAM differs {label}: {diffs[:8]}")
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected_word = original.reg_read(reg) & 0xFFFF
        if getattr(cpu,name) != expected_word:
            raise ValueError(f"{name} differs {label}: {getattr(cpu,name):04X}/{expected_word:04X}")
    return visited,edges


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick",action="store_true")
    parser.add_argument("--output",type=Path)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    body,decoded,_ = source()
    visited,edges,count = set(),set(),0
    with tempfile.TemporaryDirectory(prefix="poolrad-u6c-3f31-") as tmp:
        lib = library(Path(tmp))

        def run(label,**options):
            nonlocal visited,edges,count
            memory,cpu = make_case(body,**options)
            sites,branches = compare(label,memory,cpu,decoded,lib)
            visited |= sites
            edges |= branches
            count += 1

        for type_byte in range(256):
            for kind in (0,2,3):
                run(f"type-{type_byte}-{kind}",type_byte=type_byte,kind=kind)
        for table in range(0,256,16 if args.quick else 1):
            for type_byte in (16,17,18):
                run(f"table-{type_byte}-{table}",type_byte=type_byte,table=table)
        for value in range(256):
            run(f"signed-gate-{value}",gates=[value]*8)
        for enabled in range(256):
            run(f"enabled-{enabled}",gates=[1 if enabled & (1 << i) else 0 for i in range(8)])
        for position in range(8):
            for value in (0,1,127,128,255):
                gates = [0]*8
                gates[position] = value
                run(f"position-{position}-{value}",gates=gates)
        for offset in range(0x7F50,0x8009,4 if args.quick else 1):
            run(f"record-frame-{offset:04X}",record=(0x2000,offset))
        for sp in range(0x4088,0x40B9,4 if args.quick else 1):
            run(f"table-frame-{sp:04X}",ss=0x1000,sp=sp)
        for ss in (0,0x2000,0xFFFF):
            for sp in (0,1,2,3,4,5,6,7,8,9,10,11,0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):
                run(f"frame-wrap-{ss:04X}-{sp:04X}",ss=ss,sp=sp)
        for segment in (0,0x4000,0xFFFF):
            for offset in (0,1,0xFF69,0xFF6A,0xFFEB,0xFFFF):
                run(f"record-wrap-{segment:04X}-{offset:04X}",record=(segment,offset))
        rng = random.Random(0x6C3F31)
        for index in range(8 if args.quick else 128):
            run(f"seeded-{index}",type_byte=rng.randrange(256),kind=rng.randrange(256),table=rng.randrange(256),
                gates=[rng.randrange(256) for _ in range(8)],ds=rng.choice((0,0x1000,0xF000,0xFFFF)),
                flags=rng.choice((0x0202,0x0ED7,0x0047,0x0A93)),background=rng.randbytes(SIZE))
    required = {(ip,target) for ip in BRANCHES for target in (int(decoded[ip].op_str,16),ip+decoded[ip].size)}
    if visited != set(decoded) or edges != required:
        raise ValueError("instruction or conditional-edge coverage incomplete")
    report = dict(target="OVL_006C_3F31",source_sha256=BODY_SHA,cases=count,
                  covered_sites=len(visited),source_sites=len(decoded),conditional_edges=len(edges),
                  compared_ram_bytes=SIZE,compared_cpu_words=14,masked_flags=0,
                  aligned_near_callers=["056C:+1D5C","29AE:+2FE8","3FB3:+4018","408C:+41C1"],
                  natural_game_route_measured=False)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":
    main()
