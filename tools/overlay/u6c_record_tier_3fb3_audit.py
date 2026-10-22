#!/usr/bin/env python3
"""Complete U6C +3FB3/+3F31 full-state parity and architectural boundaries."""

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
from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_AX, UC_X86_REG_CX, UC_X86_REG_DX

from overlay_inventory import find_units, parse_mz
from u6c_record_table_sum_3f31_audit import (
    Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16,
    source as leaf_source, make_case as leaf_case,
)

ENTRY, END = 0x3FB3, 0x408C
BODY_SHA = "bf1e24f3fbf0660c08259b756b918b0685b1488c3c530599e55e80778ebd397f"


def source():
    leaf,leaf_decoded,_ = leaf_source()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if sha256(exe).hexdigest() != EXE_SHA or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("source provenance differs")
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],ovr) if u["stub_para"] == 0x6C)
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"]+unit["code_size"]]
    body = code[ENTRY:END]
    dec = list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,ENTRY))
    if len(body) != 217 or sha256(body).hexdigest() != BODY_SHA or len(dec) != 88 or sum(i.size for i in dec) != 217:
        raise ValueError("U6C caller extent differs")
    decoded = {i.address:i for i in dec}
    if dec[-1].address != 0x4089 or dec[-1].mnemonic != "retf" or dec[-1].op_str != "4":
        raise ValueError("caller return differs")
    calls = [(i.address,i.mnemonic,i.op_str) for i in dec if "call" in i.mnemonic]
    if calls != [(0x4018,"call","0x3f31")] or decoded[0x4017].op_str != "cs":
        raise ValueError("composed callee differs")
    for ip in (0x3FD6,0x3FE7,0x402E,0x404B,0x406D):
        if body[ip-ENTRY] != 0x98 or decoded[ip].size != 1:
            raise ValueError("16-bit CBW differs")
    for ip in (0x402F,0x4055,0x4077):
        if body[ip-ENTRY] != 0x99 or decoded[ip].size != 1:
            raise ValueError("16-bit CWD differs")
    for i in dec:
        if i.mnemonic.startswith("j") and int(i.op_str,16) not in decoded:
            raise ValueError("branch leaves caller")
        if i.mnemonic.startswith("int"):
            raise ValueError("caller gained interrupt instruction")
    if any(ENTRY <= p < END for p in unit["fixups"]):
        raise ValueError("caller gained fixup")
    caller = {i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[0x209F:0x29AE],0x209F)}
    for site in (0x2531,0x2546,0x2786,0x279B):
        if caller[site].mnemonic != "call" or caller[site].op_str != "0x3fb3" or caller[site-1].op_str != "cs":
            raise ValueError("aligned incoming caller differs")
        if caller[site-9].op_str != "word ptr [0x5d94]" or caller[site-5].op_str != "word ptr [0x5d92]":
            raise ValueError("active-character arguments differ")
    return body,{**leaf_decoded,**decoded},dict(leaf=leaf,caller_suffix=code[0x2528:0x2534])


CHECKER_ROWS = ('OVL_006C_3FB3',)


