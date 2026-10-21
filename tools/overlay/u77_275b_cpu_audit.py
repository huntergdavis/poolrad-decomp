#!/usr/bin/env python3
"""U77 [275B,27DF) original-byte oracle: the caller's 132 bytes against our C.

Design: the original side executes the caller's real bytes under Unicorn. At the
two far calls (275B+0x0C -> 0x2767 into 00BA:0BB8, and 0x27A8 into 00BA:13AB) it
substitutes the same credited raw resident models the C side composes, so any
difference is attributable to the caller alone - the residents are accepted
contracts, not the subject of this differential.

Compared at the stop: all 14 CPU words, status, and every byte of 1 MiB RAM.
No flag masking, no instruction substitution inside the body, no host callback
standing in for original code.

Exit status: 0 when every case matches; 1 on the first mismatch.
"""
import argparse
import ctypes
import subprocess
import sys
import tempfile
from hashlib import sha256
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BP, UC_X86_REG_BX,
                               UC_X86_REG_CS, UC_X86_REG_CX, UC_X86_REG_DI,
                               UC_X86_REG_DS, UC_X86_REG_DX, UC_X86_REG_ES,
                               UC_X86_REG_FLAGS, UC_X86_REG_IP, UC_X86_REG_SI,
                               UC_X86_REG_SP, UC_X86_REG_SS)

from overlay_inventory import find_units, parse_mz

ROOT = Path(__file__).resolve().parents[2]
MEDIA = ROOT / "resources/DOS/POOLRAD"
CHECKER_ROWS = ("OVL_0077_275B",)

SIZE = 1 << 20
MASK = SIZE - 1
UNIT = 0x77
ENTRY = 0x275B
END = 0x27DF   # RETF 8 at 27DC occupies three bytes
CALLER_CS = 0x0814
RESIDENT_CS = 0x00BA
FIRST_CALL = 0x2767        # lcall 00BA:0BB8
FIRST_RETURN = 0x276C
SECOND_CALL = 0x27A8       # lcall 00BA:13AB
SECOND_RETURN = 0x27AD
REGISTERS = ((UC_X86_REG_AX, "ax"), (UC_X86_REG_BX, "bx"), (UC_X86_REG_CX, "cx"),
             (UC_X86_REG_DX, "dx"), (UC_X86_REG_SI, "si"), (UC_X86_REG_DI, "di"),
             (UC_X86_REG_BP, "bp"), (UC_X86_REG_SP, "sp"), (UC_X86_REG_CS, "cs"),
             (UC_X86_REG_IP, "ip"), (UC_X86_REG_DS, "ds"), (UC_X86_REG_ES, "es"),
             (UC_X86_REG_SS, "ss"), (UC_X86_REG_FLAGS, "flags"))


# Named exclusions, with their evidence. The bar is "every start and every
# feasible outcome, or say which one and why" - never a widened mask.
#
# 0x277B needs no exclusion. 00BA:0BB8 zeroes target +0xC7 and then walks the
# item chain at +0xC8/+0xCA, executing `inc byte es:[di+0xC7]` at 0C2C once per
# node (each node's next pointer sits at +0x2A/+0x2C). Fixtures with a NULL chain
# leave the byte at 0 and never take the branch; the 16- and 20-node chain
# fixtures push it past 15 and DO take it, so the start is covered and this set
# is deliberately empty. The checker's EXCLUSION NO LONGER TRUE guard fails if a
# reader re-adds an exclusion for a start that a fixture reaches.
EXCLUDED_STARTS = {}

# Excluded conditional SIDES, with the argument that makes them unreachable.
#   At 27C2 `cmp dx,bx` with dx = 0 and bx = sign-extension of (AX + 1500), where
#   AX is the 00BA:13AB result. That body selects its return from AL and every
#   value lies in [-350, 15000] (0xFEA2/0xFF06/0xFF6A, 100, 200, 350, then
#   500+250k, 2000+1000k, 7500, 9000+3000k, else 0), so AX + 1500 lies in
#   [1150, 16500]: bit 15 is never set, bx = 0, `cmp 0,0` is equal, and control
#   always reaches `cmp ax,cx`. Neither sign branch can be taken. They would
#   need a return outside [-1500, 31267], which the decode excludes for every AL.
EXCLUDED_SIDES = {
    (0x27C4, 0x27CC): "by contract: 13AB returns in [-350,15000], so AX+1500 in [1150,16500] keeps bit 15 clear and bx=0",
    (0x27C6, 0x27D0): "same contract: bx=0 leaves cmp 0,0 equal, so no strict less",
}


