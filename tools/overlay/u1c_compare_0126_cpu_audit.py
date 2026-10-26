#!/usr/bin/env python3
"""Original/C evidence for U1C:0126 with raw operand and compare dependencies.

Every caller instruction boundary observes CPU14 and a digest of all 1 MiB;
the final stop compares RAM bytes directly. Original code is read from owner
media, never embedded. This audit claims no identity, credit or readiness.
"""
import argparse
from collections import Counter
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from unicorn import x86_const


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


REGISTERS = [getattr(x86_const, "UC_X86_REG_" + name.upper()) for name, _ in Cpu._fields_[:-1]] + [x86_const.UC_X86_REG_EFLAGS]

ROOT = Path(__file__).resolve().parents[2]
SIZE = 1 << 20
ENTRY, END = 0x0126, 0x01A3
BODY_SHA = "330e74c38244df80bd2af04209126a207b5ebdf5dcf97c4781d4b960e2a52404"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
CALLER, U25, SYSTEM, STUB = 0x8800, 0x7800, 0x0AF8, 0x002B
SITES = {0x0130: (0x002A, 0x0000, 1), 0x0152: (0x0B25, None, 2),
         0x0166: (0x0B25, None, 2), 0x0177: (0x006B, 0x1815, 3),
         0x0181: (0x0025, 0x0136, 4), 0x018C: (0x0025, 0x0136, 4),
         0x019A: (0x0070, 0x18EB, 5)}
CHECKER_ROWS = ("OVL_001C_0126",)
SOURCES = ["overlay_u1c_compare_0126_cpu.c", "overlay_u25_loader_0000.c",
           "overlay_u25_slot_value_0136.c", "overlay_u25_string_cores.c",
           "overlay_u25_entry1_0136.c", "overlay_u25_load_word_0e0c.c",
           "overlay_u25_party_index_0756.c", "overlay_u25_compare_strings_1815_cpu.c",
           "overlay_u25_compare_words_18eb_cpu.c", "system_strstore_fullstate_0b25.c",
           "system_string_move_leaves_cpu.c", "system_runtime_leaves_cpu.c"]


class Limits(ctypes.Structure):
    _fields_ = [("system_cs", ctypes.c_uint16), ("max_operands", ctypes.c_size_t),
                ("max_string_iterations", ctypes.c_size_t), ("max_compare_instructions", ctypes.c_size_t)]


class Diagnostic(ctypes.Structure):
    _fields_ = [("callee", ctypes.c_int), ("status", ctypes.c_int)]


Observer = ctypes.CFUNCTYPE(None, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), ctypes.c_void_p)


def check(value, message):
    if not value:
        raise ValueError(message)


def physical(seg, off):
    return ((seg << 4) + (off & 65535)) & (SIZE - 1)


def word(memory, seg, off, value):
    at = physical(seg, off)
    memory[at] = value & 255
    memory[(at + 1) & (SIZE - 1)] = value >> 8


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA, "START.EXE hash")
    check(sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR hash")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    code = {n: ovr[units[n]["ovr_offset"]:units[n]["ovr_offset"] + units[n]["code_size"]] for n in (0x1C, 0x25)}
    body = code[0x1C][ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(sha256(body).hexdigest() == BODY_SHA and len(body) == 125, "0126 body hash/extent")
    check(len(decoded) == 50 and sum(i.size for i in decoded) == 125, "0126 complete decode")
    check(decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "", "0126 return")
    check({i.address for i in decoded if i.mnemonic == "lcall"} == set(SITES), "0126 call sites")
    for at, (offset, entry, _) in SITES.items():
        check(at + 3 in units[0x1C]["fixups"], "0126 segment fixup")
        check(struct.unpack_from("<HH", code[0x1C], at + 1) == (offset, STUB if entry is not None else SYSTEM), "0126 target")
    check(units[0x25]["entries"][1] == 0x0136 and units[0x25]["entries"][2] == 0 and
          units[0x25]["entries"][15] == 0x1815 and units[0x25]["entries"][16] == 0x18EB, "compare stub entries")
    caller = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[0x1C][0x2F58:0x2F5C], 0x2F58))
    check([(i.address, i.mnemonic, i.op_str) for i in caller] ==
          [(0x2F58, "push", "cs"), (0x2F59, "call", "0x126")], "dispatcher far-frame call")
    return dict(code=code, image=bytes(unpack(exe)[0]), starts={i.address for i in decoded},
                branches={i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic in ("jae", "jb")},
                receipt=dict(bytes=125, instructions=50, sha256=BODY_SHA,
                             file_offset=units[0x1C]["ovr_offset"] + ENTRY, calls=len(SITES),
                             dispatcher_call=0x2F59, dispatcher_return=0x2F5C))


