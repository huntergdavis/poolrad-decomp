#!/usr/bin/env python3
"""Compare complete U25 string cores and SYSTEM dependencies on synthetic RAM."""
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

RAM_SIZE = 1 << 20
NAMES = "ax bx cx dx si di bp sp cs ip ds es ss flags".split()
REGS = [getattr(xr, "UC_X86_REG_" + ("EFLAGS" if n == "flags" else n.upper())) for n in NAMES]
LOOPS = {0x1172, 0x1327, 0x13D3, 0x1453, 0x14D2}
# file, start, end, byte hash; no original bytes are embedded.
SPANS = [
    ("sys", 0xB0B, 0xB25, "4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f"),
    ("sys", 0xB25, 0xB49, "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"),
    ("sys", 0xB98, 0xBC4, "c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64"),
    ("sys", 0xC3B, 0xC4D, "b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484"),
    ("u25", 0x6D3, 0x6FB, "17523f3ee51e77b906204fc675d61e6d6d9a83dfed02333125a53ee014924715"),
    ("u25", 0x6FB, 0x756, "e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347"),
    ("u25", 0x1153, 0x12FB, "51ffb0a24648a5c249b00849c36964bfb21ebdc2bc2802269d11d4e47dad5fcd"),
    ("u25", 0x12FB, 0x154D, "16bf5a9e5c1fe284cdaca16e5711975077d7f4b89369617db3a2dc0b77c6960e"),
]


class Cpu(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in NAMES]


def check(value, message):
    if not value:
        raise AssertionError(message)


def physical(segment, offset):
    return ((segment << 4) + (offset & 65535)) & (RAM_SIZE - 1)


def put16(ram, segment, offset, value):
    p = physical(segment, offset)
    ram[p] = value & 255
    ram[(p + 1) & (RAM_SIZE - 1)] = value >> 8


def far_pointer(ram, ds, offset, segment, target):
    put16(ram, ds, offset, target)
    put16(ram, ds, offset + 2, segment)


def source_audit(exe_path, ovr_path):
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    check(sha256(exe).hexdigest() == "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
          "unsupported START.EXE")
    check(sha256(ovr).hexdigest() == "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
          "unsupported GAME.OVR")
    mz = parse_mz(exe)
    unit = next(u for u in find_units(exe[mz["header_bytes"]:], ovr) if u["stub_para"] == 0x25)
    check(unit["ovr_offset"] == 0x74D3 and unit["memory_segment"] == 0x2B, "U25 descriptor")
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    bodies, decoded = {}, {}
    for kind, start, end, digest in SPANS:
        source, base = (exe, 0xAC76) if kind == "sys" else (ovr, unit["ovr_offset"])
        data = source[base + start:base + end]
        check(sha256(data).hexdigest() == digest, f"source mismatch {kind}:{start:04X}")
        ins = list(md.disasm(data, start))
        check(ins[-1].address + ins[-1].size == end, "decode extent")
        for i in ins:
            if i.mnemonic == "lcall":
                check(i.address + 3 in unit["fixups"], "missing call relocation")
                check(i.op_str in ("0xaf8, 0xb0b", "0xaf8, 0xb25", "0xaf8, 0xb98", "0xaf8, 0xc3b"),
                      "unexpected external call")
            if i.mnemonic == "call":
                check(i.op_str in ("0x6d3", "0x6fb"), "unexpected local call")
        bodies[kind, start], decoded[kind, start] = data, ins
        print(f"{kind}:{start:04X} file={base+start:#x} bytes={len(data)} instructions={len(ins)} SHA256={digest}")
    return bodies, decoded


def build(root, directory):
    path = directory / "u25-strings.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-shared", "-fPIC",
                    "-I", str(root / "include"), str(root / "src/recovered/overlay_u25_string_cores.c"),
                    "-o", str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    for name in ("pr_u25_unpack_text_1153", "pr_u25_string_by_address_12fb"):
        f = getattr(lib, name)
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu),
                      ctypes.c_uint16, ctypes.c_size_t]
        f.restype = ctypes.c_int
    return lib