class Cpu(ctypes.Structure):
    _fields_ = [("ax", ctypes.c_uint16), ("bx", ctypes.c_uint16),
                ("cx", ctypes.c_uint16), ("dx", ctypes.c_uint16),
                ("si", ctypes.c_uint16), ("di", ctypes.c_uint16),
                ("bp", ctypes.c_uint16), ("sp", ctypes.c_uint16),
                ("cs", ctypes.c_uint16), ("ip", ctypes.c_uint16),
                ("ds", ctypes.c_uint16), ("es", ctypes.c_uint16),
                ("ss", ctypes.c_uint16), ("flags", ctypes.c_uint16)]


class Diagnostic(ctypes.Structure):
    _fields_ = [("first_status", ctypes.c_int), ("second_status", ctypes.c_int),
                ("calls", ctypes.c_uint)]


def physical(segment, offset):
    return (((segment << 4) + offset) & MASK)


def word(image, at):
    return image[at] | (image[(at + 1) & MASK] << 8)


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def source():
    """The caller's bytes and pins, read from the owner media without writing it."""
    exe = (MEDIA / "START.EXE").read_bytes()
    ovr = (MEDIA / "GAME.OVR").read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == UNIT)
    body = ovr[unit["ovr_offset"] + ENTRY:unit["ovr_offset"] + END]
    starts = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(body) == 132, f"expected a 132-byte body, read {len(body)}")
    check(len(starts) == 45, f"expected 45 starts, decoded {len(starts)}")
    check(starts[-1].mnemonic == "retf", "body does not end in RETF")
    return body, starts


def build(body):
    """RAM image, CPU, and the object fixtures the body reads."""
    ram = bytearray(SIZE)
    ram[physical(CALLER_CS, ENTRY):physical(CALLER_CS, ENTRY) + len(body)] = body
    cpu = Cpu()
    cpu.cs = CALLER_CS
    cpu.ip = ENTRY
    cpu.ds = 0x1000
    cpu.es = 0x1000
    cpu.ss = 0x2000
    cpu.sp = 0x8000
    cpu.bp = 0x7F00
    # Declared entry register file, mirrored by the native harness so both sides
    # start from the same machine state.
    cpu.ax = 0x0000; cpu.bx = 0x0000; cpu.cx = 0x0000; cpu.dx = 0x0000
    cpu.si = 0x0000; cpu.di = 0x0000
    return ram, cpu


