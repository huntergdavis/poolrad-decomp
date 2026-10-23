#!/usr/bin/env python3
"""Original-byte differential for OVL_009E_0000, the effect dispatcher, composed with the raw-CPU credited unit-0x38 handlers (phase 1, PM i21762).

Unicorn runs the ORIGINAL bytes: the dispatcher (unit 0x9E +0000), its loaded stub JMP, the handler's unit-0x38 code and everything the handler calls (the hash-checked expanded START.EXE image at load
segment 0). The C model runs the dispatcher and the handler's merged raw-CPU model. Compared after every case: CPU14, ALL 1,048,576 RAM bytes, the stop and the remaining budget; FLAGS under the shared
declared pinned splits (tools/overlay/pinned_shift_flags.py: shifts, MULW, DIV). Slots outside the raw-composed set STOP at the LCALL with a NAMED gap and are compared at that arrival; indices outside 1..139
are REFUSED and must leave CPU, RAM and budget untouched. Media stay private under ignored resources/.
"""
import argparse
import csv
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import tempfile
import time

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
import u38_effects_install_31a2_audit as I
import system_getmem_023f_audit as GM
from pinned_shift_flags import LastFlagWriter, declared_flags_split
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('OVL_009E_0000',)
BODY_SHA = None
CALLER_CS, DISP_CS, DS, SS = 0x3800, 0x4800, 0x1800, 0x2800     # unit 0x38 code, unit 0x9E code, data, stack (the layout the other U38 audits use)
ENTRY_RET = (0x8000, 0x9000)
SP_MIN, SP_MAX = 0x0200, 0xFFEE
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'REFUSED', 3: 'GAP', 4: 'CODE', 5: 'LIMIT', 6: 'CALLEE', 7: 'TRANSFER'}
KIND = {0: 'refused', 1: 'raw', 2: 'semantic-gap', 3: 'uncredited-gap', 4: 'deferred-gap', 5: 'adapted'}
COMPOSED = (1, 5)                # kinds whose handler is run (a raw model or a semantic model through its adapter)
ADAPTED = {0x010D}               # unit-0x38 entries whose credited SEMANTIC model is composed through a phase-2 adapter
RAW_DEFERRED = {0x3134}          # credited raw model whose world (the 1693 display closure and the bus) is not composed here
BUDGET = 100000
LIST_ENTRIES = {0x1363, 0x1472, 0x1730, 0x2814, 0x27D9}     # the handlers that walk a record's linked list and allocate
LIMITS = dict(loopbacks=4096, system_calls=4096, heap_steps=4096, links=4096)
ORACLE_CACHE = {}               # the original's result per (RAM, CPU, budget): the mutants reuse it
CODE_RANGES = []                 # filled by build_world: (lo, hi) physical ranges that must stay unchanged


def check(condition, message):
    if not condition:
        raise ValueError(message)


