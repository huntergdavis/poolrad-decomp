#!/usr/bin/env python3
"""Pinned original/C full-state audit for unit 6F:+032F. Media stays private."""
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile
import gc
import argparse
import json

from capstone import (Cs, CS_ARCH_X86, CS_MODE_16, CS_GRP_RET,
                      CS_GRP_INT, CS_GRP_JUMP)
from capstone.x86 import X86_OP_IMM
from overlay_inventory import find_units, parse_mz
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
RAM_SIZE = 1 << 20
ENTRY, END = 0x032F, 0x0476
BODY_SHA = "2df5a5980843eb72e7ed5382d4d4d452d541c22ebfe0d43ee065947b016c2dde"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
FIELDS = "ax bx cx dx si di bp sp cs ip ds es ss flags".split()
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
             UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
             UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in FIELDS]


def check(ok, message):
    if not ok:
        raise ValueError(message)


def physical(segment, offset):
    return ((segment << 4) + (offset & 0xffff)) & (RAM_SIZE - 1)


def put_word(memory, segment, offset, value):
    at = physical(segment, offset)
    memory[at] = value & 255
    memory[(at + 1) & (RAM_SIZE - 1)] = value >> 8


def put_far(memory, segment, offset, target):
    at = physical(segment, offset)
    for i, value in enumerate((target[1] & 255, target[1] >> 8,
                               target[0] & 255, target[0] >> 8)):
        memory[(at + i) & (RAM_SIZE - 1)] = value


def add_media_arguments(parser):
    parser.add_argument("--exe", type=Path, default=ROOT / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path, default=ROOT / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--report", type=Path)


def source_audit(exe_path=None, ovr_path=None):
    ovr = (ovr_path or ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA,
          "original GAME.OVR identity mismatch")
    body = ovr[0x1e315:0x1e45c]
    check(len(body) == 327 and sha256(body).hexdigest() == BODY_SHA,
          "original leaf extent/hash mismatch")
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(instructions) == 121 and instructions[-1].address == 0x0473 and
          instructions[-1].mnemonic == "retf" and instructions[-1].op_str == "8" and
          instructions[-1].address + instructions[-1].size == END,
          "complete instruction extent mismatch")
    starts = {i.address for i in instructions}
    for ins in instructions:
        check("call" not in ins.mnemonic and ins.mnemonic not in ("int", "iret"),
              "leaf gained a call/interrupt")
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str, 16) in starts, "branch leaves original body")
    exe = (exe_path or ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(sha256(exe).hexdigest() ==
          "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
          "original START.EXE identity mismatch")
    units = {u["stub_para"]: u for u in
             find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit = units[0x6f]
    check(unit["ovr_offset"] == 0x1dfe6 and unit["memory_segment"] == 0x7c and
          unit["entries"][3] == 0x0482, "U6F source/stub identity mismatch")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    decoded = reachable_caller(code, 0x0482, 0x069e)
    for site in (0x0546, 0x056A):
        check(decoded[site - 1].mnemonic == "push" and
              decoded[site - 1].op_str == "cs" and
              decoded[site].mnemonic == "call" and decoded[site].op_str == "0x32f",
              "aligned caller changed")
    # Nine upstream calls to entry 3, distinct from the two direct leaf calls.
    parent = units[0x6c]
    parent_code = ovr[parent["ovr_offset"]:parent["ovr_offset"] + parent["code_size"]]
    upstream = []
    for start in (0x306d, 0x382d):
        function = next(f for f in parent["functions"] if f["rel"] == start)
        for site, ins in reachable_caller(parent_code, start,
                                          start + function["size_upper_bound"]).items():
            if (ins.mnemonic == "lcall" and len(ins.operands) == 2 and
                    all(op.type == X86_OP_IMM for op in ins.operands) and
                    tuple(op.imm for op in ins.operands) == (0x7c, 0x2f)):
                check(site + 3 in parent["fixups"], "upstream segment fixup missing")
                upstream.append(site)
    check(set(upstream) == {0x3192, 0x3233, 0x3862, 0x38a3, 0x3b52,
                            0x3b69, 0x3ba7, 0x3c5a, 0x3cf9}, "upstream caller set changed")
    print("source: 327 bytes/121 starts, no calls, RETF 8; "
          "2 aligned direct calls and 9 fixup-backed upstream calls")
    return body, instructions


def reachable_caller(code, start, end):
    """Bounded local CFG, with calls assumed returning; never decode data gaps."""
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    pending, decoded, occupied = [start], {}, {}
    while pending:
        at = pending.pop()
        if at in decoded:
            continue
        check(start <= at < end and at not in occupied, "caller branch outside bound/overlap")
        ins = next(decoder.disasm(code[at:min(end, at+15)], at, count=1), None)
        check(ins is not None and at + ins.size <= end, "caller invalid instruction")
        check(not any(i in occupied for i in range(at, at+ins.size)), "caller instruction overlap")
        decoded[at] = ins
        occupied.update((i, at) for i in range(at, at+ins.size))
        if ins.group(CS_GRP_RET) or ins.group(CS_GRP_INT):
            continue
        if ins.group(CS_GRP_JUMP):
            check(len(ins.operands) == 1 and ins.operands[0].type == X86_OP_IMM,
                  "caller indirect branch")
            pending.append(ins.operands[0].imm)
            if ins.mnemonic == "jmp":
                continue
        pending.append(at + ins.size)
    return decoded


def build_c(directory):
    library = directory / "u6f-032f.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u6f_vector_merge_032f.c"),
                    "-o", str(library)], check=True, capture_output=True, text=True)
    result = ctypes.CDLL(str(library))
    result.pr_u6f_vector_merge_032f.argtypes = [ctypes.POINTER(ctypes.c_uint8),
                                              ctypes.c_size_t, ctypes.POINTER(Cpu)]
    result.pr_u6f_vector_merge_032f.restype = ctypes.c_int
    return result


