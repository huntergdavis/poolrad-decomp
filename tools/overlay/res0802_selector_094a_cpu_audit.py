#!/usr/bin/env python3
"""Original/C evidence for the existing inline RES_0802:094A raw leaf.

No original bytes are emitted. Run heavy executions through poolrad-heavy.sh.
Default execution is the full dimension sweep; --smoke is explicitly reduced.
"""
import argparse
from collections import Counter
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import tempfile
import time

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_PROT_ALL
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)
from resident_loader_map import unpack, relocations, HEADER_BYTES
from overlay_inventory import find_units

ROOT = Path(__file__).resolve().parents[2]
SIZE = 1 << 20
MASK = SIZE - 1
ENTRY, END, IMAGE_AT = 0x094A, 0x095D, 0x0896A
BODY_SHA = "9cb0afed6f73df887b61f47778db5d55bd9063b59c8f6067ab0c38ec3350d205"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
CHECKER_ROWS = ("RES_0802_094A",)
SOURCES = ["overlay_u25_store_word_0c77"]
C_SOURCE = "src/recovered/overlay_u25_store_word_0c77.c"
SP_MIN, SP_MAX = 2, 0xFFF9
FLAG_BITS = (0, 2, 4, 6, 7, 9, 10, 11, 12, 13, 14)
NAMES = "ax bx cx dx si di bp sp cs ip ds es ss flags".split()
REGS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
        UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
        UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in NAMES]


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def pin_source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    require(sha256(exe).hexdigest() == START_SHA, "START.EXE hash")
    require(sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR hash")
    image, origin, fill, _ = unpack(exe)
    body = bytes(image[IMAGE_AT:IMAGE_AT + 19])
    require(sha256(body).hexdigest() == BODY_SHA, "expanded body hash")
    require([HEADER_BYTES + x for x in origin[IMAGE_AT:IMAGE_AT + 19]] ==
            list(range(0x89A1, 0x89B4)), "contiguous copied provenance")
    require(not any(fill[IMAGE_AT:IMAGE_AT + 19]), "body contains loader fill")
    sites, _ = relocations(exe, image)
    require(not set(sites) & set(range(IMAGE_AT - 1, IMAGE_AT + 19)),
            "body has an unhandled loader relocation")
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    ins = list(md.disasm(body, ENTRY))
    require(len(ins) == 8 and sum(i.size for i in ins) == 19, "complete eight-instruction body")
    require([i.mnemonic for i in ins] ==
            ["push", "mov", "mov", "mov", "mov", "mov", "pop", "retf"],
            "body control-flow/write shape changed")
    require(ins[-1].op_str == "2", "far return consumes one word")
    units = find_units(exe[HEADER_BYTES:], ovr)
    callers = []
    for unit_id, site, begin, size, digest in (
        (0x25, 0x0C03, 0x0A74, 515, "258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938"),
        (0x6F, 0x048C, 0x0482, 540, "a939d6fc0f5514a7efd9e9a08cbff48ef14ff41eddd13652284fd381489240aa"),
    ):
        u = next(u for u in units if u["stub_para"] == unit_id)
        start = u["ovr_offset"]
        caller = ovr[start + begin:start + begin + size]
        require(sha256(caller).hexdigest() == digest, "caller body hash")
        call = list(md.disasm(ovr[start + site:start + site + 5], site))
        require(len(call) == 1 and call[0].mnemonic == "lcall" and
                call[0].op_str == "0x802, 0x94a", "caller target")
        require(site + 3 in u["fixups"], "far caller lacks segment fixup")
        callers.append(dict(unit=unit_id, site=site, segment_fixup=site + 3,
                            body_start=begin, body_bytes=size, body_sha256=digest))
    return body, dict(body_sha256=BODY_SHA, image_offset=IMAGE_AT, bytes=19,
                      packed_offset=0x89A1, instructions=8, callees=0,
                      starts=[i.address for i in ins], callers=callers,
                      start_sha256=START_SHA, game_ovr_sha256=OVR_SHA)


def library(directory, mutant="none"):
    """Compile the production static helper, not a second implementation."""
    original = (ROOT / C_SOURCE).read_text()
    changed = original
    if mutant == "early_argument":
        before = """    w8(ram, c->ds, 0x5377u, (uint8_t)c->ax);
    al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));"""
        after = """    uint8_t prior = (uint8_t)c->ax;
    al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
    w8(ram, c->ds, 0x5377u, prior);"""
        require(changed.count(before) == 1, "ordering mutant anchor")
        changed = changed.replace(before, after)
    require(mutant in ("none", "early_argument", "flip_cf"), "unknown mutant")
    wrapper = directory / (mutant + ".c")
    wrapper.write_text(changed + "\nvoid audit_selector(uint8_t *ram, pr_u25_store_word_cpu *c) {\n"
                       "    selector_push(ram, c);\n" +
                       ("    c->flags ^= 1u;\n" if mutant == "flip_cf" else "") + "}\n")
    out = directory / (mutant + ".so")
    subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-Wconversion", "-Wsign-conversion", "-Wshadow", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), "-x", "c", "-", "-o", str(out)],
                   input=wrapper.read_text(), text=True, check=True)
    lib = ctypes.CDLL(str(out))
    lib.audit_selector.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(Cpu)]
    lib.audit_selector.restype = None
    return lib


