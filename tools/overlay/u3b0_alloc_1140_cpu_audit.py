#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U3B0 +1140, composing SYSTEM GetMem (0AF8:023F), plus a
DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so GetMem runs from its original
bytes, loads unit 0x3B0, builds synthetic Turbo Pascal heaps and output pointers, and compares the entire 1 MiB
RAM and all 14 CPU words between the original under Unicorn and the C model, with no mask. The original decides each
stop: the fetch after its own RETF 6 (return), a GetMem site whose live bytes differ when fetched (CODE), GetMem reaching
RunError 00E2, an indirect HeapError call through a non-default pointer, or its free-list walk past the host cap
(CALLEE), the allocation-call budget (LIMIT), or a RET/RETF that does not come back to its call (TRANSFER). Any fetch outside the pre-decoded spans is an
error. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16
import system_getmem_023f_audit as g

ENTRY, END, BODY_RETF = 0x1140, 0x11F3, 0x11F0
SITES = (0x114A, 0x11A8)
CALLER_CS, DS, SS = 0x8800, g.DS, g.SS
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER, LIMIT = 0, 2, 3, 4, 5
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
FLAGS = g.FLAGS


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image, gdecoded, _, digests = g.source()
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u = units[0x3B0]
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[ENTRY:END]
    insns = list(md.disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in insns if "call" in i.mnemonic]
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    sha = sha256(body).hexdigest()
    check(len(body) == 179 and len(insns) == 58 and sum(i.size for i in insns) == 179
          and insns[-1].address == BODY_RETF and insns[-1].mnemonic == "retf" and insns[-1].op_str == "6"
          and insns[-1].address + insns[-1].size == END and code[END:END+3] == bytes.fromhex("5589e5")
          and calls == [(s, "0xaf8, 0x23f") for s in SITES]
          and all(s + 3 in set(u["fixups"]) for s in SITES)
          and sha == "fdced3a4a7f0c069d55a9a4fef8a048eca70e649ec66559b84240259d2280c5c",
          f"U3B0 +1140 extent/calls differ: {calls}")
    decoded = {("u3b0", i.address): i for i in insns}
    decoded.update({("sys", a): i for a, i in gdecoded.items()})
    print(f"U3B0 [1140,11F3): 179 bytes / 58 instructions, no trailing DATA; GAME file {u['ovr_offset'] + ENTRY:#x}; sha {sha}")
    return dict(conditional=conditional, starts=[i.address for i in insns], sha=sha,
                file_offset=u["ovr_offset"] + ENTRY, getmem_spans=digests), code, image, decoded