def case(name, *, type_byte=0x0C, quantity=4, count=1, base=0x0100,
         sp=0x7FF4, flags=0x0202, saturate=None, nodes=0, bp=0x7F00):   # post-CALL SP: frame at SS:SP
    """One fixture: object fields plus the frame words the body reads."""
    ram, cpu = build(CASE_BODY)
    cpu.sp = sp
    cpu.bp = bp   # the caller's BP is saved and restored; a low byte other than 0 exposes a word store over [bp+0]
    cpu.flags = flags
    target_segment, target_offset = 0x3000, 0x0100
    record_segment, record_offset = 0x4000, 0x0200
    # Stack frame as the caller builds it: return IP/CS at SS:SP, then the two
    # far pointers the body reads at [bp+6]/[bp+8] and [bp+0x0A]/[bp+0x0C],
    # where bp = SP-2 after the prologue. 8 argument bytes, hence RETF 8.
    stack = [(0x0000, 0xDEAD), (record_offset, record_segment),
             (target_offset, target_segment)]
    at = physical(cpu.ss, cpu.sp)
    for offset, segment in stack:
        ram[at] = offset & 0xFF
        ram[(at + 1) & MASK] = offset >> 8
        ram[(at + 2) & MASK] = segment & 0xFF
        ram[(at + 3) & MASK] = segment >> 8
        at = (at + 4) & MASK
    # object fields
    ram[physical(target_segment, (target_offset + 0xC7) & 0xFFFF)] = type_byte
    at = physical(target_segment, (target_offset + 0x102) & 0xFFFF)
    ram[at] = base & 0xFF
    ram[(at + 1) & MASK] = (base >> 8) & 0xFF
    at = physical(record_segment, (record_offset + 0x37) & 0xFFFF)
    ram[at] = quantity & 0xFF
    ram[(at + 1) & MASK] = (quantity >> 8) & 0xFF
    ram[physical(record_segment, (record_offset + 0x39) & 0xFFFF)] = count
    if nodes:
        # 00BA:0BB8 zeroes target +0xC7, then walks the chain at +0xC8/+0xCA and
        # increments +0xC7 once per node (inc at 0C2C); each node's next pointer
        # lives at +0x2A/+0x2C. Enough nodes push +0xC7 above 15 and reach 0x277B.
        def put(at, value):
            ram[at] = value & 0xFF
            ram[(at + 1) & MASK] = (value >> 8) & 0xFF
        node_segment, node_offset = 0x5000, 0x0000
        put(physical(target_segment, (target_offset + 0xC8) & 0xFFFF), node_offset)
        put(physical(target_segment, (target_offset + 0xCA) & 0xFFFF), node_segment)
        for index in range(nodes):
            here = node_offset + index * 0x40
            following = 0 if index == nodes - 1 else here + 0x40
            put(physical(node_segment, (here + 0x2A) & 0xFFFF), following)
            put(physical(node_segment, (here + 0x2C) & 0xFFFF), node_segment if following else 0)
            at = physical(node_segment, here)
            put((at + 0x37) & MASK, 3)
            ram[(at + 0x39) & MASK] = 0
    if saturate is not None:
        for segment in (target_segment, record_segment):
            base_at = physical(segment, 0)
            for offset in range(0x200):
                ram[(base_at + offset) & MASK] = saturate
    return name, ram, cpu


def cases():
    """Boundaries that decide the flag: type 15/16, count 0/1/255, sign edge."""
    specs = []
    for type_byte in (0x00, 0x0E, 0x0F, 0x10, 0xFF):
        specs.append(dict(type_byte=type_byte))
    for count in (0, 1, 2, 255):
        specs.append(dict(count=count, quantity=7, base=0x0100))
    for quantity in (0, 1, 0xFFFF):
        specs.append(dict(quantity=quantity, count=3, base=0x0200))
    # sign-extend boundary around the 1500 constant and the comparison
    for base in (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF):
        specs.append(dict(base=base, quantity=2, count=1))
    for sp in (0x0100, 0x7FFE, 0xFFF0):
        specs.append(dict(sp=sp))
    for flags in (0x0002, 0x00D7, 0x08D7, 0x0402):
        specs.append(dict(flags=flags))
    # the caller's BP is not always a multiple of 256: the three byte stores at [bp-2] and [bp-1] must not touch [bp+0]
    for bp in (0x7F01, 0x7FFE, 0x1234, 0xFFFF, 0x00FF):
        specs.append(dict(bp=bp))
        specs.append(dict(bp=bp, type_byte=0x10))
        specs.append(dict(bp=bp, quantity=0xFFFF, count=255, base=0xFFFF))
    # saturated objects drive the resident's result high enough that the
    # comparison can reach the `jg` side of the 32-bit test
    for fill in (0x00, 0x7F, 0x80, 0xFF):
        specs.append(dict(quantity=0xFFFF, count=255, base=0xFFFF, saturate=fill))
    # item chains: 16 and 20 nodes push target +0xC7 past 15, reaching 0x277B
    specs.append(dict(nodes=16))
    specs.append(dict(nodes=20, quantity=0, count=0))
    names = []
    for index, spec in enumerate(specs):
        names.append((f"case{index}", spec))
    return names


def library(directory):
    source_paths = [str(ROOT / "src/recovered" / name) for name in (
        "overlay_0077_275b_cpu.c", "resident_00ba_character_0bb8_cpu.c",
        "resident_00ba_character_helpers_cpu.c", "overlay_u9e_entry16_10e4.c",
        "overlay_u9e_entry17_110d.c")]
    target = Path(directory) / "u77_275b.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-O2", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    *source_paths, "-o", str(target)], check=True)
    lib = ctypes.CDLL(str(target))
    lib.pr_ovl0077_275b_cpu.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                        ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t),
                                        ctypes.POINTER(Diagnostic)]
    lib.pr_ovl0077_275b_cpu.restype = ctypes.c_int
    return lib


