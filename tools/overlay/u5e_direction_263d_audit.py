#!/usr/bin/env python3
"""Hash-pin and compare the original U5E +263D chain with C on synthetic RAM.

Original bytes are read only from owner-supplied media; output is metadata.
Requires Capstone and Unicorn. No callee is replaced by a result-only stub.
"""
import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import x86_const as uc_regs

from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ("OVL_005E_263D",)
RAM_SIZE = 1 << 20
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
CALLER_SHA = "7983c5f0344f78278aa7227b2a7ae964a5ab77cfe2be70a42a91bf179cd3c297"
PREDICATE_SHA = "59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae"
NAMES = "ax bx cx dx si di bp sp cs ip ds es ss flags".split()
REGISTERS = [getattr(uc_regs, "UC_X86_REG_" + ("EFLAGS" if n == "flags" else n.upper()))
             for n in NAMES]


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in NAMES]


class Layout(ctypes.Structure):
    _fields_ = [("slots_cs", ctypes.c_uint16), ("predicate_cs", ctypes.c_uint16)]


def check(value, message):
    if not value:
        raise AssertionError(message)


# Declared AF split (#1433 pinned rule; VP i21571). DOSBox-X 5a9c7c53 sets AF after SHL/SHR/SAR with a non-zero count
# (src/cpu/flags.cpp get_AF: `count & 1Fh`), and the model follows it (times_four). Unicorn leaves AF clear. So FLAGS may differ
# in AF (bit 4) ONLY when the last arithmetic-flag writer the original executed before the stop was such a shift, and then
# the C must have AF = 1; every other bit, and every other case, is compared exactly. The audit counts the cases that use it.
FLAG_WRITERS = {"add", "adc", "sub", "sbb", "cmp", "and", "or", "xor", "test", "inc", "dec", "neg",
                "shl", "sal", "shr", "sar", "rol", "ror", "rcl", "rcr", "mul", "imul", "div", "idiv", "cmpsb", "cmpsw",
                "scasb", "scasw", "popf", "sahf", "iret"}
DECODER = Cs(CS_ARCH_X86, CS_MODE_16)
AF_SPLIT_USED = [0]


def last_writer_is_nonzero_shift(uc, address, size, state):
    """Called before each original instruction: remember whether the last flag writer is a shift with a non-zero count."""
    ins = next(DECODER.disasm(bytes(uc.mem_read(address, size)), address), None)
    if ins is None or ins.mnemonic not in FLAG_WRITERS:
        return
    if ins.mnemonic in ("shl", "sal", "shr", "sar"):
        operand = ins.op_str.split(",")[-1].strip()
        count = (uc.reg_read(uc_regs.UC_X86_REG_CL) if operand == "cl" else int(operand, 0)) & 0x1F
        state[0] = count != 0
    else:
        state[0] = False


def physical(segment, offset):
    return ((segment << 4) + (offset & 0xFFFF)) & (RAM_SIZE - 1)


def put16(memory, segment, offset, value):
    p = physical(segment, offset)
    memory[p] = value & 255
    memory[(p + 1) & (RAM_SIZE - 1)] = (value >> 8) & 255


