#!/usr/bin/env python3
"""Original-byte/C audit from display suffix copy entry to 00BA:1887."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/recovered"))
sys.path.insert(0, str(ROOT / "tools/overlay"))
import check_resident_0709_064f_first_ega as first  # noqa: E402
import check_resident_00ba_1858_continuation as tail  # noqa: E402
from check_resident_0709_0626_postcopy_control import compare_cpu  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

HELPER_SHA = "4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f"
LITERAL_SHA = "4bd4cdfadef19a0803e203a5c05b37a6a3e79e76d25454fa559743b912409844"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"


def run_to(memory, cpu, target, limit, record_writes=False):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, first.RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for value, reg in zip(cpu, first.REGISTERS):
        emu.reg_write(reg, value)
    result, writes = [], []

    def on_code(uc, address, size, user):
        del size, user
        if address == first.physical(*target):
            result.append((bytes(uc.mem_read(0, first.RAM_SIZE)),
                           first.uc_values(uc)))
            uc.emu_stop()

    def on_write(uc, access, address, size, value, user):
        del uc, access, value, user
        writes.append((address, size))

    emu.hook_add(UC_HOOK_CODE, on_code)
    if record_writes:
        emu.hook_add(UC_HOOK_MEM_WRITE, on_write)
    emu.emu_start(first.physical(cpu[8], cpu[9]), 0, count=limit)
    assert len(result) == 1, (target, "missing original stop")
    return result[0], writes


def call(model, memory, cpu):
    ram = (ctypes.c_uint8 * first.RAM_SIZE).from_buffer_copy(memory)
    state = first.Cpu(*cpu)
    assert model(ram, first.RAM_SIZE, ctypes.byref(state)) == 0
    return bytes(ram), tuple(getattr(state, name) for name, _ in first.Cpu._fields_)


def check_case(stops, body, literal, helper, continuation, copy_return,
               empty_source=False):
    memory = bytearray(stops[5][1])
    initial = stops[5][2]
    assert initial[8:10] == (0x00BA, 0x1858)
    ss, bp = initial[12], initial[6]
    assert memory[first.physical(ss, bp + 6)] != 0
    for seg, off, code in ((0x00BA, 0x1813, literal),
                           (0x00BA, 0x1816, body),
                           (0x0AF8, 0x0B0B, helper)):
        at = first.physical(seg, off)
        memory[at:at + len(code)] = code
    (entry_ram, entry_cpu), _ = run_to(memory, initial, (0x0AF8, 0x0B0B), 100)
    c_ram, c_cpu = call(continuation, memory, initial)
    assert c_ram == entry_ram, "1858 entry RAM"
    compare_cpu("1858 entry", c_cpu, entry_cpu)
    assert entry_cpu[8:10] == (0x0AF8, 0x0B0B)
    if empty_source:
        entry_ram = bytearray(entry_ram)
        entry_ram[first.physical(0x00BA, 0x1813)] = 0
    source_len = entry_ram[first.physical(0x00BA, 0x1813)]
    dest = (entry_cpu[6] - 4) & 0xFFFF
    expected_writes = [(first.physical(ss, dest + n), 1)
                       for n in range(source_len + 1)]
    (expected_ram, expected_cpu), writes = run_to(
        entry_ram, entry_cpu, (0x00BA, 0x1887), 30, record_writes=True)
    actual_ram, actual_cpu = call(copy_return, entry_ram, entry_cpu)
    assert actual_ram == expected_ram, (empty_source, "copy RAM")
    compare_cpu("1887 copy return", actual_cpu, expected_cpu)
    assert writes == expected_writes, (empty_source, writes, expected_writes)
    assert expected_ram[first.physical(ss, dest):
                        first.physical(ss, dest) + source_len + 1] == \
        entry_ram[first.physical(0x00BA, 0x1813):
                  first.physical(0x00BA, 0x1813) + source_len + 1]
    assert actual_cpu[8:10] == (0x00BA, 0x1887)
    assert actual_cpu[7] == (entry_cpu[7] + 8) & 0xFFFF


def main():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(raw) == 65231 and sha256(raw).hexdigest() == EXE_SHA
    image, *_ = unpack(raw)
    body = tail.full_body()
    literal = image[0x23B3:0x23B6]
    helper = raw[0xB781:0xB79B]
    assert len(literal) == 3 and sha256(literal).hexdigest() == LITERAL_SHA
    assert len(helper) == 26 and sha256(helper).hexdigest() == HELPER_SHA
    assert sha256((ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
                  ).hexdigest() == first.OVR_SHA
    overlays = first.source()
    prefix = first.resident_prefix()
    printer, copy = first.original_bodies()
    drawer, upcase = first.original_extra()
    with tempfile.TemporaryDirectory(prefix="poolrad-1887-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "display_copy.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-shared", "-fPIC", "-I", str(ROOT / "tools/overlay"),
            "-I", str(ROOT / "include"),
            str(ROOT / "tools/replay/resident_0709_064f_first_ega.c"),
            str(ROOT / "tools/replay/resident_00ba_1858_continuation.c"),
            str(ROOT / "tools/replay/resident_00ba_1887_copy_return.c"),
            str(ROOT / "src/replay/slot_a_saved_copy_1965.c"),
            "-o", str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        glyph_entry = lib.pr_064f_first_glyph_entry
        glyph_entry.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                ctypes.POINTER(first.Cpu)]
        glyph_entry.restype = ctypes.c_int
        ega_change = lib.pr_064f_first_ega_change
        ega_change.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                               ctypes.POINTER(first.Cpu),
                               ctypes.POINTER(first.EgaReport)]
        ega_change.restype = ctypes.c_int
        continuation = lib.pr_1858_display_continuation
        copy_return = lib.pr_1887_display_copy_return
        for model in (continuation, copy_return):
            model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                              ctypes.POINTER(first.Cpu)]
            model.restype = ctypes.c_int
        baseline = empty = 0
        for site, (code, _) in zip(first.SITES, overlays):
            if site[-1] == 0:
                continue
            for color in range(3):
                for glyph in (0x54, 0x74):
                    _, _, stops = first.run_case(
                        site, code, glyph, color, prefix, printer, copy,
                        drawer, upcase, glyph_entry, ega_change,
                        trace_return=True)
                    check_case(stops, body, literal, helper, continuation,
                               copy_return)
                    baseline += 1
                    if glyph == 0x54:
                        check_case(stops, body, literal, helper, continuation,
                                   copy_return, empty_source=True)
                        empty += 1
    assert (baseline, empty) == (12, 6)
    print("RES_00BA:1887 SYSTEM copy return: 12/12 original literal, "
          "6/6 empty-source variants; full original-byte/C 1MiB RAM "
          "and 14-field CPU parity")


if __name__ == "__main__":
    main()
