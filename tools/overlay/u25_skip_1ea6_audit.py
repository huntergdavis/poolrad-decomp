#!/usr/bin/env python3
"""Hash-pin and compare the original U25 +1EA6 skip-statement composition."""
import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import x86_const as xr
from overlay_inventory import find_units, parse_mz
from u25_string_cores_audit import (Cpu, RAM_SIZE, NAMES, REGS, LOOPS, physical,
    far_pointer, check)

from u25_loader_0000_audit import source_audit as loader_sources, fixture as loader_fixture


def source_audit(exe_path, ovr_path):
    bodies, decoded = loader_sources(exe_path, ovr_path)
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x25)
    check(unit["entries"][22] == 0x1EA6, "entry 22 identity")
    data = ovr[unit["ovr_offset"]+0x1EA6:unit["ovr_offset"]+0x1FB1]
    digest = "a18580355e2e8766ca756e3e562e3fdde0ffe89dc65e9312dbe72f7074b0e2df"
    check(sha256(data).hexdigest() == digest, "skip-statement source hash")
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, 0x1EA6))
    check(len(ins) == 133 and ins[-1].address == 0x1FB0 and
          ins[-1].mnemonic == "retf", "skip-statement extent")
    sites = [0x1EFD, 0x1F27, 0x1F5D, 0x1F6E, 0x1F7B, 0x1F8C, 0x1F99, 0x1FA6]
    check([(i.address, i.op_str) for i in ins if "call" in i.mnemonic] ==
          [(site, "0") for site in sites], "eight loader calls")
    for site in sites:
        check(data[site - 0x1EA6 - 1] == 0x0E, "aligned PUSH CS")
    bodies["u25", 0x1EA6], decoded["u25", 0x1EA6] = data, ins
    print(f"u25:1EA6 bytes={len(data)} instructions={len(ins)} SHA256={digest}")
    return bodies, decoded

def build(root, directory):
    output = directory / "u25-skip.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-shared", "-fPIC", "-I", str(root / "include"),
        str(root / "src/recovered/overlay_u25_skip_1ea6.c"),
        str(root / "src/recovered/overlay_u25_loader_0000.c"),
        str(root / "src/recovered/overlay_u25_string_cores.c"), "-o", str(output)], check=True)
    lib = ctypes.CDLL(str(output))
    lib.pr_u25_skip_statement_1ea6.argtypes = [ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint16, ctypes.c_size_t, ctypes.c_size_t]
    lib.pr_u25_skip_statement_1ea6.restype = ctypes.c_int
    return lib



def fixture(opcode, operands, **kwargs):
    memory, cpu, system_cs = loader_fixture(operands, **kwargs)
    cpu.ip = 0x1EA6
    pc = kwargs.get("pc", 0x9900)
    memory[physical(0x6300, 0x1000 + pc + 0x6700)] = opcode
    return memory, cpu, system_cs

def compare(label, fixture, budgets, bodies, decoded, lib, emulator, coverage):
    memory, cpu, system_cs = fixture
    for (kind, start), data in bodies.items():
        segment = system_cs if kind == "sys" else cpu.cs
        copy = bytearray(data)
        for i in decoded[kind, start]:
            if i.mnemonic == "lcall":
                p = i.address - start + 3
                copy[p:p+2] = system_cs.to_bytes(2, "little")
        p = physical(segment, start)
        memory[p:p+len(copy)] = copy
    emulator.mem_write(0, bytes(memory))
    for n, r in zip(NAMES, REGS):
        emulator.reg_write(r, getattr(cpu, n))
    remaining, stop = list(budgets), [None]

    locations, instructions = {}, {}
    for (kind, _), ins in decoded.items():
        segment = system_cs if kind == "sys" else cpu.cs
        for item in ins:
            locations[physical(segment, item.address)] = (kind, item.address)
            instructions[physical(segment, item.address)] = item
    return_address = physical(cpu.cs, 0x7000)
    calls, returning = [], [None]

    def instruction(uc, address, size, user):
        del size, user
        if returning[0] is not None:
            expected_cs, expected_ip = returning[0]
            returning[0] = None
            actual = (uc.reg_read(xr.UC_X86_REG_CS), uc.reg_read(xr.UC_X86_REG_IP) & 65535)
            if actual != (expected_cs, expected_ip):
                stop[0] = 4
                uc.emu_stop()
                return
        if address == return_address:
            stop[0] = 0
            uc.emu_stop()
            return
        check(address in locations, f"{label}: unexpected original PC {address:05X}")
        kind, ip = locations[address]
        item = instructions[address]
        if item.mnemonic == "call" and item.op_str in ("0x1153", "0x12fb"):
            remaining[1] = budgets[1]
        if item.mnemonic in ("call", "lcall"):
            calls.append((cpu.cs if kind == "u25" else system_cs, ip + item.size))
        elif item.mnemonic == "retf" and calls:
            returning[0] = calls.pop()
        coverage.add((kind, ip))
        if kind == "u25" and (ip == 0x22 or ip in LOOPS):
            budget_index = 0 if ip == 0x22 else 1
            if remaining[budget_index] == 0:
                stop[0] = 2 if budget_index == 0 else 3
                uc.emu_stop()
            else:
                remaining[budget_index] -= 1

    hook = emulator.hook_add(UC_HOOK_CODE, instruction)
    emulator.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=5000000)
    emulator.hook_del(hook)
    check(stop[0] is not None, f"{label}: original exceeded instruction safety limit")
    c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    c_cpu = Cpu.from_buffer_copy(cpu)
    status = lib.pr_u25_skip_statement_1ea6(c_ram, RAM_SIZE, ctypes.byref(c_cpu), system_cs,
                                          budgets[0], budgets[1])
    check(status == stop[0], f"{label}: status {status} != {stop[0]}")
    expected, actual = bytes(emulator.mem_read(0, RAM_SIZE)), bytes(c_ram)
    if expected != actual:
        delta = [hex(i) for i, (x, y) in enumerate(zip(expected, actual)) if x != y]
        raise AssertionError(f"{label}: RAM differs at {delta[:16]}")
    for n, r in zip(NAMES, REGS):
        got, want = getattr(c_cpu, n), emulator.reg_read(r) & 65535
        check(got == want, f"{label}: {n} C={got:04X}, original={want:04X}")
    return status



