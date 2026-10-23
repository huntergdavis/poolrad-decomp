#!/usr/bin/env python3
"""Hash-locked original/C load-word differential; stdout is counts/hashes only.

Requires local Capstone 5 and Unicorn 2. Owner media, captures, and builds
remain under ignored resources/. Optional native RAM is captured-state
emulation with a reconstructed entry frame, not a new native entry trace.
"""

import argparse
import ctypes
from hashlib import sha256
import json
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
from pinned_shift_flags import LastFlagWriter, SPLIT_BY_RULE, declared_flags_split, split_used

ROOT = Path(__file__).resolve().parents[2]
SIZE = 1 << 20
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPANS = (
    (0x06FB, 0x0756, "e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347"),
    (0x0756, 0x07BC, "b307e1dd310b369eeb068ef1e0fd7b84667e59382144bec02e5295b974c3fcb9"),
    (0x07BC, 0x0A74, "6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451"),
    (0x0E0C, 0x0F53, "e8f999e62ee3f04b194908f90ce7c0e15b0478a2af16a3843a1a076c86c67ce5"),
)
REGS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
        UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
        UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def require(ok, message):
    if not ok:
        raise ValueError(message)


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (SIZE - 1)


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & (SIZE - 1)] = (value >> 8) & 255


def word(ram, seg, off):
    at = physical(seg, off)
    return ram[at] | (ram[(at + 1) & (SIZE - 1)] << 8)


def pointer(ram, ds, off, target_seg, target_off):
    at = physical(ds, off)
    for i, value in enumerate((target_off & 255, target_off >> 8, target_seg & 255, target_seg >> 8)):
        ram[(at + i) & (SIZE - 1)] = value


def source(ovr):
    data = ovr.read_bytes()
    require(len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA, "owner overlay hash mismatch")
    bodies, instructions = {}, {}
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    for start, end, digest in SPANS:
        body = data[29907 + start:29907 + end]
        require(sha256(body).hexdigest() == digest, "original body hash mismatch")
        decoded = list(md.disasm(body, start))
        require(sum(i.size for i in decoded) == len(body), "incomplete body decode")
        bodies[start] = body
        for i in decoded:
            instructions[i.address] = (i.mnemonic, i.size)
    require(len(instructions) == 421, "original instruction set differs")
    return bodies, instructions


CHECKER_ROWS = ('OVL_0025_0E0C',)


def library(directory):
    path = directory / "load-word.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-shared", "-fPIC", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u25_load_word_0e0c.c"),
        str(ROOT / "src/recovered/overlay_u25_party_index_0756.c"), "-o", str(path)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u25_load_word_0e0c.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u25_load_word_0e0c.restype = ctypes.c_int
    return lib