def library(directory):
    path = directory / "u3b01140.so"
    sources = ["overlay_u3b0_entry6_1140.c", "system_getmem_023f_cpu.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], check=True)
    function = ctypes.CDLL(str(path)).pr_u3b0_entry6_1140_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu),
                         ctypes.c_uint, ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def build(code, image, rng, *, nodes, blocks=(), top=(g.HEAP + 0x400, 0), end=(g.HEAP + 0x1000, 0),
          heap_error=(0x00A9, g.SYSTEM_CS), sp=0x8000, ss=SS, flags=0x0602,
          output=None, background=None, patch=(), writes=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    g.heap(memory, blocks=blocks, top=top, end=end, heap_error=heap_error)
    outseg, outoff = output or (DS, 0x7000)
    for i, value in enumerate((RET_IP, RET_CS, outoff, outseg, nodes)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
    for segment, offset, data in writes:
        at=physical(segment,offset)
        for j,value in enumerate(data): memory[(at+j)&(SIZE-1)]=value
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), True


def compare(label, memory, cpu, high, target, decoded, function, cap=64, allocations=128, count=2000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(stop=None, returning=False, returned=False, previous=None, last=None, steps=0, calls=[], unexpected=None, sites=0, callee=-1, closure=set(), closure_edges=set())
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        if state["returning"]:
            state["returning"] = False
            if (cs, ip) != state["calls"].pop():
                state["callee"] = g.TRANSFER if state["calls"] else 0
                state["stop"] = CALLEE if state["calls"] else TRANSFER          # nested inside GetMem, or a direct callee
                return emulator.emu_stop()
        if state["last"] == ("sys", g.ENTRY_RETF): state["callee"] = 0
        kind = "u3b0" if cs == CALLER_CS else "sys" if cs == g.SYSTEM_CS else None
        if kind == "sys" and ip == g.RUNERROR_IP:
            state["callee"] = g.RUNERROR
            state["stop"] = CALLEE
            return emulator.emu_stop()
        key = (kind, ip)
        if key not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        if kind == "sys" and ip in g.HEAP_ERROR_SITES and bytes(emulator.mem_read(physical(cpu.ds, 0x4414), 4)) != struct.pack("<HH", 0x00A9, g.SYSTEM_CS):
            state["callee"] = g.HEAP_ERROR
            state["stop"] = CALLEE
            return emulator.emu_stop()
        if kind == "sys" and ip == g.WALK_HEAD and state["last"] in (("sys", a) for a in g.WALK_JAS):
            state["steps"] += 1
            if state["steps"] > cap:
                state["callee"] = g.LIMIT
                state["stop"] = CALLEE
                return emulator.emu_stop()
        ins = decoded[key]
        if kind == "u3b0" and ENTRY <= ip < END:
            if ip in SITES:
                if bytes(emulator.mem_read(address, 3)) != b"\x9a\x3f\x02":
                    state["stop"] = CODE
                    return emulator.emu_stop()
                if state["sites"] >= allocations:
                    state["stop"] = LIMIT
                    return emulator.emu_stop()
                state["sites"] += 1
                state["steps"] = 0
            visited.add(ip)
            if state["previous"] in target["conditional"] and state["previous"] != ip:
                edges.add((state["previous"], ip))
            state["previous"] = ip
        else:
            state["previous"] = None
        state["closure"].add(key)
        if state["last"] in decoded:
            prior=decoded[state["last"]]
            if prior.mnemonic.startswith("j") and prior.mnemonic != "jmp":
                state["closure_edges"].add((state["last"],key))
        if ins.mnemonic in ("call", "lcall"):
            state["calls"].append((cs, (ip + ins.size) & 0xFFFF))
        elif ins.mnemonic in ("ret", "retf"):
            if kind == "u3b0" and ip == BODY_RETF:
                state["returned"] = True
            else:
                state["returning"] = True
        state["last"] = key
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(CALLER_CS, ENTRY), 2 * SIZE, count=count)
    check(state["unexpected"] is None, f"original left the decoded code at {state['unexpected']}: {label}")
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee_status = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), allocations, cap, ctypes.byref(callee_status))
    check(status == state["stop"], f"C status {status} (callee {callee_status.value}), original stopped with "
          f"{NAMES[state['stop']]}: {label}")
    check(callee_status.value == state["callee"], f"callee diagnostic differs: {label}: {callee_status.value}/{state['callee']}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges, callee_status.value, state["sites"], state["closure"], state["closure_edges"]


def cases(rng):
    spec=[]
    for i in range(240):
        blocks, top, end = g.random_heap(rng)
        spec.append((f"random-{i}", dict(nodes=rng.choice((0,1,2,3,4,7,16,0x7fff,0x8000,0xffff)),
            blocks=blocks,top=top,end=end,flags=FLAGS[i%len(FLAGS)],sp=rng.randrange(0x400,0xf000)&~1)))
    for count in (0,1,2,3,16,127,0x8000,0xffff):
        spec.append((f"bump-{count}",dict(nodes=count)))
    for n in (1,2,3):
        spec += [(f"exact-{n}",dict(nodes=n,blocks=((g.HEAP+0x10,0,3,0),))),
                 (f"split-{n}",dict(nodes=n,blocks=((g.HEAP+0x10,8,12,8),))),
                 (f"fail-call-{n}",dict(nodes=4,end=(g.HEAP+0x400+3*(n-1),0)))]
    for end in ((g.HEAP+0x400,0),(g.HEAP+0x1000,0)):
        spec.append((f"custom-heap-error-{end}",dict(nodes=2,end=end,heap_error=(0x1234,0x5678))))
    spec.append(("cycle",dict(nodes=2,blocks=((g.HEAP+0x10,0,0,0),),
          writes=((g.HEAP+0x10,0,struct.pack("<HHHH",0,g.HEAP+0x10,0,0)),),cap=3)))
    # A successful first call must not consume the second call's walk budget.
    spec.append(("per-call-budget",dict(nodes=3,blocks=((g.HEAP+0x10,0,0,8),),cap=1)))
    for site in SITES:
        for delta,value in ((0,0x90),(1,0x40),(2,0x03)):
            spec.append((f"guard-{site:04x}-{delta}",dict(nodes=3,patch=((site+delta,value),))))
    for n in (0,1,2,3):
        spec.append((f"allocation-limit-{n}",dict(nodes=7,allocations=n)))
    spec.append(("guard-before-budget",dict(nodes=2,patch=((SITES[0],0x90),),allocations=0)))
    for sp,ss in ((0x40,SS),(0xfff6,SS),(0x400,0xffff),(0x8000,DS)):
        spec.append((f"stack-{ss:04x}-{sp:04x}",dict(nodes=3,sp=sp,ss=ss)))
    for output in ((DS,0xffff),(SS,0x8000),(DS,0x440c)):
        spec.append((f"output-alias-{output}",dict(nodes=1,output=output)))
    spec.append(("random-background",dict(nodes=4,background=bytes(rng.randbytes(SIZE)))))
    return spec


def differential(target, code, image, decoded, function):
    rng=random.Random(0x3B01140)
    visited,edges,closure,closure_edges=set(),set(),set(),set()
    stops,diagnostics,sites={}, {},0
    for label, options in cases(rng):
        cap=options.pop("cap",64); allocations=options.pop("allocations",128)
        memory,cpu,high=build(code,image,rng,**options)
        stop,v,e,callee,n,cv,ce=compare(label,memory,cpu,high,target,decoded,function,cap,allocations)
        stops[NAMES[stop]]=stops.get(NAMES[stop],0)+1
        diagnostics[str(callee)]=diagnostics.get(str(callee),0)+1
        visited|=v;edges|=e;closure|=cv;closure_edges|=ce;sites+=n
        if not label.startswith("random"): print(label,NAMES[stop],callee)
    possible={(s,o) for s,outs in target["conditional"].items() for o in outs}
    check(visited==set(target["starts"]) and edges==possible,"caller coverage incomplete")
    all_edges={(key,(key[0],out)) for key,i in decoded.items()
               if i.mnemonic.startswith("j") and i.mnemonic!="jmp"
               for out in (i.address+i.size,int(i.op_str,16))}
    result=dict(cases=sum(stops.values()),stops=stops,callee_statuses=diagnostics,
        caller_starts=len(visited),caller_outcomes=len(edges),site_fetches=sites,
        closure_starts=len(closure),closure_total_starts=len(decoded),
        closure_outcomes=len(closure_edges),closure_total_outcomes=len(all_edges),
        missing_starts=sorted(f"{k}:{a:04X}" for k,a in decoded if (k,a) not in closure),
        missing_edges=sorted(f"{k}:{a:04X}->{o:04X}" for (k,a),(_,o) in all_edges-closure_edges))
    print("Original-byte full 1 MiB RAM / CPU14 unmasked:",result)
    return result


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, CALLER_N, DATA_DS, ARENA_N = 0x7000, 0x8800, 0x5000, 0x6000
MAIN_AT, SPAN_AT, TABLE_AT, CASES_AT, CODE_AT = 0x0300, 0x0C00, 0x1200, 0x1400, 0x2000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, ARENA_BYTES, SPAN_BYTES = STACK_TOP - 0x100, 0x100, 0x4000, 0x0500
CASE_SIZE, NATIVE_CASES = 40, 64
RECORD_BYTES = 30 + WINDOW_SIZE + 0x10000 + ARENA_BYTES
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases(rng):
    result=[]
    for n in range(NATIVE_CASES):
        blocks=[]
        if n%4==1: blocks=[(ARENA_N+0x10,3,0)]
        if n%4==2: blocks=[(ARENA_N+0x10,12,8)]
        if n%4==3: blocks=[(ARENA_N+0x10,0,8),(ARENA_N+0x20,3,0)]
        top=(ARENA_N+0x100,n%2*8)
        end=(top[0]+(0 if n%8==0 else 3 if n%8==1 else 0x100),top[1])
        nodes=(0,1,2,3,5,8,0x8000,0xffff)[n%8]
        if n%16 < 3:
            blocks=[]; nodes=3
            end=(top[0]+3*(n%16),top[1])  # fail calls one, two, three
        result.append(dict(flags=n%len(NATIVE_FLAGS),index=0,word=nodes,
                           head=(0,0),blocks=blocks,top=top,end=end))
    return result


def case_bytes(case):
    data = struct.pack("<BBH2H4HB", case["flags"], case["index"], case["word"], case["head"][1], case["head"][0], case["top"][1], case["top"][0],
                       case["end"][1], case["end"][0], len(case["blocks"]))
    for seg, paragraphs, granule in case["blocks"]:
        data += struct.pack("<HBB", seg, paragraphs, granule)
    return data + bytes(CASE_SIZE - len(data))


def native_state(memory, case, tables):
    at = physical(DATA_DS, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
    at = physical(ARENA_N, 0); memory[at:at + ARENA_BYTES] = bytes([0xA5]) * ARENA_BYTES
    put16(memory, DATA_DS, 0x4400, 0); put16(memory, DATA_DS, 0x4402, ARENA_N)
    put16(memory, DATA_DS, 0x4404, case["top"][1]); put16(memory, DATA_DS, 0x4406, case["top"][0])
    put16(memory, DATA_DS, 0x4408, case["end"][1]); put16(memory, DATA_DS, 0x440A, case["end"][0])
    put16(memory, DATA_DS, 0x4414, 0x00A9); put16(memory, DATA_DS, 0x4416, SYS_N)
    first = (0, case["blocks"][0][0]) if case["blocks"] else (case["top"][1], case["top"][0])
    put16(memory, DATA_DS, 0x440C, first[0]); put16(memory, DATA_DS, 0x440E, first[1])
    for i, (seg, paragraphs, granule) in enumerate(case["blocks"]):
        nxt = (0, case["blocks"][i + 1][0]) if i + 1 < len(case["blocks"]) else (case["top"][1], case["top"][0])
        put16(memory, seg, 0, nxt[0]); put16(memory, seg, 2, nxt[1]); put16(memory, seg, 4, granule); put16(memory, seg, 6, paragraphs)



def native_audit(code, image, function):
    directory = ROOT / "resources/oracle-runs/u3b0-1140-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    span = image[physical(g.SYSTEM_CS, 0):physical(g.SYSTEM_CS, SPAN_BYTES)]
    unit = bytearray(code)
    for site in SITES:
        struct.pack_into("<H", unit, site + 3, SYS_N)
    rng = random.Random(0x3B01140D)
    tables = bytes(rng.randrange(256) for _ in range(0x140))
    cases = native_cases(rng)
    check(SPAN_AT + len(span) <= TABLE_AT and TABLE_AT + len(tables) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= CODE_AT
          and CODE_AT + len(unit) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, label, length in ((SYS_N, "span", len(span)), (CALLER_N, "unit", len(unit))):
        t += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", "    xor di,di", f"    mov cx,{length:#x}", "    rep movsb"]
    t += [f"    mov ax,{SYS_N:#x}", "    mov es,ax", "    mov byte ptr es:[0xe2],0xea", "    mov word ptr es:[0xe3],offset trap", "    mov word ptr es:[0xe5],cs",
          "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
          f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
          "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
          "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
          "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
          "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
          f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
          f"    mov ax,{ARENA_N:#x}", "    mov es,ax", "    xor di,di", f"    mov cx,{ARENA_BYTES // 2:#x}", "    mov ax,0xa5a5", "    rep stosw",
          f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov word ptr es:[0x4400],0", f"    mov word ptr es:[0x4402],{ARENA_N:#x}",
          "    mov ax,word ptr [si+8]", "    mov word ptr es:[0x4404],ax", "    mov ax,word ptr [si+10]", "    mov word ptr es:[0x4406],ax",
          "    mov ax,word ptr [si+12]", "    mov word ptr es:[0x4408],ax", "    mov ax,word ptr [si+14]", "    mov word ptr es:[0x440a],ax",
          "    mov word ptr es:[0x4414],0xa9", f"    mov word ptr es:[0x4416],{SYS_N:#x}",
          "    mov ax,word ptr [si+8]", "    mov dx,word ptr [si+10]", "    cmp byte ptr [si+16],0", "    je list_head",
          "    xor ax,ax", "    mov dx,word ptr [si+17]", "list_head:", "    mov word ptr es:[0x440c],ax", "    mov word ptr es:[0x440e],dx",
          "    xor ch,ch", "    mov cl,byte ptr [si+16]", "    jcxz blocks_done", "    lea bx,[si+17]", "block_loop:",
          "    mov es,word ptr [bx]", "    xor ax,ax", "    mov dx,word ptr [bx+4]", "    cmp cx,1", "    jne block_link",
          "    mov ax,word ptr [si+8]", "    mov dx,word ptr [si+10]", "block_link:",
          "    mov word ptr es:[0],ax", "    mov word ptr es:[2],dx", "    mov al,byte ptr [bx+3]", "    xor ah,ah", "    mov word ptr es:[4],ax",
          "    mov al,byte ptr [bx+2]", "    xor ah,ah", "    mov word ptr es:[6],ax", "    add bx,4", "    loop block_loop", "blocks_done:",
          "    push cs", "    pop es", "    push word ptr [si+2]", f"    mov ax,{DATA_DS:#x}", "    push ax",
          "    mov ax,0x7000", "    push ax",
          "    mov bl,byte ptr [si]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        t.append(f"    mov {name},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:", "    mov word ptr cs:[status],0", "    jmp record",
          "trap:", "    mov word ptr cs:[status],1", "record:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds", f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cmp word ptr cs:[status],0", "    jne trapped",
          f"    mov word ptr cs:[{OUTPUT + 16:#x}],cs", f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", "    jmp segments",
          "trapped:", f"    mov word ptr cs:[{OUTPUT + 16:#x}],{SYS_N:#x}", f"    mov word ptr cs:[{OUTPUT + 18:#x}],0xe2", "segments:",
          "    mov ax,word ptr cs:[status]", f"    mov word ptr cs:[{OUTPUT + 28:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 30:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{30 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{ARENA_N:#x}", "    mov ds,ax", "    xor dx,dx", f"    mov cx,{ARENA_BYTES:#x}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    push cs", "    pop ds", "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "status: .word 0",
          f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("span", span, SPAN_AT), ("tables", tables, TABLE_AT), ("cases", b"".join(case_bytes(c) for c in cases), CASES_AT),
                               ("unit", bytes(unit), CODE_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(t) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    statuses = {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(SYS_N, 0); memory[at:at + len(span)] = span
        memory[physical(SYS_N, 0xE2):physical(SYS_N, 0xE2) + 5] = struct.pack("<BHH", 0xEA, 0, 0)
        at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
        native_state(memory, case, tables)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (case["word"], DATA_DS, 0x7000, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee_status = ctypes.c_int(99)
        status = function(ram, SIZE, ctypes.byref(cpu), 128, 64, ctypes.byref(callee_status))
        trapped = struct.unpack_from("<H", record, 28)[0]
        check((status == OK and trapped == 0) or (status == CALLEE and callee_status.value == g.RUNERROR and trapped == 1),
              f"native case {n}: C status {status}/{callee_status.value}, DOS trapped {trapped}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        got += bytes(ram[physical(DATA_DS, 0):physical(DATA_DS, 0) + 0x10000]) + bytes(ram[physical(ARENA_N, 0):physical(ARENA_N, 0) + ARENA_BYTES])
        if got != record[30:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[30:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        key = "return" if status == OK else "GetMem RunError"
        statuses[key] = statuses.get(key, 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({statuses}); all 14 CPU words, the {WINDOW_SIZE}-byte stack window, "
          f"the whole 64 KiB DS segment and the {ARENA_BYTES:#x}-byte heap arena; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U3B0 at {CALLER_N:04X}, SYSTEM at {SYS_N:04X} (RunError trapped at 00E2)")
    return dict(cases=len(cases), statuses=statuses, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image, decoded = source()
    report = dict(target=dict(sha256=target["sha"], bytes=179, instructions=58, file_offset=target["file_offset"],getmem_spans=target["getmem_spans"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u3b01140-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, image, decoded, function)
            if args.native:
                report["native"] = native_audit(code, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