# ---------------------------------------------------------------------------------------------------- media and the slot table
def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u38, u9e = units[0x38], units[0x9E]
    code38 = ovr[u38["ovr_offset"]:u38["ovr_offset"] + u38["code_size"]]
    code9e = ovr[u9e["ovr_offset"]:u9e["ovr_offset"] + u9e["code_size"]]
    check(u9e["entries"][1] == 0 and u9e["memory_segment"] == 0xB0, "unit 0x9E entry 1 / stub segment differs")
    body = code9e[0:38]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0))
    check(len(decoded) == 16 and sum(i.size for i in decoded) == 38, "dispatcher extent differs")
    text = [f"{i.mnemonic} {i.op_str}" for i in decoded]
    expect = ["push bp", "mov bp, sp", "push word ptr [bp + 0xe]", "push word ptr [bp + 0xc]", "push word ptr [bp + 0xa]", "push word ptr [bp + 8]",
              "mov al, byte ptr [bp + 6]", "push ax", "mov al, byte ptr [bp + 0x10]", "xor ah, ah", "mov di, ax", "shl di, 1", "shl di, 1",
              "lcall 0x6828", "pop bp", "retf 0xc"]
    check([t for t in text[:13]] == expect[:13] and text[14] == "pop bp" and text[15] == "retf 0xc", f"dispatcher decode differs: {text}")
    check(decoded[13].mnemonic == "lcall" and decoded[13].address == 0x1E and decoded[13].size == 4 and bytes(body[0x1E:0x22]) == bytes([0xFF, 0x9D, 0x28, 0x68]),
          "the dispatch LCALL [DI+6828h] differs")
    check(not any(p for p in u9e["fixups"] if p < 38), "the dispatcher carries a relocation")
    # 31A2's installs: slot = (destination - 6828)/4; the pointer is (stub offset, stub segment) -> unit entry (stub offset = 0x20 + 5*entry)
    _, _, inst = I.source()
    slots = {}
    for seg, off, dest in zip(inst["segments"], inst["targets"], inst["destinations"]):
        slot = (dest - 0x6828) // 4
        unit = next(u for u in units.values() if u["memory_segment"] == seg)
        number, rem = divmod(off - 0x20, 5)
        check(rem == 0, "stub offset is not 0x20 + 5*entry")
        slots[slot] = dict(stub=(seg, off), unit=unit["stub_para"], entry=unit["entries"][number], number=number)
    check(sorted(slots) == [s for s in range(1, 140) if s != 88], "31A2's slot set differs")
    u86 = units[0x86]
    slots[88] = dict(stub=(0x96, 0x015B), unit=0x86, entry=u86["entries"][(0x15B - 0x20) // 5], number=(0x15B - 0x20) // 5)
    strict = {r["id"]: r["evidence_doc"] for r in csv.DictReader(open(ROOT / "docs/re/strict_full_c_addresses.csv"))}
    for s, v in slots.items():
        ident = f"OVL_{v['unit']:04X}_{v['entry']:04X}"
        if s == 88:
            v["kind"] = 3
            check(ident not in strict, "slot 88's handler is credited now: re-derive the table")
        elif ident in strict and strict[ident].endswith("_CPU.md"):
            v["kind"] = 4 if v["entry"] in RAW_DEFERRED else 1
        elif ident in strict:
            v["kind"] = 5 if v["entry"] in ADAPTED and v["unit"] == 0x38 else 2
        else:
            v["kind"] = 3
        v["id"] = (v["unit"] << 16) | v["entry"]
    u77 = units[0x77]
    code77 = ovr[u77["ovr_offset"]:u77["ovr_offset"] + u77["code_size"]]
    return dict(code77=code77, u77=u77, image=image, code38=code38, code9e=code9e, body=body, decoded=decoded, units=units, slots=slots, u38=u38, u9e=u9e)


def library(directory, override=None):
    """All of src/recovered in one shared object (the same set CMake builds); `override` maps a source stem to a replacement path (the mutants)."""
    directory = Path(directory)
    objs = []
    jobs = []
    for path in sorted((ROOT / "src/recovered").glob("*.c")):
        stem = path.stem
        use = Path(override[stem]) if override and stem in override else path
        obj = directory / f"{stem}.o"
        objs.append(obj)
        if not obj.exists() or obj.stat().st_mtime < use.stat().st_mtime or (override and stem in override):
            jobs.append((use, obj))
    from concurrent.futures import ThreadPoolExecutor

    def compile_one(job):
        use, obj = job
        subprocess.run(["cc", "-std=c11", "-O1", "-fPIC", "-w", "-I", str(ROOT / "include"), "-c", str(use), "-o", str(obj)], check=True, capture_output=True, text=True)
    with ThreadPoolExecutor(max_workers=3) as pool:
        list(pool.map(compile_one, jobs))
    so = directory / "u9e0000.so"
    subprocess.run(["cc", "-shared", "-o", str(so), *map(str, objs)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(so))
    f = lib.pr_u9e_dispatch_0000_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t), ctypes.c_void_p, ctypes.c_void_p]
    f.restype = ctypes.c_int
    for name in ("slot_kind", "slot_stub_off", "slot_stub_seg", "slot_entry", "slot_id"):
        g = getattr(lib, f"pr_u9e_dispatch_0000_{name}"); g.argtypes = [ctypes.c_uint]; g.restype = ctypes.c_uint32 if name == "slot_id" else ctypes.c_int
    return lib


class Limits(ctypes.Structure):
    _fields_ = [("loopbacks", ctypes.c_uint), ("system_calls", ctypes.c_uint), ("heap_steps", ctypes.c_uint), ("links", ctypes.c_size_t)]


class Diag(ctypes.Structure):
    _fields_ = [("slot", ctypes.c_int), ("handler_id", ctypes.c_uint32), ("gap", ctypes.c_int), ("callee_status", ctypes.c_int), ("stub_off", ctypes.c_uint16), ("stub_seg", ctypes.c_uint16)]


def verify_table(src, lib):
    """The C slot table equals the one derived from the media, slot by slot (the claim 'generated from the media')."""
    for s in range(256):
        want = src["slots"].get(s)
        kind = lib.pr_u9e_dispatch_0000_slot_kind(s)
        if want is None:
            check(kind == 0, f"slot {s}: C kind {kind}, expected refused")
            continue
        check(kind == want["kind"], f"slot {s}: C kind {kind}, media {want['kind']}")
        check((lib.pr_u9e_dispatch_0000_slot_stub_seg(s), lib.pr_u9e_dispatch_0000_slot_stub_off(s)) == want["stub"], f"slot {s}: stub differs")
        check(lib.pr_u9e_dispatch_0000_slot_entry(s) == want["entry"], f"slot {s}: entry differs")
        check(lib.pr_u9e_dispatch_0000_slot_id(s) == want["id"], f"slot {s}: id differs")
    from u9e_dispatch_table_audit import inventory          # the independent DS:6828 inventory (a different decoder of the same installers): same slot -> handler map
    rows = {r["slot"]: r for r in inventory()["slots"]}
    for s, v in src["slots"].items():
        check(rows[s]["handler_id"] == f"OVL_{v['unit']:04X}_{v['entry']:04X}", f"slot {s}: the independent inventory says {rows[s]['handler_id']}")
    return {KIND[k]: sum(1 for s in range(256) if lib.pr_u9e_dispatch_0000_slot_kind(s) == k) for k in KIND}


# ---------------------------------------------------------------------------------------------------- the world
def build_world(src):
    memory = bytearray([0xA5]) * SIZE
    image = src["image"]
    memory[0:len(image)] = image
    for lo, code in ((physical(CALLER_CS, 0), src["code38"]), (physical(DISP_CS, 0), src["code9e"])):
        memory[lo:lo + len(code)] = code
    CODE_RANGES[:] = [(physical(CALLER_CS, 0), physical(CALLER_CS, 0) + len(src["code38"])), (physical(DISP_CS, 0), physical(DISP_CS, 0) + len(src["code9e"]))]
    # the loaded stubs: unit 0x38 (stub segment 0x41), every entry's far JMP to its loaded code; unit 0x9E likewise for entry 1 (the dispatcher itself)
    u38 = src["u38"]
    for number, entry in enumerate(u38["entries"]):
        at = physical(0x41, 0x20 + 5 * number)
        memory[at:at + 5] = struct.pack("<BHH", 0xEA, entry, CALLER_CS)
    for number, entry in enumerate(src["u9e"]["entries"]):                      # unit 0x9E (stub segment 0xB0): the dice chain and the list append the handlers call, loaded form
        at = physical(0xB0, 0x20 + 5 * number)
        memory[at:at + 5] = struct.pack("<BHH", 0xEA, entry, DISP_CS)
    # the installer's table: far pointers (stub offset, stub segment) at DS:6828h + 4*slot
    for s, v in src["slots"].items():
        put16(memory, DS, 0x6828 + 4 * s, v["stub"][1]); put16(memory, DS, 0x682A + 4 * s, v["stub"][0])
    return memory


ARENA_LO, ARENA_HI = 0x60000, 0x88000             # one 160 KiB pointer arena filled from a small alphabet so equality and branch tests are hit
ARENAS = (0x6000, 0x6800, 0x7000, 0x7800)         # far-pointer segments into it
ALPHABET = (0, 0, 1, 1, 2, 3, 4, 5, 0x7F, 0x80, 0xFF, 0x34, 0x35, 0x0B)


def fixture(base, rng, idx, *, sp=0x8000, flags=None, ax=None, words=None, idx_word=None, arenas=True, ds_noise=True, regs=None, poke=(), heap=True, lists=False):
    memory = bytearray(base)
    if arenas:
        table = bytes(rng.choice(ALPHABET) for _ in range(256))
        memory[ARENA_LO:ARENA_HI] = rng.randbytes(ARENA_HI - ARENA_LO).translate(table)
    if ds_noise:
        for off in range(0, 0x6828, 2):
            if rng.random() < 0.02:
                put16(memory, DS, off, rng.randrange(0x10000))
        put16(memory, DS, 0x5D92, rng.choice((0x0100, 0x0200, 0x0000, 0x8000))); put16(memory, DS, 0x5D94, rng.choice(ARENAS))
        for off in (0x6817, 0x6818, 0x6819, 0x681B, 0x6822, 0x6823, 0x6825, 0x6DBF):
            memory[physical(DS, off)] = rng.choice(ALPHABET)
    for pseg, poff, data in poke:
        at = physical(pseg, poff); memory[at:at + len(data)] = data
    if heap:                                                          # a SYSTEM heap (free list, HeapPtr, HeapEnd) for the handlers that allocate; below the unit-0x38 code
        blocks, top, end = GM.random_heap(rng)
        end = (min(end[0], 0x3700), end[1])
        GM.heap(memory, blocks=blocks, top=top, end=end)
    # the dispatcher's entry frame: [BP+6] byte word, [BP+8], [BP+0A], far pointer ([BP+0C] offset, [BP+0E] segment), then the index word
    off = lambda: rng.choice((rng.randrange(0x10000), 0x0100, 0x0200, 0x0000, 0x8000, 0x0108))
    w = list(words) if words else [rng.randrange(0x10000), off(), off(), off(), rng.choice(ARENAS)]
    if lists:                                                         # the list/heap handlers: the record at w3/w4 has a far head (+7Fh/+81h) over 0..3 nodes linked at +5/+7, nil-terminated
        rec = (w[4], w[3])
        n = rng.choice((0, 0, 1, 2, 3))
        nodes = [(rng.choice(ARENAS), 0x2000 + 0x40 * j + 0x400 * rng.randrange(4)) for j in range(n)]
        head = nodes[0] if nodes else (0, 0)
        put16(memory, rec[0], (rec[1] + 0x7F) & 0xFFFF, head[1]); put16(memory, rec[0], (rec[1] + 0x81) & 0xFFFF, head[0])
        for j, (seg, off) in enumerate(nodes):
            nxt = nodes[j + 1] if j + 1 < len(nodes) else (0, 0)
            put16(memory, seg, (off + 5) & 0xFFFF, nxt[1]); put16(memory, seg, (off + 7) & 0xFFFF, nxt[0])
    frame = [ENTRY_RET[1], ENTRY_RET[0], w[0], w[1], w[2], w[3], w[4], (idx_word if idx_word is not None else (rng.randrange(256) << 8) | idx)]
    for k, v in enumerate(frame):
        put16(memory, SS, (sp + 2 * k) & 0xFFFF, v)
    reg = regs or tuple(rng.randrange(0x10000) for _ in range(7))
    cpu = Cpu(ax if ax is not None else reg[0], *reg[1:], sp, DISP_CS, 0x0000, DS, rng.randrange(0x10000), SS,
              ((flags if flags is not None else rng.choice((0x0202, 0x0002, 0x0A87, 0x0E57, 0x08D7, 0x0003, 0x0402))) & ~0x0200) | 0x0002)
    return memory, cpu


# ---------------------------------------------------------------------------------------------------- the oracle
def run_original(src, memory, cpu, budget):
    """Unicorn on the original bytes. One unit of budget per executed instruction of the dispatcher itself (CS = DISP_CS, IP < 0x24); the stub JMP and the handler are uncounted.
    Stops: RETURNED (RETF to the entry frame's return address), LIMIT (budget zero at the next counted arrival), GAP (arrival at the LCALL of a slot that is not raw-composed),
    TRANSFER (a return goes elsewhere than its call site)."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    for base in (0, SIZE):
        uc.mem_map_ptr(base, SIZE, 7, ctypes.addressof(backing))
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(r, getattr(cpu, n))
    left = [budget]; calls = []; pending = [None]; status = [None]; seen = set(); writer = LastFlagWriter()
    decoder = Cs(CS_ARCH_X86, CS_MODE_16); cache = {}

    def word(seg, off):
        at = physical(seg, off & 0xFFFF)
        return backing[at] | (backing[(at + 1) & (SIZE - 1)] << 8)
    entry_ret = (word(cpu.ss, cpu.sp + 2), word(cpu.ss, cpu.sp))
    idx = backing[physical(cpu.ss, (cpu.sp + 0x0E) & 0xFFFF)]
    kinds = {s: v["kind"] for s, v in src["slots"].items()}

    def hook(uc_, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        writer.finish(uc)
        if pending[0] is not None:
            target, expected = pending[0]; pending[0] = None
            if expected is None:
                status[0] = 'RETURNED' if target == entry_ret else 'TRANSFER'; uc.emu_stop(); return
            if target != expected:
                status[0] = 'TRANSFER'; uc.emu_stop(); return
        counted = cs == DISP_CS and ip < 0x24
        if counted:
            if not left[0]:
                status[0] = 'LIMIT'; uc.emu_stop(); return
            if ip == 0x1E and kinds.get(idx, 0) not in COMPOSED:
                status[0] = 'GAP'; uc.emu_stop(); return
            left[0] -= 1
            seen.add(ip)
        writer.step(uc, address)
        raw = bytes(uc.mem_read(address, 8))
        key = (address, raw)
        if key not in cache:
            cache[key] = next(decoder.disasm(raw, ip), None)
        ins = cache[key]
        if ins is None:
            return
        if ins.mnemonic in ('call', 'lcall'):
            calls.append((cs, (ip + ins.size) & 0xFFFF))
        if ins.mnemonic == 'retf':
            sp = uc.reg_read(R.UC_X86_REG_SP); ss = uc.reg_read(R.UC_X86_REG_SS)
            pending[0] = ((word(ss, sp + 2), word(ss, sp)), calls.pop() if calls else None)
        elif ins.mnemonic == 'ret':
            sp = uc.reg_read(R.UC_X86_REG_SP); ss = uc.reg_read(R.UC_X86_REG_SS)
            pending[0] = ((cs, word(ss, sp)), calls.pop() if calls else None)
    uc.hook_add(UC_HOOK_CODE, hook)
    try:
        uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=400000)
    except Exception as e:                                           # a wild execution: report it as such, never as a pass
        raise ValueError(f"original execution failed: {e}")
    check(status[0] is not None, 'original did not reach a stop within 400,000 instructions')
    out = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return status[0], bytes(backing), out, left[0], seen, writer


def run_c(lib, memory, cpu, budget, idx_limits=None):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget); lim = Limits(**LIMITS); d = Diag()
    s = lib.pr_u9e_dispatch_0000_cpu(ram, SIZE, ctypes.byref(out), ctypes.byref(left), ctypes.byref(lim), ctypes.byref(d))
    return STATUS[s], bytes(ram), out, left.value, d


def compare(label, src, memory, cpu, budget, lib, mem_before):
    """Returns (expected status, seen starts, extras). Raises on the first difference."""
    idx = memory[physical(cpu.ss, (cpu.sp + 0x0E) & 0xFFFF)]
    astat, aram, acpu, aleft, diag = run_c(lib, memory, cpu, budget)
    if not (idx == 0 or idx >= 140):
        key = (sha256(bytes(memory)).digest(), bytes(cpu), budget)
        if key not in ORACLE_CACHE:
            estat, eram, ecpu, eleft, seen, writer = run_original(src, memory, cpu, budget)
            ORACLE_CACHE[key] = (estat, eram, ecpu, eleft, seen, writer)
            if len(ORACLE_CACHE) > 400:
                ORACLE_CACHE.pop(next(iter(ORACLE_CACHE)))
        estat, eram, ecpu, eleft, seen, writer = ORACLE_CACHE[key]
    if idx == 0 or idx >= 140:                                       # the declared domain: refused, nothing touched. (The original would have called through garbage.)
        check(astat == 'REFUSED', f"{label}: C {astat}, expected REFUSED for index {idx}")
        check(aram == bytes(memory) and aleft == budget, f"{label}: REFUSED changed RAM or budget")
        for n, _ in Cpu._fields_:
            check(getattr(acpu, n) == getattr(cpu, n), f"{label}: REFUSED changed {n}")
        return 'REFUSED', set(), dict(refused=True)
    check(astat == estat, f"{label}: status C {astat} original {estat}")
    check(aleft == eleft, f"{label}: budget left C {aleft} original {eleft}")
    for n, _ in Cpu._fields_:
        if n == 'flags':
            problem = declared_flags_split(acpu.flags, ecpu.flags, writer)
            check(problem is None, f"{label}: {problem}")
            continue
        check(getattr(acpu, n) == getattr(ecpu, n), f"{label}: {n} C {getattr(acpu, n):04x} original {getattr(ecpu, n):04x}")
    if aram != eram:
        diffs = [i for i in range(SIZE) if aram[i] != eram[i]]
        raise ValueError(f"{label}: RAM {[(hex(i), aram[i], eram[i]) for i in diffs[:10]]} ({len(diffs)} bytes)")
    for lo, hi in CODE_RANGES:
        check(eram[lo:hi] == bytes(mem_before[lo:hi]), f"{label}: code written")
    return estat, seen, dict(diag=diag, split=writer.shift_nonzero or writer.division is not None)


# ---------------------------------------------------------------------------------------------------- the three data-dependent callers
CS77 = 0x5800
CALLERS = (       # (name, unit, function entry, ip of the MOV AL,ES:[DI+k] that reads the record byte, the LCALL 00B0:0025 site, entry-frame words)
    ('u38+2F30', 0x38, 0x2EDA, 0x2F2C, 0x2F40),
    ('u77+1543', 0x77, 0x14D0, 0x152F, 0x1543),
    ('u77+1663', 0x77, 0x14D0, 0x164F, 0x1663),
)


def caller_world(src, base):
    memory = bytearray(base)
    at = physical(CS77, 0); memory[at:at + len(src["code77"])] = src["code77"]
    for number, entry in enumerate(src["u9e"]["entries"]):                      # the loaded stubs of unit 0x9E: the caller's LCALLs land on the dispatcher and its siblings
        at = physical(0xB0, 0x20 + 5 * number); memory[at:at + 5] = struct.pack("<BHH", 0xEA, entry, DISP_CS)
    return memory


def reach_dispatcher(src, memory, cpu, read_ip, read_cs, bad, cap=300000):
    """Run the ORIGINAL caller bytes from the function entry. At the instruction that reads the record byte, force that byte to `bad`; stop on arrival at the dispatcher's entry (after the stub JMP).
    Returns (RAM, CPU) at that arrival, or None when the caller did not get there (a different random state is tried)."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    for base in (0, SIZE):
        uc.mem_map_ptr(base, SIZE, 7, ctypes.addressof(backing))
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(r, getattr(cpu, n))
    state = dict(done=False, forced=False)

    def hook(uc_, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if cs == read_cs and ip == read_ip and not state['forced']:
            es = uc.reg_read(R.UC_X86_REG_ES); di = uc.reg_read(R.UC_X86_REG_DI)
            k = 0x3D if read_ip == 0x2F2C else 0x3E
            backing[physical(es, (di + k) & 0xFFFF)] = bad; state['forced'] = True
        if cs == 0xB0 and ip == 0x52:              # the caller's preceding LCALL 00B0:0052 (unit 0x9E +0E52, RETF 0Ch) is replaced by its return: HOST SUBSTITUTION, declared in the doc
            sp = uc.reg_read(R.UC_X86_REG_SP); ss = uc.reg_read(R.UC_X86_REG_SS)
            at = physical(ss, sp)
            rip = backing[at] | (backing[at + 1] << 8); rcs = backing[at + 2] | (backing[at + 3] << 8)
            uc.reg_write(R.UC_X86_REG_SP, (sp + 4 + 0x0C) & 0xFFFF); uc.reg_write(R.UC_X86_REG_CS, rcs); uc.reg_write(R.UC_X86_REG_IP, rip)
            state['skipped'] = state.get('skipped', 0) + 1
            return
        if cs == DISP_CS and ip == 0:
            state['done'] = True; uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE, hook)
    try:
        uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=cap)
    except Exception:
        return None
    if not (state['done'] and state['forced']):
        return None
    return bytes(backing), Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))