def library(directory):
    path = directory / "u6c_3fb3.so"
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Wpedantic","-Werror",
        "-fPIC","-shared","-I",str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u6c_record_tier_3fb3.c"),
        str(ROOT / "src/recovered/overlay_u6c_record_table_sum_3f31.c"),"-o",str(path)],
        check=True,capture_output=True,text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u6c_record_tier_3fb3.argtypes = [ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    lib.pr_u6c_record_tier_3fb3.restype = ctypes.c_int
    return lib


def make_case(body,leaf,**options):
    memory,cpu = leaf_case(leaf,**options)
    at = physical(cpu.cs,ENTRY)
    memory[at:at+len(body)] = body
    cpu.ip = ENTRY
    return memory,cpu


def compare(label,memory,cpu,decoded,lib):
    original = Uc(UC_ARCH_X86,UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing))
    original.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        original.reg_write(reg,getattr(cpu,name))
    code_start = physical(cpu.cs,0x3F31)
    expected_code = memory[code_start:code_start+END-0x3F31]
    visited,edges,previous,stop,idiv = set(),set(),[None],[None],[False]

    def on_instruction(uc,address,size,user):
        del size,user
        ip = uc.reg_read(UC_X86_REG_IP)
        if not code_start <= address < code_start+END-0x3F31 or ip not in decoded:
            stop[0] = 3
            uc.emu_stop()
            return
        if previous[0] is not None and decoded[previous[0]].mnemonic.startswith("j") and decoded[previous[0]].mnemonic != "jmp":
            edges.add((previous[0],ip))
        previous[0] = ip
        visited.add(ip)
        if ip == 0x4089:
            stop[0] = 0
            uc.emu_stop()
        if ip in (0x4056,0x4078):
            dividend = (uc.reg_read(UC_X86_REG_DX) << 16) | uc.reg_read(UC_X86_REG_AX)
            if dividend & 0x80000000:
                dividend -= 0x100000000
            divisor = uc.reg_read(UC_X86_REG_CX)
            if divisor & 0x8000:
                divisor -= 0x10000
            quotient = (abs(dividend)//abs(divisor)) * (-1 if (dividend < 0) != (divisor < 0) else 1) if divisor else 0
            if divisor == 0 or not -32768 <= quotient <= 32767:
                stop[0] = 2
                uc.emu_stop()
            else:
                idiv[0] = True

    hook = original.hook_add(UC_HOOK_CODE,on_instruction)
    original.emu_start(physical(cpu.cs,ENTRY),2*SIZE,count=3000)
    if stop[0] is None:
        raise ValueError(f"original did not reach return/fault/transfer: {label}")
    original.hook_del(hook)
    if stop[0] == 0:
        original.emu_start(physical(cpu.cs,0x4089),2*SIZE,count=1)
    if bytes(backing[code_start:code_start+END-0x3F31]) != expected_code:
        raise ValueError(f"immutable code changed: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = lib.pr_u6c_record_tier_3fb3(ram,SIZE,ctypes.byref(cpu))
    if status != stop[0]:
        raise ValueError(f"status differs {label}: {status}/{stop[0]}")
    actual,expected = bytes(ram),bytes(backing)
    if actual != expected:
        diffs = [i for i,(a,b) in enumerate(zip(actual,expected)) if a != b]
        raise ValueError(f"RAM differs {label}: {diffs[:8]}")
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected_word = original.reg_read(reg) & 0xFFFF
        mask = (0xFFFF ^ 0x08D5) if name == "flags" and idiv[0] else 0xFFFF
        if (getattr(cpu,name) ^ expected_word) & mask:
            raise ValueError(f"{name} differs {label}: {getattr(cpu,name):04X}/{expected_word:04X}")
    return visited,edges,status,idiv[0]


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
    body,decoded,meta = source()
    visited,edges,count = set(),set(),0
    results = dict(returns=0,divide_errors=0,transfers=0,divisions=0,clamps=0)
    with tempfile.TemporaryDirectory(prefix="poolrad-u6c-3fb3-") as tmp:
        lib = library(Path(tmp))

        def run(label,**options):
            nonlocal visited,edges,count
            memory,cpu = make_case(body,meta["leaf"],**options)
            sites,branches,status,idiv = compare(label,memory,cpu,decoded,lib)
            visited |= sites
            edges |= branches
            count += 1
            results[{0:"returns",2:"divide_errors",3:"transfers"}[status]] += 1
            if status == 0:
                results["divisions" if idiv else "clamps"] += 1

        for table in range(0,256,16 if args.quick else 1):
            for gate in (1,2,16,30,63,127):
                run(f"table-gate-{table}-{gate}",table=table,gates=[gate]*8)
        for value in range(256):
            run(f"signed-gate-{value}",gates=[value]*8)
        for enabled in range(256):
            run(f"enabled-{enabled}",gates=[4 if enabled & (1 << i) else 0 for i in range(8)])
        for type_byte in range(256):
            for kind in (2,3):
                run(f"type-{type_byte}-{kind}",type_byte=type_byte,kind=kind,gates=[30]*8)
        for offset in range(0x7F50,0x8009,4 if args.quick else 1):
            run(f"record-frame-{offset:04X}",record=(0x2000,offset))
        for sp in range(0x4070,0x40C0,4 if args.quick else 1):
            run(f"table-frame-{sp:04X}",ss=0x1000,sp=sp)
        for ss in (0,0x2000,0xFFFF):
            for sp in tuple(range(0,33))+(0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):
                run(f"frame-wrap-{ss:04X}-{sp:04X}",ss=ss,sp=sp)
        for segment in (0,0x4000,0xFFFF):
            for offset in (0,1,0xFF69,0xFF6A,0xFFEB,0xFFFF):
                run(f"record-wrap-{segment:04X}-{offset:04X}",record=(segment,offset))
        rng = random.Random(0x6C3FB3)
        for index in range(8 if args.quick else 128):
            run(f"seeded-{index}",type_byte=rng.randrange(256),kind=rng.randrange(256),table=rng.randrange(256),
                gates=[rng.randrange(256) for _ in range(8)],ds=rng.choice((0,0x1000,0xF000,0xFFFF)),
                flags=rng.choice((0x0202,0x0ED7,0x0047,0x0A93)),background=rng.randbytes(SIZE))
    required = {(ip,target) for ip,ins in decoded.items() if ins.mnemonic.startswith("j") and ins.mnemonic != "jmp"
                for target in (int(ins.op_str,16),ip+ins.size)}
    if visited != set(decoded) or edges != required or any(n == 0 for n in results.values()):
        raise ValueError(f"site/edge/boundary coverage incomplete: sites {len(visited)}/{len(decoded)}, edges {len(edges)}/{len(required)}, {results}")
    report = dict(target="OVL_006C_3FB3",source_sha256=BODY_SHA,cases=count,
                  covered_sites=len(visited),source_sites=len(decoded),conditional_edges=len(edges),
                  caller_sites=88,callee_sites=44,compared_ram_bytes=SIZE,compared_cpu_words=14,
                  undefined_arithmetic_flags_masked_only_after_idiv=True,**results,
                  aligned_incoming_callers=["209F:+2531","209F:+2546","209F:+2786","209F:+279B"],
                  natural_game_route_measured=False)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":
    main()
