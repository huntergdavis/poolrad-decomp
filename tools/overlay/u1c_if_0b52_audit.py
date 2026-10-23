#!/usr/bin/env python3
"""Hash-pin and compare the original U1C +0B52 IF-family composition."""
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
    put16, far_pointer, check)

from u25_skip_1ea6_audit import source_audit as skip_sources, fixture as skip_fixture



def source_audit(exe_path, ovr_path):
    bodies, decoded = skip_sources(exe_path, ovr_path)
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit = units[0x1C]
    data = ovr[unit["ovr_offset"]+0xB52:unit["ovr_offset"]+0xBC8]
    digest = "11830c28e2ca43d40e80db72b6374a46e73b4e0cc0522ba9728e7026e3718061"
    check(sha256(data).hexdigest() == digest, "IF source hash")
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, 0xB52))
    check(len(ins) == 41 and ins[-1].address == 0xBC7 and
          ins[-1].mnemonic == "retf", "IF extent excludes trailing data")
    sites = [0xB67, 0xB79, 0xB8B, 0xB9D, 0xBAF, 0xBC1]
    check([(i.address, i.op_str) for i in ins if "call" in i.mnemonic] ==
          [(site, "0x2b, 0x8e") for site in sites], "six skip calls")
    check(all(site + 3 in unit["fixups"] for site in sites), "IF segment fixups")
    check(units[0x25]["entries"][22] == 0x1EA6, "skip-statement stub target")
    bodies["u1c", 0xB52], decoded["u1c", 0xB52] = data, ins
    print(f"u1c:0B52 bytes={len(data)} instructions={len(ins)} SHA256={digest}")
    return bodies, decoded

CHECKER_ROWS = ('OVL_001C_0B52',)

def build(root, directory):
    output = directory / "u1c-if.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-shared", "-fPIC", "-I", str(root / "include"),
        str(root / "src/recovered/overlay_u1c_if_0b52.c"),
        str(root / "src/recovered/overlay_u25_skip_1ea6.c"),
        str(root / "src/recovered/overlay_u25_loader_0000.c"),
        str(root / "src/recovered/overlay_u25_string_cores.c"), "-o", str(output)], check=True)
    lib = ctypes.CDLL(str(output))
    lib.pr_u1c_if_0b52.argtypes = [ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint16, ctypes.c_uint16, ctypes.c_size_t, ctypes.c_size_t]
    lib.pr_u1c_if_0b52.restype = ctypes.c_int
    return lib




def fixture(opcode, flag, next_opcode, operands, *, pc=0x9900, **kwargs):
    memory, cpu, system_cs = skip_fixture(next_opcode, operands, pc=(pc + 1) & 65535, **kwargs)
    base = kwargs.get("base", 0)
    u25_cs = cpu.cs
    cpu.cs, cpu.ip = base + 0x5000, 0xB52
    put16(memory, cpu.ss, cpu.sp + 2, cpu.cs)
    put16(memory, cpu.ds, 0x49ED, pc)
    for i in range(6):
        memory[physical(cpu.ds, 0x6F78 + i)] = flag
    memory[physical(cpu.ds, 0x6F7F)] = opcode
    return memory, cpu, (u25_cs, system_cs, base + 0x2B)

def install(memory, cpu, u25_cs, system_cs, stub_cs, bodies, decoded):
    segments = {"u1c": cpu.cs, "u25": u25_cs, "sys": system_cs}
    for (kind, start), data in bodies.items():
        segment = segments[kind]
        copy = bytearray(data)
        for i in decoded[kind, start]:
            if i.mnemonic == "lcall":
                p = i.address - start + 3
                copy[p:p+2] = (stub_cs if kind == "u1c" else system_cs).to_bytes(2, "little")
        p = physical(segment, start)
        memory[p:p+len(copy)] = copy
    # Loaded overlay stub: real far JMP, no synthetic callee return/results.
    stub_address = physical(stub_cs, 0x8E)
    memory[stub_address:stub_address+5] = bytes((0xEA, 0xA6, 0x1E,
                                                u25_cs & 255, u25_cs >> 8))
    return segments