def library(directory, mutant=None):
    output = directory / ((mutant or "candidate") + ".so")
    paths = [str(ROOT / "src/recovered" / s) for s in SOURCES]
    command = ["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include")]
    text = None
    if mutant:
        text = Path(paths[0]).read_text()
        if mutant == "wrong_sub_flags":
            before = "c->sp = subtract(c, c->sp, 0x0204u, 16u);"
            check(text.count(before) == 1, "flag mutant anchor")
            text = text.replace(before, before + " c->flags ^= 0x08D5u;")
        elif mutant == "reversed_words":
            a = "STEP(0x0194u); push(&m, read16(&m, c->ss, (uint16_t)(c->bp - 2u)));"
            b = "STEP(0x0197u); push(&m, read16(&m, c->ss, (uint16_t)(c->bp - 4u)));"
            check(a in text and b in text, "word mutant anchors")
            text = text.replace(a, a.replace("- 2u", "- 4u")).replace(b, b.replace("- 4u", "- 2u"))
        elif mutant == "second_tag_ignored":
            before = "read8(&m, c->ds, 0x7007u)"
            check(text.count(before) == 1, "tag mutant anchor")
            text = text.replace(before, "read8(&m, c->ds, 0x7006u)")
        else:
            raise ValueError(mutant)
        command += ["-x", "c", "-", "-x", "none"] + paths[1:]
    else:
        command += paths
    subprocess.run(command + ["-o", str(output)], input=text, text=True, check=True, capture_output=True)
    dll = ctypes.CDLL(str(output))
    fn = dll.pr_u1c_compare_0126
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Limits),
                   ctypes.POINTER(Diagnostic), Observer, ctypes.c_void_p]
    fn.restype = ctypes.c_int
    return fn


def build(src, spec):
    memory = bytearray([0xA5]) * SIZE
    memory[:len(src["image"])] = src["image"]
    for seg, n in ((CALLER, 0x1C), (U25, 0x25)):
        at = physical(seg, 0)
        memory[at:at + len(src["code"][n])] = src["code"][n]
    for offset, entry in ((0x25, 0x136), (0x2A, 0), (0x6B, 0x1815), (0x70, 0x18EB)):
        at = physical(STUB, offset)
        memory[at:at + 5] = struct.pack("<BHH", 0xEA, entry, U25)
    ds, ss, sp = spec.get("ds", 0x1800), spec.get("ss", 0x2800), spec.get("sp", 0x8000)
    for off, seg in ((0x49D2, 0x6000), (0x49D6, 0x6100), (0x49DA, 0x6200), (0x49DE, 0x6300)):
        word(memory, ds, off, 0x1000); word(memory, ds, off + 2, seg)
    word(memory, ds, 0x49ED, 0x9900)
    for seg in (0x6000, 0x6100, 0x6200):
        at = physical(seg, 0x1000)
        memory[at:at + 1024] = bytes(1024)
        word(memory, seg, 0x1000, 0x51)
    operands = spec.get("operands", ((0, 1, 0), (0, 2, 0)))
    data = []
    for tag, value, third in operands:
        data += [tag, value]
        if tag in (1, 2, 3, 0x81): data.append(third)
    at = physical(0x6300, 0x1001)
    memory[at:at + len(data) + 512] = bytes(data) + bytes(512)
    for slot, data in enumerate(spec.get("strings", (b"alpha", b"beta"))):
        at = physical(ds, 0x70C8 + 256 * slot)
        memory[at:at + len(data) + 1] = bytes([len(data)]) + data
    word(memory, ss, sp, 0x7000); word(memory, ss, sp + 2, 0x9001)
    rng = random.Random(spec.get("seed", 0x126))
    registers = [rng.randrange(65536) for _ in range(7)]
    cpu = Cpu(*registers, sp, CALLER, ENTRY, ds, rng.randrange(65536), ss, spec.get("flags", 0x0ED7))
    for seg, off, value in spec.get("patch", ()):
        memory[physical(seg, off)] = value
    return memory, cpu