def source_audit(exe_path, ovr_path):
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "unsupported START.EXE")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "unsupported GAME.OVR")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u5e, slots, predicate = (units[n] for n in (0x5E, 0x3C6, 0x3C2))
    check(u5e["ovr_offset"] + 0x263D == 0x14E38 and
          u5e["entries"][18] == 0x263D and u5e["memory_segment"] == 0x68,
          "caller descriptor mismatch")
    check(slots["memory_segment"] == 0x3E0 and
          slots["entries"][12:16] == [0xBE7, 0xC0F, 0xC37, 0xC5F], "slot descriptor mismatch")
    check(predicate["memory_segment"] == 0x3DB and predicate["entries"][4] == 0x57B,
          "predicate descriptor mismatch")
    spans = [(0x5E, 0x263D, 0x26B0), (0x3C6, 0xBE7, 0xC37),
             (0x3C6, 0xC5F, 0xCB9), (0x3C2, 0x57B, 0x909)]
    bodies = {}
    decoded = {}
    for unit, start, end in spans:
        file_start = units[unit]["ovr_offset"] + start
        data = ovr[file_start:file_start + end - start]
        instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, start))
        check(instructions[-1].address + instructions[-1].size == end, "incomplete decode")
        bodies[(unit, start)] = data
        decoded[(unit, start)] = instructions
        print(f"u{unit:X} [{start:04X},{end:04X}): file {file_start:#x}; "
              f"{len(data)} bytes/{len(instructions)} instructions; SHA256 {sha256(data).hexdigest()}")
    check(sha256(bodies[(0x5E, 0x263D)]).hexdigest() == CALLER_SHA, "caller hash mismatch")
    check(sha256(bodies[(0x3C2, 0x57B)]).hexdigest() == PREDICATE_SHA, "predicate hash mismatch")
    calls = [(i.address, i.op_str) for i in decoded[(0x5E, 0x263D)] if i.mnemonic == "lcall"]
    check(calls == [(0x2649, "0x3e0, 0x5c"), (0x2657, "0x3e0, 0x61"),
                    (0x2665, "0x3e0, 0x5c"), (0x2673, "0x3e0, 0x61"),
                    (0x2693, "0x3db, 0x34")], "caller targets changed")
    check(all(site + 3 in u5e["fixups"] for site, _ in calls), "missing far-call fixup")
    check(not any("call" in i.mnemonic for i in decoded[(0x3C2, 0x57B)]), "predicate is not a leaf")
    return bodies, decoded


def build(root, directory):
    output = directory / "u5e_263d.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-shared", "-fPIC", "-I", str(root / "include"),
                    str(root / "src/recovered/overlay_u5e_direction_263d.c"),
                    str(root / "src/recovered/overlay_u3c2_entry4_057b.c"),
                    "-o", str(output)], check=True)
    library = ctypes.CDLL(str(output))
    library.pr_u5e_direction_263d.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                              ctypes.POINTER(Cpu), ctypes.POINTER(Layout), ctypes.c_size_t]
    library.pr_u5e_direction_263d.restype = ctypes.c_int
    library.pr_u3c2_entry4_057b.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                           ctypes.POINTER(Cpu)]
    library.pr_u3c2_entry4_057b.restype = ctypes.c_int
    return library


def install(memory, cpu, layout, bodies, base):
    segments = {0x5E: cpu.cs, 0x3C6: layout.slots_cs, 0x3C2: layout.predicate_cs}
    for (unit, offset), data in bodies.items():
        p = physical(segments[unit], offset)
        memory[p:p + len(data)] = data
    # Only the five linker segment words are relocated. The real far stubs
    # execute EA jumps into each loaded code body; they have no stack writes.
    for offset in (0x264C, 0x265A, 0x2668, 0x2676):
        put16(memory, cpu.cs, offset, base + 0x3E0)
    put16(memory, cpu.cs, 0x2696, base + 0x3DB)
    for stub_cs, stub_ip, target_cs, target_ip in (
            (base + 0x3E0, 0x5C, layout.slots_cs, 0xBE7),
            (base + 0x3E0, 0x61, layout.slots_cs, 0xC0F),
            (base + 0x3DB, 0x34, layout.predicate_cs, 0x57B)):
        p = physical(stub_cs, stub_ip)
        memory[p:p + 5] = bytes([0xEA, target_ip & 255, target_ip >> 8,
                                 target_cs & 255, target_cs >> 8])


