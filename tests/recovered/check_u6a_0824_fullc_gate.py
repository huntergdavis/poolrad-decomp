#!/usr/bin/env python3
"""u6A:+0824 follow-on gate; private native evidence is optional, never inferred.

Reuses PR391's pinned original-byte runner. Random property checks independently
check low-seven-bit ordering, whole-byte multiplicities, and write boundaries.
"""
import argparse
from collections import Counter
import ctypes
import gc
from hashlib import sha256
import json
from pathlib import Path
import random
import re
import tempfile

import check_overlay_u6a_sort_0824 as base
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from capstone import Cs, CS_ARCH_X86, CS_MODE_16


def compare(label, memory, cpu, source, library):
    """Original execution with a coherent A20-disabled high-memory mirror."""
    start = base.physical(cpu.cs, 0x0824)
    base.check(memory[start:start + len(source)] == source, "loaded body mismatch")
    original_ram = (ctypes.c_uint8 * base.RAM_SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    pointer = ctypes.addressof(original_ram)
    original.mem_map_ptr(0, base.RAM_SIZE, 7, pointer)
    original.mem_map_ptr(base.RAM_SIZE, 0x10000, 7, pointer)
    for (name, _), register in zip(base.Cpu._fields_, base.REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    returned = [False]
    visited, edges, previous = set(), set(), [None]
    def stop(emulator, address, size, user):
        if not start <= address < start + len(source):
            returned[0] = True
            emulator.emu_stop()
        else:
            here = address - start + 0x0824
            visited.add(here)
            if previous[0] is not None:
                edges.add((previous[0], here))
            previous[0] = here
    original.hook_add(UC_HOOK_CODE, stop)
    original.emu_start(start, 0x110000, count=100000)
    base.check(returned[0], f"original failed to return: {label}")
    ram = (ctypes.c_uint8 * base.RAM_SIZE).from_buffer_copy(memory)
    base.check(library.pr_u6a_sort_0824(ram, len(ram), ctypes.byref(cpu)) == 0,
               f"C rejected {label}")
    base.check(bytes(ram) == bytes(original_ram), f"full RAM mismatch: {label}")
    for (name, _), register in zip(base.Cpu._fields_, base.REGISTERS):
        base.check(getattr(cpu, name) == original.reg_read(register) & 0xffff,
                   f"{name} mismatch: {label}")
    return visited, edges


def properties(before, after, entry, returned, values, segment, offset):
    indices = [base.physical(segment, (offset + 0x17 + i) & 0xffff)
               for i in range(21)]
    result = [after[i] for i in indices]
    base.check([v & 127 for v in result] == sorted(v & 127 for v in values),
               "independent key-order oracle failed")
    base.check(Counter(result) == Counter(values), "whole-byte multiset changed")
    # PUSH BP and three local bytes are the only writes outside the array.
    bp = (entry.sp - 2) & 0xffff
    allowed = set(indices)
    a = base.physical(entry.ss, bp)
    allowed.update((a, (a + 1) & (base.RAM_SIZE - 1)))
    allowed.update(base.physical(entry.ss, (bp - n) & 0xffff)
                   for n in (1, 2, 3))
    outside = bytearray(after)
    for i in allowed:
        outside[i] = before[i]
    base.check(outside == before, "write outside array/frame")
    for name in ("bx", "cx", "si", "ds", "ss", "bp"):
        base.check(getattr(entry, name) == getattr(returned, name),
                   f"preserved register changed: {name}")
    base.check(returned.sp == (entry.sp + 4) & 0xffff, "RETF stack mismatch")
    base.check(returned.flags == (entry.flags & ~0x08d5) | 0x44,
               "final CMP flags mismatch")


def stress(source, library):
    rng = random.Random(0x6a0824)
    visited, edges = set(), set()
    for case in range(128):
        values = [rng.randrange(256) for _ in range(21)]
        # Keep code, pointer table, array and stack disjoint. Exercise both
        # offset and physical wrap without accidentally modifying executable RAM.
        segment, offset = ((0xffff, 0x0000) if case % 4 == 0 else
                           (0x4000, 0xfff0) if case % 4 == 1 else
                           (0x4000, rng.randrange(0x100, 0xfe00)))
        memory, cpu = base.make_case(source, values, segment=segment, offset=offset,
                                     sp=(0xffff if case % 3 == 0 else 0x8000))
        for name in ("ax", "bx", "cx", "dx", "si", "di", "bp", "es"):
            setattr(cpu, name, rng.randrange(65536))
        # Legal real-mode user flags, excluding TF/IF (external events).
        cpu.flags = 2 | (rng.randrange(65536) & 0x0cd5)
        entry = base.Cpu.from_buffer_copy(cpu)
        before = bytes(memory)
        starts, transitions = compare(f"stress-{case}", memory, cpu, source, library)
        visited.update(starts)
        edges.update(transitions)
        ram = (ctypes.c_uint8 * base.RAM_SIZE).from_buffer_copy(before)
        returned = base.Cpu.from_buffer_copy(entry)
        base.check(library.pr_u6a_sort_0824(ram, len(ram), ctypes.byref(returned)) == 0,
                   "C stress rejected input")
        properties(before, bytes(ram), entry, returned, values, segment, offset)
        gc.collect()  # Release Unicorn callback cycles between 1 MiB fixtures.
        if (case + 1) % 16 == 0:
            print(f"original/C/property cases passed: {case + 1}/128", flush=True)
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(source, 0x0824))
    base.check(visited == {ins.address for ins in decoded}, "incomplete instruction coverage")
    outcomes = set()
    for ins in decoded:
        if ins.mnemonic.startswith("j") and ins.mnemonic != "jmp":
            outcomes.update(((ins.address, int(ins.op_str, 16)),
                             (ins.address, ins.address + ins.size)))
    # With array/frame/pointer disjoint, i runs 0..19, so JA at +0838
    # cannot take its i>20 edge. Do not claim that alias-only edge was tested.
    feasible = outcomes - {(0x0838, 0x08b2)}
    base.check(feasible <= edges, "feasible conditional branch coverage incomplete")
    print(f"coverage: {len(visited)} instruction starts, {len(outcomes & edges)}/"
          f"{len(outcomes)} conditional outcomes; +0838 taken edge unmeasured")
    # Invalid-input checks must leave both CPU and every RAM byte unchanged.
    ram = (ctypes.c_uint8 * base.RAM_SIZE).from_buffer_copy(memory)
    before_cpu, before_ram = bytes(cpu), bytes(ram)
    for pointer, size, registers in ((None, len(ram), ctypes.byref(cpu)),
                                     (ram, len(ram) - 1, ctypes.byref(cpu)),
                                     (ram, len(ram), None)):
        base.check(library.pr_u6a_sort_0824(pointer, size, registers) == 1,
                   "invalid input accepted")
        base.check(bytes(cpu) == before_cpu and bytes(ram) == before_ram,
                   "invalid input mutated state")


def native(directory, source, library):
    """Require exact full-RAM leaf entry/return capture, with pinned provenance.

    A manifest is evidence metadata, not proof of execution. Its native-origin
    claim and debugger log must be independently reviewed before any credit.
    """
    manifest = json.loads((directory / "manifest.json").read_text())
    base.check(manifest["origin"] == "dosbox-x-native-breakpoints",
               "capture is not labelled native")
    base.check(manifest["game_ovr_sha256"] == base.OVR_SHA, "media hash mismatch")
    base.check(manifest["body_sha256"] == base.SPAN_SHA, "body hash mismatch")
    log = (directory / "debugger.log").read_bytes()
    base.check(sha256(log).hexdigest() == manifest["debugger_log_sha256"],
               "debugger log hash mismatch")
    memories, cpus = [], []
    fields = {name for name, _ in base.Cpu._fields_}
    for phase in ("entry", "return"):
        data = (directory / f"{phase}.bin").read_bytes()
        base.check(len(data) == base.RAM_SIZE, "native RAM must cover full 1 MiB")
        base.check(sha256(data).hexdigest() == manifest[f"{phase}_sha256"],
                   f"{phase} RAM hash mismatch")
        regs = manifest[f"{phase}_cpu"]
        base.check(set(regs) == fields and all(type(v) is int and 0 <= v <= 65535
                                              for v in regs.values()),
                   f"invalid {phase} CPU")
        memories.append(data)
        cpus.append(base.Cpu(**regs))
    entry, returned = cpus
    # Registers must occur in the hashed debugger log, rather than exist only
    # as a second, independently editable description in the manifest.
    names = "CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS".split()
    snapshots = re.findall(
        rb"EV of 'CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS' is:\s*"
        rb"(?:LOG:\s*)?((?:[0-9A-Fa-f]{1,8}\s+){13}[0-9A-Fa-f]{1,8})", log)
    logged = [{n.lower(): int(v, 16) for n, v in zip(names, line.split())}
              for line in snapshots]
    base.check(len(logged) >= 2 and manifest["entry_cpu"] == logged[-2] and
               manifest["return_cpu"] == logged[-1],
               "native CPU values do not match final debugger EV snapshots")
    base.check(entry.ip == 0x0824, "native capture is not at leaf entry")
    base.check(returned.cs == entry.cs and returned.ip == 0x0a51,
               "native capture is not at aligned caller return")
    def word(seg, off):
        a = base.physical(seg, off)
        return memories[0][a] | memories[0][(a + 1) & (base.RAM_SIZE - 1)] << 8
    base.check(word(entry.ss, entry.sp) == 0x0a51 and
               word(entry.ss, (entry.sp + 2) & 0xffff) == entry.cs,
               "native far-return frame mismatch")
    ovr = base.OVR.read_bytes()
    # +0A4D PUSH CS / +0A4E near CALL, followed by caller return +0A51.
    caller = ovr[0x17831 + 0x0a4d:0x17831 + 0x0a51]
    a = base.physical(entry.cs, 0x0a4d)
    base.check(memories[0][a:a + 4] == caller and caller[0] == 0x0e,
               "native caller bytes do not match original")
    active = (word(entry.ds, 0x5d94), word(entry.ds, 0x5d92))
    node = (word(entry.ds, 0x5d98), word(entry.ds, 0x5d96))
    seen = set()
    while node != (0, 0) and node not in seen and len(seen) < 256:
        seen.add(node)
        node = (word(node[0], (node[1] + 0x106) & 0xffff),
                word(node[0], (node[1] + 0x104) & 0xffff))
    base.check(active in seen, "active pointer is not an observed party-list node")
    def returned_word(off):
        a = base.physical(entry.ds, off)
        return memories[1][a] | memories[1][(a + 1) & (base.RAM_SIZE - 1)] << 8
    base.check(active == (returned_word(0x5d94), returned_word(0x5d92)),
               "native active pointer changed during leaf")
    candidate = base.Cpu.from_buffer_copy(entry)
    compare("native-entry-replay", bytearray(memories[0]), candidate, source, library)
    ram = (ctypes.c_uint8 * base.RAM_SIZE).from_buffer_copy(memories[0])
    candidate = base.Cpu.from_buffer_copy(entry)
    base.check(library.pr_u6a_sort_0824(ram, len(ram), ctypes.byref(candidate)) == 0,
               "native C call rejected")
    base.check(bytes(ram) == memories[1], "native returned RAM differs from C")
    base.check(bytes(candidate) == bytes(returned), "native returned CPU differs from C")
    print("native full-RAM/CPU parity passed; origin and route require independent review")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path, help="private capture directory")
    parser.add_argument("--require-native", action="store_true",
                        help="fail closed when native capture is absent")
    args = parser.parse_args()
    if args.require_native and args.native is None:
        parser.error("strict gate requires --native; synthetic parity earns no credit")
    source = base.load_source()
    with tempfile.TemporaryDirectory(prefix="u6a-0824-fullc-") as tmp:
        library = base.build_library(Path(tmp))
        stress(source, library)
        print("128 deterministic original/C full-RAM/CPU cases and independent properties passed")
        if args.native:
            native(args.native, source, library)
        else:
            print("native gate UNMEASURED: no native capture supplied; no strict credit")


if __name__ == "__main__":
    main()