def fingerprint(cpu, data):
    return tuple(getattr(cpu, n) for n, _ in Cpu._fields_), sha256(data).hexdigest()


def compare(src, fn, spec, *, observe=True):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    memory, cpu = build(src, spec)
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 2 * SIZE); uc.mem_write(0, bytes(memory)); uc.mem_write(SIZE, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS): uc.reg_write(reg, getattr(cpu, name))
    traces, visited, edges = [], set(), set()
    state = dict(stop=None, last=None, active=None, expected=None, returned=False, operands=0, string_left=0,
                 compare_left=spec.get("compare_limit", 256), diagnostic=None)
    limits = Limits(SYSTEM, spec.get("operand_limit", 32), spec.get("string_limit", 4096), spec.get("compare_limit", 256))
    base = physical(CALLER, 0)

    def stop(status, diagnostic=None):
        state["stop"], state["diagnostic"] = status, diagnostic
        uc.emu_stop()

    def current():
        return Cpu(*(uc.reg_read(r) & 65535 for r in REGISTERS))

    def hook(emulator, address, size, user):
        del emulator, size, user
        c = current()
        if state["returned"]:
            return stop(0 if (c.cs, c.ip) == (0x9001, 0x7000) else 4)
        if c.cs == CALLER and ENTRY <= c.ip < END:
            if state["active"] is not None:
                if c.ip != state["expected"]: return stop(4)
                state["active"] = None
            visited.add(c.ip)
            if state["last"] in src["branches"]: edges.add((state["last"], c.ip))
            state["last"] = c.ip
            if observe: traces.append(fingerprint(c, bytes(uc.mem_read(0, SIZE))))
            if c.ip in SITES:
                off, entry, kind = SITES[c.ip]
                raw = bytes(uc.mem_read(address, 5))
                if raw[:3] != struct.pack("<BH", 0x9A, off): return stop(2)
                if entry is not None:
                    at = physical(struct.unpack_from("<H", raw, 3)[0], off)
                    if bytes(uc.mem_read(at, 3)) != struct.pack("<BH", 0xEA, entry): return stop(2)
                state["active"], state["expected"] = kind, c.ip + 5
            if c.ip == 0x01A2: state["returned"] = True
        else:
            state["last"] = None
            if c.cs not in (U25, SYSTEM, STUB): return stop(4)
            if c.cs == U25:
                if state["active"] == 1 and c.ip == 0x22:
                    if state["operands"] == limits.max_operands: return stop(3, (1, 2))
                    state["operands"] += 1
                if c.ip in (0x1153, 0x12FB): state["string_left"] = limits.max_string_iterations
                if c.ip in (0x1172, 0x1327, 0x13D3, 0x1453, 0x14D2):
                    if state["string_left"] == 0: return stop(3, (1, 3))
                    state["string_left"] -= 1
                if state["active"] == 3 and 0x1815 <= c.ip < 0x18EB:
                    if state["compare_left"] == 0: return stop(3, (3, 2))
                    state["compare_left"] -= 1

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for k in range(size):
            at = address + k
            emulator.mem_write((at & (SIZE - 1)) + (SIZE if at < SIZE else 0), bytes([(value >> (8 * k)) & 255]))

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.hook_add(UC_HOOK_MEM_WRITE, mirror)
    uc.emu_start(base + ENTRY, 2 * SIZE, count=2000000)
    check(state["stop"] is not None, "original instruction safety limit; not a return")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    actual_traces = []

    @Observer
    def observation(data, size, pointer, context):
        del context
        actual_traces.append(fingerprint(pointer.contents, ctypes.string_at(data, size)))

    diag = Diagnostic(0, -1)
    status = fn(ram, SIZE, ctypes.byref(cpu), ctypes.byref(limits), ctypes.byref(diag), observation if observe else Observer(), None)
    expected_cpu = current()
    cpu_diff = {n: [getattr(cpu, n), getattr(expected_cpu, n)] for n, _ in Cpu._fields_ if getattr(cpu, n) != getattr(expected_cpu, n)}
    trace_difference = next((i for i, (a, b) in enumerate(zip(actual_traces, traces)) if a != b), None)
    trace_equal = len(actual_traces) == len(traces) and trace_difference is None
    result = dict(equal=(status == state["stop"] and not cpu_diff and bytes(ram) == bytes(uc.mem_read(0, SIZE)) and trace_equal),
                  status=status, original_status=state["stop"], diagnostic=[diag.callee, diag.status],
                  cpu=cpu_diff, ram_equal=bytes(ram) == bytes(uc.mem_read(0, SIZE)), trace_equal=trace_equal,
                  trace_difference=trace_difference, trace_lengths=[len(actual_traces), len(traces)])
    if trace_difference is not None:
        a, b = actual_traces[trace_difference], traces[trace_difference]
        result["first_observation"] = dict(ip=b[0][9], cpu={n: [x, y] for (n, _), x, y in zip(Cpu._fields_, a[0], b[0]) if x != y}, ram_equal=a[1] == b[1])
    if state["diagnostic"] is not None:
        result["equal"] &= (diag.callee, diag.status) == state["diagnostic"]
    return result, visited, edges