def fixture(body, mode, count=5, global_count=20, *, sp=0x8000,
            ds=0x1000, ss=0xe000, cs=0xf000, dest=(0x4000, 0x0100),
            source=(0x5000, 0x0200), fill=0xa5):
    memory = bytearray([fill]) * RAM_SIZE
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789a, 0x9abc, 0xbcde,
              0x7777, sp, cs, ENTRY, ds, 0x1111, ss, 0x7202)
    at = physical(cpu.cs, ENTRY)
    memory[at:at + len(body)] = body
    put_word(memory, ss, sp, 0x1234)
    put_word(memory, ss, (sp + 2) & 0xffff, 0xf000)
    put_far(memory, ss, (sp + 4) & 0xffff, dest)
    put_far(memory, ss, (sp + 8) & 0xffff, source)
    memory[physical(ds, 0x4a1e)] = mode
    put_word(memory, source[0], source[1] + 0x11, count)
    put_far(memory, dest[0], dest[1] + 0x13, (0x6000, 0x0300))
    put_far(memory, source[0], source[1] + 0x13, (0x7000, 0x0400))
    put_far(memory, ds, 0x5eb2, (0x3000, 0x0500))
    put_word(memory, 0x3000, 0x0511, global_count)
    for i in range(32):
        memory[physical(dest[0], dest[1] + 0x17 + i)] = (i*17 + 0x5a) & 255
        memory[physical(source[0], source[1] + 0x17 + i)] = (i*13 + 0xa5) & 255
        memory[physical(0x6000, 0x0300 + i)] = (i*7 + 0xaa) & 255
        memory[physical(0x7000, 0x0400 + i)] = (i*11 + 0xf0) & 255
    return memory, cpu


