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
import zlib
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
ADAPTED = {0x010D, 0x011D, 0x0C06, 0x0C15, 0x171F, 0x25A2, 0x2A6D, 0x2B63, 0x10C1, 0x03E1, 0x06E4, 0x0A3D, 0x2A28, 0x013B, 0x05D6, 0x064E, 0x0BAC, 0x0C26, 0x1452, 0x2490, 0x2700, 0x0377, 0x03AC, 0x0918, 0x12B2, 0x25BC, 0x2D38, 0x04BA}               # unit-0x38 entries whose credited SEMANTIC model is composed through a phase-2 adapter
ADAPTED |= {0x01C3, 0x24D9, 0x268A, 0x2E37}
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
    code_written = any(eram[lo:hi] != bytes(mem_before[lo:hi]) for lo, hi in CODE_RANGES)     # C and the original agree byte for byte, but both overwrote resident code: outside the composed contract, reported
    return estat, seen, dict(diag=diag, split=writer.shift_nonzero or writer.division is not None, code_written=code_written)


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
    # phase 2, batch 1: the byte values that reach every flag outcome of each handler's last flag writer (slot -> handler: 2 +011D, 35 +0C06, 36 +0C15, 42 +10C1, 71 +171F, 93 +25A2, 114 +2A6D, 118 +2B63)
    EDGE = (0x00, 0x01, 0x03, 0x04, 0x05, 0x0F, 0x10, 0x7F, 0x80, 0x84, 0x85, 0xFE, 0xFF)
    def adapt(tag, slot, **kv):
        add(f"adapt{tag}-" + "-".join(f"{v:02x}" for v in kv.values()), slot, poke=[(DS, a, bytes([v])) for a, v in zip(ADDR[tag], kv.values())])
    ADDR = {"011d": (0x6825, 0x6822), "0c06": (0x6816, 0x6822), "0c15": (0x6816, 0x6822), "171f": (0x6822, 0x681E), "10c1": (0x681A,), "25a2": (0x6819, 0x6818), "2a6d": (0x6819, 0x6818), "2b63": (0x6819, 0x6818), "03e1": (0x6819, 0x6818, 0x6816), "06e4": (0x6819, 0x6818, 0x6816), "0a3d": (0x6818,), "2a28": (0x6819, 0x6818, 0x681C)}
    for a in EDGE:
        for b in (0x00, 0x01, 0x7F, 0x80, 0xFF):
            adapt("011d", 2, a=a, b=b); adapt("0c06", 35, a=a, b=b); adapt("0c15", 36, a=a, b=b)
        for b in (0x00, 0x01, 0x55):
            adapt("171f", 71, a=a, b=b)
        adapt("10c1", 42, a=a)
    for tag, slot, mask in (("25a2", 93, 0x01), ("2a6d", 114, 0x04), ("2b63", 118, 0x02)):
        for gate in (0x00, mask, 0xFF ^ mask, 0xFF, 0x80 | mask, 0x40):
            for a in (0x00, 0x01, 0x02, 0x03, 0x7F, 0x80, 0xFE, 0xFF):
                adapt(tag, slot, g=gate, a=a)
    # phase 2, batch S (slots 10 +03E1, 20 +06E4, 29 +0A3D, 113 +2A28)
    for tag, slot, mask in (("03e1", 10, 0x02), ("06e4", 20, 0x01)):
        for gate in (0x00, mask, 0xFF ^ mask, 0xFF):
            for a in (0x00, 0x01, 0x03, 0x80, 0xFF):
                for c3 in (0x00, 0x0D, 0x0F, 0x7D, 0x7E, 0xFD, 0xFF):
                    adapt(tag, slot, g=gate, a=a, c=c3)
    for a in range(256):
        adapt("0a3d", 29, a=a)
    for gate in (0x00, 0x01, 0xFE, 0xFF):
        for a in (0x00, 0x01, 0x02, 0x05, 0x80, 0xFF):
            for lim in (0x00, 0x01, 0x02, 0x03, 0x05, 0x10, 0x7F, 0x80, 0xFF):
                adapt("2a28", 113, g=gate, a=a, l=lim)
    # phase 2, batch P1 (pointer handlers: slots 3 +013B, 14 +05D6, 17 +064E, 33 +0BAC, 37 +0C26, 58 +1452, 89 +2490, 98 +2700). The record/source/P/Q live in the arena segments below; offsets near FFFFh
    # make the original's 16-bit ES:[DI+k] wrap at 64 KiB inside the segment (the adapters copy in and out with that wrap).
    RSEG, PSEG, QSEG, SSEG = 0x6000, 0x6400, 0x6800, 0x6C00
    def pk(seg, off, vals):
        return [(seg, (off + k) & 0xFFFF, bytes([v])) for k, v in vals.items()]
    def ds(vals):
        return [(DS, a, bytes([v])) for a, v in vals.items()]
    def padd(tag, slot, label, words, poke):
        add(f"adapt{tag}-{label}", slot, words=words, poke=poke)
    for s3 in (0x00, 0x01, 0x7F, 0x80, 0xFF, 0xA5):
        for lab, roff, soff, sseg in (("a", 0x0100, 0x0200, SSEG), ("wrec", 0xFFF0, 0x0200, SSEG), ("wsrc", 0x0100, 0xFFFE, SSEG), ("alias", 0x0100, 0x0112, RSEG)):
            padd("05d6", 14, f"{s3:02x}-{lab}", [0x5A, soff, sseg, roff, RSEG], pk(sseg, soff, {3: s3}) + pk(RSEG, roff, {0x15: 0x55}) * (lab != "alias"))
    for r111 in (0x00, 0x38, 0x39, 0x3A, 0xFF):
        for v16 in (0x00, 0x7F, 0xFF):
            for v1b in (0x0E, 0x0F, 0x10, 0x00, 0xFF):
                for lab, roff in (("a", 0x0100), ("w", 0xFF00)):
                    padd("064e", 17, f"{r111:02x}-{v16:02x}-{v1b:02x}-{lab}", [0x5A, 0x0100, SSEG, roff, RSEG], pk(RSEG, roff, {0x111: r111}) + ds({0x6816: v16, 0x681B: v1b, 0x6818: 0x33}))
    R112, V22 = (0x00, 0x04, 0xFC), (0x00, 0x04, 0xFF, 0x80)
    i = 0
    for r111 in (0x00, 0x03, 0x04, 0x05, 0x80, 0xFF):
        for v16 in (0x00, 0x04, 0x05, 0x7C, 0x80, 0xFF):
            lab, roff = (("a", 0x0100), ("w", 0xFEFF))[i % 2]
            padd("0bac", 33, f"{r111:02x}-{v16:02x}-{lab}", [0x5A, 0x0100, SSEG, roff, RSEG], pk(RSEG, roff, {0x111: r111, 0x112: R112[i % 3]}) + ds({0x6816: v16, 0x6822: V22[i % 4]}))
            i += 1
    for p3 in (0x00, 0x01, 0x7F, 0x80, 0xFF, 0x40):
        for poff in (0x0300, 0xFFFE, 0x0000):
            for roff in (0x0100, 0xFFF8):
                padd("0c26", 37, f"{p3:02x}-{poff:04x}-{roff:04x}", [0x5A, 0x0100, SSEG, roff, RSEG], pk(RSEG, roff, {0x108: poff & 0xFF, 0x109: poff >> 8, 0x10A: PSEG & 0xFF, 0x10B: PSEG >> 8}) + pk(PSEG, poff, {3: p3}) + ds({0x681E: 0x11, 0x6822: 0x22}))
    for v1d in (0x00, 0x01, 0xFF):
        for poff in (0x0300, 0xFFFC, 0xFFFA):
            for roff in (0x0100, 0xFFF8):
                padd("1452", 58, f"{v1d:02x}-{poff:04x}-{roff:04x}", [0x5A, 0x0100, SSEG, roff, RSEG], pk(RSEG, roff, {0x108: poff & 0xFF, 0x109: poff >> 8, 0x10A: PSEG & 0xFF, 0x10B: PSEG >> 8}) + pk(PSEG, poff, {6: 0x77}) + ds({0x681D: v1d, 0x681A: 0x5A}))
    for s3 in (0x00, 0x0F, 0x10, 0x1F, 0x11, 0x80, 0xF0, 0xFF, 0x7F, 0x20):
        for v3, v22 in ((0, 0), (0, 1), (1, 0), (0, 0xFF), (1, 0xFF)):
            for soff in (0x0100, 0xFFFE):
                padd("2490", 89, f"{s3:02x}-{v3:02x}-{v22:02x}-{soff:04x}", [0x5A, soff, SSEG, 0x0100, RSEG], pk(SSEG, soff, {3: s3}) + ds({0x6DE3: v3, 0x6822: v22}))
    for r11b in (0x00, 0x10, 0xFC, 0xFD, 0xFE, 0xFF, 0x7F, 0x80):
        for r32 in (0x00, 0x10, 0x11, 0x7F, 0x80, 0xFF):
            for roff in (0x0100, 0xFF00):
                padd("2700", 98, f"{r11b:02x}-{r32:02x}-{roff:04x}", [0x5A, 0x0100, SSEG, roff, RSEG], pk(RSEG, roff, {0x11B: r11b, 0x32: r32}))
    for q9f in (0x03, 0x04, 0x05, 0x00, 0xFF):
        for v22, v18 in ((0xFE, 0xFE), (0xFF, 0x7F), (0x7E, 0xFF), (0x00, 0x00), (0x7F, 0x7E)):
            for lab, poff, qoff, roff in (("a", 0x0300, 0x0400, 0x0100), ("wp", 0xFFF8, 0x0400, 0x0100), ("wq", 0x0300, 0xFF70, 0x0100), ("wr", 0x0300, 0x0400, 0xFEFF), ("wr2", 0x0300, 0x0400, 0xFFF8)):
                pokes = pk(RSEG, roff, {0x108: poff & 0xFF, 0x109: poff >> 8, 0x10A: PSEG & 0xFF, 0x10B: PSEG >> 8})
                pokes += pk(PSEG, poff, {0xA: qoff & 0xFF, 0xB: qoff >> 8, 0xC: QSEG & 0xFF, 0xD: QSEG >> 8}) + pk(QSEG, qoff, {0x9F: q9f}) + ds({0x6822: v22, 0x6818: v18})
                padd("013b", 3, f"{q9f:02x}-{v22:02x}-{v18:02x}-{lab}", [0x5A, 0x0100, SSEG, roff, RSEG], pokes)
    # phase 2, batch P2 (slots 8 and 45 +0377, 9 and 46 +03AC): the GLOBAL active-character pointer DS:[5D92]/[5D94] and the character record byte +0A0h against three constants
    for tag, slots, ks, vals in (("0377", (8, 45), (2, 5, 8), (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xFF, 0x82, 0x85, 0x88)), ("03ac", (9, 46), (0, 3, 6), (0, 1, 2, 3, 4, 5, 6, 7, 0x80, 0x83, 0x86, 0xFF))):
        for si, slot in enumerate(slots):
            sel = vals if si == 0 else (ks[0], ks[2], 1, 0x81)
            for a0 in sel:
                for v22 in ((0x00, 0x02, 0x03, 0x81, 0x82, 0xFF) if si == 0 else (0x02, 0x80)):
                    for lab, roff in (("a", 0x0100), ("w", 0xFF70)):
                        padd(tag, slot, f"s{slot}-{a0:02x}-{v22:02x}-{lab}", [0x5A, 0x0100, SSEG, 0x0100, RSEG], pk(RSEG, roff, {0xA0: a0}) + ds({0x5D92: roff & 0xFF, 0x5D93: roff >> 8, 0x5D94: RSEG & 0xFF, 0x5D95: RSEG >> 8, 0x6822: v22, 0x6816: 0x7F}))
    # +0918 (slot 25): the active character DS:[5D92]/[5D94] with a sidecar list (head at +7Fh/+81h, 9-byte nodes: type at +0, next far pointer at +5/+7, nil-terminated); the handler looks up key 18h through the
    # resident BA:2199 (the original really runs the resident code). Layouts: plain, character offset FF90h (+7Fh/+81h wrap), node offsets near FFFFh (+5/+7 wrap).
    def sidecar(label, types, layout, v22):
        coff = 0xFF90 if layout == "cw" else 0x0100
        noff = [(0xFFFC - 0x20 * j) & 0xFFFF if layout == "nw" else 0x2000 + 0x40 * j for j in range(len(types))]
        pokes = []
        if types:
            pokes += pk(RSEG, coff, {0x7F: noff[0] & 0xFF, 0x80: noff[0] >> 8, 0x81: PSEG & 0xFF, 0x82: PSEG >> 8})
        else:
            pokes += pk(RSEG, coff, {0x7F: 0, 0x80: 0, 0x81: 0, 0x82: 0})
        for j, t in enumerate(types):
            nxt, nseg = (noff[j + 1], PSEG) if j + 1 < len(types) else (0, 0)
            pokes += pk(PSEG, noff[j], {0: t, 5: nxt & 0xFF, 6: nxt >> 8, 7: nseg & 0xFF, 8: nseg >> 8})
        pokes += ds({0x5D92: coff & 0xFF, 0x5D93: coff >> 8, 0x5D94: RSEG & 0xFF, 0x5D95: RSEG >> 8, 0x6822: v22, 0x681E: 0x55})
        padd("0918", 25, f"{label}-{layout}-{v22:02x}", [0x5A, 0x0100, SSEG, 0x0100, RSEG], pokes)
    for label, types in (("e", []), ("hit1", [0x18]), ("miss1", [0x17]), ("m-h", [0x17, 0x18]), ("h-m", [0x18, 0x17]), ("mmh", [0x17, 0x17, 0x18]), ("mmm", [0x17, 0x00, 0xFF]), ("hh", [0x18, 0x18])):
        for layout in ("a", "cw", "nw"):
            for v22 in (0x00, 0x04, 0x05, 0x83, 0xFF):
                sidecar(label, types, layout, v22)
    import itertools
    for n in (3, 4):
        for combo in itertools.product((0x18, 0x17), repeat=n):
            for layout in ("a", "nw"):
                for v22 in (0x00, 0xFF):
                    sidecar("seq" + "".join("H" if t == 0x18 else "m" for t in combo), list(combo), layout, v22)
    # wiring batch W1: slots 49 +12B2, 94 +25BC, 122 +2D38 (credited CPU-level models that already compose their callees)
    for s3 in (0x00, 0x10, 0x1F, 0x0F, 0xFF):
        for r10e in (0, 1, 2):
            for v22 in (0x00, 0x80, 0xFF):
                for lab, roff, soff in (("a", 0x0100, 0x0200), ("w", 0xFF00, 0xFFFE)):
                    padd("12b2", 49, f"{s3:02x}-{r10e:02x}-{v22:02x}-{lab}", [0x5A, soff, SSEG, roff, RSEG], pk(SSEG, soff, {3: s3}) + pk(RSEG, roff, {0x10E: r10e}) + ds({0x6822: v22, 0x6816: 0x7F}))
    for v in (0x00, 0x01, 0x80, 0x81, 0x7E, 0xFF):
        for v18 in (0x00, 0x01, 0x02, 0x81, 0xFF):
            add(f"adapt25bc-{v:02x}-{v18:02x}", 94, poke=[(DS, 0x5589 + 16 * i, bytes([v])) for i in range(256)] + ds({0x6818: v18}))
    for p2e in (0x56, 0x55, 0x00):
        for v19 in (0x00, 0x01, 0x08, 0x09, 0x06):
            for seed in (0x0000, 0x0001, 0x1234, 0xFFFF):
                padd("2d38", 122, f"{p2e:02x}-{v19:02x}-{seed:04x}", [0x5A, 0x0100, SSEG, 0x0100, RSEG],
                     pk(RSEG, 0x0100, {0xCC: 0x00, 0xCD: 0x03, 0xCE: PSEG & 0xFF, 0xCF: PSEG >> 8}) + pk(PSEG, 0x0300, {0x2E: p2e}) + ds({0x5D92: 0x00, 0x5D93: 0x01, 0x5D94: RSEG & 0xFF, 0x5D95: RSEG >> 8, 0x6819: v19, 0x4428: seed & 0xFF, 0x4429: seed >> 8}))
    for v19 in (0x00, 0x01, 0x08, 0x09, 0x06):
        padd("2d38", 122, f"nil-{v19:02x}", [0x5A, 0x0100, SSEG, 0x0100, RSEG], pk(RSEG, 0x0100, {0xCC: 0, 0xCD: 0, 0xCE: 0, 0xCF: 0}) + ds({0x5D92: 0x00, 0x5D93: 0x01, 0x5D94: RSEG & 0xFF, 0x5D95: RSEG >> 8, 0x6819: v19}))
    # +04BA (slots 12 and 38): the record at [BP+0C] has a candidate list (head at +7Fh/+81h, nodes with the next far pointer at +5/+7, nil-terminated) and the source node at [BP+8]. The random worlds are cyclic
    # garbage on which the original never terminates (half are skipped), so the evidence here is CONSTRUCTED lists: node types 0Ch/26h are candidates, others are skipped; the source is a separate node with
    # source[3] <= 7Fh (processed) or > 7Fh (the early exit), or the first list node (skipped as the source itself).
    import itertools
    def cand(label, slot, types, srcmode, layout):
        roff = 0x0100
        noff = [(0xFFFC - 0x20 * j) & 0xFFFF if layout == "nw" else 0x2000 + 0x40 * j for j in range(len(types))]
        pokes = pk(RSEG, roff, {0x7F: (noff[0] & 0xFF) if types else 0, 0x80: (noff[0] >> 8) if types else 0, 0x81: PSEG & 0xFF if types else 0, 0x82: PSEG >> 8 if types else 0})
        for j, t in enumerate(types):
            nxt, nseg = (noff[j + 1], PSEG) if j + 1 < len(types) else (0, 0)
            pokes += pk(PSEG, noff[j], {0: t, 5: nxt & 0xFF, 6: nxt >> 8, 7: nseg & 0xFF, 8: nseg >> 8})
        if srcmode == "first" and types:
            soff, sseg = noff[0], PSEG
            pokes += pk(PSEG, soff, {3: 0x05})
        else:
            soff, sseg = 0x0600, SSEG
            pokes += pk(SSEG, soff, {3: 0x00 if srcmode == "lo" else 0x80})
        padd("04ba", slot, f"{label}-{srcmode}-{layout}", [0x5A, soff, sseg, roff, RSEG], pokes)
    for n in (0, 1, 2, 3):
        for combo in itertools.product((0x0C, 0x26, 0x17), repeat=n):
            label = "t" + "".join({0x0C: "c", 0x26: "q", 0x17: "x"}[t] for t in combo)
            for srcmode in ("lo", "hi", "first"):
                for layout in ("a", "nw"):
                    cand(label, 12, list(combo), srcmode, layout)
    for combo in ((0x0C, 0x26), (0x26, 0x0C, 0x17), (0x17,)):
        label = "d" + "".join({0x0C: "c", 0x26: "q", 0x17: "x"}[t] for t in combo)
        for srcmode in ("lo", "first"):
            cand(label, 38, list(combo), srcmode, "a")
    # t75: explicit separated objects, including wrapping effective addresses
    # and word operands whose first byte is at FFFFh (second byte contiguous).
    def far_poke(seg, off, field, target):
        ts, to = target
        if field == 0x108:  # LES reads one contiguous four-byte operand.
            return [(seg, (off + field) & 0xFFFF, struct.pack('<HH', to, ts))]
        return [(seg, (off + field) & 0xFFFF, struct.pack('<H', to)),
                (seg, (off + field + 2) & 0xFFFF, struct.pack('<H', ts))]

    layouts = (("plain", 0x0100, 0xAB00, 0x0400),
               ("wrap", 0xFEFF, 0xFFF8, 0xFF70),
               ("word", 0xFEF7, 0xFFF5, 0xFF37))
    for label, ro, po, qo in layouts:
        chain = far_poke(RSEG, ro, 0x108, (PSEG, po)) + far_poke(PSEG, po, 0xA, (QSEG, qo))
        for v in (0, 4, 9, 10, 12, 0x80, 0xFF):
            for before in (0, 0x0F, 0x7E, 0x80, 0xFE, 0xFF):
                padd('01c3', 6, f'{label}-{v:02x}-{before:02x}', [0, 0x100, SSEG, ro, RSEG],
                     chain + pk(QSEG, qo, {0x9F: v}) + ds({0x6818: before, 0x6822: before, 0x6819: 0x55}))
    # The local belongs to the handler: dispatcher entry SP - 18 is BP,
    # so [BP-1] is entry SP - 19, including when the record crosses FFFFh.
    for tag, slot, gates in (('24d9', 90, (0, 1, 0x80)), ('268a', 97, (2, 4, 0, 0x80))):
        for gate in gates:
            for value in (0, 3, 4, 6, 7, 10, 11, 13, 14, 17, 18, 20, 21, 0xFF):
                for residue in (0, 0xFF):
                    for before in (0x0F, 0xFF):
                        ro = 0xFFF0 if residue else 0x100
                        padd(tag, slot, f'{gate:02x}-{value:02x}-{residue:02x}-{before:02x}',
                             [0, 0x100, SSEG, ro, RSEG], pk(RSEG, ro, {0x14: value}) +
                             ds({0x682A: gate, 0x6816: before}) + [(SS, 0x8000 - 19, bytes([residue]))])
    for label, ro, po, qo in layouts:
        for count in (0, 1, 2, 6):
            for pattern in ('inactive', 'miss', '76-first', '76-last', '98-fc', '98-miss'):
                for preset in (0, 0x80):
                    ns = SSEG
                    offsets = [(0xFFD5 if label == 'word' else 0xFFD0) if j == 0 and label != 'plain'
                               else 0x200 + 0x80 * j for j in range(count)]
                    chain = far_poke(RSEG, ro, 0x108, (PSEG, po)) + far_poke(PSEG, po, 0xA, (QSEG, qo))
                    chain += far_poke(QSEG, qo, 0xC8, (ns, offsets[0]) if count else (0, 0))
                    for j, no in enumerate(offsets):
                        vals = {0x34: 0 if pattern == 'inactive' else 1, 0x2F: 0, 0x30: 0, 0x31: 0}
                        if pattern == '76-first' and j == 0: vals[0x2F] = 0x76
                        if pattern == '76-last' and j == count - 1: vals[0x31] = 0x76
                        if pattern.startswith('98') and j == count - 1:
                            vals[0x30] = 0x98; vals[0x31] = 0xFC if pattern == '98-fc' else 0xFB
                        chain += pk(ns, no, vals) + far_poke(ns, no, 0x2A, (ns, offsets[j + 1]) if j + 1 < count else (0, 0))
                    padd('2e37', 126, f'{label}-{count}-{pattern}-{preset:02x}', [0, 0x100, SSEG, ro, RSEG],
                         chain + ds({0x681E: preset}))
    # Slot 11: every branch including the real resident party-count closure.
    for sel in (0, 1, 0x80):
        for a3 in (0, 0x20, 0x40, 0x80, 0xFF):
            for b10e in (0, 1, 0x80):
                for b84 in (0x7F, 0x80, 0xB3):
                    chain = pk(SSEG, 0x100, {3: a3}) + pk(RSEG, 0x100, {0x10E: b10e, 0x84: b84})
                    chain += far_poke(RSEG, 0x100, 0x108, (PSEG, 0x100))
                    # Two party nodes, one contributes at offset 0 and one at 1.
                    chain += far_poke(DS, 0, 0x5D96, (QSEG, 0x100))
                    chain += pk(QSEG, 0x100, {0x10D: 0, 0x10E: 0}) + far_poke(QSEG, 0x100, 0x104, (QSEG, 0x300))
                    chain += pk(QSEG, 0x300, {0x10D: 0, 0x10E: 1}) + far_poke(QSEG, 0x300, 0x104, (0, 0))
                    padd('0400', 11, f'{sel:02x}-{a3:02x}-{b10e:02x}-{b84:02x}', [sel, 0x100, SSEG, 0x100, RSEG], chain)
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
    t0 = time.time(); stops = {}; seen_all = set(); failures = []; skipped = []; per_slot = {}; split_states = 0; code_written = []
    cs = [c for c in cases(src) if only is None or c[0] in only or any(c[0].startswith(o) for o in only)]
    for index, (name, idx, kw, budget) in enumerate(cs):
        kw = dict(kw); budget = kw.pop('budget', budget)
        if index % 40 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        memory, cpu = fixture(base, random.Random(zlib.crc32(name.encode()) ^ 0x9E0001), idx, **kw)     # per-case seed: the world of a case does not depend on which other cases ran
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
        if extra.get('code_written'):
            code_written.append(name)
    return dict(cases=len(cs), compared=len(cs) - len(skipped) - len(failures), end_states=stops, dispatcher_starts=f'{len(seen_all)}/16', skipped_nonterminating=len(skipped),
                skipped=skipped[:200], failures=failures, split_states=split_states, code_written_equal=code_written[:20], per_slot={s: sorted(set(v)) for s, v in sorted(per_slot.items())}, seconds=round(time.time() - t0, 1))