def cases(smoke=False):
    yield "numeric", {}
    for tags in ((0x82, 0), (0, 0x82), (0x82, 0x82)):
        yield "string-branch", {"operands": [(t, 0, 0) for t in tags]}
    values = (0, 1, 0x7F, 0x80, 0xFF) if smoke else range(256)
    for side in (0, 1):
        for v in values:
            ops = [(0, 0x80, 0), (0, 0x7F, 0)]; ops[side] = (0, v, 0)
            yield "literal-byte", {"operands": ops}
    tags = (0, 1, 2, 3, 0x7F, 0x80, 0x81, 0x82, 0xFF) if smoke else range(256)
    for side in (0, 1):
        for tag in tags:
            ops = [(0, 1, 0), (0, 2, 0)]; ops[side] = (tag, 0, 0)
            yield "tag-byte", {"operands": ops}
    for a in (0, 1, 0x7FFF, 0x8000, 0xFFFF):
        for b in (0, 1, 0x7FFF, 0x8000, 0xFFFF):
            yield "literal-word", {"operands": [(2, a & 255, a >> 8), (2, b & 255, b >> 8)]}
    lengths = (0, 1, 127, 128, 254, 255) if smoke else range(256)
    for n in lengths:
        yield "string-length", {"operands": [(0x82, 0, 0), (0x82, 0, 0)], "strings": (bytes([0x41]) * n, bytes([0x41]) * n)}
    for sp in (0x0436, 0x0437, 0xFFFA, 0xFFFB):
        for tag in (0, 0x82): yield "stack-edge", {"sp": sp, "operands": [(tag, 1, 0), (tag, 2, 0)]}
    for sp in range(0x6F76, 0x6F80):
        yield "flag-frame-alias", {"ss": 0x1800, "sp": sp}
    for sp in (0x71CC, 0x72CE, 0x72D0, 0x73D0):
        yield "string-frame-alias", {"ss": 0x1800, "sp": sp, "operands": [(0x82, 0, 0), (0x82, 0, 0)]}
    for flags in (2, 0x0ED7, 0x7ED7, 0x0602): yield "flags", {"flags": flags}
    yield "stack-a20", {"ss": 0xFFFF}
    yield "data-a20", {"ds": 0xFFFF}
    for at in (0x0130, 0x0181, 0x018C, 0x019A): yield "call-code", {"patch": [(CALLER, at, 0x90)]}
    for at in (0x0152, 0x0166, 0x0177):
        yield "string-call-code", {"operands": [(0x82, 0, 0), (0x82, 0, 0)], "patch": [(CALLER, at, 0x90)]}
    for at in (0x25, 0x2A, 0x6B, 0x70):
        yield "stub-code", {"operands": [(0x82, 0, 0), (0x82, 0, 0)] if at == 0x6B else [(0, 1, 0), (0, 2, 0)], "patch": [(STUB, at, 0x90)]}
    yield "operand-limit", {"operand_limit": 1}
    for limit in (1, 2, 3, 40, 85):
        yield "compare-limit", {"operands": [(0x82, 0, 0), (0x82, 0, 0)], "compare_limit": limit}


