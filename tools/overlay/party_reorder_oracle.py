"""Shared full-state oracle for independently assigned u6A party reorder leaves."""
import argparse
from collections import deque
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
from u5e_party_value_2879_audit import Cpu, EXE_SHA, OVR_SHA, physical, put16

RAM_SIZE = 1 << 20

def check(condition, message):
    if not condition: raise ValueError(message)

def source_audit(root, target):
    exe = (root / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (root / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA, "START.EXE hash changed")
    check(sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR hash changed")
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x6A)
    check(unit["ovr_offset"] == 0x17831 and unit["memory_segment"] == 0x75,
          "u6A placement changed")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    entry, end = target["entry"], target["end"]
    body = code[entry:end]
    check(sha256(body).hexdigest() == target["digest"], "leaf hash changed")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoded = list(decoder.disasm(body, entry))
    check(len(decoded) == target["instructions"] and
          decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "" and
          decoded[-1].address + decoded[-1].size == end, "leaf extent changed")
    starts = {i.address for i in decoded}
    for ins in decoded:
        check("call" not in ins.mnemonic, "leaf gained a call")
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str, 16) in starts, "branch leaves leaf")
    caller = target["caller"]
    sites = {i.address: i for i in decoder.disasm(code[0x1780:0x1E6F], 0x1780)}
    check(sites[caller - 1].mnemonic == "push" and sites[caller - 1].op_str == "cs" and
          sites[caller].mnemonic == "call" and int(sites[caller].op_str, 16) == entry,
          "aligned caller changed")
    pending, seen, path = deque([(unit["entries"][1], [])]), set(), None
    while pending:
        at, chain = pending.popleft()
        if at in seen or not 0 <= at < len(code): continue
        seen.add(at)
        if at == entry: path = chain; break
        ins = next(decoder.disasm(code[at:at + 15], at, count=1), None)
        if not ins or ins.mnemonic.startswith("ret") or ins.mnemonic in ("int", "iret"): continue
        if ins.mnemonic == "call":
            try: pending.append((int(ins.op_str, 16), chain + [(at, int(ins.op_str, 16))]))
            except ValueError: pass
        if ins.mnemonic.startswith("j") or ins.mnemonic.startswith("loop"):
            try: pending.append((int(ins.op_str, 16), chain))
            except ValueError: pass
            if ins.mnemonic == "jmp": continue
        pending.append((at + ins.size, chain))
    check(path is not None and path[-1][0] == caller, "public-entry CFG path absent")
    print(f"u6A +{entry:04X}..+{end-1:04X}: {len(body)} bytes, {len(decoded)} instructions, "
          f"zero calls; SHA-256 {target['digest']}")
    print("public-entry static call path:", [(hex(a), hex(b)) for a, b in path])
    return body, decoded

def build_c(root, directory, target):
    path = directory / "reorder.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-shared", "-fPIC", "-I", str(root / "include"),
                    str(root / "src/recovered" / target["source"]), "-o", str(path)],
                   check=True, capture_output=True, text=True)
    library = ctypes.CDLL(str(path))
    function = getattr(library, target["symbol"])
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function

def run_c(function, memory, cpu):
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    check(function(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, "C rejected fixture")
    return bytes(ram), cpu

def fixture(body, target, nodes, active, *, ds=0x1000, ss=0x2000, sp=0x8000, flags=0x7202):
    memory = bytearray([0xA5]) * RAM_SIZE
    cs = 0x3000
    at = physical(cs, target["entry"])
    memory[at:at + len(body)] = body
    for index, (segment, offset) in enumerate(nodes):
        next_segment, next_offset = nodes[index + 1] if index + 1 < len(nodes) else (0, 0)
        put16(memory, segment, (offset + 0x104) & 0xFFFF, next_offset)
        put16(memory, segment, (offset + 0x106) & 0xFFFF, next_segment)
    put16(memory, ds, 0x5D92, nodes[active][1])
    put16(memory, ds, 0x5D94, nodes[active][0])
    put16(memory, ds, 0x5D96, nodes[0][1])
    put16(memory, ds, 0x5D98, nodes[0][0])
    put16(memory, ss, sp, 0x7000)
    put16(memory, ss, (sp + 2) & 0xFFFF, cs)
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, target["entry"], ds, 0x1111, ss, flags)
    return memory, cpu