def fixture(coords=(10, 10, 10, 10), *, base=0, count=2, slots=(1, 2),
            seed=0, ss=None, sp=0x8000, ds=None, displacement=None,
            equivalent_pointer=False):
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * RAM_SIZE
    cpu = Cpu(*(rng.randrange(65536) for _ in NAMES))
    cpu.cs, cpu.ip = base + 0x3000, 0x263D
    cpu.ds = base + 0x1000 if ds is None else ds
    cpu.ss = base + 0x2000 if ss is None else ss
    cpu.sp, cpu.bp, cpu.flags = sp, 0x7777, 0x7202
    layout = Layout(base + 0x4000, base + 0x5000)
    # Distinct nonmatching raw words in every unused table slot.
    for index in range(256):
        put16(memory, cpu.ds, 0x65B9 + 4 * index, 0x9000 + index)
        put16(memory, cpu.ds, 0x65BB + 4 * index, 0x6100)
    for index, ptr, x, y in ((slots[0], 0x100, coords[0], coords[1]),
                             (slots[1], 0x200, coords[2], coords[3])):
        if index is not None:
            put16(memory, cpu.ds, 0x65B9 + 4 * index, ptr)
            put16(memory, cpu.ds, 0x65BB + 4 * index, 0x6000)
            memory[physical(cpu.ds, 0x5F27 + 4 * index)] = x
            memory[physical(cpu.ds, 0x5F28 + 4 * index)] = y
    memory[physical(cpu.ds, 0x5F27)] = 10
    memory[physical(cpu.ds, 0x5F28)] = 10
    memory[physical(cpu.ds, 0x5F2A)] = count
    # Synthetic displacement tables, zero by default. No game data embedded.
    displacement = displacement or ([0] * 256, [0] * 256)
    for offset, values in zip((0x2736, 0x273F), displacement):
        for i, v in enumerate(values):
            memory[physical(cpu.ds, offset + i)] = v & 255
    # Establish the caller frame last, so table-alias fixtures still enter
    # with a valid return address. The arguments may intentionally alias DS.
    put16(memory, cpu.ss, sp, 0x7000)
    put16(memory, cpu.ss, sp + 2, cpu.cs)
    for offset, value in ((4, 0x0200), (6, 0x6000), (8, 0x0100), (10, 0x6000)):
        put16(memory, cpu.ss, sp + offset, value)
    if equivalent_pointer:
        # Same physical address as 6000:0100, a different raw far pointer.
        put16(memory, cpu.ss, sp + 8, 0x0110)
        put16(memory, cpu.ss, sp + 10, 0x5FFF)
    return memory, cpu, layout


def compare(label, memory, cpu, layout, bodies, library, coverage, emulator, *, base=0, budget=2048):
    install(memory, cpu, layout, bodies, base)
    emulator.mem_write(0, bytes(memory))
    for name, register in zip(NAMES, REGISTERS):
        emulator.reg_write(register, getattr(cpu, name))
    remaining = [budget]
    stop = [None]
    shift_last = [False]
    segments = {cpu.cs: 0x5E, layout.slots_cs: 0x3C6, layout.predicate_cs: 0x3C2}

    def instruction(uc, address, size, user):
        del user
        cs = uc.reg_read(uc_regs.UC_X86_REG_CS)
        ip = uc.reg_read(uc_regs.UC_X86_REG_IP) & 0xFFFF
        if cs == cpu.cs and ip == 0x7000:
            stop[0] = 0
            uc.emu_stop()
            return
        if cs in segments:
            coverage.add((segments[cs], ip))
        if (cs == cpu.cs and ip == 0x267F) or (cs == layout.slots_cs and ip == 0xC71):
            if remaining[0] == 0:
                stop[0] = 3
                uc.emu_stop()
            else:
                remaining[0] -= 1
        last_writer_is_nonzero_shift(uc, address, size, shift_last)

    hook = emulator.hook_add(UC_HOOK_CODE, instruction)
    emulator.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=2000000)
    emulator.hook_del(hook)
    check(stop[0] is not None, f"{label}: original neither returned nor exhausted budget")
    c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    c_cpu = Cpu.from_buffer_copy(cpu)
    status = library.pr_u5e_direction_263d(c_ram, RAM_SIZE, ctypes.byref(c_cpu),
                                         ctypes.byref(layout), budget)
    check(status == stop[0], f"{label}: C status {status}, original stop {stop[0]}")
    expected = bytes(emulator.mem_read(0, RAM_SIZE))
    actual = bytes(c_ram)
    if actual != expected:
        different = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise AssertionError(f"{label}: RAM mismatch at {[hex(i) for i in different[:12]]}")
    for name, register in zip(NAMES, REGISTERS):
        got, want = getattr(c_cpu, name), emulator.reg_read(register) & 0xFFFF
        if name == "flags" and got != want and shift_last[0] and got == want | 0x10:
            AF_SPLIT_USED[0] += 1          # the declared split: pinned AF=1 after a shift, Unicorn 0; nothing else differs
            continue
        check(got == want, f"{label}: {name} C={got:04x}, original={want:04x}")
    return status, c_cpu.ax & 255