def audit(smoke=False):
    src = source()
    counts, stops, visited, edges = Counter(), Counter(), set(), set()
    with tempfile.TemporaryDirectory(prefix="u1c0126-") as td:
        directory = Path(td)
        fn = library(directory)
        refused = 0
        for change in ("low_sp", "high_sp", "wrong_ip", "operand_zero", "string_zero", "compare_zero", "short_ram"):
            memory, cpu = build(src, {})
            limits = Limits(SYSTEM, 32, 4096, 256)
            size = SIZE
            if change == "low_sp": cpu.sp = 0x0435
            if change == "high_sp": cpu.sp = 0xFFFC
            if change == "wrong_ip": cpu.ip = ENTRY + 1
            if change == "operand_zero": limits.max_operands = 0
            if change == "string_zero": limits.max_string_iterations = 0
            if change == "compare_zero": limits.max_compare_instructions = 0
            if change == "short_ram": size -= 1
            before = bytes(cpu)
            ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
            diag = Diagnostic(55, 66)
            status = fn(ram, size, ctypes.byref(cpu), ctypes.byref(limits), ctypes.byref(diag), Observer(), None)
            check(status == 1 and bytes(cpu) == before and bytes(ram) == memory and
                  (diag.callee, diag.status) == (55, 66), f"non-atomic entry refusal: {change}")
            refused += 1
        for index, (group, spec) in enumerate(cases(smoke)):
            result, starts, branches = compare(src, fn, spec)
            check(result["equal"], f"case {index} {group} {spec}: {result}")
            counts[group] += 1; stops[result["status"]] += 1; visited |= starts; edges |= branches
        check(visited == src["starts"], f"missing starts: {sorted(src['starts'] - visited)}")
        expected = {(at, dest) for at, options in src["branches"].items() for dest in options}
        check(edges == expected, f"missing branch outcomes: {expected - edges}")
        mutants = {}
        for name in ("wrong_sub_flags", "reversed_words", "second_tag_ignored"):
            fn = library(directory, name)
            for index, (group, spec) in enumerate(cases(True)):
                result, _, _ = compare(src, fn, spec)
                if not result["equal"]:
                    mutants[name] = dict(killed=True, case=index, group=group, witness=result)
                    break
            check(name in mutants, f"surviving mutant: {name}")
    return dict(source=src["receipt"], cases=sum(counts.values()), groups=dict(counts), stops=dict(stops),
                starts=len(visited), outcomes=len(edges), cpu_words=14, ram_bytes=SIZE, masks=[],
                boundary_observation="every caller instruction: CPU14 + SHA256(all RAM)",
                c_only_atomic_entry_refusals=refused, mutants=mutants)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--smoke", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory() as td: library(Path(td))
        print("CHECKER_BUILD_OK")
        return
    report = audit(args.smoke)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