def cases(smoke=False):
    """Lexicographic enumerations, not random author-selected favorable states."""
    yield "baseline", {}
    values = range(0x10000) if not smoke else (0, 1, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF)
    for name in ("bp", "ds", "ss", "ax", "return_ip", "return_cs", "argument"):
        for v in values:
            yield name, {name: v}
    for v in (range(SP_MIN, SP_MAX + 1) if not smoke else (2, 3, 0x5373, 0x5376, 0xFFF8, 0xFFF9)):
        yield "sp", {"sp": v}
    for v in range(1 << len(FLAG_BITS)):
        yield "flags", {"flags": 2 | sum(((v >> i) & 1) << b for i, b in enumerate(FLAG_BITS))}
    for v in range(256):
        yield "prior_save", {"saved": v}
    byte_values = range(256) if not smoke else (0, 1, 127, 128, 254, 255)
    for old in byte_values:
        for arg in byte_values:
            yield "selector_argument_product", {"old": old, "argument": 0xA500 | arg}
    # Alias DS:5376 onto every byte of saved BP/return IP/return CS/argument.
    # DS:5377 then aliases its adjacent byte. Sweep each data input.
    for delta in range(-2, 6):
        for v in byte_values:
            yield "frame_alias_old", {"ss": 0x2000, "ds": 0x2000, "sp": 0x5376 - delta, "old": v}
            yield "frame_alias_argument", {"ss": 0x2000, "ds": 0x2000, "sp": 0x5376 - delta, "argument": 0xA500 | v}
    # Same physical alias with different segments, plus A20 wrap.
    for ds, ss in ((0x2100, 0x2000), (0xFFFF, 0)):
        for delta in range(-2, 6):
            sp = (physical(ds, 0x5376) - (ss << 4) - delta) & 0xFFFF
            for v in byte_values:
                yield "segment_alias", {"ds": ds, "ss": ss, "sp": sp, "old": v, "argument": 0xA500 | (255-v)}
    # Unread preserved registers: walking-one/zero sentinels, not exhaustive.
    for name in ("bx", "cx", "dx", "si", "di", "es"):
        for bit in range(16):
            for v in (1 << bit, 0xFFFF ^ (1 << bit)):
                yield "preserved_register", {name: v}


