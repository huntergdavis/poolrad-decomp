#!/usr/bin/env python3
"""Hash-locked original/C complete slot-value differential; stdout is counts/hashes only.

Requires local Capstone 5 and Unicorn 2. Owner media, captures, and builds
remain under ignored resources/. Optional native RAM is captured-state
emulation with a reconstructed entry frame, not a new native entry trace.
"""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_PROT_ALL
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

from u25_load_word_0e0c_audit import (Cpu, REGS, SIZE, source as load_sources,
    make_case as load_fixture, cases as load_cases, physical, put16, word, require)

ROOT = Path(__file__).resolve().parents[2]


def source(ovr):
    bodies, instructions = load_sources(ovr)
    data = ovr.read_bytes()
    for start, end, digest in (
        (0x136, 0x1B6, "0dd25983c54922e1ebe6f9f1b693f32c6a1c2b69199ddf110fc7d040134ec42b"),
        (0x6AF, 0x6D3, "53565df24104754f01dff68b10cf0dda3fe72ad394a5253c611ceb594d93e862"),
    ):
        body = data[29907 + start:29907 + end]
        require(sha256(body).hexdigest() == digest, "slot-value original hash")
        decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start))
        require(sum(i.size for i in decoded) == len(body), "slot-value decode extent")
        calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
        require(calls == ([(0x18E, "0xe0c"), (0x1A7, "0x6af")] if start == 0x136 else []),
                "slot-value call graph")
        for site, _ in calls:
            require(body[site - start - 1] == 0x0E, "aligned PUSH CS")
        bodies[start] = body
        for i in decoded:
            instructions[i.address] = (i.mnemonic, i.size)
        print(f"u25:{start:04X} bytes={len(body)} instructions={len(decoded)} SHA256={digest}")
    require(len(instructions) == 494, "complete chain instruction count")
    return bodies, instructions


CHECKER_ROWS = ('OVL_0025_0136',)


def library(directory):
    path = directory / "slot-value.so"
    files = ("overlay_u25_slot_value_0136.c", "overlay_u25_entry1_0136.c",
             "overlay_u25_load_word_0e0c.c", "overlay_u25_party_index_0756.c")
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-shared", "-fPIC", "-I", str(ROOT / "include"),
        *[str(ROOT / "src/recovered" / name) for name in files], "-o", str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u25_slot_value_0136.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                         ctypes.POINTER(Cpu)]
    lib.pr_u25_slot_value_0136.restype = ctypes.c_int
    return lib


def slot(ram, cpu, address, *, index=1, tag=1):
    cpu.ip = 0x136
    signed = index if index < 128 else index - 256
    ram[physical(cpu.ds, 0x7085 + signed)] = address & 255
    ram[physical(cpu.ds, 0x7045 + signed)] = address >> 8
    ram[physical(cpu.ds, 0x7005 + signed)] = tag
    put16(ram, cpu.ss, cpu.sp + 4, 0xAB00 | index)
    return ram, cpu


def make_case(bodies, address, *, index=1, tag=1, **kwargs):
    return slot(*load_fixture(bodies, address, **kwargs), address, index=index, tag=tag)

def compare(label, ram, initial, bodies, instructions, lib, visited):
    for start, body in bodies.items():
        at = physical(initial.cs, start)
        require(ram[at:at + len(body)] == body, "case overlaps immutable code")
    baseline = bytes(ram)
    original_ram = (ctypes.c_uint8 * SIZE).from_buffer(ram)
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    for base in (0, SIZE):
        uc.mem_map_ptr(base, SIZE, UC_PROT_ALL, ctypes.addressof(original_ram))
    for (name, _), reg in zip(Cpu._fields_, REGS):
        uc.reg_write(reg, getattr(initial, name))
    calls, stopped, stepping = [], [], [False]
    count = [0]

    def hook(emulator, at, length, user):
        del at, length, user
        if stepping[0]:
            return
        ip = emulator.reg_read(UC_X86_REG_IP)
        cs = emulator.reg_read(UC_X86_REG_CS)
        require(cs == initial.cs and ip in instructions, "unexpected original execution before return")
        visited.add(ip)
        count[0] += 1
        require(count[0] <= 100000, "original exceeded bounded audit execution")
        mnemonic, size = instructions[ip]
        if mnemonic == "call":
            calls.append((cs, (ip + size) & 0xFFFF))
        if mnemonic == "retf":
            stopped.append(True)
            emulator.emu_stop()

    uc.hook_add(UC_HOOK_CODE, hook)
    expected_status = 0
    while True:
        stopped.clear()
        start = (uc.reg_read(UC_X86_REG_CS) << 4) + uc.reg_read(UC_X86_REG_IP)
        uc.emu_start(start, 2 * SIZE, count=100001)
        require(stopped, "original failed to reach a far return")
        stepping[0] = True
        uc.emu_start((uc.reg_read(UC_X86_REG_CS) << 4) + uc.reg_read(UC_X86_REG_IP), 2 * SIZE, count=1)
        stepping[0] = False
        if not calls:
            break
        target = calls.pop()
        actual = (uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP))
        if actual != target:
            expected_status = 2
            break
    candidate_ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(baseline)
    candidate = Cpu.from_buffer_copy(initial)
    status = lib.pr_u25_slot_value_0136(candidate_ram, SIZE, ctypes.byref(candidate))
    require(status == expected_status, f"status mismatch {label}: {status}/{expected_status}")
    expected, actual = bytes(original_ram), bytes(candidate_ram)
    if expected != actual:
        offsets = [i for i, (a, b) in enumerate(zip(expected, actual)) if a != b][:8]
        raise ValueError(f"RAM mismatch {label}: physical offsets {offsets}")
    for (name, _), reg in zip(Cpu._fields_, REGS):
        value = uc.reg_read(reg) & 0xFFFF
        require(getattr(candidate, name) == value,
                f"register mismatch {label} {name}: {getattr(candidate, name):04x}/{value:04x}")
    return status