def compare(label, memory, cpu, body, library, coverage=False):
    original_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    pointer = ctypes.addressof(original_ram)
    original.mem_map_ptr(0, RAM_SIZE, 7, pointer)
    original.mem_map_ptr(RAM_SIZE, 0x10000, 7, pointer)
    for name, register in zip(FIELDS, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, ENTRY)
    ret = start + 0x0473 - ENTRY
    reached, visited, edges, previous = [False], set(), set(), [None]
    def on_instruction(emulator, address, size, user):
        if coverage:
            here = address - start + ENTRY
            visited.add(here)
            if previous[0] is not None:
                edges.add((previous[0], here))
            previous[0] = here
        if address == ret:
            reached[0] = True
            emulator.emu_stop()
    hook = original.hook_add(UC_HOOK_CODE, on_instruction,
                             begin=start if coverage else ret,
                             end=ret)
    original.emu_start(start, 0x110000, count=15000000)
    check(reached[0], f"original return budget exhausted: {label}")
    check(bytes(original_ram[start:start+len(body)]) == body,
          f"immutable code contract violated: {label}")
    original.hook_del(hook)
    ss, sp = original.reg_read(UC_X86_REG_SS), original.reg_read(UC_X86_REG_SP)
    if sp >= 0xfffe:
        # Fifteen directed native frames establish wrapped stack word offsets.
        # Unicorn wrongly reads the CS word linearly at these two offsets.
        def stack_word(offset):
            at = physical(ss, offset)
            return original_ram[at] | (original_ram[(at + 1) & (RAM_SIZE - 1)] << 8)
        ip, cs = stack_word(sp), stack_word((sp + 2) & 0xffff)
        original.reg_write(UC_X86_REG_CS, cs)
        original.reg_write(UC_X86_REG_IP, ip)
        original.reg_write(UC_X86_REG_SP, (sp + 12) & 0xffff)
    else:
        original.emu_start(ret, 0x110000, count=1)  # Stop after the original RETF.
    actual_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_cpu = Cpu.from_buffer_copy(cpu)
    check(library.pr_u6f_vector_merge_032f(actual_ram, RAM_SIZE,
                                          ctypes.byref(actual_cpu)) == 0,
          f"C rejected fixture: {label}")
    expected = bytes(original_ram)
    check(bytes(actual_ram) == expected, f"full 1 MiB RAM mismatch: {label}")
    for name, register in zip(FIELDS, REGISTERS):
        value = original.reg_read(register) & 0xffff
        check(getattr(actual_cpu, name) == value,
              f"{name} mismatch {label}: C={getattr(actual_cpu,name):04x} original={value:04x}")
    return visited, edges, sp >= 0xfffe