def run(bodies, decoded, library, random_cases):
    emulator = Uc(UC_ARCH_X86, UC_MODE_16)
    emulator.mem_map(0, RAM_SIZE)
    coverage, results = set(), set()
    total = 0

    def case(label, coords=(10, 10, 10, 10), *, budget=2048, **kwargs):
        nonlocal total
        memory, cpu, layout = fixture(coords, **kwargs)
        status, result = compare(label, memory, cpu, layout, bodies, library, coverage, emulator,
                                 base=kwargs.get("base", 0), budget=budget)
        if status == 0:
            results.add(result)
        total += 1

    for dx in range(-3, 4):
        for dy in range(-3, 4):
            case(f"geometry-{dx}-{dy}", (20, 10, 20 + dx, 10 + dy))
    compass = ([0, 1, 1, 1, 0, -1, -1, -1, 0],
               [-1, -1, 0, 1, 1, 1, 0, -1, 0])
    for direction, (dx, dy) in enumerate(zip(*compass)):
        case(f"compass-{direction}", (20, 10, 20 + 5 * dx, 10 + 5 * dy),
             displacement=compass)
    for coordinate in range(4):
        for value in (0, 24, 25, 49, 50, 127, 128, 255):
            coords = [10] * 4
            coords[coordinate] = value
            case(f"boundary-{coordinate}-{value}", tuple(coords), budget=270)
    case("missing-both", slots=(None, None))
    case("missing-first", slots=(None, 2))
    case("count-zero-slot255", count=0, slots=(255, 254))
    case("count-zero-no-match", count=0, slots=(None, None))
    case("count-zero-slot-zero", count=0, slots=(0, 2))
    case("equivalent-address-distinct-pointer", (1, 1, 20, 10), equivalent_pointer=True)
    case("direction-eight", (10, 10, 11, 11), displacement=(
        [0, 100, 100, 100, 0, -100, -100, -100, 0],
        [-100, -100, 0, 100, 100, 100, 0, -100, 0]))
    case("search-budget", budget=1, slots=(255, 254), count=0)
    case("predicate-budget", budget=6)
    case("relocated", (20, 10, 19, 12), base=0x1234)
    case("lowest-stack", sp=34)
    case("highest-stack", sp=0xFFF4)
    case("same-DS-SS", ds=0x2000)
    # Shared-memory aliases: argument/local frame words can be read as table
    # pointers/coordinate bytes. Setups remain synthetic; full RAM is compared.
    for sp in (0x5F32, 0x5F40, 0x65E0, 0x6600):
        case(f"stack-table-alias-{sp:04x}", ss=0x1000, sp=sp, budget=1100)
    rng = random.Random(0x5E263D)
    for i in range(random_cases):
        coords = tuple(rng.randrange(limit) for limit in (50, 25, 50, 25))
        table = ([rng.randrange(256) for _ in range(9)],
                 [rng.randrange(256) for _ in range(9)])
        case(f"random-{i}", coords, seed=i, displacement=table,
             base=rng.choice((0, 0x1234)))
    caller_starts = {i.address for i in decoded[(0x5E, 0x263D)]}
    reached = {ip for unit, ip in coverage if unit == 0x5E}
    check(caller_starts <= reached, f"uncovered caller instructions: {sorted(caller_starts - reached)}")
    print(f"PASS: {total} original-chain/C cases; all 14 CPU words and all {RAM_SIZE} RAM bytes equal "
          f"(FLAGS through the declared AF split in {AF_SPLIT_USED[0]} case(s): last flag writer a non-zero shift, C AF=1 pinned, Unicorn 0)")
    print(f"Caller coverage {len(caller_starts & reached)}/{len(caller_starts)} instructions; "
          f"returned direction bytes {sorted(results)}; budget stops compared at exact boundaries")


