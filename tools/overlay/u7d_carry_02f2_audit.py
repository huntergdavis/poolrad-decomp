#!/usr/bin/env python3
"""Full-state and synthetic native parity for complete u7D +02F2."""
import argparse
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
from party_reorder_oracle import check, build_c, run_c
from u5e_party_value_2879_audit import Cpu, EXE_SHA, OVR_SHA, physical, put16

RAM_SIZE = 1 << 20
ENTRY, END = 0x02F2, 0x03A3
DIGEST = "8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9"
TARGET = {"symbol": "pr_u7d_carry_02f2", "source": "overlay_u7d_carry_02f2.c"}

def source_audit(root):
    exe = (root / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (root / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA, "START.EXE hash changed")
    check(sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR hash changed")
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x7D)
    check(unit["ovr_offset"] == 0x23DCC and unit["entries"][1] == 0x03D5,
          "unit/public entry placement changed")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    body = code[ENTRY:END]
    check(len(body) == 177 and sha256(body).hexdigest() == DIGEST, "leaf hash changed")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoded = list(decoder.disasm(body, ENTRY))
    check(len(decoded) == 69 and decoded[-1].address == 0x03A0 and
          decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "4" and
          decoded[-1].address + decoded[-1].size == END, "leaf extent changed")
    starts = {i.address for i in decoded}
    for ins in decoded:
        check("call" not in ins.mnemonic, "leaf gained a call")
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str, 16) in starts, "branch leaves leaf")
    caller = {i.address: i for i in decoder.disasm(code[0x03D5:0x047A], 0x03D5)}
    check(caller[0x042E].mnemonic == "push" and caller[0x042E].op_str == "cs" and
          caller[0x042F].mnemonic == "call" and int(caller[0x042F].op_str, 16) == ENTRY,
          "aligned public-entry caller changed")
    print(f"u7D +02F2..+03A2: 177 bytes, 69 instructions, zero calls; SHA-256 {DIGEST}")
    print("public entry 1 +03D5: aligned PUSH CS +042E / CALL +042F -> +02F2")
    return body, decoded

def fixture(body, limits, values, nodes, *, ds=0x1000, ss=0x2000, sp=0x8000,
            array_segment=0x6000, array_offset=0x100, flags=0x7202, reload_alias=False):
    memory = bytearray([0xA5]) * RAM_SIZE
    cs = 0x3000
    at = physical(cs, ENTRY); memory[at:at + len(body)] = body
    for i, value in enumerate(limits): put16(memory, ds, 0x363A + 2*i, value)
    for i, value in enumerate(values): put16(memory, array_segment, (array_offset + 2*i) & 0xFFFF, value)
    for i, (segment, offset) in enumerate(nodes):
        next_segment, next_offset = nodes[i+1] if i+1 < len(nodes) else (0, 0)
        put16(memory, segment, (offset + 0x30) & 0xFFFF, 0xFFFF if i == 0 else 0x7FFF+i)
        put16(memory, segment, (offset + 0x104) & 0xFFFF, next_offset)
        put16(memory, segment, (offset + 0x106) & 0xFFFF, next_segment)
    segment, offset = nodes[0] if nodes else (0, 0)
    put16(memory, ds, 0x5D96, offset); put16(memory, ds, 0x5D98, segment)
    for displacement, value in [(0, 0x7000), (2, cs), (4, array_offset), (6, array_segment)]:
        put16(memory, ss, (sp + displacement) & 0xFFFF, value)
    if reload_alias:
        # INC changes the local pointer offset, so the second LES must reload it.
        segment, offset = nodes[0]
        for delta in (0, 1):
            put16(memory, segment, (offset + delta + 0x104) & 0xFFFF, 0)
            put16(memory, segment, (offset + delta + 0x106) & 0xFFFF, 0)
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, ENTRY, ds, 0x1111, ss, flags)
    return memory, cpu

