#!/usr/bin/env python3
"""Original-byte control parity for GAME.OVR u77:+299C.

All external calls are deliberately stubbed on both sides. This verifies
branch and call order, the selected-byte store, and AL return only.
"""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_CS, UC_X86_REG_IP,
                              UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS,
                              UC_X86_REG_SP)

ROOT = Path(__file__).resolve().parents[2]
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "04b04f1a72210df4e943e51139826ea6a1ab487b254dd717b7c20e9f4c361b99"
RAM_SIZE = 1 << 20
CALLS = {
    0x29B8: 0, 0x29D2: 1, 0x29EC: 2, 0x2A05: 3,
    0x2A1E: 4, 0x2A37: 5, 0x2A50: 6, 0x2A5F: 7,
    0x2A9B: 8, 0x2AA0: 9, 0x2AB6: 10, 0x2AD9: 11,
    0x2AE3: 12, 0x2AE8: 13, 0x2AF6: 14,
}


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    body = data[145878:146247]
    assert len(body) == 369 and sha256(body).hexdigest() == SPAN_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x299C))
    assert len(ins) == 164 and ins[-1].address == 0x2B0A
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "0xc"
    assert ins[-1].address + ins[-1].size == 0x2B0D
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    starts = {x.address for x in ins}
    assert all(int(x.op_str, 16) in starts for x in ins
               if x.mnemonic.startswith("j"))
    return body


def write16(memory, address, value):
    memory[address] = value & 255
    memory[address + 1] = value >> 8 & 255


def run_original(body, mode, selected, state, final):
    cs, ds, ss = 0xF000, 0x1000, 0xE000
    output_seg, output_off = 0x3000, 0x0100
    state_off = 0x0200
    mem = bytearray(RAM_SIZE)
    code_addr = cs * 16 + 0x299C
    mem[code_addr:code_addr + len(body)] = body
    write16(mem, output_seg * 16 + state_off, state & 0xFFFF)
    write16(mem, ds * 16 + 0x5D92, 0x0100)
    write16(mem, ds * 16 + 0x5D94, 0x4000)
    mem[0x4000 * 16 + 0x0100] = 3
    sp = 0x8000
    args = (0x0712, cs, output_off, output_seg, state_off, output_seg,
            0x0059, mode)
    for i, value in enumerate(args):
        write16(mem, ss * 16 + sp + 2 * i, value)
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, RAM_SIZE)
    uc.mem_write(0, bytes(mem))
    for reg, value in ((UC_X86_REG_CS, cs), (UC_X86_REG_IP, 0x299C),
                       (UC_X86_REG_DS, ds), (UC_X86_REG_ES, 0x1111),
                       (UC_X86_REG_SS, ss), (UC_X86_REG_SP, sp),
                       (UC_X86_REG_AX, 0)):
        uc.reg_write(reg, value)
    calls = []
    done = [False]

    def on_code(emu, address, size, user):
        del size, user
        off = address - cs * 16
        if off in CALLS:
            event = CALLS[off]
            calls.append(event)
            if event in (7, 14):
                result = selected if event == 7 else final
                ax = emu.reg_read(UC_X86_REG_AX)
                emu.reg_write(UC_X86_REG_AX, (ax & 0xFF00) | result)
            emu.reg_write(UC_X86_REG_IP, off + 5)
        elif not code_addr <= address < code_addr + len(body):
            done[0] = True
            emu.emu_stop()

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.emu_start(code_addr, RAM_SIZE, count=1000)
    assert done[0], (mode, selected, state, calls)
    slot = uc.mem_read(output_seg * 16 + output_off, 1)[0]
    return slot, uc.reg_read(UC_X86_REG_AX) & 255, calls


def main():
    body = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u77-299c-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u77_spell_299c.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u77_spell_299c
        callback_type = ctypes.CFUNCTYPE(ctypes.c_uint8, ctypes.c_void_p,
                                         ctypes.c_int)
        model.argtypes = [ctypes.c_uint8, ctypes.POINTER(ctypes.c_uint8),
                          ctypes.POINTER(ctypes.c_int16), callback_type,
                          ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8)]
        model.restype = ctypes.c_int
        count = 0
        for mode in range(9):
            for selected in (0, 1, 255):
                for state in (-32768, -1, 0, 32767):
                    final = (mode * 19 + selected * 3 + state) & 255
                    expected = run_original(body, mode, selected, state, final)
                    calls = []

                    @callback_type
                    def external(context, event):
                        del context
                        calls.append(event)
                        return selected if event == 7 else final if event == 14 else 0

                    slot = ctypes.c_uint8(0xA5)
                    result = ctypes.c_uint8(0xDE)
                    state_word = ctypes.c_int16(state)
                    assert model(mode, ctypes.byref(slot),
                                 ctypes.byref(state_word), external, None,
                                 ctypes.byref(result)) == 0
                    observed = (slot.value, result.value, calls)
                    assert observed == expected, (mode, selected, state,
                                                  observed, expected)
                    count += 1
    print(f"u77:+299C original-byte control parity: {count} cases")


if __name__ == "__main__":
    main()
