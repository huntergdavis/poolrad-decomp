#!/usr/bin/env python3
"""Hash-pin and compare the original U25 +0000 operand-loader composition."""
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
    put16, far_pointer, check, fixture as cpu_fixture, source_audit as string_sources)


def source_audit(exe_path, ovr_path):
    bodies, decoded = string_sources(exe_path, ovr_path)
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x25)
    check(unit["entries"][2] == 0 and unit["entries"][9] == 0x6AF, "loader/word entries")
    for start, end, digest in [
        (0, 0x136, "750610f1ea8c924651ee615595556377352cf441f3fd7e27a01c79ed3f6259cd"),
        (0x6AF, 0x6D3, "53565df24104754f01dff68b10cf0dda3fe72ad394a5253c611ceb594d93e862"),
    ]:
        data = ovr[unit["ovr_offset"]+start:unit["ovr_offset"]+end]
        check(sha256(data).hexdigest() == digest, "loader source hash")
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, start))
        check(ins[-1].address + ins[-1].size == end, "loader decode extent")
        calls = [(i.address, i.op_str) for i in ins if "call" in i.mnemonic]
        check(calls == ([(0xBC, "0x1153"), (0x110, "0x6af"), (0x11E, "0x12fb")]
                        if start == 0 else []), "loader call graph")
        bodies["u25", start], decoded["u25", start] = data, ins
        print(f"u25:{start:04X} file={unit['ovr_offset']+start:#x} bytes={len(data)} "
              f"instructions={len(ins)} SHA256={digest}")
    return bodies, decoded


def build(root, directory):
    output = directory / "u25-loader.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-shared", "-fPIC", "-I", str(root / "include"),
        str(root / "src/recovered/overlay_u25_loader_0000.c"),
        str(root / "src/recovered/overlay_u25_string_cores.c"), "-o", str(output)], check=True)
    lib = ctypes.CDLL(str(output))
    lib.pr_u25_load_operands_0000.argtypes = [ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint16, ctypes.c_size_t, ctypes.c_size_t]
    lib.pr_u25_load_operands_0000.restype = ctypes.c_int
    return lib