def run(bodies, decoded, lib, random_cases):
    emulator = Uc(UC_ARCH_X86, UC_MODE_16)
    emulator.mem_map(0, RAM_SIZE)
    coverage, statuses, total = set(), {}, 0

    def case(label, opcode, operands, *, budgets=(1024, 512), mutate=None, **kwargs):
        nonlocal total
        state = fixture(opcode, operands, **kwargs)
        if mutate:
            mutate(*state)
        status = compare(label, state, budgets, bodies, decoded, lib, emulator, coverage)
        statuses[status] = statuses.get(status, 0) + 1
        total += 1
        if total % 50 == 0:
            print(f"Compared {total} skip cases; latest {label}")

    mixed = [bytes((1, 0x34, 0x12)), bytes((0x80, 3, 4, 32, 63)),
             bytes((0x81, 0, 0x49)), bytes((0x80, 0)), bytes((0xFF, 0xDA))]
    # Every possible latch value; all eight loader call sites and fallback.
    for opcode in range(256):
        case(f"opcode-{opcode:02X}", opcode, (mixed * 3)[:14], seed=opcode)
    for tag in range(256):
        if tag in (1, 2, 3):
            operand = bytes((tag, 0x34, 0x12))
        elif tag == 0x80:
            operand = bytes((tag, 3, 4, 32, 63))
        elif tag == 0x81:
            operand = bytes((tag, 0, 0x49))
        else:
            operand = bytes((tag, 0x47))
        case(f"tag-{tag:02X}", 1, [operand])
    for address in (0, 0x4900, 0x6B00, 0x6B01, 0x9700, 0xA000, 0xC000, 0xFFFF):
        case(f"address-{address:04X}", 1, [bytes((0x81, address & 255, address >> 8))])
    for pc in (0xFFFC, 0xFFFE, 0xFFFF):
        case(f"pc-wrap-{pc:04X}", 0x29, (mixed * 3)[:14], pc=pc)
        case(f"fallback-wrap-{pc:04X}", 0xFF, [], pc=pc)
    case("operand-limit", 4, [bytes((0, 1))]*3, budgets=(1, 512))
    case("packed-limit", 1, [bytes((0x80, 12)) + bytes([0xFF])*12], budgets=(4, 2))
    case("address-limit", 1, [bytes((0x81, 0, 0x49))], unterminated=True, budgets=(4, 270))
    case("relocated", 4, mixed[:3], base=0x1234)
    for sp in (0x232, 0xFFFC):
        case(f"stack-{sp:04X}", 4, mixed[:3], sp=sp)
    for sp in (0x72EA, 0x72EC, 0x72FA, 0x7018, 0x7098, 0x6F81, 0x49EF):
        case(f"stack-table-alias-{sp:04X}", 4, mixed[:3], ss=0x1000, sp=sp,
             budgets=(16, 64))
    # Script source aliases the opcode latch; fetch must precede the write.
    def latch_alias(memory, cpu, system_cs):
        del system_cs
        pc = 0x9900
        off = (0x6F7F - pc - 0x6700) & 0xFFFF
        far_pointer(memory, cpu.ds, 0x49DE, cpu.ds, off)
        memory[physical(cpu.ds, 0x6F7F)] = 0xFF
    case("source-latch-alias", 0, [], mutate=latch_alias)
    rng = random.Random(0x251EA6)
    for i in range(random_cases):
        operands = []
        for _ in range(14):
            tag = rng.choice((0, 1, 2, 3, 0xFF, 0x80, 0x81))
            if tag in (1, 2, 3):
                operand = bytes((tag, rng.randrange(256), rng.randrange(256)))
            elif tag == 0x80:
                length = rng.randrange(13)
                operand = bytes((tag, length)) + rng.randbytes(length)
            elif tag == 0x81:
                address = rng.choice((0x4900, 0x6B00, 0x6B01, 0x9700, 0xA000, 0xC000))
                operand = bytes((tag, address & 255, address >> 8))
            else:
                operand = bytes((tag, rng.randrange(256)))
            operands.append(operand)
        case(f"random-{i}", rng.randrange(62), operands, seed=i,
             base=rng.choice((0, 0x1234)))
    print(f"Compared: {total} skip cases; all 14 CPU words and {RAM_SIZE} RAM bytes equal; "
          f"statuses {statuses}")
    for start in (0x1EA6, 0, 0x6AF, 0x1153, 0x12FB):
        starts = {("u25", i.address) for i in decoded["u25", start]}
        print(f"Coverage u25:{start:04X}: {len(starts & coverage)}/{len(starts)}")
        # +1EA6 supplies only positive counts; loader +0017 is the
        # zero-count jump, covered by the independent +0000 audit.
        excluded = {("u25", 0x17)} if start == 0 else set()
        check(starts - coverage == excluded,
              f"unexpected coverage gaps: {sorted(starts - coverage)}")
    print("PASS: full-state comparison and reachable-instruction coverage")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--random-cases", type=int, default=100)
    args = parser.parse_args()
    bodies, decoded = source_audit(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-skip-") as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, decoded, library, args.random_cases)


if __name__ == "__main__":
    main()