def predicate_audit(bodies, decoded, library, random_cases):
    """Test the dependency's complete ABI, including paths the caller cannot reach."""
    emulator = Uc(UC_ARCH_X86, UC_MODE_16)
    emulator.mem_map(0, RAM_SIZE)
    coverage = set()
    cases = [((10, 10, 20, 20), mode, None) for mode in range(256)]
    cases.extend([
        ((10, 10, 10, 10), 100, None),  # equality overrides unrecognized mode
        ((10, 10, 11, 9), 1, ([0, 1] + [0] * 7, [0, -1] + [0] * 7)),
    ])
    for index in range(4):
        for value in (0, 24, 25, 49, 50, 127, 128, 255):
            coords = [10] * 4
            coords[index] = value
            cases.append((tuple(coords), 0, None))
    rng = random.Random(0x3C2057B)
    for _ in range(random_cases):
        cases.append((tuple(rng.randrange(n) for n in (50, 25, 50, 25)), rng.randrange(9),
                      ([rng.randrange(256) for _ in range(9)],
                       [rng.randrange(256) for _ in range(9)])))
    cases = [(coords, mode, table, {}) for coords, mode, table in cases]
    for sp in (0x2738, 0x2740, 0x2743, 0x2748):
        for mode in (0, 8, 9, 255):
            cases.append(((10, 10, 20, 20), mode, None, {"ss": 0x1000, "sp": sp}))
    for index, (coords, mode, table, options) in enumerate(cases):
        memory, cpu, layout = fixture(seed=index, displacement=table, **options)
        install(memory, cpu, layout, bodies, 0)
        return_cs = cpu.cs
        cpu.cs, cpu.ip = layout.predicate_cs, 0x57B
        # Original far-call frame followed by Pascal's reversed argument order.
        for offset, value in enumerate((0x7000, return_cs, mode, coords[3],
                                         coords[2], coords[1], coords[0])):
            put16(memory, cpu.ss, cpu.sp + 2 * offset, value)
        emulator.mem_write(0, bytes(memory))
        for name, register in zip(NAMES, REGISTERS):
            emulator.reg_write(register, getattr(cpu, name))
        returned = [False]

        def instruction(uc, address, size, user):
            del address, size, user
            cs = uc.reg_read(uc_regs.UC_X86_REG_CS)
            if cs != cpu.cs:
                returned[0] = True
                uc.emu_stop()
            else:
                coverage.add(uc.reg_read(uc_regs.UC_X86_REG_IP) & 0xFFFF)

        hook = emulator.hook_add(UC_HOOK_CODE, instruction)
        emulator.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=10000)
        emulator.hook_del(hook)
        check(returned[0], f"predicate-{index}: original did not return")
        c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
        c_cpu = Cpu.from_buffer_copy(cpu)
        check(library.pr_u3c2_entry4_057b(c_ram, RAM_SIZE, ctypes.byref(c_cpu)) == 0,
              f"predicate-{index}: C rejected")
        expected = bytes(emulator.mem_read(0, RAM_SIZE))
        actual = bytes(c_ram)
        if actual != expected:
            different = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
            raise AssertionError(f"predicate-{index}, coords={coords}, mode={mode}: "
                                 f"RAM mismatch at {[hex(i) for i in different[:12]]}")
        for name, register in zip(NAMES, REGISTERS):
            got, want = getattr(c_cpu, name), emulator.reg_read(register) & 0xFFFF
            check(got == want, f"predicate-{index}, coords={coords}, mode={mode}: "
                  f"{name} C={got:04x}, original={want:04x}")
    starts = {i.address for i in decoded[(0x3C2, 0x57B)]}
    print(f"PASS: {len(cases)} standalone predicate CPU/RAM cases; "
          f"instruction coverage {len(coverage & starts)}/{len(starts)}")
    check(starts <= coverage, f"uncovered predicate instructions: {sorted(starts - coverage)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--ovr", type=Path)
    parser.add_argument("--random-cases", type=int, default=200)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the C library without media, print CHECKER_BUILD_OK, and exit")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    if args.compile_only:
        with tempfile.TemporaryDirectory(prefix="poolrad-u5e-263d-") as directory:
            build(root, Path(directory))
        print("CHECKER_BUILD_OK")
        return
    if args.exe is None or args.ovr is None:
        parser.error("--exe and --ovr are required unless --compile-only")
    bodies, decoded = source_audit(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix="poolrad-u5e-263d-") as directory:
        library = build(root, Path(directory))
        predicate_audit(bodies, decoded, library, args.random_cases)
        from loaded_stub_checks import verify_refusals
        memory,cpu,layout=fixture()
        install(memory,cpu,layout,bodies,0)
        stub_guards=verify_refusals(memory,cpu,lambda ram,state: library.pr_u5e_direction_263d(ram,RAM_SIZE,ctypes.byref(state),ctypes.byref(layout),2048),
                                    [(cpu.cs,p) for p in (0x2649,0x2657,0x2665,0x2673,0x2693)])
        print("LOADED_STUB_GUARDS",stub_guards,flush=True)
        run(bodies, decoded, library, args.random_cases)


if __name__ == "__main__":
    main()