# ---------------------------------------------------------------------------------------------------- mutation
EQUIVALENT = {      # a survivor named here carries a written proof
    '+0C15/+171F SUB AF from a ^ r only': 'the subtrahend is the constant 4 (bit 4 clear) in both handlers, so (a ^ 4 ^ r) & 10h equals (a ^ r) & 10h for every byte',
    '+25A2/+2A6D/+2B63 AH is not cleared': 'AH is already zero at the handler: the dispatcher body executes XOR AH,AH before the LCALL and nothing between it and these handlers writes AH, so XOR AH,AH inside the handler (modelled by the explicit clear) changes nothing the caller can see',
    '+10C1 AX keeps AH': 'the same proof: the dispatcher\'s XOR AH,AH before the LCALL leaves AH = 0, so the handler\'s own XOR AH,AH is a no-op for AH',
    '+0A3D AX is the low byte only (AH kept)': 'AH is already zero at the handler (the dispatcher body executes XOR AH,AH before the LCALL and nothing in between writes AH), and the result a - (a >> 2) is at most 255, so writing the whole AX or only AL leaves the same AX',
    '+0A3D SUB OF omitted': 'both operands are at most 255 and the result at most 255, so bit 15 of (a ^ b) & (a ^ r) is 0 for every byte: the 16-bit SUB never sets OF here',
    '+2A28 the limit-0 early exit leaves AL = 0': 'on the limit-0 path the gate [6819]&1 is set, so the first line already wrote AL = 1 (the masked bit) and the original MOV AL,1 writes the same value: removing the second write changes nothing the caller can see',
    '+0A3D SUB CF omitted': 'the subtrahend is a >> 2, never larger than a, so a - (a >> 2) never borrows: the original CF is 0 for every byte and the model computes 0 with or without the borrow rule',
    '+03E1/+06E4 ADD CF from the wrong comparison': 'the addend is the constant 3, so the byte result r never equals a: r < a and r <= a are the same test for every byte',
    '+03E1/+06E4 ADD AF from a ^ r only': 'the addend is the constant 3 (bit 4 clear), so (a ^ 3 ^ r) & 10h equals (a ^ r) & 10h for every byte',
    '+0C06 first INC flags dropped': 'the second INC byte [6822] (no instruction between) overwrites AF, OF, SF, ZF and PF and both INCs preserve CF, so the first INC\'s flags are dead at the return in the original and in the model',
    '+0C15 first SUB flags dropped': 'the second SUB byte [6816],4 overwrites every arithmetic flag (CF, PF, AF, ZF, SF, OF) with nothing between them, so the first SUB\'s flags are dead at the return',
    '+25A2/+2A6D/+2B63 the OR flags put SF at bit 6': 'the OR AL,AL result is the masked bit (0 or the mask 01h/04h/02h): bit 6 and bit 7 are both clear for every value it can take, so SF is 0 whichever bit is tested',
    '+010D ADD computes AF from the wrong operand': 'the INC byte [6822] that follows the ADD (no instruction between them) overwrites AF, OF, SF, ZF and PF; only the ADD\'s CF reaches the return. The ADD\'s non-CF flags are therefore dead at the handler\'s return in the original and in the model, and any change to how add8_flags computes them is unobservable (also: b is always 5, whose bit 4 is 0)',
    '+010D ADD tests AF at bit 5, not bit 4': 'the same proof: the ADD\'s AF is overwritten by the following INC before the return, so no AF rule inside add8_flags is observable',
}