def differential(body, decoded, function, target):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn import x86_const as regs
    registers = [getattr(regs, "UC_X86_REG_" + ("EFLAGS" if n == "flags" else n.upper()))
                 for n, _ in Cpu._fields_]
    specifications = []
    for mixed in (0, 1):
        for n in range(1, 9):
            nodes = [(0x4000 + (i % 2) * 0x1000 * mixed, 0x100 + i * 0x200) for i in range(n)]
            specifications += [(f"list-{mixed}-{n}-{a}", nodes, a, {}) for a in range(n)]
    rng = random.Random(target["entry"])
    for index in range(100):
        n = rng.randrange(1, 13)
        nodes = [(0x4000 + rng.randrange(4) * 0x1000, 0x100 + i * 0x200) for i in range(n)]
        specifications.append((f"random-{index}", nodes, rng.randrange(n), {}))
    nodes = [(0x4000, 0x100 + i * 0x200) for i in range(4)]
    specifications += [(f"stack-{sp:04x}", nodes, 0, {"sp": sp})
                       for sp in (0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 0xFFFD, 0xFFFE, 0xFFFF)]
    specifications += [
        ("successor-local-head-alias", nodes, 0, {"ss": 0x1000, "sp": 0x5DA4}),
        ("current-head-successor-active-alias", nodes, 0, {"ss": 0x1000, "sp": 0x5DA0}),
        ("previous-head-current-active-alias", nodes, 0, {"ss": 0x1000, "sp": 0x5D9C}),
        ("DS-SS-shared", nodes, 2, {"ss": 0x1000}),
        ("node-offset-wrap", [(0x4000, 0xFF00), (0x5000, 0xFF00)], 1, {}),
        ("A20-node", [(0xFFFF, 0x100), (0x5000, 0x100)], 1, {}),
        ("A20-stack", nodes, 2, {"ss": 0xFFFF, "sp": 0x11}),
        ("A20-DS", nodes, 0, {"ds": 0xFFFF}),
        ("preserved-DF", nodes, 2, {"flags": 0x0603}),
    ]
    visited, edges = set(), set()
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded
                   if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    for label, nodes, active, options in specifications:
        memory, cpu = fixture(body, target, nodes, active, **options)
        original = Uc(UC_ARCH_X86, UC_MODE_16)
        original.mem_map(0, 2 * RAM_SIZE)
        original.mem_write(0, bytes(memory)); original.mem_write(RAM_SIZE, bytes(memory))
        for (name, _), register in zip(Cpu._fields_, registers): original.reg_write(register, getattr(cpu, name))
        returned, previous = [False], [None]
        start = physical(cpu.cs, target["entry"])
        last = physical(cpu.cs, decoded[-1].address)
        def instruction(emulator, address, size, user):
            del size, user
            if returned[0]: emulator.emu_stop(); return
            check(start <= address < start + len(body), f"left body before return: {label}")
            here = address - start + target["entry"]
            visited.add(here)
            if previous[0] in conditional: edges.add((previous[0], here))
            previous[0] = here
            if address == last: returned[0] = True
        def mirror(emulator, access, address, size, value, user):
            del access, user
            for byte in range(size):
                raw = address + byte; at = raw & (RAM_SIZE - 1)
                emulator.mem_write(at + RAM_SIZE if raw < RAM_SIZE else at,
                                   bytes([(value >> (8 * byte)) & 0xFF]))
        original.hook_add(UC_HOOK_CODE, instruction)
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
        original.emu_start(start, 2 * RAM_SIZE, count=20000)
        check(returned[0], f"original fixture did not terminate within 20,000 instructions: {label}")
        expected = bytes(original.mem_read(0, RAM_SIZE))
        actual, cpu = run_c(function, memory, cpu)
        if actual != expected:
            differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
            raise ValueError(f"RAM mismatch {label}: {differences[:8]}")
        for (name, _), register in zip(Cpu._fields_, registers):
            check(getattr(cpu, name) == original.reg_read(register) & 0xFFFF,
                  f"{name} mismatch {label}: C={getattr(cpu,name):04x}, original={original.reg_read(register)&0xffff:04x}")
    check(len(visited) == len(decoded), f"instruction coverage {len(visited)}/{len(decoded)}")
    missing = {(site, outcome) for site, outcomes in conditional.items() for outcome in outcomes} - edges
    print(f"{len(specifications)} original-byte cases passed: 1 MiB RAM, 14 CPU words, "
          f"{len(visited)} instruction starts; missing conditional edges {sorted(missing)}")
    check(not missing, "conditional outcome coverage incomplete")
    return missing

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
    mov word ptr [0x5d92], ax
    mov ax, cs
    mov word ptr [0x5d94], ax
    lodsw
    mov word ptr [0x5d96], ax
    mov ax, cs
    mov word ptr [0x5d98], ax
'''
    for index in range(6):
        at = 0x4104 + index * 0x200
        text += (f"    lodsw\n    mov word ptr [{at}], ax\n    or ax, ax\n"
                 f"    jz zero_{index}\n    mov ax, cs\nzero_{index}:\n"
                 f"    mov word ptr [{at+2}], ax\n")
    text += r'''
    lodsw
    mov word ptr [case_meta], ax
    mov word ptr [case_ptr], si
    push 0x0202
    popf
    mov ax, 0x1234
    mov bx, 0x3456
    mov cx, 0x5678
    mov dx, 0x789a
    mov si, 0x9abc
    mov di, 0xbcde
    mov bp, 0x7777
    push cs
    call leaf