def fixture(operands, *, count=None, pc=0x9900, unterminated=False, **options):
    memory, cpu, system_cs = cpu_fixture(0, **options)
    for offset, segment in ((0x49D2, 0x6000), (0x49D6, 0x6100),
                            (0x49DA, 0x6200), (0x49DE, 0x6300), (0x5D92, 0x6400)):
        far_pointer(memory, cpu.ds, offset, segment, 0x1000)
    # Synthetic window strings: 12 word characters, or an unreachable terminator.
    for segment in (0x6000, 0x6100, 0x6200):
        for i in range(256 if unterminated else 12):
            put16(memory, segment, 0x1000 + 2*i, (i % 95) + 32)
        if not unterminated:
            put16(memory, segment, 0x1018, 0)
    # Script-window address A000 and an arbitrary synthetic active-name string.
    memory[physical(0x6300, 0x1700):physical(0x6300, 0x1700)+4] = b"abc" + bytes(1)
    memory[physical(0x6400, 0x1000):physical(0x6400, 0x1000)+5] = bytes([4]) + b"Test"
    put16(memory, cpu.ds, 0x49ED, pc)
    stream = b"".join(operands)
    for i, value in enumerate(stream):
        memory[physical(0x6300, 0x1000 + pc + 1 + i + 0x6700)] = value
    # Place a synthetic next opcode, not a string terminator in the operand stream.
    memory[physical(0x6300, 0x1000 + pc + len(stream) + 1 + 0x6700)] = 0x7E
    for offset, value in ((0, 0x7000), (2, cpu.cs),
                          (4, 0xAB00 | (len(operands) if count is None else count))):
        put16(memory, cpu.ss, cpu.sp + offset, value)
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
    status = lib.pr_u25_load_operands_0000(c_ram, RAM_SIZE, ctypes.byref(c_cpu), system_cs,
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

    def case(label, operands, *, budgets=(1024, 512), **kwargs):
        nonlocal total
        state = fixture(operands, **kwargs)
        status = compare(label, state, budgets, bodies, decoded, lib, emulator, coverage)
        statuses[status] = statuses.get(status, 0) + 1
        total += 1
        if total % 50 == 0:
            print(f"Compared {total} loader cases; latest {label}")

    for tag in range(256):
        if tag in (1, 2, 3):
            operand = bytes((tag, 0x34, 0x12))
        elif tag == 0x80:
            operand = bytes((tag, 3, 4, 32, 63))
        elif tag == 0x81:
            operand = bytes((tag, 0, 0x49))
        else:
            operand = bytes((tag, 0x47))
        case(f"tag-{tag:02X}", [operand])
    for count in (0, 2, 14, 63, 64, 65, 127, 128, 254, 255):
        case(f"count-{count}", [bytes((i % 4, i & 255, 0x12)) if i % 4 else bytes((0, i & 255))
                               for i in range(count)])
    case("empty-string", [bytes((0x80, 0))])
    case("mixed", [bytes((1, 0x34, 0x12)), bytes((0x80, 3, 4, 32, 63)),
                   bytes((0x81, 0, 0x49)), bytes((0x80, 0)), bytes((0xFF, 0xDA))])
    for address in (0, 0x4900, 0x6B00, 0x6B01, 0x9700, 0xA000, 0xC000, 0xFFFF):
        case(f"address-{address:04X}", [bytes((0x81, address & 255, address >> 8))])
    for pc in (0xFFFC, 0xFFFE, 0xFFFF):
        case(f"pc-wrap-{pc:04X}", [bytes((1, 0xAB, 0xCD)), bytes((0x80, 3, 4, 32, 63))], pc=pc)
    case("operand-limit", [bytes((0, 1))]*3, budgets=(1, 512))
    case("packed-limit", [bytes((0x80, 12)) + bytes([0xFF])*12], budgets=(4, 2))
    case("address-limit", [bytes((0x81, 0, 0x49))], unterminated=True, budgets=(4, 270))
    case("relocated", [bytes((0x81, 0, 0x49)), bytes((0x80, 3, 4, 32, 63))], base=0x1234)
    for sp in (0x22A, 0xFFFA):
        case(f"stack-{sp:04X}", [bytes((0x80, 3, 4, 32, 63))], sp=sp)
    for sp in (0x72E2, 0x72E4, 0x72F2, 0x7010, 0x7090):
        case(f"stack-table-alias-{sp:04X}", [bytes((0x80, 3, 4, 32, 63)), bytes((1, 2, 3))],
             ss=0x1000, sp=sp, budgets=(16, 64))
    rng = random.Random(0x250000)
    for i in range(random_cases):
        operands = []
        for _ in range(rng.randrange(1, 25)):
            tag = rng.choice((0, 1, 2, 3, 0xFF, 0x80, 0x81))
            if tag in (1, 2, 3):
                operands.append(bytes((tag, rng.randrange(256), rng.randrange(256))))
            elif tag == 0x80:
                length = rng.randrange(13)
                operands.append(bytes((tag, length)) + rng.randbytes(length))
            elif tag == 0x81:
                address = rng.choice((0x4900, 0x6B00, 0x6B01, 0x9700, 0xA000, 0xC000))
                operands.append(bytes((tag, address & 255, address >> 8)))
            else:
                operands.append(bytes((tag, rng.randrange(256))))
        case(f"random-{i}", operands, seed=i, base=rng.choice((0, 0x1234)))
    print(f"PASS: {total} loader cases; all 14 CPU words and {RAM_SIZE} RAM bytes equal; "
          f"statuses {statuses}")
    for start in (0, 0x6AF):
        starts = {("u25", i.address) for i in decoded["u25", start]}
        print(f"Coverage u25:{start:04X}: {len(starts & coverage)}/{len(starts)}")
        check(starts <= coverage, f"uncovered loader/helper: {sorted(starts - coverage)}")
    for start in (0x1153, 0x12FB):
        starts = {("u25", i.address) for i in decoded["u25", start]}
        print(f"Composed string coverage u25:{start:04X}: {len(starts & coverage)}/{len(starts)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--random-cases", type=int, default=100)
    args = parser.parse_args()
    bodies, decoded = source_audit(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-loader-") as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, decoded, library, args.random_cases)


if __name__ == "__main__":
    main()