def specifications():
    result = []
    limits = [i+3 for i in range(7)]
    for mask in range(128):
        nodes = [(0x4000+(i%2)*0x1000, 0x100+i*0x200) for i in range(mask%7)]
        result.append((f"carry-mask-{mask}", limits,
                       [limits[i] if mask & (1 << i) else 0 for i in range(7)], nodes, {}))
    rng = random.Random(0x7D02F2)
    for case in range(100):
        result.append((f"random-{case}", [rng.randrange(65536) for _ in range(7)],
                       [rng.randrange(65536) for _ in range(7)],
                       [(0x4000, 0x100+i*0x200) for i in range(rng.randrange(7))], {}))
    nodes = [(0x4000, 0x100), (0x5000, 0x300)]
    for sp in (0, 1, 2, 3, 4, 5, 6, 7, 8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF):
        result.append((f"stack-{sp:04x}", limits, limits, nodes,
                       {"sp": sp, "expect_nonreturn": sp in (5, 6)}))
    for sp in (5, 6):
        result.append((f"stack-empty-{sp:04x}", limits, limits, [], {"sp": sp}))
    for name, options, party in [
        ("DS-SS-shared", {"ss": 0x1000}, nodes),
        ("A20-stack", {"ss": 0xFFFF, "sp": 0x11}, nodes),
        ("A20-DS", {"ds": 0xFFFF}, nodes),
        ("A20-array", {"array_segment": 0xFFFF}, nodes),
        ("A20-record", {}, [(0xFFFF, 0x100), (0x5000, 0x300)]),
        ("array-offset-wrap", {"array_offset": 0xFFFD}, nodes),
        ("record-word-offset-wrap", {}, [(0x4000, 0xFFCF), (0x5000, 0x300)]),
        ("party-local-reload-alias", {"reload_alias": True}, [(0x2000, 0x7FC9)]),
        ("preserved-DF", {"flags": 0x0603}, nodes),
        ("incoming-arithmetic-flags", {"flags": 0x0ED7}, nodes),
    ]: result.append((name, limits, limits, party, options))
    result.append(("array-table-alias", [3]*7, [3]*7, nodes,
                   {"array_segment": 0x1000, "array_offset": 0x363A}))
    result.append(("array-frame-alias", [0xFFFF]*7, [0]*7, [],
                   {"array_segment": 0x2000, "array_offset": 0x7FF0}))
    result.append(("zero-thresholds", [0]*7, [0xFFFF]*7, nodes, {}))
    result.append(("max-thresholds", [0xFFFF]*7, [0xFFFF]*7, nodes, {}))
    return result