def set_record_77(memory, rec_seg, rec_off, read_ip):
    """Steer unit 0x77 +14D0 to the push of the record byte: the record (+34h/+36h/+2Eh), the DS table rows for its type and the active character's two bytes, so [BP-2] stays 0 on the +1663 path."""
    rec = lambda off: physical(rec_seg, (rec_off + off) & 0xFFFF)
    typ = 3
    memory[rec(0x2E)] = typ
    if read_ip == 0x152F:                               # +1543: the record is marked (+34h != 0), not flagged (+36h == 0)
        memory[rec(0x34)] = 1; memory[rec(0x36)] = 0
    else:                                               # +1663: the record is unmarked; the type's table rows and the active character decide [BP-2]
        memory[rec(0x34)] = 0
        memory[physical(DS, typ * 16 + 0x5583)] = 0; memory[physical(DS, typ * 16 + 0x5582)] = 0xFF; memory[physical(DS, typ * 16 + 0x558F)] = 0xFF
        off5d = memory[physical(DS, 0x5D92)] | (memory[physical(DS, 0x5D93)] << 8); seg5d = memory[physical(DS, 0x5D94)] | (memory[physical(DS, 0x5D95)] << 8)
        memory[physical(seg5d, (off5d + 0x100) & 0xFFFF)] = 0; memory[physical(seg5d, (off5d + 0xB0) & 0xFFFF)] = 0xFF