class Engine:
    """CPU resets each case; identical RAM persists only after full comparison.

    Every byte the pinned body can read before writing is explicitly seeded.
    fresh_memory retains the slower independent-baseline mode for cross-checks.
    """
    def __init__(self, body, fresh_memory=False):
        self.body = body
        self.fresh_memory = fresh_memory
        self.agreed = True
        self.original = (ctypes.c_uint8 * SIZE)()
        self.c_ram = (ctypes.c_uint8 * SIZE)()
        self.template = ctypes.create_string_buffer(bytes(range(256)) * (SIZE // 256))
        ctypes.memmove(self.original, self.template, SIZE)
        ctypes.memmove(self.c_ram, self.template, SIZE)
        self.uc = Uc(UC_ARCH_X86, UC_MODE_16)
        for base in (0, SIZE):
            self.uc.mem_map_ptr(base, SIZE, UC_PROT_ALL, ctypes.addressof(self.original))
        self.memcmp = ctypes.CDLL(None).memcmp
        self.memcmp.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
        self.memcmp.restype = ctypes.c_int

    def run(self, change, lib):
        require(self.agreed, "previous case failed: refusing to reuse divergent state")
        if self.fresh_memory:
            ctypes.memmove(self.original, self.template, SIZE)
            ctypes.memmove(self.c_ram, self.template, SIZE)
        cpu = Cpu(0xA65B, 0x1357, 0x2468, 0xABCD, 0x5678, 0x6789,
                  0x4321, 0x8000, 0x1802, ENTRY, 0x3000, 0x6000, 0x7000, 0x0ED7)
        for name, value in change.items():
            if name in NAMES:
                setattr(cpu, name, value)
        require(SP_MIN <= cpu.sp <= SP_MAX, "case outside admitted entry-SP window")
        require(cpu.flags & ~0x7ED7 == 0 and cpu.flags & 2, "reserved or TF flags")
        touched = {physical(cpu.ss, cpu.sp + d) for d in range(-2, 6)}
        touched |= {physical(cpu.ds, 0x5376), physical(cpu.ds, 0x5377)}
        # A sweep may place data at the usual code location. Relocate the
        # relocation-free body rather than omit that input or overwrite code.
        cpu.cs = next(cs for cs in (0x1802, 0x4802, 0x8802)
                      if not touched & set(range(physical(cs, ENTRY), physical(cs, ENTRY) + 19)))
        def put8(at, value):
            self.original[at] = self.c_ram[at] = value
        def put16(off, value):
            at = physical(cpu.ss, off)
            put8(at, value & 255)
            put8((at + 1) & MASK, value >> 8)
        put16(cpu.sp - 2, 0xDEAD)
        put16(cpu.sp, change.get("return_ip", 0x1234))
        put16(cpu.sp + 2, change.get("return_cs", 0x9000))
        put16(cpu.sp + 4, change.get("argument", 0xA55A))
        # Shared physical bytes cannot take independent values. Data is seeded
        # after the frame; the report describes this ordering for alias cases.
        put8(physical(cpu.ds, 0x5376), change.get("old", 0x3C))
        put8(physical(cpu.ds, 0x5377), change.get("saved", 0xC3))
        at = physical(cpu.cs, ENTRY)
        self.uc.mem_write(at, self.body)  # also invalidates translated code
        ctypes.memmove(ctypes.addressof(self.c_ram) + at, self.body, len(self.body))
        c_cpu = Cpu.from_buffer_copy(cpu)
        self.uc.reg_write_batch([(reg, getattr(cpu, name)) for name, reg in zip(NAMES, REGS)])
        self.uc.emu_start((cpu.cs << 4) + ENTRY, 2 * SIZE, count=8)
        lib.audit_selector(self.c_ram, ctypes.byref(c_cpu))
        mismatches = {}
        self.last_original_cpu = tuple(v & 0xFFFF for v in self.uc.reg_read_batch(REGS))
        for name, original in zip(NAMES, self.last_original_cpu):
            candidate = getattr(c_cpu, name)
            if original != candidate:
                mismatches[name] = dict(original=original, candidate=candidate)
        same_ram = self.memcmp(self.original, self.c_ram, SIZE) == 0
        self.agreed = not mismatches and same_ram
        return dict(cpu=mismatches, ram_equal=same_ram)


def audit(smoke=False, progress=False):
    started = time.monotonic()
    body, provenance = pin_source()
    counts = Counter()
    chosen = sha256()
    with tempfile.TemporaryDirectory(prefix="poolrad-094a-") as td:
        directory = Path(td)
        good = library(directory)
        engine = Engine(body)
        for group, change in cases(smoke):
            result = engine.run(change, good)
            require(not result["cpu"] and result["ram_equal"],
                    f"parity failure group={group} input={change} result={result}")
            counts[group] += 1
            chosen.update(json.dumps([group, change], sort_keys=True).encode() + b"\n")
            if progress and sum(counts.values()) % 10000 == 0:
                print(json.dumps(dict(progress_cases=sum(counts.values()),
                                      elapsed_s=round(time.monotonic()-started, 3))), flush=True)
        mutants = {}
        for mutant in ("flip_cf", "early_argument"):
            lib = library(directory, mutant)
            engine = Engine(body)
            for index, (group, change) in enumerate(cases(True), 1):
                result = engine.run(change, lib)
                if result["cpu"] or not result["ram_equal"]:
                    mutants[mutant] = dict(killed=True, cases_to_witness=index,
                        group=group, input=change, differences=result)
                    break
            require(mutant in mutants, f"SURVIVED mutant: {mutant}")
    return dict(schema_version=1, target="RES_0802_094A",
        scope="reduced smoke" if smoke else "full declared dimension sweep",
        source=provenance, cases=sum(counts.values()), groups=dict(counts),
        case_input_sha256=chosen.hexdigest(), cpu_words=14, ram_bytes=SIZE,
        masks=[], mutants=mutants, elapsed_s=round(time.monotonic()-started, 3),
        candidate_sha256=sha256((ROOT / C_SOURCE).read_bytes()).hexdigest(),
        audit_sha256=sha256(Path(__file__).read_bytes()).hexdigest(),
        not_verified=["Cartesian product of all state dimensions", "TF/interrupt delivery",
                      "stack offsets outside 0002..FFF9", "mutable code", "native DOSBox parity",
                      "natural game reachability", "ownership or credit", "caller closure/unlocks"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--smoke", action="store_true", help="reduced harness check; not acceptance coverage")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--progress", action="store_true")
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory(prefix="poolrad-094a-build-") as td:
            library(Path(td))
        print("CHECKER_BUILD_OK")
        return
    report = audit(args.smoke, args.progress)
    text = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text)
    print(text, end="")


if __name__ == "__main__":
    main()