def run(bodies, instructions, lib):
    visited, total, transfers = set(), 0, 0

    def case(label, ram, cpu):
        nonlocal total, transfers
        transfers += compare(label, ram, cpu, bodies, instructions, lib, visited) == 2
        total += 1
        if total % 250 == 0:
            print(f"Compared {total} slot-value cases; latest {label}")

    # Signed-index and independent tag sweeps include the unwritten-result path.
    for index in range(256):
        for tag in (0, 1, 2, 0xFF):
            case(f"index-{index:02X}-tag-{tag:02X}",
                 *make_case(bodies, 0xC04F, index=index, tag=tag))
    for tag in range(256):
        case(f"tag-{tag:02X}", *make_case(bodies, 0x6B18, tag=tag))
    # Reuse every original load-word fixture, now reached through +0136.
    addresses = [0, 0xB1, 0xFB, 0xFC, 0x33D, 0x35F, 0x48FF, 0x4900, 0x49FD,
        0x4CFF, 0x4D00, 0x6AFF, 0x6F00, 0x96FF, 0x9700, 0x98FF, 0x9900,
        0xB6FF, 0xB700, 0xC04A, 0xC04B, 0xC04C, 0xC04D, 0xC04E, 0xC04F,
        0xC050, 0xC059, 0xFFFF]
    addresses = sorted(set(addresses) | set(range(0x6B00, 0x6F00)))
    for label, ram, cpu in load_cases(bodies, addresses, False):
        address = word(ram, cpu.ss, cpu.sp + 4)
        case("composed-" + label, *slot(ram, cpu, address))
    # Table writes and the original prologue/local stores share real RAM.
    for index in (0, 1, 0x7F, 0x80, 0xFF):
        for sp in (0x7009, 0x704F, 0x708F, 0x7010, 0x7051, 0x7091):
            for tag in (0, 1, 2, 0xFF):
                case(f"table-alias-{index:02X}-{sp:04X}-{tag:02X}",
                     *make_case(bodies, 0xC04F, index=index, tag=tag, ss=0x1000, sp=sp))
    for sp in (0, 1, 2, 3, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0xFFFE, 0xFFFF):
        for tag in (0, 2, 0x81, 0xFF):
            case(f"pair-stack-{sp:04X}-{tag:02X}",
                 *make_case(bodies, 0xBEEF, tag=tag, sp=sp))
    for tag in (0, 2, 0x81, 0xFF):
        case(f"pair-A20-{tag:02X}", *make_case(bodies, 0xBEEF, tag=tag, sp=0xF, ss=0xFFFF))
    for sp in (0x10, 0x11, 0x12, 0x13, 0xFFFE, 0xFFFF):
        for ss in (0x2000, 0xFFFF):
            for tag in (0, 1, 2, 0xFF):
                case(f"far-frame-{sp:04X}-{ss:04X}-{tag:02X}",
                     *make_case(bodies, 0xC04F, sp=sp, ss=ss, tag=tag))
    print(f"Compared: {total} slot-value cases; all 14 CPU words/all {SIZE} RAM bytes equal; "
          f"internal transfers={transfers}")
    for start, body in sorted(bodies.items()):
        offsets = {i.address for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, start)}
        print(f"Coverage u25:{start:04X}: {len(offsets & visited)}/{len(offsets)}")
    # Fixed classifier returns 0..4, so its caller's guard +0EA4 is unreachable.
    require(set(instructions) - visited == {0xEA4}, "unexpected instruction coverage gap")
    print("PASS: full-state comparison and reachable-instruction coverage")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ovr", type=Path,
                        help="GAME.OVR path; not needed with --compile-only")
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    args = parser.parse_args()
    if not args.compile_only and args.ovr is None:
        parser.error("--ovr is required unless --compile-only is given")
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies, instructions = source(args.ovr)
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-slot-") as folder:
        run(bodies, instructions, library(Path(folder)))


if __name__ == "__main__":
    main()