def caller_fixtures(src, lib, base, tries=400):
    """For each data-dependent far caller: drive its original bytes to the dispatcher with an out-of-range record byte and prove the refusal fires with CPU, RAM and budget untouched;
    the in-range control (16) goes through the full differential from the very same live caller state."""
    results = []; world = caller_world(src, base)
    for name, unit, entry, read_ip, site in CALLERS:
        code_cs = CALLER_CS if unit == 0x38 else CS77
        row = dict(site=name, refused={}, control=None, tries=0)
        for bad in (0, 140, 255, 16):
            for attempt in range(tries):
                rng = random.Random(0xCA11 + 1000 * len(results) + attempt)
                memory, cpu = fixture(world, rng, 16)
                # the caller's own entry frame: its return address and arguments (a far record pointer at [BP+6] for unit 0x77, at [BP+8] for unit 0x38), then run its original bytes
                sp = 0x8000
                put16(memory, SS, sp, 0x9000); put16(memory, SS, sp + 2, 0x8000)
                rec_off, rec_seg = rng.choice((0x0100, 0x0200, 0x0300)), rng.choice(ARENAS)
                if unit == 0x38:             # +2EDA: [BP+6] byte, [BP+8]/[BP+0A] the record far pointer, [BP+0C]/[BP+0E] another
                    words = [0, rec_off, rec_seg, rng.choice((0x0100, 0x0200)), rng.choice(ARENAS)]
                else:                        # +14D0: [BP+6]/[BP+8] the record far pointer
                    words = [rec_off, rec_seg, rng.choice((0x0100, 0x0200)), rng.choice(ARENAS), rng.randrange(0x10000)]
                    set_record_77(memory, rec_seg, rec_off, read_ip)
                    memory[physical(rec_seg, (rec_off + 0x3E) & 0xFFFF)] = 0xFF          # the function's first read of +3Eh decides the path (> 7Fh); the forced byte is the SECOND read
                for k, w in enumerate(words):
                    put16(memory, SS, sp + 4 + 2 * k, w)
                cpu = Cpu(cpu.ax, cpu.bx, cpu.cx, cpu.dx, cpu.si, cpu.di, 0x7777, sp, code_cs, entry, DS, 0, SS, 0x0202)
                got = reach_dispatcher(src, memory, cpu, read_ip, code_cs, bad)
                row['tries'] += 1
                if got is None:
                    continue
                ram, live = got
                check(ram[physical(live.ss, (live.sp + 0x0E) & 0xFFFF)] == bad, f"{name}: the record byte did not reach the index slot")
                if bad == 16:                     # the in-range control: the very same live caller state goes through the full differential (NOP handler, slot 16)
                    estat, _, _ = compare(f"{name}-control", src, ram, live, BUDGET, lib, world)
                    check(estat == 'RETURNED', f"{name}: control ended {estat}")
                    row['control'] = attempt + 1
                    break
                status, aram, acpu, aleft, diag = run_c(lib, ram, live, BUDGET)
                check(status == 'REFUSED' and aram == ram and aleft == BUDGET and all(getattr(acpu, n) == getattr(live, n) for n, _ in Cpu._fields_),
                      f"{name}: index {bad} was not refused cleanly ({status})")
                row['refused'][bad] = attempt + 1
                break
            check(bad in row['refused'] or (bad == 16 and row['control']), f"{name}: the original caller never reached the dispatcher for index {bad} in {tries} states")
        results.append(row)
    return results