def original(ram_in, cpu_in, lib):
    """Execute the caller's original bytes, substituting the credited residents."""
    ram = bytearray(ram_in)
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer(ram)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    for register, _ in REGISTERS:
        if register not in (UC_X86_REG_CS, UC_X86_REG_IP):
            uc.reg_write(register, getattr(cpu_in, _))
    uc.reg_write(UC_X86_REG_CS, cpu_in.cs)
    uc.reg_write(UC_X86_REG_IP, cpu_in.ip)
    state = {"stop": None, "first": -1, "second": -1, "starts": set(), "edges": set()}

    def call_resident(entry, return_ip, which):
        """Push a far-call frame, run the credited resident model, resume."""
        state["first" if which == 1 else "second"] = 0
        for value in (cpu_in.cs if False else uc.reg_read(UC_X86_REG_CS),
                      return_ip):
            sp = (uc.reg_read(UC_X86_REG_SP) - 2) & 0xFFFF
            uc.reg_write(UC_X86_REG_SP, sp)
            at = physical(uc.reg_read(UC_X86_REG_SS), sp)
            ram[at] = value & 0xFF
            ram[(at + 1) & MASK] = value >> 8
        resident = Cpu()
        for register, field in REGISTERS:
            setattr(resident, field, uc.reg_read(register))
        resident.cs = RESIDENT_CS
        resident.ip = entry
        budget = ctypes.c_size_t(1 << 20)
        status = lib.pr_res00ba_0bb8_cpu(backing, SIZE, ctypes.byref(resident),
                                         ctypes.byref(budget)) if which == 1 else \
            lib.pr_res00ba_helpers_cpu(backing, SIZE, ctypes.byref(resident),
                                       ctypes.byref(budget))
        check(status == 0, f"resident {which} status {status}")
        for register, field in REGISTERS:
            uc.reg_write(register, getattr(resident, field))

    def hook(emulator, address, size, user):
        ip = emulator.reg_read(UC_X86_REG_IP)
        # Successor-based branch accounting: when the previous instruction was a
        # conditional jump, this IP IS the side it took. No flag arithmetic, so a
        # wrong CF/ZF guess cannot retire an untaken path (JBE is CF|ZF).
        previous = state.get("pending")
        if previous is not None:
            state["edges"].add((previous, ip))
            state["pending"] = None
        instruction = DECODED.get(ip)
        if instruction is not None and instruction.mnemonic.startswith("j") \
                and instruction.mnemonic != "jmp":
            state["pending"] = ip
        state["starts"].add(ip)
        if ip == FIRST_CALL:
            call_resident(0x0BB8, FIRST_RETURN, 1)
        elif ip == SECOND_CALL:
            call_resident(0x13AB, SECOND_RETURN, 2)
        elif ip == 0:
            state["stop"] = "return"
            emulator.emu_stop()


    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(cpu_in.cs, cpu_in.ip), 0, count=200000)
    check(state["stop"] == "return", "original did not reach the return frame")
    result = Cpu()
    for register, field in REGISTERS:
        setattr(result, field, uc.reg_read(register))
    return result, bytes(ram), state


def compare(name, ram, cpu, lib):
    expected, expected_ram, state = original(ram, cpu, lib)
    actual = Cpu.from_buffer_copy(bytes(cpu))
    actual.ip = cpu.ip
    budget = ctypes.c_size_t(0)
    diagnostic = Diagnostic(-1, -1, 0)
    working = (ctypes.c_uint8 * SIZE).from_buffer_copy(bytes(ram))
    status = lib.pr_ovl0077_275b_cpu(working, SIZE, ctypes.byref(actual),
                                     ctypes.byref(budget), ctypes.byref(diagnostic))
    check(status == 0, f"{name}: C status {status}")
    for register, field in REGISTERS:
        want = getattr(expected, field)
        got = getattr(actual, field)
        check(want == got, f"{name}: {field} {got:04X} != original {want:04X}")
    produced = bytes(working)
    if produced != expected_ram:
        differences = [(hex(i), a, b) for i, (a, b) in
                       enumerate(zip(produced, expected_ram)) if a != b]
        raise AssertionError(f"{name}: RAM differs at {differences[:8]} "
                            f"({len(differences)} bytes)")
    return diagnostic, state["starts"], state["edges"]