def fixture(entry, *, seed=0, base=0, ds=None, ss=None, sp=0x8000):
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * RAM_SIZE
    cpu = Cpu(*(rng.randrange(65536) for _ in NAMES))
    cpu.cs, cpu.ip, cpu.ds, cpu.ss = base + 0x3000, entry, base + 0x1000, base + 0x2000
    if ds is not None:
        cpu.ds = ds
    if ss is not None:
        cpu.ss = ss
    cpu.sp, cpu.bp, cpu.flags = sp, 0x7777, 0x7602  # include DF; runtime CLD must change it
    system_cs = base + 0x4000
    return memory, cpu, system_cs


def frame(memory, cpu, first, second):
    for index, value in enumerate((0x7000, cpu.cs, first, second)):
        put16(memory, cpu.ss, cpu.sp + 2 * index, value)


def unpack_fixture(packed, *, slot=1, pc=0, **options):
    memory, cpu, system_cs = fixture(0x1153, **options)
    far_pointer(memory, cpu.ds, 0x49DE, 0x6000, 0x1000)
    put16(memory, cpu.ds, 0x49ED, pc)
    for i, value in enumerate(packed):
        memory[physical(0x6000, 0x1000 + pc + i + 1 + 0x6700)] = value
    frame(memory, cpu, slot | 0xAB00, len(packed) | 0xCD00)
    return memory, cpu, system_cs


def address_fixture(address, values, *, slot=1, high_zero=False, **options):
    memory, cpu, system_cs = fixture(0x12FB, **options)
    for offset, segment in ((0x49D2, 0x6000), (0x49D6, 0x6100),
                            (0x49DA, 0x6200), (0x49DE, 0x6300), (0x5D92, 0x6400)):
        far_pointer(memory, cpu.ds, offset, segment, 0x1000)
    if address == 0x6B00:
        memory[physical(0x6400, 0x1000)] = len(values)
        for i, value in enumerate(values):
            memory[physical(0x6400, 0x1001 + i)] = value
    else:
        if 0x4900 <= address <= 0x4CFF:
            segment, disp, scale = 0x6000, 0x6E00, 2
        elif 0x6B00 <= address <= 0x6EFF:
            segment, disp, scale = 0x6100, 0x2A00, 2
        elif 0x9700 <= address <= 0x98FF:
            segment, disp, scale = 0x6200, -0x2E00, 2
        else:
            segment, disp, scale = 0x6300, 0x6700, 1
        for i, value in enumerate(values):
            offset = 0x1000 + scale * ((address + i) & 65535) + disp
            if scale == 2:
                put16(memory, segment, offset, value | (0x1200 if high_zero else 0))
            else:
                memory[physical(segment, offset)] = value
        if len(values) < 256:
            offset = 0x1000 + scale * ((address + len(values)) & 65535) + disp
            if scale == 2:
                put16(memory, segment, offset, 0)
            else:
                memory[physical(segment, offset)] = 0
    frame(memory, cpu, address, slot | 0xAB00)
    return memory, cpu, system_cs


