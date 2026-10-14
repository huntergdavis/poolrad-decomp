#!/usr/bin/env python3
"""Hash-locked original/C store-word differential; stdout is counts/hashes only.

Requires local Capstone 5 and Unicorn 2. Owner media, captures, and builds
remain under ignored resources/. Untranslated external far calls stop before execution with the exact
argument stack. No full-entry strict-C claim or invented callback result.
"""

import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import select
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_PROT_ALL
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
SIZE = 1 << 20
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RESIDENT_KEY = 0x1896A
RESIDENT_SITES = set()
SPANS = (
    (0x06FB, 0x0756, "e41b64e0284b3dff9398b1d8fe08f2c8c773f8521975021c8d10734118d39347"),
    (0x0A74, 0x0C77, "258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938"),
    (0x0C77, 0x0E0C, "3a7c7ed533244cf2eef7ab02bc9ac923f335b52099cf3e6f452fbe38f4e366c5"),
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
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    require(len(exe) == 65231 and sha256(exe).hexdigest() == START_SHA, "owner executable hash mismatch")
    resident = exe[0x89A1:0x89B4]
    require(sha256(resident).hexdigest() == "9cb0afed6f73df887b61f47778db5d55bd9063b59c8f6067ab0c38ec3350d205",
            "resident selector body hash mismatch")
    bodies[RESIDENT_KEY] = resident
    for i in md.disasm(resident, 0x094A):
        require(i.address not in instructions, "resident and overlay instruction keys collide")
        instructions[i.address] = (i.mnemonic, i.size)
        RESIDENT_SITES.add(i.address)
    require(len(RESIDENT_SITES) == 8 and len(instructions) == 340, "original instruction set differs")
    return bodies, instructions


def library(directory):
    path = directory / "store-word.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-shared", "-fPIC", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u25_store_word_0c77.c"),
        "-o", str(path)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u25_store_word_0c77.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u25_store_word_0c77.restype = ctypes.c_int
    return lib


def make_case(bodies, address, value=0x137, *, sp=0x8000, ds=0x1000, ss=0x2000):
    ram = bytearray(bytes(range(256)) * (SIZE // 256))
    cpu = Cpu(0x9234, 0x1357, 0x2468, 0xABCD, 0x5678, 0x6789,
              0x4321, sp, 0x3000, 0x0C77, ds, 0x6000, ss, 0x0AD7)
    for offset, seg in ((0x49D2, 0x4000), (0x49D6, 0x4200), (0x49DA, 0x4400), (0x49DE, 0x4600)):
        pointer(ram, ds, offset, seg, 0x0100)
    pointer(ram, ds, 0x5D92, 0x5000, 0x0100)
    pointer(ram, ds, 0x5D96, 0, 0)
    # Seed unused frame bytes so unexpected stores remain observable.
    put16(ram, ss, (sp - 4) & 0xFFFF, 0xD35A)
    put16(ram, ss, (sp - 22) & 0xFFFF, 0xBEEF)
    put16(ram, ss, sp, 0x1111)
    put16(ram, ss, (sp + 2) & 0xFFFF, 0x7000)
    put16(ram, ss, (sp + 4) & 0xFFFF, value)
    put16(ram, ss, (sp + 6) & 0xFFFF, address)
    for start, body in bodies.items():
        at = physical(0x0802, 0x094A) if start == RESIDENT_KEY else physical(cpu.cs, start)
        ram[at:at + len(body)] = body
    return ram, cpu


def compare(label, ram, initial, bodies, instructions, lib, visited):
    for start, body in bodies.items():
        at = physical(0x0802, 0x094A) if start == RESIDENT_KEY else physical(initial.cs, start)
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
        require((cs == initial.cs and ip in instructions and ip not in RESIDENT_SITES) or
                (cs == 0x0802 and ip in RESIDENT_SITES), "unexpected original execution before return")
        visited.add(ip)
        count[0] += 1
        require(count[0] <= 1000000, "original exceeded bounded audit execution")
        mnemonic, size = instructions[ip]
        if mnemonic == "call" or mnemonic == "lcall" and ip == 0x0C03:
            calls.append((cs, (ip + size) & 0xFFFF))
        if mnemonic == "lcall" and ip != 0x0C03:
            stopped.append("dependency")
            emulator.emu_stop()
        if mnemonic == "retf":
            stopped.append("return")
            emulator.emu_stop()

    uc.hook_add(UC_HOOK_CODE, hook)
    expected_status = 0
    while True:
        stopped.clear()
        start = (uc.reg_read(UC_X86_REG_CS) << 4) + uc.reg_read(UC_X86_REG_IP)
        uc.emu_start(start, 2 * SIZE, count=1000001)
        require(stopped, "original failed to reach a boundary")
        if stopped[0] == "dependency":
            expected_status = 3
            break
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
    status = lib.pr_u25_store_word_0c77(candidate_ram, SIZE, ctypes.byref(candidate))
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


ADDRESSES = [0, 0xB1, 0xFB, 0xFC, 0xB8, 0xB9, 0x3DE, 0x3DF,
    0x48FF, 0x4900, 0x49FC, 0x49FD, 0x49FE, 0x49FF, 0x4CFF, 0x4D00,
    0x6AFF, 0x6B00, 0x6B1F, 0x6B20, 0x6B70, 0x6B71, 0x6BB8, 0x6BBB,
    0x6BBD, 0x6BBF, 0x6BC1, 0x6BC3, 0x6BF7, 0x6BF9, 0x6C00, 0x6C0C,
    0x6E12, 0x6E22, 0x6E24, 0x6E26, 0x6E79, 0x6E82, 0x6EFF, 0x6F00,
    0x96FF, 0x9700, 0x98FF, 0x9900, 0xB6FF, 0xB700, 0xBF67, 0xBF68,
    0xC04A, 0xC04B, 0xC04C, 0xC04D, 0xC04E, 0xC04F, 0xC059, 0xC05F, 0xD022, 0xFFFF]


def cases(bodies, quick):
    addresses = ADDRESSES if quick else sorted(set(ADDRESSES) | set(range(0x6B00, 0x6F00)))
    for address in addresses:
        yield f"address-{address:04x}", *make_case(bodies, address)
    for address in (0x6B00, 0x6BB8, 0x6C00, 0x6C0C, 0x6E12, 0x6E22, 0x6E24, 0x6E26, 0xC04D):
        for value in (0, 1, 2, 3, 0x7F, 0x80, 0x81, 0x87, 0xB2, 0xB3, 0xFFFF):
            yield f"boundary-{address:04x}-{value}", *make_case(bodies, address, value)
    if quick:
        return
    for address in (0xC04B, 0xC04C, 0xC04D, 0xC059, 0xC05F,
                    0x6B00, 0x6B20, 0x6B70, 0x6BB8, 0x6BF9, 0x6C00, 0x6C0C):
        for value in range(256):
            yield f"byte-{address:04x}-{value}", *make_case(bodies, address, value)
    for address in (0x3DE, 0xB8, 0xB9, 0x6BBB, 0x6BBD, 0x6BBF, 0x6BC1, 0x6BC3, 0x6BF7):
        for value in (0, 1, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFF00, 0xFFFF):
            yield f"word-{address:04x}-{value}", *make_case(bodies, address, value)
    for sp in (0, 1, 2, 3, 9, 10, 11, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFFFE, 0xFFFF):
        for address in (0xB8, 0xC04B, 0xC04C, 0xC04D, 0x4900, 0x6B20, 0x6C0C, 0x6E12, 0x9900):
            yield f"stack-{sp:04x}-{address:04x}", *make_case(bodies, address, 7, sp=sp)
    for sp in (0xF, 0x10, 0x11, 0x15, 0x20):
        yield f"A20-stack-{sp}", *make_case(bodies, 0xC04B, 7, sp=sp, ss=0xFFFF)
    yield "A20-globals", *make_case(bodies, 0xC059, 7, ds=0xFFFF)
    for address in (0xC04B, 0xC04C, 0x4900, 0x6B20, 0x6C0C):
        for sp in (0x84E2, 0x5D98, 0x49D6):
            yield f"DS-SS-alias-{address:04x}-{sp:04x}", *make_case(bodies, address, 7, sp=sp, ds=0x2000)
    # The window store must change the active pointer BEFORE the record LES.
    ram, cpu = make_case(bodies, 0x6B20, 7)
    pointer(ram, cpu.ds, 0x49D6, cpu.ds, 0x5D52)
    yield "window-aliases-active-record-pointer", ram, cpu
    # The first record store changes the pointer reloaded for the second byte.
    ram, cpu = make_case(bodies, 0x6C0C, 0x81)
    pointer(ram, cpu.ds, 0x5D92, cpu.ds, 0x5C84)
    yield "record-pair-reloads-aliased-pointer", ram, cpu
    # The prior-selector store aliases the resident's pushed argument byte.
    yield "selector-argument-alias", *make_case(bodies, 0x6E12, 0xFF42, sp=0x5389, ds=0x2000)


def check_nontermination(bodies):
    ram, cpu = make_case(bodies, 0xC04D, 0, sp=0x6AB3, ds=0x2000)
    memory = (ctypes.c_uint8 * SIZE).from_buffer(ram)
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    for base in (0, SIZE):
        uc.mem_map_ptr(base, SIZE, UC_PROT_ALL, ctypes.addressof(memory))
    for (name, _), reg in zip(Cpu._fields_, REGS):
        uc.reg_write(reg, getattr(cpu, name))
    uc.emu_start((cpu.cs << 4) + cpu.ip, 2 * SIZE, count=10000)
    require(uc.reg_read(UC_X86_REG_CS) == cpu.cs and
            uc.reg_read(UC_X86_REG_IP) in {0xD91, 0xD95, 0xD98, 0xD9B, 0xD9D, 0xDA2, 0xDD0, 0xDD4},
            "original did not remain in the expected alias loop")
    process = subprocess.Popen([sys.executable, str(Path(__file__).resolve()), "--nontermination-child"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        ready, _, _ = select.select([process.stdout], [], [], 10)
        require(ready and process.stdout.readline().strip() == "ready", "C nontermination probe failed to start")
        try:
            process.wait(timeout=1)
        except subprocess.TimeoutExpired:
            return dict(original_loop_instructions=10000, c_no_return_seconds=1)
        raise ValueError("C incorrectly returned from the alias loop")
    finally:
        if process.poll() is None:
            process.kill()
        process.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--nontermination-check", action="store_true", help="bounded original/C alias-loop probe only")
    parser.add_argument("--nontermination-child", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    bodies, instructions = source(ROOT / "resources/DOS/POOLRAD/GAME.OVR")
    if args.nontermination_child:
        directory = ROOT / "resources/u25-store-word-parity"
        directory.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=directory) as folder:
            lib = library(Path(folder))
            ram, cpu = make_case(bodies, 0xC04D, 0, sp=0x6AB3, ds=0x2000)
            memory = (ctypes.c_uint8 * SIZE).from_buffer(ram)
            print("ready", flush=True)
            lib.pr_u25_store_word_0c77(memory, SIZE, ctypes.byref(cpu))
        return
    if args.nontermination_check:
        print(json.dumps(check_nontermination(bodies), sort_keys=True))
        return
    visited, counts = set(), {0: 0, 2: 0, 3: 0}
    directory = ROOT / "resources/u25-store-word-parity"
    directory.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=directory) as folder:
        lib = library(Path(folder))
        for label, ram, cpu in cases(bodies, args.quick):
            counts[compare(label, ram, cpu, bodies, instructions, lib, visited)] += 1
    print(json.dumps(dict(cases=sum(counts.values()), completed_cases=counts[0],
        internal_return_transfers=counts[2], dependency_stops=counts[3],
        visited_instruction_sites=len(visited), total_instruction_sites=len(instructions),
        unvisited_original_addresses=[f"{offset:04X}" for offset in sorted(set(instructions) - visited)],
        game_ovr_sha256=OVR_SHA, start_exe_sha256=START_SHA), sort_keys=True))


if __name__ == "__main__":
    main()