# ---------------------------------------------------------------------------------------------------- cases
def cases(src, per_slot=16, quick=False):
    rng = random.Random(0x9E0000)
    out = []

    def add(name, idx, **kw):
        out.append((name, idx, kw, BUDGET))
    raw = [s for s in range(1, 140) if src["slots"][s]["kind"] in COMPOSED]
    for s in raw:
        for k in range(2 if quick else per_slot):
            add(f"slot{s:03d}-random{k}", s, lists=src["slots"][s]["entry"] in LIST_ENTRIES)
    for s in range(1, 140):
        if src["slots"][s]["kind"] not in COMPOSED:
            add(f"gap{s:03d}-{KIND[src['slots'][s]['kind']]}", s)
            add(f"gap{s:03d}-limit0", s, budget=13)               # the budget ends at the LCALL: LIMIT beats GAP
    for idx in (0, 140, 141, 160, 200, 255):
        add(f"refused-{idx}", idx)
    for k in range(8):
        add(f"refused-random{k}", rng.choice([0] + list(range(140, 256))))
    # TRANSFER: slot 31 is +133F, whose resident zeroes target+3, +0, +7, +6 where target is the far pointer stored at character+108h. Aim that target at the dispatcher's frames:
    #   the ENTRY return slot (sp): CS's high byte (sp+3) is zeroed, so the dispatcher's final RETF goes to 0000:9000; the handler's return slot (sp-16): its return IP/CS bytes are zeroed.
    # phase 2, slot 1 (+010D, ADD byte [6825],5 then INC byte [6822]): the byte values that reach every flag outcome of both instructions
    for v25 in (0x00, 0x0A, 0x0B, 0x7A, 0x7B, 0x7F, 0x80, 0xFA, 0xFB, 0xFF):
        for v22 in (0x00, 0x0F, 0x7F, 0x80, 0xFF):
            add(f"adapt010d-{v25:02x}-{v22:02x}", 1, poke=[(DS, 0x6825, bytes([v25])), (DS, 0x6822, bytes([v22]))])
    for tag, delta in (("final", 0), ("handler", -16), ("final-ip", -3), ("args", 6)):
        sp = 0x8000
        target = struct.pack("<HH", (sp + delta) & 0xFFFF, SS)
        add(f"transfer-133f-{tag}", 31, sp=sp, words=[0, 0x0100, 0x0200, 0x0100, 0x6000], poke=[(0x6000, 0x0100 + 0x108, target)])
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f"sp-{sp:04x}-slot16", 16, sp=sp)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f"flags-{fl:04x}-slot16", 16, flags=fl)
    for b in range(0, 40):
        add(f"budget-slot16-{b}", 16, budget=b)
        add(f"budget-slot004-{b}", 4, budget=b)
    return out


