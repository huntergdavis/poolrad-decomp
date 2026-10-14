#!/usr/bin/env python3
"""Value parity for GAME.OVR u77:+28BE using its original instructions.

The two Pascal runtime arithmetic calls are supplied explicitly. This is an
original-byte code-path check, not a native reached-frame claim.
"""

import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
                              UC_X86_REG_DX, UC_X86_REG_CS, UC_X86_REG_IP,
                              UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS,
                              UC_X86_REG_SP)

ROOT = Path(__file__).resolve().parents[2]
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "aa2a58ca02e77cb9ceebd2247f3291fcb7e531832a5856af9b5322ec06218934"
CODE_SHA = "39dbab268eac0658980e3ef072352603eefac1cfb2b532412376cf2e970081c4"
RAM_SIZE = 1 << 20
WEIGHTS = (1, 10, 100, 200, 1000)


def check_source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    span = data[145656:145878]
    assert len(span) == 222 and sha256(span).hexdigest() == SPAN_SHA
    code = span[:123]
    assert sha256(code).hexdigest() == CODE_SHA
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, 0x28BE))
    assert instructions[-1].address == 0x2936
    assert instructions[-1].mnemonic == "retf"
    assert instructions[-1].op_str == "4"
    assert instructions[-1].address + instructions[-1].size == 0x2939
    assert [i.address for i in instructions if i.mnemonic == "lcall"] == [
        0x28FD, 0x2922]
    starts = {i.address for i in instructions}
    for i in instructions:
        if i.mnemonic.startswith("j"):
            assert int(i.op_str, 16) in starts
    assert span[123] == 9  # length prefix of the first adjacent text item
    return code


def write16(memory, address, value):
    memory[address] = value & 255
    memory[address + 1] = (value >> 8) & 255


def run_original(code, values, weights):
    cs, ds, es, ss = 0xF000, 0x1000, 0x3000, 0xE000
    memory = bytearray(RAM_SIZE)
    code_addr = cs * 16 + 0x28BE
    memory[code_addr:code_addr + len(code)] = code
    for i, value in enumerate(values):
        write16(memory, es * 16 + 0x0100 + 0x88 + 2 * i, value)
    for i, value in enumerate(weights):
        p = ds * 16 + 0x0D24 + 4 * i
        write16(memory, p, value)
        write16(memory, p + 2, value >> 16)
    sp = 0x8000
    for offset, value in ((0, 0x0712), (2, cs), (4, 0x0100), (6, es)):
        write16(memory, ss * 16 + sp + offset, value)
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for reg, value in ((UC_X86_REG_CS, cs), (UC_X86_REG_IP, 0x28BE),
                       (UC_X86_REG_DS, ds), (UC_X86_REG_ES, 0x1111),
                       (UC_X86_REG_SS, ss), (UC_X86_REG_SP, sp)):
        emu.reg_write(reg, value)
    calls = []
    done = [False]

    def on_code(uc, address, size, user):
        del size, user
        off = address - cs * 16
        if off in (0x28FD, 0x2922):
            ax = uc.reg_read(UC_X86_REG_AX)
            dx = uc.reg_read(UC_X86_REG_DX)
            cx = uc.reg_read(UC_X86_REG_CX)
            bx = uc.reg_read(UC_X86_REG_BX)
            left = dx << 16 | ax
            right = bx << 16 | cx
            if off == 0x28FD:
                result = (left * right) & 0xFFFFFFFF
            else:
                assert right != 0
                result = left // right
            uc.reg_write(UC_X86_REG_AX, result & 0xFFFF)
            uc.reg_write(UC_X86_REG_DX, result >> 16)
            uc.reg_write(UC_X86_REG_IP, off + 5)
            calls.append(off)
        elif not code_addr <= address < code_addr + len(code):
            done[0] = True
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(code_addr, RAM_SIZE, count=1000)
    assert done[0] and calls == [0x28FD] * 5 + [0x2922], (done, calls)
    return emu.reg_read(UC_X86_REG_DX) << 16 | emu.reg_read(UC_X86_REG_AX)


def main():
    code = check_source()
    rng = random.Random(0x7728BE)
    cases = [(0,) * 5, (1,) * 5, (65535,) * 5,
             (0, 0, 0, 0, 1), (65535, 0, 0, 0, 0),
             (0, 65535, 0, 0, 0), (0, 0, 65535, 0, 0),
             (0, 0, 0, 65535, 0), (0, 0, 0, 0, 65535)]
    cases += [tuple(rng.randrange(65536) for _ in range(5))
              for _ in range(31)]
    with tempfile.TemporaryDirectory(prefix="poolrad-u77-28be-") as tmp:
        path = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u77_weighted_28be.c"),
                        "-o", str(path)], check=True)
        model = ctypes.CDLL(str(path)).pr_u77_weighted_28be
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(ctypes.c_uint32),
                          ctypes.POINTER(ctypes.c_uint32)]
        model.restype = ctypes.c_int
        for values in cases:
            record = (ctypes.c_uint8 * 0x120)()
            for i, value in enumerate(values):
                record[0x88 + 2 * i] = value & 255
                record[0x89 + 2 * i] = value >> 8
            weights = (ctypes.c_uint32 * 5)(*WEIGHTS)
            result = ctypes.c_uint32(0xDEADBEEF)
            assert model(record, len(record), weights, ctypes.byref(result)) == 0
            original = run_original(code, values, WEIGHTS)
            assert result.value == original, (values, result.value, original)
    print(f"u77:+28BE original-byte value parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
