#!/usr/bin/env python3
"""U38:16E6 bounded CPU14/RAM audit; neither callee executes here."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE

# Reuse only CPU layout, real-mode addressing helpers, and media identity pins.
import check_overlay_u38_target_select_101d as h

CHECKER_ROWS = ("OVL_0038_16E6",)
ROOT = Path(__file__).resolve().parents[2]
SOURCES = ["src/recovered/overlay_u38_roll_dispatch_16e6.c"]
BODY_SHA = "26319a64c3724af34e74edb22250387fe28a05e17d3be802e26ef2db1f2bbcee"
BASE = "dadfec5806de11c14e41f28cd9b4ad69a6090c59"
PATTERN = bytes(range(256)) * (h.RAM_SIZE // 256)
IPS = (0x16E6, 0x16FA, 0x1707)


def extent(body):
    assert len(body) == 37, "extent_length"
    assert sha256(body).hexdigest() == BODY_SHA, "extent_digest"
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x16E6))
    assert len(ins) == 18 and sum(i.size for i in ins) == 37, "extent_decode"
    assert [(i.address, i.mnemonic, i.op_str) for i in ins
            if i.mnemonic in ("call", "lcall")] == [
                (0x16F5, "lcall", "0xb0, 0x48"),
                (0x1704, "call", "0x15eb")], "extent_calls"
    assert (ins[-1].address, ins[-1].mnemonic, ins[-1].op_str) == (
        0x1708, "retf", "0xa"), "extent_return"


def source():
    import sys
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from overlay_inventory import find_units, parse_mz
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = h.OVR.read_bytes()
    assert sha256(exe).hexdigest() == h.EXE_SHA, "exe_digest"
    assert sha256(ovr).hexdigest() == h.OVR_SHA, "ovr_digest"
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x38)
    assert unit["entries"][57] == 0x16E6, "entry_identity"
    assert min(f["rel"] for f in unit["functions"]
               if f["rel"] > 0x16E6) == 0x170B, "next_boundary"
    assert [x for x in unit["fixups"] if 0x16E6 <= x < 0x170B] == [0x16F8]
    start = unit["ovr_offset"] + 0x16E6
    assert start == 66369
    body = ovr[start:start + 37]
    extent(body)
    next_ins = next(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
        ovr[start + 37:start + 53], 0x170B))
    attacks = {"one_byte_short": body[:-1],
               "one_instruction_long": body + bytes(next_ins.bytes),
               "changed_call": body[:16] + bytes([body[16] ^ 1]) + body[17:]}
    rejected = {}
    for name, candidate in attacks.items():
        try:
            extent(candidate)
        except AssertionError as exc:
            rejected[name] = str(exc)
        else:
            raise AssertionError(f"extent_attack_survived:{name}")
    return body, rejected


def compile_model(directory, text, name):
    src, lib = directory / (name + ".c"), directory / (name + ".so")
    src.write_text(text)
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(src), "-o", str(lib)], check=True)
    model = ctypes.CDLL(str(lib)).pr_u38_roll_dispatch_16e6
    model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                      ctypes.POINTER(h.Cpu), ctypes.c_uint16, ctypes.c_int]
    model.restype = ctypes.c_int
    return model


def fixture(body, phase, ax, sp, cs, roll_cs, flags):
    memory = bytearray(PATTERN)
    pos = h.physical(cs, 0x16E6)
    memory[pos:pos + len(body)] = body
    h.put16(memory, cs, 0x16F8, roll_cs)
    ss = 0x3000
    cpu = h.Cpu(ax, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                0x7777, sp, cs, IPS[phase], 0x3000, 0x1111, ss, flags)
    if phase == 2:
        for i, word in enumerate((0xABCD, 0x1234, 0xD000)):
            h.put16(memory, ss, sp + 2 * i, word)
        stop = h.physical(0xD000, 0x1234)
    elif phase == 1:
        stop = h.physical(cs, 0x15EB)
    else:
        for i, word in enumerate((0x1234, 0xD000, 0xA101, 0xA202,
                                  0xA303, 0xA404, 0xA505)):
            h.put16(memory, ss, sp + 2 * i, word)
        stop = h.physical(roll_cs, 0x0048)
    memory[stop] = 0x90  # synthetic inert landing byte; hook stops before execution
    return memory, cpu, stop


def compare(body, model, case):
    phase, ax, sp, cs, roll_cs, flags = case
    memory, cpu, stop = fixture(body, *case)
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, h.RAM_SIZE)
    emu.mem_write(0, bytes(memory))
    for (name, _), reg in zip(h.Cpu._fields_, h.REGISTERS):
        emu.reg_write(reg, getattr(cpu, name))
    reached = []

    def on_code(machine, address, size, data):
        del size, data
        if address == stop:
            reached.append(address)
            machine.emu_stop()
        else:
            assert h.physical(cs, IPS[phase]) <= address < h.physical(cs, 0x170B), "original_left_phase"

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(h.physical(cs, IPS[phase]), h.RAM_SIZE, count=30)
    assert reached == [stop], "original_boundary_not_reached"
    ram = (ctypes.c_uint8 * h.RAM_SIZE).from_buffer_copy(memory)
    status = model(ram, h.RAM_SIZE, ctypes.byref(cpu), cs, phase)
    assert status == (2, 3, 0)[phase], f"phase_{phase}_stop_status"
    expected_cpu = tuple(emu.reg_read(reg) & 0xFFFF for reg in h.REGISTERS)
    for (name, _), actual, expected in zip(h.Cpu._fields_, h.snap(cpu), expected_cpu):
        assert actual == expected, f"phase_{phase}_cpu_{name}:{actual:04x}!={expected:04x}"
    assert bytes(ram) == bytes(emu.mem_read(0, h.RAM_SIZE)), f"phase_{phase}_full_ram"
    # No implicit callee execution or resume: the exact stop-state is refused.
    if phase < 2:
        before = (bytes(ram), h.snap(cpu))
        for attempted_phase in range(3):
            assert model(ram, h.RAM_SIZE, ctypes.byref(cpu), cs, attempted_phase) == 1, "callee_resume_refused"
            assert (bytes(ram), h.snap(cpu)) == before, "callee_stop_unchanged"
    return sha256(bytes(ram) + bytes(cpu)).hexdigest()


def invalid_arguments(body, model):
    memory, cpu, _ = fixture(body, 0, 0xA5CD, 0x8000, 0xF000, 0x3456, 0xED7)
    ram = (ctypes.c_uint8 * h.RAM_SIZE).from_buffer_copy(memory)
    before = (bytes(ram), h.snap(cpu))
    cases = [(ram, h.RAM_SIZE - 1, ctypes.byref(cpu), 0xF000, 0),
             (ram, h.RAM_SIZE + 1, ctypes.byref(cpu), 0xF000, 0),
             (None, h.RAM_SIZE, ctypes.byref(cpu), 0xF000, 0),
             (ram, h.RAM_SIZE, None, 0xF000, 0),
             (ram, h.RAM_SIZE, ctypes.byref(cpu), 0xF001, 0),
             (ram, h.RAM_SIZE, ctypes.byref(cpu), 0xF000, 99),
             (ram, h.RAM_SIZE, ctypes.byref(cpu), 0xF000, 1),
             (ram, h.RAM_SIZE, ctypes.byref(cpu), 0xF000, 2)]
    for args in cases:
        assert model(*args) == 1, "argument_refused"
        assert (bytes(ram), h.snap(cpu)) == before, "argument_unchanged"
    return len(cases)


def physical_wrap(body, model):
    # Synthetic address-contract check, NOT an original-byte Unicorn case.
    memory, cpu, _ = fixture(body, 0, 0xA5CD, 0x11, 0xF000, 0x3456, 2)
    cpu.ss, cpu.bp = 0xFFFF, 0xABCD
    h.put16(memory, cpu.ss, 0x1D, 0x1234)
    h.put16(memory, cpu.ss, 0x1B, 0x5678)
    ram = (ctypes.c_uint8 * h.RAM_SIZE).from_buffer_copy(memory)
    assert model(ram, h.RAM_SIZE, ctypes.byref(cpu), 0xF000, 0) == 2
    assert (ram[0xFFFFF], ram[0]) == (0xCD, 0xAB), "physical_word_wrap"
    assert h.get16(ram, cpu.ss, 0xD) == 0x1234, "wrapped_argument_e"
    assert h.get16(ram, cpu.ss, 0xB) == 0x5678, "wrapped_argument_c"
    assert cpu.sp == 3 and cpu.bp == 0xF, "physical_wrap_frame"


def mutations():
    return {
        "argument_order": ("cpu->bp + 0xEu", "cpu->bp + 0xCu"),
        "roll_count": ("| 1u);", "| 2u);"),
        "roll_sides": ("| 9u);", "| 8u);"),
        "roll_return_frame": ("push16(ram, cpu, 0x16FAu);", "push16(ram, cpu, 0x16FBu);"),
        "stack_offset": ("a = physical(cpu->ss, cpu->sp);", "a = physical(cpu->ss, (uint16_t)(cpu->sp + 2u));"),
        "boundary_flags_only": ("cpu->ip = 0x15EBu;", "cpu->flags ^= 0x08D5u; cpu->ip = 0x15EBu;"),
        "stack_high_byte": ("ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);", "ram[(a + 1u) & RAM_MASK] = 0;"),
        "ah_not_cleared": ("cpu->ax & 0x00FFu", "cpu->ax & 0xFFFFu"),
        "roll_bias": ("input + 10u", "input + 11u"),
        "parity_flag": ("? 0u : 0x0004u", "? 0x0004u : 0u"),
        "auxiliary_carry": ("? 0x0010u : 0u", "? 0u : 0x0010u"),
        "preserved_flags": ("cpu->flags & (uint16_t)~ARITH_FLAGS", "cpu->flags & 0u"),
        "dispatch_high_byte": ("(cpu->ax & 0xFF00u) | 0xFEu", "0xFEu"),
        "dispatch_return_frame": ("push16(ram, cpu, 0x1707u);", "push16(ram, cpu, 0x1708u);"),
        "continue_past_15eb": ("return PR_U38_16E6_DISPATCH_STOP;", "/* faulty implicit fallthrough */"),
        "resume_at_callee": ("cpu->ip != expected_ip", "(cpu->ip != expected_ip && cpu->ip != 0x15EBu)"),
        "return_cleanup": ("cpu->sp + 10u", "cpu->sp + 8u"),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mutants", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    body, rejected = source()
    text = (ROOT / SOURCES[0]).read_text()
    flags = (2, 0x602, 0xED7, 0x8D7)
    stacks = (0x8000, 2, 1, 0xFFF8)
    prefix = [(0, ax, sp, cs, roll, flag)
              for ax in (0x00CD, 0xA5CD)
              for sp in stacks for cs in (0xF000, 0xC000)
              for roll in (0x00BA, 0x3456) for flag in flags]
    middle = [(1, high | result, stacks[result % 4],
               (0xF000, 0xC000)[result % 2], 0x3456, flags[result % 4])
              for high in (0, 0xA500) for result in range(256)]
    tail = [(2, ax, sp, cs, 0x3456, flag)
            for ax in (0x00CD, 0xA5CD) for sp in stacks
            for cs in (0xF000, 0xC000) for flag in flags]
    killed, hashes = {}, []
    private = ROOT / "resources/t6-u38-16e6"
    private.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="checker-", dir=private) as tmp:
        directory = Path(tmp)
        model = compile_model(directory, text, "model")
        for case in prefix + middle + tail:
            hashes.append(compare(body, model, case))
        refusals = invalid_arguments(body, model)
        physical_wrap(body, model)
        if args.mutants:
            probes = [(0, 0xA5CD, 0x8000, 0xF000, 0x3456, 0xED7),
                      (1, 0xA506, 0x8000, 0xF000, 0x3456, 0xED7),
                      (1, 0xA5F6, 1, 0xC000, 0x3456, 0x602),
                      (2, 0xA5CD, 2, 0xF000, 0x3456, 0xED7)]
            for name, (old, new) in mutations().items():
                assert text.count(old) == 1, (name, "mutation anchor")
                mutant = compile_model(directory, text.replace(old, new), name)
                try:
                    for case in probes:
                        compare(body, mutant, case)
                    invalid_arguments(body, mutant)
                    physical_wrap(body, mutant)
                except AssertionError as exc:
                    killed[name] = str(exc)
                else:
                    raise AssertionError(f"mutant_survived:{name}")
    report = {
        "row": CHECKER_ROWS[0], "claim": "bounded original-byte wrapper phases only",
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "base": BASE, "body_sha256": BODY_SHA, "bytes": 37, "instructions": 18,
        "sources_sha256": {p: sha256((ROOT / p).read_bytes()).hexdigest() for p in SOURCES},
        "checker_sha256": sha256(Path(__file__).read_bytes()).hexdigest(),
        "cases": {"enter": len(prefix), "after_roll": len(middle), "after_dispatch": len(tail)},
        "comparison": "CPU14 and entire 1MiB RAM; no masks",
        "reached_sites": ["loaded-roll:0048", "U38:15EB", "synthetic-caller:D000:1234"],
        "argument_refusals": refusals, "physical_wrap_synthetic_cases": 1,
        "extent_attacks_rejected": rejected, "compiled_mutants_killed": killed,
        "result_sha256": sha256("\n".join(hashes).encode()).hexdigest(),
        "dosbox_x": {"executed": False, "reason": "Unicorn phase audit, not DOSBox native execution"},
        "not_verified": ["real roll callee", "U38:15EB callee", "native DOS execution",
                         "full composed function", "live dispatcher reachability", "strict credit"],
    }
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