MUTANTS = (
    ('t75 +01C3 drops the local class byte', [('m->ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)klass;', '(void)klass;')]),
    ('t75 +01C3 clears AH', [('set_al_(c, (uint8_t)klass); add8_flags(c, a6818, (uint8_t)klass);', 'c->ax = (uint16_t)klass; add8_flags(c, a6818, (uint8_t)klass);')]),
    ('t75 +01C3 flags use the stored value', [('add8_flags(c, a6818, (uint8_t)klass);', 'add8_flags(c, st.b6818, (uint8_t)klass);')]),
    ('t75 +01C3 skips semantic effects', [('pr_u38_entry6(&st, rec, chain_resolve, &x, &klass) != PR_U38_OK', '(void)pr_u38_entry6, 0')]),
    ('t75 chain word reads wrap within the segment', [('out[k + 1u] = (uint8_t)(v >> 8u);', 'out[k + 1u] = xb_byte(m, seg, off, k + 1u);')]),
    ('t75 chain LES wraps within the segment', [('rec[0x108u + k] = m->ram[(at + k) & MASK];', 'rec[0x108u + k] = xb_byte(m, seg, off, 0x108u + k);')]),
    ('t75 +24D9 uses the 268A gate', [('static int h_24d9(call_t *m) { return range_adapter(m, 0); }', 'static int h_24d9(call_t *m) { return range_adapter(m, 1); }')]),
    ('t75 +268A uses the 24D9 gate', [('static int h_268a(call_t *m) { return range_adapter(m, 1); }', 'static int h_268a(call_t *m) { return range_adapter(m, 0); }')]),
    ('t75 range residue replaced by zero', [('residue = rd8(m->ram, c->ss, (uint16_t)(c->bp - 1u));', 'residue = 0;')]),
    ('t75 range loses AH of the sum', [('c->ax = (uint16_t)(before + bracket);', 'c->ax = (uint8_t)(before + bracket);')]),
    ('t75 range uses byte ADD flags', [('add16_flags(c, before, (uint16_t)bracket);', 'add8_flags(c, before, (uint8_t)bracket);')]),
    ('t75 range fails to store bracket local', [('m->ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)bracket;', '(void)bracket;')]),
    ('t75 +2E37 inner counter stops at two', [('m->ram[physical(c->ss, (uint16_t)(c->bp - 5u))] = 3u;', 'm->ram[physical(c->ss, (uint16_t)(c->bp - 5u))] = 2u;')]),
    ('t75 +2E37 loses the link segment local', [('wr16(m->ram, c->ss, (uint16_t)(c->bp - 2u), c->dx);', '(void)c->dx;')]),
    ('t75 +2E37 zeroes DX', [('c->ax = (uint16_t)(c->ax | c->dx); /* top-of-loop OR, including nil */', 'c->ax = (uint16_t)(c->ax | c->dx); c->dx = 0; /* top-of-loop OR, including nil */')]),
    ('t75 +2E37 caps the walk at four nodes', [('m->lim->loopbacks, &nodes, &matched);', '4u, &nodes, &matched);')]),
    ('t75 +2E37 tests flag before nil', [('if (c->ax == 0u) logic16_flags(c, c->ax); else sub8_flags(c, st.b681e, 0u);', 'if (st.b681e) sub8_flags(c, st.b681e, 0u); else logic16_flags(c, c->ax);')]),
    ('t75 +2E37 omits semantic flag writeback', [('ds_set8(m->ram, c, 0x681Eu, st.b681e);', '(void)st.b681e;')]),
    ('t75 +0400 routes to NOP', [('{0x0400, h_0400}', '{0x0400, h_nop}')]),
    ('t75 +0400 gives party count zero budget', [('pr_u38_handler_0400_cpu(m->ram, m->size, &x, LIM->loopbacks, &m->status_raw)', 'pr_u38_handler_0400_cpu(m->ram, m->size, &x, 0, &m->status_raw)')]),
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
    ('+010D INC clears CF instead of preserving it', [('    if (a == 0x7Fu) f |= 0x0800u;\n    c->flags = (uint16_t)((c->flags & ~(ARITH & ~0x0001u)) | f);   /* CF is preserved */', '    if (a == 0x7Fu) f |= 0x0800u;\n    c->flags = (uint16_t)((c->flags & ~ARITH) | f);')]),
    ('+010D ADD computes AF from the wrong operand', [("    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)", "    if ((a ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)")]),
    ('+010D ADD tests AF at bit 5, not bit 4', [("    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)", "    if ((a ^ b ^ r) & 0x20u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)")]),
    ('+010D leaves the saved BP unwritten (no PUSH BP write)', [("static void frame_enter(call_t *m) { push(m->ram, m->cpu, m->cpu->bp); m->cpu->bp = m->cpu->sp; }", "static void frame_enter(call_t *m) { m->cpu->sp = (uint16_t)(m->cpu->sp - 2u); m->cpu->bp = m->cpu->sp; }")]),
    ('+010D pops 8 argument bytes, not 0Ah', [("frame_leave(m, 0x0Au);\n    return 0;\n}\n\n\nstatic void sub8_flags", "frame_leave(m, 0x08u);\n    return 0;\n}\n\n\nstatic void sub8_flags")]),
    ('+010D skips the semantic model (the DS bytes stay)', [("if (pr_u38_entry2(&st) != PR_U38_OK) return 1;", "(void)pr_u38_entry2;")]),
    ('+011D CF test polarity (a < 5 becomes a <= 5)', [('(a6825 < 5u ? 0x0001u : 0u)', '(a6825 <= 5u ? 0x0001u : 0u)')]),
    ('+011D DEC clears CF instead of preserving it', [('(ARITH & ~0x0001u)) | f);   /* CF is preserved */\n}\nstatic void logic8_flags', 'ARITH) | f);\n}\nstatic void logic8_flags')]),
    ('+011D DEC sets OF at 7Fh, not 80h', [('if (a == 0x80u) f |= 0x0800u;', 'if (a == 0x7Fu) f |= 0x0800u;')]),
    ('+011D DEC flags from the stored value, not the original', [('dec8_flags(m->cpu, a6822);', 'dec8_flags(m->cpu, st.b6822);')]),
    ('+011D skips the semantic model', [('pr_u38_entry3(&st, &paths) != PR_U38_OK', '(void)pr_u38_entry3, 0')]),
    ('+011D pops 8 argument bytes, not 0Ah', [('frame_leave(m, 0x0Au);\n    return 0;\n}\n/* +0C06', 'frame_leave(m, 0x08u);\n    return 0;\n}\n/* +0C06')]),
    ('+0C06 flags come from the [6816] INC, not the [6822] INC', [('inc8_flags(m->cpu, a6816); inc8_flags(m->cpu, a6822);', 'inc8_flags(m->cpu, a6822); inc8_flags(m->cpu, a6816);')]),
    ('+0C06 first INC flags dropped', [('inc8_flags(m->cpu, a6816); inc8_flags(m->cpu, a6822);', '(void)a6816; inc8_flags(m->cpu, a6822);')]),
    ('+0C06 skips the semantic model', [('pr_rec_u38_entry30(&mods) != PR_REC_EFFECT_OK', '(void)pr_rec_u38_entry30, 0')]),
    ('+0C06 pops 8 argument bytes, not 0Ah', [('frame_leave(m, 0x0Au);\n    return 0;\n}\n/* +0C15', 'frame_leave(m, 0x08u);\n    return 0;\n}\n/* +0C15')]),
    ('+0C15 flags come from the [6822] SUB, not the [6816] SUB', [('sub8_flags(m->cpu, a6822, 4u); sub8_flags(m->cpu, a6816, 4u);', 'sub8_flags(m->cpu, a6816, 4u); sub8_flags(m->cpu, a6822, 4u);')]),
    ('+0C15 first SUB flags dropped', [('sub8_flags(m->cpu, a6822, 4u); sub8_flags(m->cpu, a6816, 4u);', '(void)a6822; sub8_flags(m->cpu, a6816, 4u);')]),
    ('+0C15 skips the semantic model', [('pr_rec_u38_entry31(&mods) != PR_REC_EFFECT_OK', '(void)pr_rec_u38_entry31, 0')]),
    ('+0C15 pops 8 argument bytes, not 0Ah', [('frame_leave(m, 0x0Au);\n    return 0;\n}\n/* +171F', 'frame_leave(m, 0x08u);\n    return 0;\n}\n/* +171F')]),
    ('+0C15/+171F SUB borrow test a <= b (CF)', [('    if (a < b) f |= 0x0001u;\n    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u)', '    if (a <= b) f |= 0x0001u;\n    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u)')]),
    ('+0C15/+171F SUB OF from the wrong operand sign', [('if ((uint8_t)((a ^ b) & (a ^ r)) & 0x80u) f |= 0x0800u;', 'if ((uint8_t)((a ^ r)) & 0x80u) f |= 0x0800u;')]),
    ('+0C15/+171F SUB AF from a ^ r only', [('    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)((a ^ b) &', '    if ((a ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)((a ^ b) &')]),
    ('+171F skips the semantic model', [('pr_u38_entry59(&st) != PR_U38_OK', '(void)pr_u38_entry59, 0')]),
    ('+171F SUB flags from the stored value', [('sub8_flags(m->cpu, a6822, 4u);\n    frame_leave(m, 0x0Au);\n    return 0;\n}\n/* +25A2', 'sub8_flags(m->cpu, st.b6822, 4u);\n    frame_leave(m, 0x0Au);\n    return 0;\n}\n/* +25A2')]),
    ('+171F pops 8 argument bytes, not 0Ah', [('frame_leave(m, 0x0Au);\n    return 0;\n}\n/* +25A2', 'frame_leave(m, 0x08u);\n    return 0;\n}\n/* +25A2')]),
    ('+25A2/+2A6D/+2B63 the zero-gate AL is not written', [('logic8_flags(m->cpu, masked); set_al_(m->cpu, masked);', 'logic8_flags(m->cpu, masked);')]),
    ('+25A2/+2A6D/+2B63 AH is not cleared', [('m->cpu->ax = (uint16_t)(a6818 >> 1); shr16_flags(m->cpu, a6818);', 'm->cpu->ax = (uint16_t)((m->cpu->ax & 0xFF00u) | (a6818 >> 1)); shr16_flags(m->cpu, a6818);')]),
    ('+25A2/+2A6D/+2B63 the OR flags omit ZF', [('    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    c->flags = (uint16_t)((c->flags & ~ARITH) | f);\n}\nstatic void shr16_flags', '    if (r & 0x80u) f |= 0x0080u;\n    c->flags = (uint16_t)((c->flags & ~ARITH) | f);\n}\nstatic void shr16_flags')]),
    ('+25A2/+2A6D/+2B63 the OR flags invert PF', [('static void logic8_flags(cpu_t *c, uint8_t r)                          /* AND/OR/XOR byte: CF=OF=AF=0, ZF, SF at bit 7, PF */\n{\n    uint16_t f = parity(r);', 'static void logic8_flags(cpu_t *c, uint8_t r)                          /* AND/OR/XOR byte: CF=OF=AF=0, ZF, SF at bit 7, PF */\n{\n    uint16_t f = parity(r) ^ 0x0004u;')]),
    ('+25A2/+2A6D/+2B63 the OR flags put SF at bit 6', [('static void logic8_flags(cpu_t *c, uint8_t r)                          /* AND/OR/XOR byte: CF=OF=AF=0, ZF, SF at bit 7, PF */\n{\n    uint16_t f = parity(r);\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u)', 'static void logic8_flags(cpu_t *c, uint8_t r)                          /* AND/OR/XOR byte: CF=OF=AF=0, ZF, SF at bit 7, PF */\n{\n    uint16_t f = parity(r);\n    if (!r) f |= 0x0040u;\n    if (r & 0x40u)')]),
    ('+25A2/+2A6D/+2B63/+10C1 SHR CF from bit 1', [('    if (a & 1u) f |= 0x0001u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x8000u) f |= 0x0080u;', '    if (a & 2u) f |= 0x0001u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x8000u) f |= 0x0080u;')]),
    ('+25A2/+2A6D/+2B63/+10C1 SHR does not set AF (pinned)', [('f = (uint16_t)(parity((uint8_t)r) | 0x0010u);\n    if (a & 1u)', 'f = (uint16_t)(parity((uint8_t)r));\n    if (a & 1u)')]),
    ('+25A2/+2A6D/+2B63/+10C1 SHR OF from bit 7 of the byte', [('    if (a & 0x8000u) f |= 0x0800u;\n    c->flags = (uint16_t)((c->flags & ~ARITH) | f);\n}\nstatic void load_mods', '    if (a & 0x80u) f |= 0x0800u;\n    c->flags = (uint16_t)((c->flags & ~ARITH) | f);\n}\nstatic void load_mods')]),
    ('+25A2/+2A6D/+2B63/+10C1 SHR sets ZF when the source is zero, not the result', [('    if (a & 1u) f |= 0x0001u;\n    if (!r) f |= 0x0040u;', '    if (a & 1u) f |= 0x0001u;\n    if (!a) f |= 0x0040u;')]),
    ('+25A2 skips the semantic model', [('if (model(&st, &paths) != PR_U38_OK) return 1;', '(void)model;')]),
    ('+25A2/+2A6D/+2B63 pops 8 argument bytes, not 0Ah', [('frame_leave(m, 0x0Au);\n    return 0;\n}\nstatic int h_25a2', 'frame_leave(m, 0x08u);\n    return 0;\n}\nstatic int h_25a2')]),
    ('+2A6D tests the +25A2 mask', [('halve_adapter(m, 0x04u, pr_u38_entry98)', 'halve_adapter(m, 0x01u, pr_u38_entry98)')]),
    ('+2B63 tests the +2A6D mask', [('halve_adapter(m, 0x02u, pr_u38_entry102)', 'halve_adapter(m, 0x04u, pr_u38_entry102)')]),
    ('+25A2 tests the +2B63 mask', [('halve_adapter(m, 0x01u, pr_u38_entry77)', 'halve_adapter(m, 0x02u, pr_u38_entry77)')]),
    ('+10C1 skips the semantic model', [('pr_u38_entry36(&st) != PR_U38_OK', '(void)pr_u38_entry36, 0')]),
    ('+10C1 AX keeps AH', [('m->cpu->ax = (uint16_t)(a681a >> 1); shr16_flags(m->cpu, a681a);', 'm->cpu->ax = (uint16_t)((m->cpu->ax & 0xFF00u) | (a681a >> 1)); shr16_flags(m->cpu, a681a);')]),
    ('+10C1 pops 8 argument bytes, not 0Ah', [('frame_leave(m, 0x0Au);\n    return 0;\n}\n\n\nstatic void sub16_flags', 'frame_leave(m, 0x08u);\n    return 0;\n}\n\n\nstatic void sub16_flags')]),
    ('+03E1 tests the +06E4 mask', [('gated_halve_add3_regs(m->cpu, 0x02u,', 'gated_halve_add3_regs(m->cpu, 0x01u,')]),
    ('+06E4 tests the +03E1 mask', [('gated_halve_add3_regs(m->cpu, 0x01u,', 'gated_halve_add3_regs(m->cpu, 0x02u,')]),
    ('+03E1 skips the semantic model', [('pr_rec_u38_entry10(&mods, &applied) != PR_REC_EFFECT_OK', '(void)pr_rec_u38_entry10, 0')]),
    ('+06E4 skips the semantic model', [('pr_u38_entry18(&st, &paths) != PR_U38_OK', '(void)pr_u38_entry18, 0')]),
    ('+03E1/+06E4 AX keeps the old value on the taken path', [('if (masked) { c->ax = (uint16_t)(a6818 >> 1); add8_flags(c, a6816, 3u); }', 'if (masked) { add8_flags(c, a6816, 3u); }')]),
    ('+03E1/+06E4 the ADD flags are the SHR flags', [('if (masked) { c->ax = (uint16_t)(a6818 >> 1); add8_flags(c, a6816, 3u); }', 'if (masked) { c->ax = (uint16_t)(a6818 >> 1); shr16_flags(c, a6818); (void)a6816; }')]),
    ('+03E1/+06E4 the ADD adds 2', [('add8_flags(c, a6816, 3u); }', 'add8_flags(c, a6816, 2u); }')]),
    ('+03E1/+06E4 the gate-clear AL is not written', [('logic8_flags(c, masked); set_al_(c, masked);\n    if (masked) { c->ax', 'logic8_flags(c, masked);\n    if (masked) { c->ax')]),
    ('+03E1/+06E4 ADD CF from the wrong comparison', [('    if (r < a) f |= 0x0001u;\n    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)', '    if (r <= a) f |= 0x0001u;\n    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)')]),
    ('+03E1/+06E4 ADD AF from a ^ r only', [('    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)', '    if ((a ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)')]),
    ('+03E1/+06E4 ADD OF from the wrong sign', [('if ((uint8_t)(~(a ^ b) & (a ^ r)) & 0x80u) f |= 0x0800u;\n    c->flags = (uint16_t)((c->flags & ~ARITH) | f);\n}\nstatic void inc8_flags', 'if ((uint8_t)((a ^ r)) & 0x80u) f |= 0x0800u;\n    c->flags = (uint16_t)((c->flags & ~ARITH) | f);\n}\nstatic void inc8_flags')]),
    ('+03E1/+06E4 ADD ZF omitted', [('    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)', '    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)')]),
    ('+03E1/+06E4 ADD SF at bit 6', [('    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x80u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)', '    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x40u) f |= 0x0080u;\n    if ((uint8_t)(~(a ^ b)')]),
    ('+0A3D skips the semantic model', [('pr_u38_entry25(&st) != PR_U38_OK', '(void)pr_u38_entry25, 0')]),
    ('+0A3D DX is not set', [('m->cpu->dx = (uint16_t)(a >> 2); m->cpu->ax', 'm->cpu->ax')]),
    ('+0A3D DX is a >> 1', [('m->cpu->dx = (uint16_t)(a >> 2);', 'm->cpu->dx = (uint16_t)(a >> 1);')]),
    ('+0A3D AX is the low byte only (AH kept)', [('m->cpu->ax = (uint16_t)(a - (a >> 2));', 'set_al_(m->cpu, (uint8_t)(a - (a >> 2)));')]),
    ('+0A3D SUB flags from the 8-bit subtract', [('sub16_flags(m->cpu, a, (uint16_t)(a >> 2));', 'sub8_flags(m->cpu, a, (uint8_t)(a >> 2));')]),
    ('+0A3D SUB AF from a ^ r only', [('    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x8000u)', '    if ((a ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x8000u)')]),
    ('+0A3D SUB OF omitted', [('    if ((uint16_t)((a ^ b) & (a ^ r)) & 0x8000u) f |= 0x0800u;\n', '    if (0) f |= 0x0800u;\n')]),
    ('+0A3D SUB CF omitted', [('    if (a < b) f |= 0x0001u;\n    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x8000u)', '    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n    if (!r) f |= 0x0040u;\n    if (r & 0x8000u)')]),
    ('+0A3D SUB SF at bit 7', [('if (r & 0x8000u) f |= 0x0080u;\n    if ((uint16_t)', 'if (r & 0x80u) f |= 0x0080u;\n    if ((uint16_t)')]),
    ('+0A3D pops 8 argument bytes, not 0Ah', [('sub16_flags(m->cpu, a, (uint16_t)(a >> 2));\n    frame_leave(m, 0x0Au);', 'sub16_flags(m->cpu, a, (uint16_t)(a >> 2));\n    frame_leave(m, 0x08u);')]),
    ('+2A28 skips the semantic model', [('pr_u38_entry97(&st, &iterations) != PR_U38_OK', '(void)pr_u38_entry97, 0')]),
    ('+2A28 the limit-0 early exit leaves AL = 0', [('set_al_(c, 1u); sub8_flags(c, 1u, limit);', 'sub8_flags(c, 1u, limit);')]),
    ("+2A28 the limit-0 flags are the final CMP's (limit,limit)", [('set_al_(c, 1u); sub8_flags(c, 1u, limit);', 'set_al_(c, 1u); sub8_flags(c, limit, limit);')]),
    ('+2A28 the counter local [BP-1] is not written', [('            m->ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = limit;\n', '')]),
    ('+2A28 the limit local [BP-2] is not written', [('        m->ram[physical(c->ss, (uint16_t)(c->bp - 2u))] = limit;\n', '')]),
    ('+2A28 the counter local holds limit - 1', [('m->ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = limit;', 'm->ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)(limit - 1u);')]),
    ('+2A28 the final AL is 1', [('set_al_(c, limit); sub8_flags(c, limit, limit);', 'sub8_flags(c, limit, limit);')]),
    ('+2A28 the final CMP flags are omitted', [('set_al_(c, limit); sub8_flags(c, limit, limit);', 'set_al_(c, limit);')]),
    ('+2A28 the loop gate is mask 02h', [('logic8_flags(c, (uint8_t)(a6819 & 1u)); set_al_(c, (uint8_t)(a6819 & 1u));\n    if (a6819 & 1u) {', 'logic8_flags(c, (uint8_t)(a6819 & 2u)); set_al_(c, (uint8_t)(a6819 & 2u));\n    if (a6819 & 2u) {')]),
    ('+2A28 pops 8 argument bytes, not 0Ah', [('    frame_leave(m, 0x0Au);\n    return 0;\n}\n\n\n/* ---- batch P', '    frame_leave(m, 0x08u);\n    return 0;\n}\n\n\n/* ---- batch P')]),
    ('+P1 xb_byte reads without the 64K offset wrap', [('static uint8_t xb_byte(const call_t *m, uint16_t seg, uint16_t off, unsigned k) { return m->ram[physical(seg, (uint16_t)(off + k))]; }', 'static uint8_t xb_byte(const call_t *m, uint16_t seg, uint16_t off, unsigned k) { return m->ram[(physical(seg, off) + k) & 0xFFFFFu]; }')]),
    ('+P1 xb_flush writes back without the 64K offset wrap', [('x->m->ram[physical(x->seg[i], (uint16_t)(x->off[i] + k))] = x->cur[i][k];', 'x->m->ram[(physical(x->seg[i], x->off[i]) + k) & 0xFFFFFu] = x->cur[i][k];')]),
    ('+P1 xb_flush is skipped (the writes are lost)', [('            if (x->cur[i][k] != x->orig[i][k]) x->m->ram', '            if (0 && x->cur[i][k] != x->orig[i][k]) x->m->ram')]),
    ('+P1 ES is left unchanged', [('static void set_es_di(cpu_t *c, uint16_t seg, uint16_t off) { c->es = seg; c->di = off; }', 'static void set_es_di(cpu_t *c, uint16_t seg, uint16_t off) { (void)seg; c->di = off; }')]),
    ('+P1 DI is left unchanged', [('static void set_es_di(cpu_t *c, uint16_t seg, uint16_t off) { c->es = seg; c->di = off; }', 'static void set_es_di(cpu_t *c, uint16_t seg, uint16_t off) { (void)off; c->es = seg; }')]),
    ('+05D6 skips the semantic model', [('pr_u38_entry13(rec, src) == PR_U38_OK', '((void)pr_u38_entry13, PR_U38_OK) == PR_U38_OK')]),
    ('+05D6 AL is source[2]', [('xb_flush(x); set_al_(m->cpu, src[3]);', 'xb_flush(x); set_al_(m->cpu, src[2]);')]),
    ('+05D6 AL is not written', [('xb_flush(x); set_al_(m->cpu, src[3]);', 'xb_flush(x);')]),
    ('+05D6 the record pointer is the source pointer', [('set_al_(m->cpu, src[3]); set_es_di(m->cpu, rs, ro);', 'set_al_(m->cpu, src[3]); set_es_di(m->cpu, ss_, so);')]),
    ('+064E the CMP flags use 0Eh', [('sub8_flags(m->cpu, a681b, 0x0Fu); set_es_di(m->cpu, rs, ro);', 'sub8_flags(m->cpu, a681b, 0x0Eu); set_es_di(m->cpu, rs, ro);')]),
    ('+064E skips the semantic model', [('pr_u38_entry16(&st, rec, &paths) == PR_U38_OK', '((void)pr_u38_entry16, PR_U38_OK) == PR_U38_OK')]),
    ('+064E no flag writer (the flags stay)', [('        sub8_flags(m->cpu, a681b, 0x0Fu); set_es_di(m->cpu, rs, ro);', '        (void)a681b; set_es_di(m->cpu, rs, ro);')]),
    ('+064E pops 8 argument bytes, not 0Ah', [('        frame_leave(m, 0x0Au); rc = 0;\n    }\n    return rc;\n}\n/* +0BAC', '        frame_leave(m, 0x08u); rc = 0;\n    }\n    return rc;\n}\n/* +0BAC')]),
    ('+0BAC the SUB flags come from [6822]', [('sub8_flags(m->cpu, a6816, 4u); set_es_di(m->cpu, rs, ro);', 'sub8_flags(m->cpu, st.b6822, 4u); set_es_di(m->cpu, rs, ro);')]),
    ('+0BAC skips the semantic model', [('pr_u38_entry28(&st, rec) == PR_U38_OK', '((void)pr_u38_entry28, PR_U38_OK) == PR_U38_OK')]),
    ('+0BAC the SUB flags use the stored value', [('sub8_flags(m->cpu, a6816, 4u); set_es_di(m->cpu, rs, ro);', 'sub8_flags(m->cpu, st.b6816, 4u); set_es_di(m->cpu, rs, ro);')]),
    ('+0C26 the CMP is against 1, not 0', [('sub8_flags(m->cpu, xb_byte(m, ps, po, 3u), 0u); set_es_di(m->cpu, ps, po);', 'sub8_flags(m->cpu, xb_byte(m, ps, po, 3u), 1u); set_es_di(m->cpu, ps, po);')]),
    ('+0C26 ES:DI is the record pointer, not P', [('sub8_flags(m->cpu, xb_byte(m, ps, po, 3u), 0u); set_es_di(m->cpu, ps, po);', 'sub8_flags(m->cpu, xb_byte(m, ps, po, 3u), 0u); set_es_di(m->cpu, rs, ro);')]),
    ('+0C26 skips the semantic model', [('pr_u38_entry32(&st, rec, xb_resolve, x, &paths) == PR_U38_OK', '((void)pr_u38_entry32, PR_U38_OK) == PR_U38_OK')]),
    ('+0C26 the CMP flags are unsigned-style (SF cleared)', [('sub8_flags(m->cpu, xb_byte(m, ps, po, 3u), 0u); set_es_di(m->cpu, ps, po);', 'sub8_flags(m->cpu, xb_byte(m, ps, po, 3u), 0u); m->cpu->flags = (uint16_t)(m->cpu->flags & ~0x0080u); set_es_di(m->cpu, ps, po);')]),
    ('+1452 the CMP is against 1, not 0', [('sub8_flags(m->cpu, a681d, 0u); set_es_di(m->cpu, ps, po);', 'sub8_flags(m->cpu, a681d, 1u); set_es_di(m->cpu, ps, po);')]),
    ('+1452 ES:DI is the record pointer, not P', [('sub8_flags(m->cpu, a681d, 0u); set_es_di(m->cpu, ps, po);', 'sub8_flags(m->cpu, a681d, 0u); set_es_di(m->cpu, rs, ro);')]),
    ('+1452 skips the semantic model', [('pr_u38_entry48(&st, rec, xb_resolve, x, &paths) == PR_U38_OK', '((void)pr_u38_entry48, PR_U38_OK) == PR_U38_OK')]),
    ('+2490 path A AL is source[3] & F0h', [('if (paths == PR_U38_PATH_EFFECT) al = (uint8_t)(s3 & 0x0Fu);', 'if (paths == PR_U38_PATH_EFFECT) al = (uint8_t)(s3 & 0xF0u);')]),
    ('+2490 path B2 AL is source[3] | 01h', [('else if (paths == PR_U38_PATH_SECOND) al = (uint8_t)(s3 | 0x10u);', 'else if (paths == PR_U38_PATH_SECOND) al = (uint8_t)(s3 | 0x01u);')]),
    ('+2490 path B1 AL is source[3]', [('else al = (uint8_t)(s3 & 0x10u);', 'else al = s3;')]),
    ('+2490 skips the semantic model', [('pr_u38_entry74(&st, src, &paths) == PR_U38_OK', '(void)pr_u38_entry74, 1')]),
    ('+2490 the OR flags omit ZF (logic helper shared)', [('logic8_flags(m->cpu, al); set_al_(m->cpu, al); set_es_di(m->cpu, ss_, so);', 'logic8_flags(m->cpu, (uint8_t)(al | 0x00u)); m->cpu->flags = (uint16_t)(m->cpu->flags & ~0x0040u); set_al_(m->cpu, al); set_es_di(m->cpu, ss_, so);')]),
    ('+2700 the CMP flags are swapped (lim - v)', [('xb_flush(x); sub8_flags(m->cpu, v, lim); set_al_(m->cpu, v > lim ? lim : v);', 'xb_flush(x); sub8_flags(m->cpu, lim, v); set_al_(m->cpu, v > lim ? lim : v);')]),
    ('+2700 AL is always the limit', [('set_al_(m->cpu, v > lim ? lim : v); set_es_di(m->cpu, rs, ro);', 'set_al_(m->cpu, lim); set_es_di(m->cpu, rs, ro);')]),
    ('+2700 AL is always the sum', [('set_al_(m->cpu, v > lim ? lim : v); set_es_di(m->cpu, rs, ro);', 'set_al_(m->cpu, v); set_es_di(m->cpu, rs, ro);')]),
    ('+2700 skips the semantic model', [('pr_u38_entry82(rec, &paths) == PR_U38_OK', '(void)pr_u38_entry82, 1')]),
    ('+013B DX is not set', [('m->cpu->ax = (uint16_t)(xb_byte(m, ps, po, 0xAu) | xb_byte(m, ps, po, 0xBu) << 8); m->cpu->dx = (uint16_t)(xb_byte(m, ps, po, 0xCu) | xb_byte(m, ps, po, 0xDu) << 8);', 'm->cpu->ax = (uint16_t)(xb_byte(m, ps, po, 0xAu) | xb_byte(m, ps, po, 0xBu) << 8);')]),
    ('+013B AX is P[0Ch]', [('m->cpu->ax = (uint16_t)(xb_byte(m, ps, po, 0xAu) | xb_byte(m, ps, po, 0xBu) << 8);', 'm->cpu->ax = (uint16_t)(xb_byte(m, ps, po, 0xCu) | xb_byte(m, ps, po, 0xDu) << 8);')]),
    ('+013B the equal path leaves the CMP flags', [('if (q9f == 4u) add8_flags(m->cpu, a6818, 2u); else sub8_flags(m->cpu, q9f, 4u);', 'sub8_flags(m->cpu, q9f, 4u);')]),
    ('+013B the equal path ADD flags use [6822]', [('if (q9f == 4u) add8_flags(m->cpu, a6818, 2u); else', 'if (q9f == 4u) add8_flags(m->cpu, st.b6822, 2u); else')]),
    ('+013B the CMP is against 3', [('sub8_flags(m->cpu, q9f, 4u);\n        set_es_di', 'sub8_flags(m->cpu, q9f, 3u);\n        set_es_di')]),
    ('+013B skips the semantic model', [('pr_u38_entry4(&st, rec, xb_resolve, x, &paths) == PR_U38_OK', '((void)pr_u38_entry4, PR_U38_OK) == PR_U38_OK')]),
    ('+0377/+03AC an unmatched record uses the CMP against 0', [('if (applied) sub8_flags(c, a6822, 2u); else sub8_flags(c, b, k3);', 'if (applied) sub8_flags(c, a6822, 2u); else sub8_flags(c, b, 0u);')]),
    ('+0377/+03AC an unmatched record leaves the flags', [('if (applied) sub8_flags(c, a6822, 2u); else sub8_flags(c, b, k3);', 'if (applied) sub8_flags(c, a6822, 2u);')]),
    ('+0377/+03AC a match leaves the ADD flags (not the SUB)', [('if (applied) sub8_flags(c, a6822, 2u); else', 'if (applied) add8_flags(c, mods.b6816, 2u); else')]),
    ('+0377/+03AC the SUB flags come from the stored [6822]', [('if (applied) sub8_flags(c, a6822, 2u); else', 'if (applied) sub8_flags(c, mods.b6822, 2u); else')]),
    ('+0377/+03AC ES:DI is left unchanged', [('            set_es_di(c, rs, ro);\n            frame_leave(m, 0x0Au); rc = 0;\n        }\n    }\n    return rc;\n}\nstatic int h_0377', '            frame_leave(m, 0x0Au); rc = 0;\n        }\n    }\n    return rc;\n}\nstatic int h_0377')]),
    ('+0377/+03AC the pointer offset is read from [5D94]', [('ro = rd16(m->ram, c->ds, 0x5D92u); rs = rd16(m->ram, c->ds, 0x5D94u);', 'ro = rd16(m->ram, c->ds, 0x5D94u); rs = rd16(m->ram, c->ds, 0x5D92u);')]),
    ('+0377/+03AC the model result is ignored (applied forced 0)', [('store_mods(m->ram, c, &mods); xb_flush(x);\n            if (applied) sub8_flags', 'store_mods(m->ram, c, &mods); xb_flush(x); applied = 0;\n            if (applied) sub8_flags')]),
    ('+0377 uses the +03AC model', [('(second ? pr_rec_u38_entry9(&mods, rec, &applied) : pr_rec_u38_entry8(&mods, rec, &applied))', 'pr_rec_u38_entry9(&mods, rec, &applied)')]),
    ('+0377/+03AC skip the semantic model', [('(second ? pr_rec_u38_entry9(&mods, rec, &applied) : pr_rec_u38_entry8(&mods, rec, &applied)) == PR_REC_EFFECT_OK', '((void)pr_rec_u38_entry8, PR_REC_EFFECT_OK) == PR_REC_EFFECT_OK')]),
    ('+0377/+03AC pop 8 argument bytes, not 0Ah', [('            set_es_di(c, rs, ro);\n            frame_leave(m, 0x0Au); rc = 0;\n        }\n    }\n    return rc;\n}\nstatic int h_0377', '            set_es_di(c, rs, ro);\n            frame_leave(m, 0x08u); rc = 0;\n        }\n    }\n    return rc;\n}\nstatic int h_0377')]),
    ('+03AC the third constant is 8', [('static int h_03ac(call_t *m) { return gated_alignment(m, 1, 6u); }', 'static int h_03ac(call_t *m) { return gated_alignment(m, 1, 8u); }')]),
    ('+0377 the third constant is 6', [('static int h_0377(call_t *m) { return gated_alignment(m, 0, 8u); }', 'static int h_0377(call_t *m) { return gated_alignment(m, 0, 6u); }')]),
    ('+0918 the lookup result registers are not copied back', [('COPY(&c, cpu); st = pr_res_00ba_2199_cpu(ram, size, &c, m->lim->loopbacks); COPY(cpu, &c);', 'COPY(&c, cpu); st = pr_res_00ba_2199_cpu(ram, size, &c, m->lim->loopbacks);')]),
    ('+0918 the lookup gets no loop-backs (cap 0)', [('st = pr_res_00ba_2199_cpu(ram, size, &c, m->lim->loopbacks); COPY(cpu, &c);', 'st = pr_res_00ba_2199_cpu(ram, size, &c, 0u); COPY(cpu, &c);')]),
    ('+0918 skips the handler model', [('s = (int)pr_u38_sidecar_flag_0918(m->ram, m->size, &x, res2199_lookup, m); COPY(m->cpu, &x);', 's = ((void)pr_u38_sidecar_flag_0918, 0); COPY(m->cpu, &x);')]),
    ('+0918 the model result is not copied back', [('s = (int)pr_u38_sidecar_flag_0918(m->ram, m->size, &x, res2199_lookup, m); COPY(m->cpu, &x);', 's = (int)pr_u38_sidecar_flag_0918(m->ram, m->size, &x, res2199_lookup, m);')]),
    ('+0918 the lookup is replaced by a stub that finds nothing', [('COPY(&c, cpu); st = pr_res_00ba_2199_cpu(ram, size, &c, m->lim->loopbacks); COPY(cpu, &c);\n    return st == PR_RES_2199_OK;', 'COPY(&c, cpu); (void)ram; (void)size; st = PR_RES_2199_OK; COPY(cpu, &c); (void)m;\n    return st == PR_RES_2199_OK;')]),
    ('+12B2 skips the model', [('ADAPT(h_12b2, pr_u38_101d_cpu, pr_u38_effect_match_12b2(m->ram, m->size, &x))', 'ADAPT(h_12b2, pr_u38_101d_cpu, ((void)pr_u38_effect_match_12b2, 0))')]),
    ('+12B2 runs the model on a scratch CPU', [('ADAPT(h_12b2, pr_u38_101d_cpu, pr_u38_effect_match_12b2(m->ram, m->size, &x))', 'ADAPT(h_12b2, pr_u38_101d_cpu, pr_u38_effect_match_12b2(m->ram, m->size, &(pr_u38_101d_cpu){0}))')]),
    ('+25BC skips the model', [('ADAPT_T(h_25bc, pr_u38_target_mask81_25bc_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_DOSBOX386, &dg))', 'ADAPT_T(h_25bc, ((void)pr_u38_target_mask81_25bc_cpu, 0))')]),
    ('+25BC uses the Unicorn shift profile', [('ADAPT_T(h_25bc, pr_u38_target_mask81_25bc_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_DOSBOX386, &dg))', 'ADAPT_T(h_25bc, pr_u38_target_mask81_25bc_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_UNICORN, &dg))')]),
    ('+2D38 skips the model', [('ADAPT(h_2d38, pr_u38_2d38_cpu, pr_u38_record_roll_2d38(m->ram, m->size, &x, &m->status_raw))', 'ADAPT(h_2d38, pr_u38_2d38_cpu, ((void)pr_u38_record_roll_2d38, 0))')]),
    ('+2D38 runs the model on a scratch CPU', [('ADAPT(h_2d38, pr_u38_2d38_cpu, pr_u38_record_roll_2d38(m->ram, m->size, &x, &m->status_raw))', 'ADAPT(h_2d38, pr_u38_2d38_cpu, pr_u38_record_roll_2d38(m->ram, m->size, &(pr_u38_2d38_cpu){0}, &m->status_raw))')]),
    ('+04BA skips the model', [('COPY(&x, m->cpu); s = (int)pr_u38_pair_04ba(m->ram, m->size, &x, u9e_cs, m->lim->loopbacks); COPY(m->cpu, &x);', 'COPY(&x, m->cpu); s = ((void)pr_u38_pair_04ba, 0); (void)u9e_cs; COPY(m->cpu, &x);')]),
    ('+04BA u9e_cs is 0', [('s = (int)pr_u38_pair_04ba(m->ram, m->size, &x, u9e_cs, m->lim->loopbacks);', 's = (int)pr_u38_pair_04ba(m->ram, m->size, &x, 0u, m->lim->loopbacks);')]),
    ('+04BA the loop-back budget is 0', [('s = (int)pr_u38_pair_04ba(m->ram, m->size, &x, u9e_cs, m->lim->loopbacks);', 's = (int)pr_u38_pair_04ba(m->ram, m->size, &x, u9e_cs, 0u);')]),
    ('+04BA the model result is not copied back', [('s = (int)pr_u38_pair_04ba(m->ram, m->size, &x, u9e_cs, m->lim->loopbacks); COPY(m->cpu, &x);', 's = (int)pr_u38_pair_04ba(m->ram, m->size, &x, u9e_cs, m->lim->loopbacks);')]),
    ('+04BA u9e_cs is read from the wrong frame word', [('uint16_t u9e_cs = rd16(m->ram, m->cpu->ss, (uint16_t)(m->cpu->sp + 2u));\n    COPY(&x, m->cpu); s = (int)pr_u38_pair_04ba', 'uint16_t u9e_cs = rd16(m->ram, m->cpu->ss, (uint16_t)(m->cpu->sp + 4u));\n    COPY(&x, m->cpu); s = (int)pr_u38_pair_04ba')]),
    ('the gap kind of an uncredited slot is reported as semantic', [("sl->kind == K_GAP_SEMANTIC ? PR_U9E_0000_GAP_SEMANTIC :", "sl->kind != K_RAW ? PR_U9E_0000_GAP_SEMANTIC :")]),
)
MUTANT_CASES_EXTRA = ('adapt', 'transfer-133f-', 'refused-', 'gap001-', 'gap004-', 'gap088-', 'gap135-', 'sp-', 'flags-', 'budget-slot16-', 'budget-slot004-')


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