def differential(body, decoded, function):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn import x86_const as regs
    registers = [getattr(regs, "UC_X86_REG_" + ("EFLAGS" if n == "flags" else n.upper()))
                 for n, _ in Cpu._fields_]
    visited, edges = set(), set()
    conditional = {i.address: (i.address+i.size, int(i.op_str, 16)) for i in decoded
                   if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    cases = specifications()
    nonreturns = 0
    for label, limits, values, nodes, options in cases:
        options = dict(options)
        expect_nonreturn = options.pop("expect_nonreturn", False)
        memory, cpu = fixture(body, limits, values, nodes, **options)
        original = Uc(UC_ARCH_X86, UC_MODE_16)
        original.mem_map(0, 2*RAM_SIZE)
        original.mem_write(0, bytes(memory)); original.mem_write(RAM_SIZE, bytes(memory))
        for (name, _), register in zip(Cpu._fields_, registers): original.reg_write(register, getattr(cpu, name))
        start = physical(cpu.cs, ENTRY); last = physical(cpu.cs, 0x03A0)
        returned, previous = [False], [None]
        case_visited, case_edges = set(), set()
        def instruction(emulator, address, size, user):
            del size, user
            if returned[0]: emulator.emu_stop(); return
            check(start <= address < start+len(body), f"left original body: {label}")
            here = address-start+ENTRY
            case_visited.add(here)
            if previous[0] in conditional: case_edges.add((previous[0], here))
            previous[0] = here
            if address == last: returned[0] = True
        def mirror(emulator, access, address, size, value, user):
            del access, user
            for byte in range(size):
                raw = address+byte; at = raw & (RAM_SIZE-1)
                emulator.mem_write(at+RAM_SIZE if raw < RAM_SIZE else at,
                                   bytes([(value >> (8*byte)) & 0xFF]))
        original.hook_add(UC_HOOK_CODE, instruction); original.hook_add(UC_HOOK_MEM_WRITE, mirror)
        original.emu_start(start, 2*RAM_SIZE, count=20000)
        if expect_nonreturn:
            check(not returned[0], f"expected original return-budget failure changed: {label}")
            nonreturns += 1
            continue  # Never invoke uncapped C on a nonreturning original fixture.
        check(returned[0], f"original did not return within 20,000 instructions: {label}")
        expected = bytes(original.mem_read(0, RAM_SIZE))
        actual, cpu = run_c(function, memory, cpu)
        if actual != expected:
            differences = [i for i, (a,b) in enumerate(zip(actual,expected)) if a != b]
            raise ValueError(f"RAM mismatch {label}: {differences[:8]}")
        for (name, _), register in zip(Cpu._fields_, registers):
            check(getattr(cpu,name) == original.reg_read(register) & 0xFFFF,
                  f"{name} mismatch {label}: C={getattr(cpu,name):04x}, original={original.reg_read(register)&0xffff:04x}")
        visited.update(case_visited); edges.update(case_edges)
    check(len(visited) == len(decoded), f"instruction coverage {len(visited)}/{len(decoded)}")
    missing = {(site,outcome) for site,outcomes in conditional.items() for outcome in outcomes} - edges
    check(not missing, f"missing conditional edges {sorted(missing)}")
    print(f"{len(cases)-nonreturns} original-byte cases passed: full 1 MiB RAM, all 14 CPU words, "
          f"{len(visited)} instructions, all {sum(len(x) for x in conditional.values())} conditional outcomes")
    print(f"{nonreturns} malformed original fixtures fail the 20,000-instruction return budget; C not invoked")

def native_assembly(source):
    text = r'''.code16
.intel_syntax noprefix
.global _start
_start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xf000
    mov word ptr [header], ax
    mov word ptr [header+2], offset returned
    mov word ptr [header+4], offset leaf
    mov word ptr [case_ptr], offset fixtures
    mov dx, offset filename
    xor cx, cx
    mov ah, 0x3c
    int 0x21
    jc failure
    mov word ptr [handle], ax
    mov bx, ax
    mov dx, offset header
    mov cx, 6
    mov ah, 0x40
    int 0x21
again:
    cli
    mov sp, 0xf000
    mov ax, cs
    mov es, ax
    mov di, 0xefe0
    mov ax, 0xa5a5
    mov cx, 16
    cld
    rep stosw
    mov si, word ptr [case_ptr]
    lodsw
    mov word ptr [case_mask], ax
    lodsw
    mov word ptr [case_n], ax
    or ax, ax
    jz empty
    mov ax, 0x6000
    mov word ptr [0x5d96], ax
    mov ax, cs
    jmp head
empty:
    mov word ptr [0x5d96], ax
head:
    mov word ptr [0x5d98], ax
'''
    for base in (0x363A, 0x4000):
        for i in range(7): text += f"    lodsw\n    mov word ptr [{base+2*i}], ax\n"
    for i in range(6): text += f"    lodsw\n    mov word ptr [{0x6030+i*0x200}], ax\n"
    for i in range(6):
        at = 0x6104+i*0x200
        text += (f"    lodsw\n    mov word ptr [{at}], ax\n    or ax, ax\n"
                 f"    jz zero_{i}\n    mov ax, cs\nzero_{i}:\n    mov word ptr [{at+2}], ax\n")
    text += r'''
    mov word ptr [case_ptr], si
    push 0x0002
    popf
    mov ax, 0x1234
    mov bx, 0x3456
    mov cx, 0x5678
    mov dx, 0x789a
    mov si, 0x9abc
    mov di, 0xbcde
    mov bp, 0x7777
    push cs
    push 0x4000
    push cs
    call leaf
returned:
'''
    for i, (name, _) in enumerate(Cpu._fields_[:-1]):
        text += f"    mov word ptr [output+{2*i}], " + ("offset returned" if name == "ip" else name) + "\n"
    for i in range(16):
        text += f"    mov ax, word ptr [{0xEFE0+2*i}]\n    mov word ptr [output+{28+2*i}], ax\n"
    text += "    pushf\n    pop ax\n    mov word ptr [output+26], ax\n"
    snapshots = ([(0x363A+2*i, 60+2*i) for i in range(7)] +
                 [(0x4000+2*i, 74+2*i) for i in range(7)] +
                 [(0x5D96+2*i, 88+2*i) for i in range(2)] +
                 [(0x6030+0x200*i, 92+2*i) for i in range(6)] +
                 [(0x6104+0x200*i+2*j, 104+4*i+2*j) for i in range(6) for j in range(2)])
    for address, output in snapshots:
        text += f"    mov ax, word ptr [{address}]\n    mov word ptr [output+{output}], ax\n"
    text += r'''
    mov ax, word ptr [case_mask]
    mov word ptr [output+128], ax
    mov ax, word ptr [case_n]
    mov word ptr [output+130], ax
    sti
    mov bx, word ptr [handle]
    mov dx, offset output
    mov cx, 132
    mov ah, 0x40
    int 0x21
    jc failure
    cmp ax, 132
    jne failure
    inc word ptr [index]
    cmp word ptr [index], 128
    jb again
    mov bx, word ptr [handle]
    mov ah, 0x3e
    int 0x21
    mov ax, 0x4c00
    int 0x21
failure:
    mov ax, 0x4c01
    int 0x21
filename: .asciz "RESULT.BIN"
header: .space 6
handle: .word 0
index: .word 0
case_ptr: .word 0
case_mask: .word 0
case_n: .word 0
output: .space 132
fixtures:
'''
    for mask in range(128):
        n = mask%7
        values = [mask,n] + [i+3 for i in range(7)]
        values += [i+3 if mask & (1 << i) else 0 for i in range(7)]
        values += [0xFFFF if i == 0 else 0x7FFF+i for i in range(6)]
        values += [0x6000+0x200*(i+1) if i+1<n else 0 for i in range(6)]
        text += "    .word " + ",".join(str(v) for v in values) + "\n"
    return text + 'leaf:\n.incbin "' + str(source.resolve()) + '"\n'

def native_audit(root, body, function):
    directory = root / "resources/oracle-runs/u7d-carry-02f2-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    source = directory / "leaf.bin"; source.write_bytes(body)
    asm, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm.write_text(native_assembly(source))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary",
                    str(obj), "-o", str(com)], check=True)
    check(0x100+com.stat().st_size < 0x363A, "COM overlaps tested RAM")
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n"
                      "[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists(): result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=45)
    result = result_path.read_bytes()
    check(len(result) == 6+128*132, "native output incomplete")
    segment, return_ip, leaf_ip = struct.unpack_from("<HHH", result)
    check(com.read_bytes()[leaf_ip-0x100:leaf_ip-0x100+len(body)] == body,
          "copied native leaf differs from original")
    for index in range(128):
        record = result[6+index*132:6+(index+1)*132]
        mask,n = struct.unpack_from("<HH", record,128)
        check(mask == index and n == mask%7, "native metadata mismatch")
        memory = bytearray([0xA5]) * RAM_SIZE
        for i in range(7):
            put16(memory,segment,0x363A+2*i,i+3)
            put16(memory,segment,0x4000+2*i,i+3 if mask & (1 << i) else 0)
        for i in range(6):
            offset = 0x6000+0x200*i
            next_offset = offset+0x200 if i+1<n else 0
            put16(memory,segment,offset+0x30,0xFFFF if i == 0 else 0x7FFF+i)
            put16(memory,segment,offset+0x104,next_offset)
            put16(memory,segment,offset+0x106,segment if next_offset else 0)
        put16(memory,segment,0x5D96,0x6000 if n else 0)
        put16(memory,segment,0x5D98,segment if n else 0)
        for offset,value in [(0xEFF8,return_ip),(0xEFFA,segment),(0xEFFC,0x4000),(0xEFFE,segment)]:
            put16(memory,segment,offset,value)
        cpu = Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                  0x7777,0xEFF8,segment,leaf_ip,segment,segment,segment,0x0002)
        actual,cpu = run_c(function,memory,cpu)
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",record)):
            check(getattr(cpu,name) == expected,
                  f"native {name} mismatch {mask}: C={getattr(cpu,name):04x}, DOS={expected:04x}")
        ranges = [(0xEFE0,32,28),(0x363A,14,60),(0x4000,14,74),(0x5D96,4,88)]
        ranges += [(0x6030+0x200*i,2,92+2*i) for i in range(6)]
        ranges += [(0x6104+0x200*i,4,104+4*i) for i in range(6)]
        for offset,size,position in ranges:
            at = physical(segment,offset)
            check(actual[at:at+size] == record[position:position+size],
                  f"native RAM mismatch {mask}/{offset:04x}")
    print("128 DOSBox-X synthetic cases passed: all 14 CPU words, 32 frame bytes, "
          "seven thresholds/array words, party head, all six counters/links; no flag mask")
    print("native result SHA-256 " + sha256(result).hexdigest())

def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    args = parser.parse_args()
    body, decoded = source_audit(root)
    with tempfile.TemporaryDirectory(prefix="poolrad-u7d-carry-") as temp:
        function = build_c(root, Path(temp), TARGET)
        if args.unicorn: differential(body, decoded, function)
        if args.native: native_audit(root, body, function)

if __name__ == "__main__": main()