def differential(src, lib, base, only=None, keep_going=False):
    t0 = time.time(); rng = random.Random(0x9E0001); stops = {}; seen_all = set(); failures = []; skipped = []; per_slot = {}; split_states = 0
    cs = [c for c in cases(src) if only is None or c[0] in only or any(c[0].startswith(o) for o in only)]
    for index, (name, idx, kw, budget) in enumerate(cs):
        kw = dict(kw); budget = kw.pop('budget', budget)
        if index % 40 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        memory, cpu = fixture(base, rng, idx, **kw)
        try:
            estat, seen, extra = compare(name, src, memory, cpu, budget, lib, base)
        except ValueError as e:
            if 'did not reach a stop' in str(e) or 'original execution failed' in str(e):
                skipped.append((name, str(e)[:100])); continue
            failures.append((name, str(e)[:400]))
            if not keep_going:
                raise
            continue
        stops[estat] = stops.get(estat, 0) + 1; seen_all |= seen
        per_slot.setdefault(idx, []).append(estat)
        split_states += int(bool(extra.get('split')))
    return dict(cases=len(cs), compared=len(cs) - len(skipped) - len(failures), end_states=stops, dispatcher_starts=f'{len(seen_all)}/16', skipped_nonterminating=len(skipped),
                skipped=skipped[:10], failures=failures, split_states=split_states, per_slot={s: sorted(set(v)) for s, v in sorted(per_slot.items())}, seconds=round(time.time() - t0, 1))