def differential(lib):
    reached, stops = set(), {}
    starts, edges = set(), set()
    for name, spec in cases():
        label, ram, cpu = case(name, **spec)
        diagnostic, seen, taken = compare(label, ram, cpu, lib)
        stops[name] = (diagnostic.first_status, diagnostic.second_status)
        reached.add(name)
        starts |= seen
        edges |= taken
        CASE_REACHED[name] = seen
    expected_starts = {i.address for i in DECODED.values()} - set(EXCLUDED_STARTS)
    conditional = {i.address: i for i in DECODED.values()
                   if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    reachable_sides = {(address, side)
                       for address, instruction in conditional.items()
                       if address not in EXCLUDED_STARTS
                       for side in (instruction.address + instruction.size,
                                    int(instruction.op_str, 16))}
    # Name which fixture reached each branch, so a missing side is actionable.
    branch_cases = {}
    for case_name, seen_by_case in CASE_REACHED.items():
        for address in conditional:
            if address in seen_by_case:
                branch_cases.setdefault(address, []).append(case_name)
    # The exclusion list is asserted, not assumed: if a fixture now reaches an
    # excluded start or side, the documentation claim is false and this fails.
    wrongly_reached = sorted(set(EXCLUDED_STARTS) & starts)
    if wrongly_reached:
        raise AssertionError(
            f"EXCLUSION NO LONGER TRUE: start(s) {[hex(a) for a in wrongly_reached]} "
            f"are now reached; update docs/re/U77_CAPACITY_CHECK_275B_CPU.md")
    wrongly_taken = sorted(set(EXCLUDED_SIDES) & edges)
    if wrongly_taken:
        raise AssertionError(
            f"EXCLUSION NO LONGER TRUE: side(s) "
            f"{[(hex(a), hex(b)) for a, b in wrongly_taken]} are now taken; "
            f"update docs/re/U77_CAPACITY_CHECK_275B_CPU.md")
    missing_starts = sorted(expected_starts - starts)
    missing_sides = sorted(reachable_sides - edges - set(EXCLUDED_SIDES))
    if missing_starts:
        detail = [f"{hex(a)} (no fixture reached it)" for a in missing_starts]
        raise AssertionError(f"uncovered starts: {detail}")
    if missing_sides:
        detail = []
        for address, side in missing_sides:
            instruction = conditional[address]
            detail.append(f"{hex(address)} {instruction.mnemonic} {instruction.op_str}"
                          f" -> {hex(side)} never taken; fixtures reaching the branch:"
                          f" {branch_cases.get(address, []) or 'NONE'}")
        raise AssertionError("uncovered outcomes:\n  " + "\n  ".join(detail))
    return dict(cases=len(reached), cpu_words=14, ram_bytes=SIZE,
                flag_masks=0, substitutions=0,
                starts=f"{len(starts & expected_starts)}/{len(expected_starts)} "
                       f"(+{len(EXCLUDED_STARTS)} excluded: "
                       f"{', '.join(hex(a) for a in sorted(EXCLUDED_STARTS))})",
                outcomes=f"{len(edges & reachable_sides)}/{len(reachable_sides)} "
                         f"(+{len(EXCLUDED_SIDES)} excluded sides)",
                stops={"return": sum(1 for _ in reached)},
                resident_calls=(sum(1 for s in stops.values() if s[0] == 0),
                                sum(1 for s in stops.values() if s[1] == 0)))


CASE_BODY = b""
DECODED = {}
CASE_REACHED = {}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    global CASE_BODY, DECODED, CASE_REACHED
    if args.compile_only:
        # Media-free handoff check: compile, load and bind only. Deliberately
        # before source(), which reads the owner media.
        with tempfile.TemporaryDirectory(prefix="u77-275b-") as temp:
            library(temp)
        print("CHECKER_BUILD_OK")
        return 0
    body, starts = source()
    CASE_BODY = body
    CASE_REACHED = {}
    DECODED = {instruction.address: instruction for instruction in starts}
    with tempfile.TemporaryDirectory(prefix="u77-275b-") as temp:
        lib = library(temp)
        report = differential(lib)
    print(str(report))
    return 0


if __name__ == "__main__":
    sys.exit(main())