def compare(label, fixture, budgets, bodies, decoded, lib, emulator, coverage):
    memory, cpu, (u25_cs, system_cs, stub_cs) = fixture
    segments = install(memory, cpu, u25_cs, system_cs, stub_cs, bodies, decoded)
    stub_address = physical(stub_cs, 0x8E)
    emulator.mem_write(0, bytes(memory))
    for n, r in zip(NAMES, REGS):
        emulator.reg_write(r, getattr(cpu, n))
    remaining, stop = list(budgets), [None]

    locations, instructions = {}, {}
    for (kind, _), ins in decoded.items():
        segment = segments[kind]
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
        if address == stub_address:
            return
        check(address in locations, f"{label}: unexpected original PC {address:05X}")
        kind, ip = locations[address]
        item = instructions[address]
        if item.mnemonic == "call" and item.op_str in ("0x1153", "0x12fb"):
            remaining[1] = budgets[1]
        if item.mnemonic in ("call", "lcall"):
            calls.append((segments[kind], ip + item.size))
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
    status = lib.pr_u1c_if_0b52(c_ram, RAM_SIZE, ctypes.byref(c_cpu), u25_cs, system_cs,
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

    def case(label, opcode, flag, next_opcode, operands, *, budgets=(1024, 512), **kwargs):
        nonlocal total
        state = fixture(opcode, flag, next_opcode, operands, **kwargs)
        status = compare(label, state, budgets, bodies, decoded, lib, emulator, coverage)
        statuses[status] = statuses.get(status, 0) + 1
        total += 1
        if total % 50 == 0:
            print(f"Compared {total} IF cases; latest {label}")

    mixed = [bytes((1, 0x34, 0x12)), bytes((0x80, 3, 4, 32, 63)),
             bytes((0x81, 0, 0x49)), bytes((0x80, 0)), bytes((0xFF, 0xDA))]
    for opcode in range(256):
        case(f"latch-{opcode:02X}", opcode, 0, 0x29, (mixed * 3)[:14], seed=opcode)
    for flag in range(256):
        case(f"flag-{flag:02X}", 0x16, flag, 4, mixed[:3])
    for opcode in range(0x16, 0x1C):
        for flag in (0, 1, 0x80, 0xFF):
            case(f"IF-{opcode:02X}-flag-{flag:02X}", opcode, flag, 4, mixed[:3])
    for next_opcode in range(256):
        case(f"next-opcode-{next_opcode:02X}", 0x17, 0, next_opcode, (mixed * 3)[:14])
    for address in (0, 0x4900, 0x6B00, 0x6B01, 0x9700, 0xA000, 0xC000, 0xFFFF):
        case(f"address-{address:04X}", 0x18, 0, 1,
             [bytes((0x81, address & 255, address >> 8))])
    for pc in (0xFFFC, 0xFFFE, 0xFFFF):
        case(f"pc-wrap-{pc:04X}", 0x19, 0, 0x29, (mixed * 3)[:14], pc=pc)
        case(f"true-wrap-{pc:04X}", 0x1A, 1, 0xFF, [], pc=pc)
    case("operand-limit", 0x1B, 0, 4, [bytes((0, 1))]*3, budgets=(1, 512))
    case("packed-limit", 0x16, 0, 1, [bytes((0x80, 12)) + bytes([0xFF])*12], budgets=(4, 2))
    case("address-limit", 0x17, 0, 1, [bytes((0x81, 0, 0x49))],
         unterminated=True, budgets=(4, 270))
    case("relocated", 0x18, 0, 4, mixed[:3], base=0x1234)
    for sp in (0x238, 0xFFFC):
        case(f"stack-{sp:04X}", 0x19, 0, 4, mixed[:3], sp=sp)
    for sp in (0x72F0, 0x72F2, 0x7300, 0x701E, 0x709E, 0x6F81, 0x49EF):
        case(f"stack-table-alias-{sp:04X}", 0x1A, 0, 4, mixed[:3], ss=0x1000, sp=sp,
             budgets=(16, 64))
    rng = random.Random(0x1C0B52)
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
        case(f"random-{i}", rng.randrange(0x16, 0x1C), rng.choice((0, 0, 0, 1, 255)),
             rng.randrange(62), operands, seed=i, base=rng.choice((0, 0x1234)))
    print(f"Compared: {total} IF cases; all 14 CPU words and {RAM_SIZE} RAM bytes equal; "
          f"statuses {statuses}")
    for kind, start in (("u1c", 0xB52), ("u25", 0x1EA6), ("u25", 0),
                         ("u25", 0x6AF), ("u25", 0x1153), ("u25", 0x12FB)):
        starts = {(kind, i.address) for i in decoded[kind, start]}
        print(f"Coverage {kind}:{start:04X}: {len(starts & coverage)}/{len(starts)}")
        # Skip supplies no zero count. Independent loader audit covers +0017.
        excluded = {("u25", 0x17)} if start == 0 else set()
        check(starts - coverage == excluded,
              f"unexpected coverage gaps: {sorted(starts - coverage)}")
    print("PASS: full-state comparison and reachable-instruction coverage")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path(__file__).resolve().parents[2] / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path, default=Path(__file__).resolve().parents[2] / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--random-cases", type=int, default=100)
    parser.add_argument('--compile-only', action='store_true')
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as directory:
            build(Path(__file__).resolve().parents[2], Path(directory))
        print('CHECKER_BUILD_OK')
        return
    bodies, decoded = source_audit(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-if-") as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        from loaded_stub_checks import verify_refusals
        memory,cpu,(u25_cs,system_cs,stub_cs)=fixture(0x16,0,4,[])
        install(memory,cpu,u25_cs,system_cs,stub_cs,bodies,decoded)
        stub_guards=verify_refusals(memory,cpu,lambda ram,state: library.pr_u1c_if_0b52(ram,RAM_SIZE,ctypes.byref(state),u25_cs,system_cs,1024,512),
                                    [(cpu.cs,p) for p in (0xB67,0xB79,0xB8B,0xB9D,0xBAF,0xBC1)])
        print("LOADED_STUB_GUARDS",stub_guards,flush=True)
        run(bodies, decoded, library, args.random_cases)


if __name__ == "__main__":
    main()