def make_case(bodies, address, *, sp=0x8000, ds=0x1000, ss=0x2000):
    ram = bytearray(bytes(range(256)) * (SIZE // 256))
    cpu = Cpu(0x9234, 0x1357, 0x2468, 0xABCD, 0x5678, 0x6789,
              0x4321, sp, 0x3000, 0x0E0C, ds, 0x6000, ss, 0x0AD7)
    for offset, seg in ((0x49D2, 0x4000), (0x49D6, 0x4200), (0x49DA, 0x4400), (0x49DE, 0x4600)):
        pointer(ram, ds, offset, seg, 0x0100)
    pointer(ram, ds, 0x5D92, 0x5000, 0x0100)
    pointer(ram, ds, 0x5D96, 0, 0)
    # The untouched caller and hook result words deliberately differ.
    put16(ram, ss, (sp - 4) & 0xFFFF, 0xD35A)
    put16(ram, ss, (sp - 22) & 0xFFFF, 0xBEEF)
    put16(ram, ss, sp, 0x1111)
    put16(ram, ss, (sp + 2) & 0xFFFF, 0x7000)
    put16(ram, ss, (sp + 4) & 0xFFFF, address)
    for start, body in bodies.items():
        at = physical(cpu.cs, start)
        ram[at:at + len(body)] = body
    return ram, cpu


SPLIT_ARRIVALS = [0]


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
    writer = LastFlagWriter()  # declared splits for the pinned flag rules, see pinned_shift_flags

    def hook(emulator, at, length, user):
        del length, user
        if stepping[0]:
            return
        ip = emulator.reg_read(UC_X86_REG_IP)
        cs = emulator.reg_read(UC_X86_REG_CS)
        require(cs == initial.cs and ip in instructions, "unexpected original execution before return")
        visited.add(ip)
        count[0] += 1
        require(count[0] <= 100000, "original exceeded bounded audit execution")
        writer.step(emulator, at)
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
    writer.finish(uc)
    candidate_ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(baseline)
    candidate = Cpu.from_buffer_copy(initial)
    status = lib.pr_u25_load_word_0e0c(candidate_ram, SIZE, ctypes.byref(candidate))
    require(status == expected_status, f"status mismatch {label}: {status}/{expected_status}")
    expected, actual = bytes(original_ram), bytes(candidate_ram)
    if expected != actual:
        offsets = [i for i, (a, b) in enumerate(zip(expected, actual)) if a != b][:8]
        raise ValueError(f"RAM mismatch {label}: physical offsets {offsets}")
    for (name, _), reg in zip(Cpu._fields_, REGS):
        value = uc.reg_read(reg) & 0xFFFF
        if name == "flags":
            problem = declared_flags_split(candidate.flags, value, writer)
            require(problem is None, f"register mismatch {label} flags: {problem}")
            SPLIT_ARRIVALS[0] += split_used(candidate.flags, value, writer)
            continue
        require(getattr(candidate, name) == value,
                f"register mismatch {label} {name}: {getattr(candidate, name):04x}/{value:04x}")
    return status


def cases(bodies, addresses, quick):
    for address in addresses:
        yield f"address-{address:04x}", *make_case(bodies, address)
    if quick:
        for label, ram, cpu in cases(bodies, [], False):
            if label.startswith(("record-byte-6dcf-", "record-100-", "record-10c-", "party-")):
                yield label, ram, cpu
        return
    for address, offset in ((0x33D, 0x6AAF), (0xC04B, 0x6AAD), (0xC04C, 0x6AAE),
                            (0xC04D, 0x6AAF), (0xC04E, 0x6AB0), (0xC04F, 0x6AB1)):
        for value in range(256):
            ram, cpu = make_case(bodies, address)
            ram[physical(cpu.ds, offset)] = value
            yield f"pseudo-byte-{address:04x}-{value}", ram, cpu
    for address, offset in ((0xB1, 0x6F6E), (0xFB, 0x6F6A), (0xFC, 0x6F6C)):
        for value in (0, 1, 0x100, 0x7FFF, 0x8000, 0xFFFF):
            ram, cpu = make_case(bodies, address)
            put16(ram, cpu.ds, offset, value)
            yield f"pseudo-word-{address:04x}-{value}", ram, cpu
    for address, offset in ((0x6B18, 0x14), (0x6B9B, 0x6E), (0x6BA0, 0x73),
                            (0x6BD8, 0xA0), (0x6DCF, 0x15)):
        for value in range(256):
            ram, cpu = make_case(bodies, address)
            ram[physical(0x5000, 0x100 + offset)] = value
            yield f"record-byte-{address:04x}-{value}", ram, cpu
    for a in (0, 1, 2, 255):
        ram, cpu = make_case(bodies, 0x6C00)
        ram[physical(0x5000, 0x20D)] = a
        yield f"record-100-{a}", ram, cpu
        for b in (0, 1, 2, 255):
            ram, cpu = make_case(bodies, 0x6C0C)
            ram[physical(0x5000, 0x20E)] = a
            ram[physical(0x5000, 0x20F)] = b
            yield f"record-10c-{a}-{b}", ram, cpu
    for address in (0x6DB1, 0x6DB4):
        for length in (0, 1, 3, 257):
            for target in (0, max(0, length - 1), -1):
                ram, cpu = make_case(bodies, address)
                pointer(ram, cpu.ds, 0x5D96, 0x5000 if length else 0, 0x100 if length else 0)
                pointer(ram, cpu.ds, 0x5D92, 0x9000 if target < 0 else 0x5000 + 0x20 * target, 0x100)
                for i in range(length):
                    pointer(ram, 0x5000 + 0x20 * i, 0x204,
                            0x5000 + 0x20 * (i + 1) if i + 1 < length else 0,
                            0x100 if i + 1 < length else 0)
                yield f"party-{address:04x}-{length}-{target}", ram, cpu
    for sp in (0, 1, 2, 3, 0xD, 0xE, 0xF, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFFFE, 0xFFFF):
        for address in (0xB1, 0xC04B, 0xC04D, 0x4900, 0x6B18, 0x6B40, 0x6DB1, 0x6DB4, 0x9900):
            yield f"stack-{sp:04x}-{address:04x}", *make_case(bodies, address, sp=sp)
    for sp in (0xF, 0x10, 0x11, 0x15, 0x20):
        yield f"A20-stack-{sp}", *make_case(bodies, 0xC04F, sp=sp, ss=0xFFFF)
    yield "A20-globals", *make_case(bodies, 0xC04D, ds=0xFFFF)
    for address in (0xC04B, 0xC04F, 0x4900, 0x6B18, 0x6C0D):
        for sp in (0x6AB1, 0x5D98, 0x49D6):
            yield f"DS-SS-alias-{address:04x}-{sp:04x}", *make_case(bodies, address, sp=sp, ds=0x2000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ovr", type=Path, default=ROOT / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies, instructions = source(args.ovr)
    addresses = [0, 0xB1, 0xFB, 0xFC, 0x33D, 0x35F, 0x48FF, 0x4900, 0x49FD,
        0x4CFF, 0x4D00, 0x6AFF, 0x6B00, 0x6B18, 0x6B9B, 0x6BA0, 0x6BA5, 0x6BAC,
        0x6BB8, 0x6BBB, 0x6BBD, 0x6BBF, 0x6BC1, 0x6BC3, 0x6BD8, 0x6BF7, 0x6BF9,
        0x6C00, 0x6C0C, 0x6C0D, 0x6C1B, 0x6DB1, 0x6DB4, 0x6DCF, 0x6E12, 0x6E3E,
        0x6EFF, 0x6F00, 0x96FF, 0x9700, 0x98FF, 0x9900, 0xB6FF, 0xB700,
        0xC04A, 0xC04B, 0xC04C, 0xC04D, 0xC04E, 0xC04F, 0xC050, 0xC059, 0xFFFF]
    if not args.quick:
        addresses = sorted(set(addresses) | set(range(0x6B00, 0x6F00)))
    visited, total, transfers = set(), 0, 0
    (ROOT / "resources/u25-load-word-parity").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=ROOT / "resources/u25-load-word-parity") as folder:
        lib = library(Path(folder))
        for label, ram, cpu in cases(bodies, addresses, args.quick):
            transfers += compare(label, ram, cpu, bodies, instructions, lib, visited) == 2
            total += 1
    print(json.dumps(dict(cases=total, internal_return_transfers=transfers, visited_instructions=len(visited),
        declared_flag_split_arrivals=SPLIT_ARRIVALS[0], declared_flag_split_by_rule=dict(SPLIT_BY_RULE),
        total_instructions=len(instructions), game_ovr_sha256=OVR_SHA,
        unvisited_original_addresses=[f"{offset:04X}" for offset in sorted(set(instructions) - visited)]),
        sort_keys=True))


if __name__ == "__main__":
    main()