# ---------------------------------------------------------------------------------------------------- mutation
EQUIVALENT = {      # a survivor named here carries a written proof
    '+010D ADD computes AF from the wrong operand': 'the INC byte [6822] that follows the ADD (no instruction between them) overwrites AF, OF, SF, ZF and PF; only the ADD\'s CF reaches the return. The ADD\'s non-CF flags are therefore dead at the handler\'s return in the original and in the model, and any change to how add8_flags computes them is unobservable (also: b is always 5, whose bit 4 is 0)',
    '+010D ADD tests AF at bit 5, not bit 4': 'the same proof: the ADD\'s AF is overwritten by the following INC before the return, so no AF rule inside add8_flags is observable',
}

MUTANTS = (
    ('REFUSED lets index 140 through', [("if (idx == 0u || idx >= 140u)", "if (idx == 0u || idx >= 141u)")]),
    ('REFUSED lets index 0 through', [("if (idx == 0u || idx >= 140u)", "if (idx >= 140u)")]),
    ('the budget is not read before the slot verdict (GAP beats LIMIT)', [("    if (*budget == 0u) return PR_U9E_0000_LIMIT;                                                      /* 001E LCALL", "    /* 001E LCALL")]),
    ('SHL does not set AF', [("uint16_t r = (uint16_t)(a << 1u), f = parity((uint8_t)r) | 0x0010u;", "uint16_t r = (uint16_t)(a << 1u), f = parity((uint8_t)r);")]),
    ('XOR AH,AH leaves ZF clear', [("c->flags = (uint16_t)((c->flags & ~ARITH) | 0x0044u); AT(0x0018u);", "c->flags = (uint16_t)((c->flags & ~ARITH) | 0x0004u); AT(0x0018u);")]),
    ('the first forwarded word is [BP+0C]', [("STEP(); push(ram, c, bp16(ram, c, 0x0Eu)); AT(0x0006u);", "STEP(); push(ram, c, bp16(ram, c, 0x0Cu)); AT(0x0006u);")]),
    ('the byte forwarded is [BP+8]', [("rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); AT(0x0012u);", "rd8(ram, c->ss, (uint16_t)(c->bp + 8u))); AT(0x0012u);")]),
    ('the index is the word at [BP+10], not its low byte', [("rd8(ram, c->ss, (uint16_t)(c->bp + 0x10u))); AT(0x0016u);", "rd16(ram, c->ss, (uint16_t)(c->bp + 0x10u))); AT(0x0016u);")]),
    ('the segment word is read at +6829h', [("(uint16_t)(c->di + 0x682Au));\n    d->stub_off", "(uint16_t)(c->di + 0x6829u));\n    d->stub_off")]),
    ('RETF pops 0Ah', [("c->sp = (uint16_t)(c->sp + 0x000Cu);", "c->sp = (uint16_t)(c->sp + 0x000Au);")]),
    ('the return-address (TRANSFER) check dropped', [("    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U9E_0000_TRANSFER;", "    (void)ret_ip; (void)ret_cs;")]),
    ('the handler-return (TRANSFER) check dropped', [("if (c->cs != home_cs || c->ip != DISPATCH_RETURN_IP) return PR_U9E_0000_TRANSFER;", "(void)home_cs;")]),
    ('the table-word check dropped', [("if (off != stub_off || seg != stub_seg) return PR_U9E_0000_CODE;", "(void)stub_off; (void)stub_seg;")]),
    ('the stub-opcode check dropped', [("if (rd8(ram, stub_seg, stub_off) != 0xEAu) return PR_U9E_0000_CODE;", "")]),
    ('the stub-target check dropped', [("if (jo != sl->entry || fn == NULL) return PR_U9E_0000_CODE;", "if (fn == NULL) return PR_U9E_0000_CODE;")]),
    ('the LCALL is not charged', [("--*budget; push(ram, c, home_cs);", "push(ram, c, home_cs);")]),
    ('a handler stop is treated as a return', [("if (s != 0) { d->callee_status = s; return PR_U9E_0000_CALLEE; }", "(void)s;")]),
    ('+0679 is routed to +0947', [("{0x0679, h_0679}", "{0x0679, h_0947}")]),
    ('+2976 is routed to +2984', [("{0x2976, h_2976}", "{0x2976, h_2984}")]),
    ('+2A87 uses the Unicorn shift profile', [("pr_u38_target_index_gate_2a87_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_DOSBOX386, &dg)", "pr_u38_target_index_gate_2a87_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_UNICORN, &dg)")]),
    ('+1363 gets no heap budget', [("l.max_heap_steps = m->lim->heap_steps; l.max_links = m->lim->links; COPY(&x, m->cpu);\n    s = (int)pr_u38_append_1363_cpu", "l.max_heap_steps = 0; l.max_links = m->lim->links; COPY(&x, m->cpu);\n    s = (int)pr_u38_append_1363_cpu")]),
    ('+010D adds 4, not 5, to [6825]', [("pr_u38_entry2(&st) != PR_U38_OK) return 1;\n    store_state(m->ram, m->cpu, &st);\n    add8_flags(m->cpu, a6825, 5u);", "pr_u38_entry2(&st) != PR_U38_OK) return 1;\n    st.b6825 = (uint8_t)(st.b6825 - 1u);\n    store_state(m->ram, m->cpu, &st);\n    add8_flags(m->cpu, a6825, 5u);")]),
    ('+010D INC clears CF instead of preserving it', [("c->flags = (uint16_t)((c->flags & ~(ARITH & ~0x0001u)) | f);   /* CF is preserved */", "c->flags = (uint16_t)((c->flags & ~ARITH) | f);")]),
    ('+010D ADD computes AF from the wrong operand', [("    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)", "    if ((a ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)")]),
    ('+010D ADD tests AF at bit 5, not bit 4', [("    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)", "    if ((a ^ b ^ r) & 0x20u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)")]),
    ('+010D leaves the saved BP unwritten (no PUSH BP write)', [("static void frame_enter(call_t *m) { push(m->ram, m->cpu, m->cpu->bp); m->cpu->bp = m->cpu->sp; }", "static void frame_enter(call_t *m) { m->cpu->sp = (uint16_t)(m->cpu->sp - 2u); m->cpu->bp = m->cpu->sp; }")]),
    ('+010D pops 8 argument bytes, not 0Ah', [("frame_leave(m, 0x0Au);\n    return 0;\n}\n\ntypedef", "frame_leave(m, 0x08u);\n    return 0;\n}\n\ntypedef")]),
    ('+010D skips the semantic model (the DS bytes stay)', [("if (pr_u38_entry2(&st) != PR_U38_OK) return 1;", "(void)pr_u38_entry2;")]),
    ('the gap kind of an uncredited slot is reported as semantic', [("sl->kind == K_GAP_SEMANTIC ? PR_U9E_0000_GAP_SEMANTIC :", "sl->kind != K_RAW ? PR_U9E_0000_GAP_SEMANTIC :")]),
)
MUTANT_CASES_EXTRA = ('adapt010d-', 'transfer-133f-', 'refused-', 'gap001-', 'gap004-', 'gap088-', 'gap135-', 'sp-', 'flags-', 'budget-slot16-', 'budget-slot004-')


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if p.stem == 'overlay_u9e_dispatch_0000_cpu' else p) for p in sorted((ROOT / 'src/recovered').glob('*.c'))]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-w', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u9e_dispatch_0000_cpu.c')] + srcs + ['-o', str(exe)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(src, base, libdir, only_mutants=None):
    out = []; text0 = (ROOT / 'src/recovered/overlay_u9e_dispatch_0000_cpu.c').read_text()
    raw = [s for s in range(1, 140) if src['slots'][s]['kind'] in COMPOSED]
    first = {}
    for s in raw:
        first.setdefault(src['slots'][s]['entry'], s)
    names = [f"slot{s:03d}-random{k}" for s in first.values() for k in range(3)]
    for k, (name, edits) in enumerate(MUTANTS):
        if only_mutants is not None and k not in only_mutants:
            continue
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = text0
        for old, new in edits:
            check(text.count(old) == 1, f"mutant text occurs once: {name}: {old[:60]}"); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u9e-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            work = Path(tmp) / 'objs'; work.mkdir()
            for o in Path(libdir).glob('*.o'):
                if o.stem != 'overlay_u9e_dispatch_0000_cpu':
                    (work / o.name).symlink_to(o)
            try:
                mlib = library(work, override={'overlay_u9e_dispatch_0000_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(src, mlib, base, only=list(names) + list(MUTANT_CASES_EXTRA))
                out.append((name, 'SURVIVED the differential' + c_test_verdict(tmp, path)))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:90]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:70]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    d = sum(1 for _, r in out if r.startswith('CAUGHT:')); c = sum(1 for _, r in out if r.endswith('CAUGHT BY THE C TEST'))
    return dict(total=len(out), caught=d + c, caught_by_differential=d, caught_by_c_test=c, results=out,
                proven_equivalent=[n for n, r in out if r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST') and n in EQUIVALENT],
                survivors=[n for n, r in out if ((r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST')) or r == 'COMPILE-ERROR') and n not in EQUIVALENT])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--callers', '--quick', '--mutate', '--native', '--keep-going'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--only', nargs='*'); p.add_argument('--mutant-filter', help='run only the mutants whose name contains this text (the per-adapter PRs)')
    p.add_argument('--libdir', type=Path); p.add_argument('--output', type=Path); a = p.parse_args()
    if a.compile_only:
        for f in ('source', 'library', 'verify_table', 'build_world', 'fixture', 'run_original', 'run_c', 'compare', 'cases', 'differential'):
            check(callable(globals()[f]), f)
        print('CHECKER_BUILD_OK'); return
    with tempfile.TemporaryDirectory(prefix='u9e-0000-') as tmp:
        libdir = a.libdir or Path(tmp)
        libdir.mkdir(parents=True, exist_ok=True)
        src = source(); lib = library(libdir); report = dict(table=verify_table(src, lib)); base = build_world(src)
        if a.unicorn:
            report['unicorn'] = differential(src, lib, base, a.only, a.keep_going)
            if a.output:
                a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2, default=str) + '\n')
        if a.callers:
            report['callers'] = caller_fixtures(src, lib, base, tries=60)
        if a.mutate:
            only = None if not a.mutant_filter else {k for k, (n, _) in enumerate(MUTANTS) if a.mutant_filter in n}
            report['mutation'] = mutation_check(src, base, libdir, only_mutants=only)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2, default=str) + '\n')
    print(json.dumps(report, sort_keys=True, default=str)[:3000])


if __name__ == '__main__':
    main()