def compare(label, fixture, budget, bodies, decoded, lib, emulator, coverage):
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
    remaining, stop = [budget], [None]

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
                stop[0] = 3
                uc.emu_stop()
                return
        if address == return_address:
            stop[0] = 0
            uc.emu_stop()
            return
        check(address in locations, f"{label}: unexpected original PC {address:05X}")
        kind, ip = locations[address]
        item = instructions[address]
        if item.mnemonic in ("call", "lcall"):
            calls.append((cpu.cs if kind == "u25" else system_cs, ip + item.size))
        elif item.mnemonic == "retf" and calls:
            returning[0] = calls.pop()
        coverage.add((kind, ip))
        if kind == "u25" and ip in LOOPS:
            if remaining[0] == 0:
                stop[0] = 2
                uc.emu_stop()
            else:
                remaining[0] -= 1

    hook = emulator.hook_add(UC_HOOK_CODE, instruction)
    emulator.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=2000000)
    emulator.hook_del(hook)
    check(stop[0] is not None, f"{label}: original exceeded instruction safety limit")
    c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    c_cpu = Cpu.from_buffer_copy(cpu)
    function = lib.pr_u25_unpack_text_1153 if cpu.ip == 0x1153 else lib.pr_u25_string_by_address_12fb
    status = function(c_ram, RAM_SIZE, ctypes.byref(c_cpu), system_cs, budget)
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
    coverage, counts = set(), {0x1153: 0, 0x12FB: 0}
    limits, transfers = 0, 0

    def case(label, fixture, budget=1024):
        nonlocal limits, transfers
        status = compare(label, fixture, budget, bodies, decoded, lib, emulator, coverage)
        counts[fixture[1].ip] += 1
        limits += status == 2
        transfers += status == 3
        total = sum(counts.values())
        if total % 25 == 0:
            print(f"Compared {total} cases; latest {label}")

    for length in (0, 1, 2, 3, 4, 5, 6, 191, 192, 193, 254, 255):
        for byte in (0, 1, 0x7F, 0x80, 0xFF):
            case(f"packed-{length}-{byte}", unpack_fixture(bytes([byte]) * length))
    for slot in (0, 2, 31, 127, 144, 217, 218, 255):
        case(f"packed-slot-{slot}", unpack_fixture(bytes(range(32)), slot=slot))
    case("packed-pc-wrap", unpack_fixture(bytes(range(8)), pc=0xFFFC))
    case("packed-budget", unpack_fixture(b"\xff" * 12), budget=2)
    case("packed-relocated", unpack_fixture(b"\x12\x34\x56", base=0x1234))
    case("packed-low-stack", unpack_fixture(b"\x12\x34\x56", sp=0x218))
    case("packed-high-stack", unpack_fixture(b"\x12\x34\x56", sp=0xFFF8))
    case("packed-same-DS-SS", unpack_fixture(b"\x12\x34\x56", ss=0x1000))
    for address in (0, 0x48FF, 0x4900, 0x4CFF, 0x4D00, 0x6AFF, 0x6B00, 0x6B01,
                    0x6EFF, 0x6F00, 0x96FF, 0x9700, 0x98FF, 0x9900, 0xB6FF, 0xB700, 0xFFFF):
        for length in (0, 1, 15, 255):
            case(f"address-{address:04X}-{length}", address_fixture(address, b"x" * length))
    for address in (0x4900, 0x6B01, 0x9700, 0x9900):
        case(f"nonterminating-{address:04X}", address_fixture(address, b"x" * 256), budget=270)
        case(f"lowzero-{address:04X}", address_fixture(address, bytes(3), high_zero=True))
        case(f"relocated-{address:04X}", address_fixture(address, b"abc", base=0x1234))
        case(f"same-DS-SS-{address:04X}", address_fixture(address, b"abc", ss=0x1000))
    for sp in (0x72D0, 0x72D2, 0x72E0):
        case(f"packed-output-stack-alias-{sp:04X}",
             unpack_fixture(bytes(range(1, 13)), ss=0x1000, sp=sp), budget=64)
        case(f"address-output-stack-alias-{sp:04X}",
             address_fixture(0x9900, b"abcdefghijk", ss=0x1000, sp=sp), budget=64)
    rng = random.Random(0x251153)
    for i in range(random_cases):
        length = rng.randrange(256)
        case(f"random-packed-{i}", unpack_fixture(rng.randbytes(length), seed=i,
             slot=rng.choice((0, 1, 2, 127, 255)), base=rng.choice((0, 0x1234))))
        address = rng.choice((0x4900, 0x6B00, 0x6B01, 0x9700, 0x9900, 0xC000))
        case(f"random-address-{i}", address_fixture(address, bytes(rng.randrange(1, 256) for _ in range(length)),
             seed=i, slot=rng.choice((0, 1, 2, 127, 255)), base=rng.choice((0, 0x1234))))
    print(f"PASS: {counts[0x1153]} unpack and {counts[0x12FB]} address cases; "
          f"14 CPU words + {RAM_SIZE} RAM bytes equal; {limits} exact budget stops; {transfers} transfer stops")
    for key, ins in decoded.items():
        starts = {(key[0], i.address) for i in ins}
        print(f"Coverage {key[0]}:{key[1]:04X}: {len(starts & coverage)}/{len(starts)}")
        unreachable = {("sys", 0xB3B)}  # every composed StrStore cap is FF
        check(starts <= coverage | unreachable, f"uncovered {key}: {sorted(starts - coverage)}")
        if starts - coverage:
            print("Unreached: SYSTEM +0B3B; every composed caller pushes cap FF")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--random-cases", type=int, default=100)
    args = parser.parse_args()
    bodies, decoded = source_audit(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-strings-") as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, decoded, library, args.random_cases)


if __name__ == "__main__":
    main()