def mutation_check(src, base, libdir, only_mutants=None, only_cases=None):
    out = []; text0 = (ROOT / 'src/recovered/overlay_u9e_dispatch_0000_cpu.c').read_text()
    raw = [s for s in range(1, 140) if src['slots'][s]['kind'] in COMPOSED]
    first = {}
    for s in raw:
        first.setdefault(src['slots'][s]['entry'], s)
    names = [f"slot{s:03d}-random{k}" for s in first.values() for k in range(3)]
    selected = only_cases if only_cases is not None else list(names) + list(MUTANT_CASES_EXTRA)
    baseline = differential(src, library(libdir), base, only=selected)        # the unmutated library MUST pass the case set the mutants are judged on (a failing baseline would 'catch' every mutant)
    check(baseline['compared'] > 0, 'mutation baseline compared no cases')
    print(f"Mutation baseline PASS: {baseline['cases']} cases, {baseline['compared']} compared", file=sys.stderr, flush=True)
    (Path(libdir) / 'mutation-baseline.json').write_text(json.dumps(baseline, sort_keys=True, indent=2) + '\n')
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
            for o in Path(libdir).resolve().glob('*.o'):
                if o.stem != 'overlay_u9e_dispatch_0000_cpu':
                    (work / o.name).symlink_to(o)
            try:
                mlib = library(work, override={'overlay_u9e_dispatch_0000_cpu': path})
            except subprocess.CalledProcessError as error:
                print(error.stderr, file=sys.stderr, flush=True)
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(src, mlib, base, only=selected)
                out.append((name, 'SURVIVED the differential' + c_test_verdict(tmp, path)))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:90]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:70]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    d = sum(1 for _, r in out if r.startswith('CAUGHT:')); c = sum(1 for _, r in out if r.endswith('CAUGHT BY THE C TEST'))
    return dict(baseline=baseline, total=len(out), caught=d + c, caught_by_differential=d, caught_by_c_test=c, results=out,
                proven_equivalent=[n for n, r in out if r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST') and n in EQUIVALENT],
                survivors=[n for n, r in out if r == 'COMPILE-ERROR' or (r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST') and n not in EQUIVALENT)])   # a mutant that does not build is never excused


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
            only = None if not a.mutant_filter else {k for k, (n, _) in enumerate(MUTANTS) if any(f in n for f in a.mutant_filter.split("|"))}
            check(only is None or only, 'mutant filter selected no mutations')
            report['mutation'] = mutation_check(src, base, libdir, only_mutants=only, only_cases=a.only)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2, default=str) + '\n')
    print(json.dumps(report, sort_keys=True, default=str)[:3000])


if __name__ == '__main__':
    main()