returned:
'''
    for index, (name, _) in enumerate(Cpu._fields_[:-1]):
        text += f"    mov word ptr [output+{2*index}], " + ("offset returned" if name == "ip" else name) + "\n"
    # Snapshot before PUSHF reuses return-frame RAM.
    for index in range(16):
        text += f"    mov ax, word ptr [{0xEFE0+2*index}]\n    mov word ptr [output+{36+2*index}], ax\n"
    text += "    pushf\n    pop ax\n    mov word ptr [output+26], ax\n"
    for index in range(4):
        text += f"    mov ax, word ptr [{0x5D92+2*index}]\n    mov word ptr [output+{28+2*index}], ax\n"
    for node in range(6):
        for part in range(2):
            text += (f"    mov ax, word ptr [{0x4104+node*0x200+2*part}]\n"
                     f"    mov word ptr [output+{68+node*4+part*2}], ax\n")
    text += r'''
    mov ax, word ptr [case_meta]
    mov word ptr [output+92], ax
    mov bx, word ptr [handle]
    mov dx, offset output
    mov cx, 94
    mov ah, 0x40
    int 0x21
    jc failure
    cmp ax, 94
    jne failure
    inc word ptr [index]
    cmp word ptr [index], 21
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
case_meta: .word 0
output: .space 94
fixtures:
'''
    for n in range(1, 7):
        for active in range(n):
            values = [0x4000 + 0x200*active, 0x4000] + [0x4000+0x200*(i+1) if i+1<n else 0 for i in range(6)]
            text += "    .word " + ",".join(str(v) for v in values) + f"\n    .byte {n},{active}\n"
    return text + 'leaf:\n.incbin "' + str(source.resolve()) + '"\n'

def native_audit(root, body, function, target):
    directory = root / f"resources/oracle-runs/u6a-party-{target['entry']:04x}-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    source = directory / "leaf.bin"; source.write_bytes(body)
    asm, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm.write_text(native_assembly(source))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary",
                    str(obj), "-o", str(com)], check=True)
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
    check(len(result) == 6 + 21*94, "native output incomplete")
    segment, return_ip, leaf_ip = struct.unpack_from("<HHH", result)
    check(com.read_bytes()[leaf_ip-0x100:leaf_ip-0x100+len(body)] == body,
          "native copied leaf differs from original span")
    for index in range(21):
        record = result[6+index*94:6+(index+1)*94]
        n, active = record[92:94]
        check(1 <= n <= 6 and active < n, "native metadata invalid")
        memory = bytearray([0xA5]) * RAM_SIZE
        for node in range(6):
            offset = 0x4000 + 0x200*node
            next_offset = offset+0x200 if node+1 < n else 0
            put16(memory, segment, offset+0x104, next_offset)
            put16(memory, segment, offset+0x106, segment if next_offset else 0)
        put16(memory, segment, 0x5D92, 0x4000+0x200*active)
        put16(memory, segment, 0x5D94, segment)
        put16(memory, segment, 0x5D96, 0x4000)
        put16(memory, segment, 0x5D98, segment)
        put16(memory, segment, 0xEFFC, return_ip); put16(memory, segment, 0xEFFE, segment)
        cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFFC, segment, leaf_ip, segment, segment, segment, 0x0202)
        actual, cpu = run_c(function, memory, cpu)
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == expected,
                  f"native {name} mismatch case {index}: C={getattr(cpu,name):04x}, native={expected:04x}")
        at = physical(segment, 0x5D92)
        check(actual[at:at+8] == record[28:36], f"native globals mismatch {index}")
        at = physical(segment, 0xEFE0)
        check(actual[at:at+32] == record[36:68], f"native frame mismatch {index}")
        for node in range(6):
            at = physical(segment, 0x4104+node*0x200)
            check(actual[at:at+4] == record[68+node*4:72+node*4], f"native link mismatch {index}/{node}")
    print("21 DOSBox-X synthetic cases passed: all 14 CPU words, 32 frame bytes, "
          "8 global bytes, all six node links; no flag mask")
    print("native result SHA-256 " + sha256(result).hexdigest())

def main(**target):
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    args = parser.parse_args()
    body, decoded = source_audit(root, target)
    with tempfile.TemporaryDirectory(prefix="poolrad-party-reorder-") as temp:
        function = build_c(root, Path(temp), target)
        if args.unicorn: differential(body, decoded, function, target)
        if args.native: native_audit(root, body, function, target)