def differential(body, instructions, library):
    visited, edges = set(), set()
    hybrid_cases = []
    cases = []
    for mode in (0, 1, 2, 3, 0xff):
        for count in (1, 2, 7, 21):
            cases.append((f"mode-{mode}-count-{count}", fixture(body, mode, count), True))
    for mode in range(3, 256):
        cases.append((f"invalid-mode-{mode}", fixture(body, mode, count=1), False))
    rng = random.Random(0x6f032f)
    for i in range(128):
        mode, count, total = rng.choice((0, 1, 2, 3, 0xff)), rng.randrange(1, 32), rng.randrange(4, 128)
        memory, cpu = fixture(body, mode, count, total)
        for name in ("ax", "bx", "cx", "dx", "si", "di", "bp", "es"):
            setattr(cpu, name, rng.randrange(65536))
        cpu.flags = 2 | (rng.randrange(65536) & 0x0cd5)
        for segment, offset in ((0x4000, 0x0117), (0x5000, 0x0217),
                                (0x6000, 0x0300), (0x7000, 0x0400)):
            for j in range(32):
                memory[physical(segment, offset+j)] = rng.randrange(256)
        cases.append((f"seeded-{i}", (memory, cpu), False))
    for sp in (0, 1, 2, 3, 4, 5, 0xfff9, 0xfffc, 0xfffd, 0xfffe, 0xffff):
        cases.append((f"stack-{sp:04x}", fixture(body, 1, sp=sp), False))
    for label, options in (
        ("DS-SS-shared", dict(ds=0xe000)),
        ("dest-offset-wrap", dict(dest=(0x4000, 0xfff0))),
        ("source-offset-wrap", dict(source=(0x5000, 0xfff0))),
        ("dest-A20-wrap", dict(dest=(0xffff, 0x0000))),
        ("source-A20-wrap", dict(source=(0xffff, 0x0000))),
        ("DS-A20-wrap", dict(ds=0xffff)),
    ):
        cases.append((label, fixture(body, 0, **options), False))
    # LES operands use contiguous physical bytes after effective-address
    # formation. These seams are Unicorn evidence, not a native CPU claim.
    for mode in (0, 1, 2):
        for offset in (0xffea, 0xffeb, 0xffec):
            for field, segment in (("source", 0x5000), ("dest", 0x4000)):
                cases.append((f"{field}-LES-seam-{mode}-{offset:04x}",
                    fixture(body, mode, count=2, global_count=8,
                            **{field: (segment, offset)}), False))
        for sp in (0xfff5, 0xfff6, 0xfff7, 0xfff9, 0xfffa, 0xfffb):
            cases.append((f"argument-LES-seam-{mode}-{sp:04x}",
                fixture(body, mode, count=2, global_count=8, sp=sp), False))
    for cs in (0x9000, 0xa000, 0xb000):
        for mode in (0, 1, 2, 3):
            cases.append((f"relocated-CS-{cs:04x}-mode-{mode}",
                fixture(body, mode, cs=cs), False))
    # AND writes the live source nested-pointer word, so later LES must reload.
    memory, cpu = fixture(body, 0, count=5)
    put_far(memory, 0x4000, 0x0113, (0x5000, 0x0213))
    put_far(memory, 0x5000, 0x0213, (0x7000, 0x0401))
    memory[physical(0x7000, 0x0401)] = 0xfe
    for j in range(1, 5):
        memory[physical(0x7000, 0x0400+j)] = 0xff if j > 1 else 0xfe
    cases.append(("live-source-nested-pointer-alias", (memory, cpu), False))
    # OR modifies the global far pointer before the mode-1 LES reads it.
    memory, cpu = fixture(body, 1, count=4, dest=(0x1000, 0x5e9b))
    put_far(memory, cpu.ds, 0x5eb2, (0x3000, 0x0500))
    put_word(memory, 0x3000, 0x0511, 0x0400)
    put_word(memory, 0x3000, 0x0512, 4)
    for i, value in enumerate((1, 0, 0, 0)):
        memory[physical(0x5000, 0x0217+i)] = value
    cases.append(("live-global-pointer-after-inline-OR", (memory, cpu), False))
    # The mode is read once. OR changes its RAM byte from 1 to 3, while
    # execution must still finish the already selected mode-1 path.
    memory, cpu = fixture(body, 1, dest=(0x1000, 0x4a07))
    memory[physical(cpu.ds, 0x4a1e)] = 1
    memory[physical(0x5000, 0x0217)] = 2
    cases.append(("mode-read-once-before-inline-write", (memory, cpu), False))
    memory, cpu = fixture(body, 0, count=4)
    put_far(memory, 0x4000, 0x0113, (cpu.ss, cpu.sp))
    cases.append(("pointed-write-aliases-far-return", (memory, cpu), False))
    memory, cpu = fixture(body, 1)
    put_far(memory, cpu.ss, cpu.sp, (cpu.cs, ENTRY))
    cases.append(("far-return-to-same-body-stops-after-RETF", (memory, cpu), False))
    # Mode-1 quarter-count underflow performs 65536 AND iterations.
    cases.append(("global-quarter-zero-underflow", fixture(body, 1, global_count=0, fill=0), False))
    # Zero source length also performs 65536 iterations. Keep full records
    # zero outside initialized fields to retain the immutable-code contract.
    memory, cpu = fixture(body, 0, count=0, fill=0)
    for segment in (0x4000, 0x5000):
        for j in range(32):
            memory[physical(segment, (0x0117 if segment == 0x4000 else 0x0217)+j)] = 0
    cases.append(("source-zero-underflow", (memory, cpu), False))
    for label, (memory, cpu), coverage in cases:
        sites, transitions, hybrid = compare(label, memory, cpu, body, library, coverage)
        if hybrid:
            hybrid_cases.append(label)
        visited.update(sites)
        edges.update(transitions)
        gc.collect()
    check(visited == {i.address for i in instructions}, "incomplete instruction coverage")
    conditional = set()
    for ins in instructions:
        if ins.mnemonic.startswith("j") and ins.mnemonic != "jmp":
            conditional.update(((ins.address, int(ins.op_str, 16)),
                                (ins.address, ins.address + ins.size)))
    impossible = {(0x0353, 0x03be), (0x03d8, 0x0412), (0x042d, 0x0470)}
    check(conditional - impossible <= edges, "feasible branch coverage incomplete")
    print(f"{len(hybrid_cases)} cases use the native-established RETF rule; others execute original RETF")
    print(f"{len(cases)} original/C cases: full 1 MiB RAM and all 14 CPU words equal; no flag mask")
    print(f"coverage {len(visited)}/121 starts; {len(conditional & edges)}/{len(conditional)} "
          "conditional outcomes; three unsigned-zero JA outcomes impossible")
    return dict(target="OVL_006F_032F", body_sha256=BODY_SHA,
                cases=len(cases), exact_unicorn_cases=len(cases)-len(hybrid_cases),
                native_retf_rule_cases=hybrid_cases, ram_bytes_per_case=RAM_SIZE,
                cpu_words=14, flags_mask=0, instruction_starts=len(visited),
                conditional_outcomes=len(conditional & edges),
                impossible_outcomes=sorted(impossible), exclusions=[])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-only", action="store_true")
    add_media_arguments(parser)
    args = parser.parse_args()
    body, instructions = source_audit(args.exe, args.ovr)
    if args.source_only:
        return
    with tempfile.TemporaryDirectory(prefix="poolrad-u6f-032f-") as tmp:
        report = differential(body, instructions, build_c(Path(tmp)))
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
