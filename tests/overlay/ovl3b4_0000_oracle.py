#!/usr/bin/env python3
"""Differential oracle for unit 0x3B4 +0x0000 (the unit's entry in ledger slot 2).

The 199 owner bytes at GAME.OVR file offset 0x2DB87 run under Unicorn (16-bit real mode), loaded at CS 0x03CA with the routine at IP 0.
Its two external calls go to stubs: lcall 0x3E9:0x39 (five words pushed, RETF 0xA) and lcall 0x464:0x1B52 (no arguments, RETF 0). Each stub
logs its pushed words, applies the case's DS writes, and pops its arguments with RETF. The same cases run through the C translation (the
vector driver tests/overlay/ovl3b4_0000_harness.c linked with src/recovered/overlay_u3b4_entry2_0000.c). Compared per case: the call log,
the status, and every DS byte that differs from its initial value. The status requires SP restored to entry+4 and BP to its entry value,
and every stub entry to see the stack the routine implies (SP = entry SP - 4 - 2 x argument words - 4).

The far record pointer at DS:0x49D2 points into a far window (segments 0x3000..0x70FF) written through a byte-level far table; record and
DS offsets are kept below 0xF000 so no 16-bit wrap is generated (not oracle-checked).

Mutants: one-edit copies of the translation must each differ on at least one case. The owner GAME.OVR is read at run time and never
committed; the work directory must be outside the repository.

Usage: ovl3b4_0000_oracle.py --game-ovr PATH --work-dir DIR [--cases N] [--seed S]
"""
import argparse
import hashlib
import os
import random
import subprocess
import sys

from unicorn import (Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_READ_UNMAPPED, UC_HOOK_MEM_WRITE_UNMAPPED,
                     UC_MODE_16, UC_PROT_ALL)
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES,
                               UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "src", "recovered", "overlay_u3b4_entry2_0000.c")
INC = os.path.join(ROOT, "include")
HARNESS = os.path.join(ROOT, "tests", "overlay", "ovl3b4_0000_harness.c")

FILE_OFF = 0x2DB87
SPAN_LEN = 199
SPAN_SHA = "4a0b67222fd84d903b9a817b68b7cd6398cbc864b7737e198427034682788880"

CS_SEG = 0x03CA
RET_CS = 0x0000
RET_IP = 0x0F00
DS_SEG = 0x2000
DS_SIZE = 0x10000
SS_SEG = 0x9000
RECORD_FIELDS = [0x186, 0x188]
RECORD_SEGS = [0x3000, 0x4000, 0x5000, 0x6000, 0x7000]

STUBS = {
    (0x3E9, 0x0039): ("39", 5),
    (0x464, 0x1B52): ("1b52", 0),
}
EFFECT_OFFSETS = [0x6DDC, 0x6AAF, 0x49FA, 0x49D2, 0x49D3, 0x49D4, 0x49D5, 0x3C76, 0x3C80, 0x3D00]

MUTANTS = [
    ("w5-without-plus-9", "w5 = (uint16_t)((uint16_t)(val >> 1) + 9u);", "w5 = (uint16_t)((uint16_t)(val >> 1));"),
    ("w4-quotient-not-remainder", "w4 = (uint16_t)(val % 2u);", "w4 = (uint16_t)(val / 2u);"),
    ("b2-top-band-always-3", "b2 = (rec >= 0x23u) ? 4u : 3u;", "b2 = 3u;"),
    ("w2-ah-dropped", "w2 = (uint16_t)((h_ah << 8) | b2);", "w2 = (uint16_t)(b2);"),
    ("update-modulo-5", "ds[0x6DDCu] = (uint8_t)(((uint16_t)val + 1u) % 6u);", "ds[0x6DDCu] = (uint8_t)(((uint16_t)val + 1u) % 5u);"),
    ("unsigned-first-clamp", "if ((int8_t)b1 >= 2) {", "if (b1 >= 2) {"),
]


def rd16(uc, lin):
    return int.from_bytes(uc.mem_read(lin, 2), "little")


def gen_case(rng):
    case = {"ds": {}, "far": [], "eff": {"39": [], "1b52": []}, "entry_sp": rng.randrange(0x0200, 0xF000),
            "bp0": rng.randrange(65536)}
    ds = case["ds"]
    ds[0x49FA] = rng.randrange(256)
    for off in range(0x3C76, 0x3D76):
        ds[off] = rng.randrange(256)
    ds[0x6AAF] = rng.randrange(256)
    ds[0x6DDC] = rng.randrange(256)
    rec_seg = rng.choice(RECORD_SEGS)
    rec_off = rng.randrange(0x0000, 0xF000)
    ds[0x49D2], ds[0x49D3] = rec_off & 0xFF, rec_off >> 8
    ds[0x49D4], ds[0x49D5] = rec_seg & 0xFF, rec_seg >> 8
    for field in RECORD_FIELDS:
        val = rng.choice([0, 1, 2, 0x21, 0x22, 0x23, rng.randrange(65536), rng.randrange(256)])
        case["far"].append((rec_seg, rec_off + field, val))
    for name in ("39", "1b52"):
        for _ in range(rng.randrange(3)):
            off = rng.choice(EFFECT_OFFSETS) if rng.random() < 0.7 else rng.randrange(0x3C00, 0x3E00)
            if off == 0x49D3:
                val = rng.randrange(0x30, 0x71)
            elif off == 0x49D5:
                val = rng.choice([0x30, 0x40, 0x50, 0x60, 0x70])
            else:
                val = rng.randrange(256)
            case["eff"][name].append((off, val))
    return case


def case_text(case):
    lines = ["case"]
    lines += ["ds %d %d" % (off, val) for off, val in sorted(case["ds"].items())]
    lines += ["far %d %d %d" % w for w in case["far"]]
    for name in ("39", "1b52"):
        lines += ["eff %s %d %d" % (name, off, val) for off, val in case["eff"][name]]
    lines.append("end")
    return "\n".join(lines) + "\n"


def run_unicorn_case(body, case):
    log = []
    cov = {"b1_ge2": 0, "b1_ge2a": 0, "rec_lt2": 0, "rec_ge22": 0, "call39": 0, "call1b52": 0, "arity": 0}
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000, UC_PROT_ALL)
    uc.mem_write(CS_SEG * 16, body)
    ds_init = dict(case["ds"])
    for off, val in ds_init.items():
        uc.mem_write(DS_SEG * 16 + off, bytes([val]))
    for seg, off, word in case["far"]:
        uc.mem_write(seg * 16 + off, word.to_bytes(2, "little"))
    entry_sp = case["entry_sp"]
    frame_sp = (entry_sp - 4) & 0xFFFF
    uc.mem_write(SS_SEG * 16 + entry_sp, RET_IP.to_bytes(2, "little") + RET_CS.to_bytes(2, "little"))
    uc.reg_write(UC_X86_REG_CS, CS_SEG)
    uc.reg_write(UC_X86_REG_IP, 0x0000)
    uc.reg_write(UC_X86_REG_DS, DS_SEG)
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, SS_SEG)
    uc.reg_write(UC_X86_REG_SP, entry_sp)
    uc.reg_write(UC_X86_REG_BP, case["bp0"])
    uc.reg_write(UC_X86_REG_AX, 0x5A5A)   # the entry AX is not read

    def on_code(mu, address, size, _user):
        cs = mu.reg_read(UC_X86_REG_CS)
        ip = mu.reg_read(UC_X86_REG_IP)
        if cs == CS_SEG:
            if ip == 0x0027:
                cov["b1_ge2"] += 1
            elif ip == 0x0033:
                cov["b1_ge2a"] += 1
            elif ip == 0x004F:
                cov["rec_lt2"] += 1
            elif ip == 0x006F:
                cov["rec_ge22"] += 1
        key = (cs, ip)
        if key not in STUBS:
            return
        name, nwords = STUBS[key]
        sp = mu.reg_read(UC_X86_REG_SP)
        ss = mu.reg_read(UC_X86_REG_SS) * 16

        def word_at(k):
            return rd16(mu, ss + ((sp + k) & 0xFFFF))

        args = [word_at(4 + 2 * (nwords - 1 - k)) for k in range(nwords)]
        ret_ip, ret_cs = word_at(0), word_at(2)
        if sp != (frame_sp - 2 * nwords - 4) & 0xFFFF:
            cov["arity"] += 1
        if name == "39":
            log.append("call 39 %d %d %d %d %d" % tuple(args))
            cov["call39"] += 1
        else:
            log.append("call 1b52")
            cov["call1b52"] += 1
        for off, val in case["eff"][name]:
            mu.mem_write(DS_SEG * 16 + off, bytes([val]))
        mu.reg_write(UC_X86_REG_IP, ret_ip)
        mu.reg_write(UC_X86_REG_CS, ret_cs)
        mu.reg_write(UC_X86_REG_SP, (sp + 4 + 2 * nwords) & 0xFFFF)

    faults = []

    def on_unmapped(mu, access, address, size, value, _user):
        faults.append("unmapped access %#x at CS:IP %04X:%04X" % (
            address, mu.reg_read(UC_X86_REG_CS), mu.reg_read(UC_X86_REG_IP)))
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED, on_unmapped)
    try:
        uc.emu_start(CS_SEG * 16, RET_CS * 16 + RET_IP, timeout=5000000)
    except UcError as e:
        raise RuntimeError("oracle emulation stopped (%s); %s" % (e, "; ".join(faults) or "no unmapped access"))
    if faults:
        raise RuntimeError("oracle touched unmapped memory: " + "; ".join(faults))
    returned = uc.reg_read(UC_X86_REG_CS) == RET_CS and uc.reg_read(UC_X86_REG_IP) == RET_IP
    balanced = uc.reg_read(UC_X86_REG_SP) == (entry_sp + 4) & 0xFFFF and uc.reg_read(UC_X86_REG_BP) == case["bp0"]
    ok = returned and balanced and cov["arity"] == 0
    out = list(log)
    out.append("status 0" if ok else "status 9")
    final = bytes(uc.mem_read(DS_SEG * 16, DS_SIZE))
    for off in range(DS_SIZE):
        if final[off] != ds_init.get(off, 0):
            out.append("ds %d %d" % (off, final[off]))
    out.append("endcase")
    return out, cov


def run_harness(binary, text):
    res = subprocess.run([binary], input=text, capture_output=True, text=True, timeout=600)
    if res.returncode != 0:
        raise RuntimeError("harness failed: %s" % res.stderr.strip())
    return res.stdout.splitlines()


def build(src, out):
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", INC, src, HARNESS, "-o", out], check=True)


def split_cases(lines):
    cases, cur = [], []
    for line in lines:
        cur.append(line)
        if line == "endcase":
            cases.append(cur)
            cur = []
    if cur:
        cases.append(cur + ["<unterminated>"])
    return cases


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--game-ovr", required=True, help="path to the owner GAME.OVR (read only, never committed)")
    ap.add_argument("--work-dir", required=True, help="scratch directory outside the repository")
    ap.add_argument("--cases", type=int, default=400)
    ap.add_argument("--seed", type=int, default=1)
    a = ap.parse_args()

    ovr = open(a.game_ovr, "rb").read()
    body = ovr[FILE_OFF:FILE_OFF + SPAN_LEN]
    if len(body) != SPAN_LEN or hashlib.sha256(body).hexdigest() != SPAN_SHA:
        print("FAIL the 199-byte span at unit +0x0000 is not the pinned bytes")
        return 1
    os.makedirs(a.work_dir, exist_ok=True)

    rng = random.Random(a.seed)
    cases = [gen_case(rng) for _ in range(a.cases)]
    text = "".join(case_text(c) for c in cases)
    results = [run_unicorn_case(body, c) for c in cases]
    oracle = [r[0] for r in results]
    tot = {k: sum(r[1][k] for r in results) for k in results[0][1]}
    print("COVERAGE (executed in emulation): cases with the first clamp applied (signed >= 2) %d; with the second band (>= 0x2A) %d; "
          "record byte below 2 %d; record word >= 0x22 %d; calls to 0x3E9:0x39 %d; calls to 0x464:0x1B52 %d; stub arity mismatches %d"
          % (sum(1 for r in results if r[1]["b1_ge2"]), sum(1 for r in results if r[1]["b1_ge2a"]),
             sum(1 for r in results if r[1]["rec_lt2"]), sum(1 for r in results if r[1]["rec_ge22"]),
             tot["call39"], tot["call1b52"], tot["arity"]))

    base_bin = os.path.join(a.work_dir, "harness_base")
    build(SRC, base_bin)
    base = split_cases(run_harness(base_bin, text))
    failures = 0
    for idx, (want, got) in enumerate(zip(oracle, base)):
        if want != got:
            failures += 1
            if failures <= 3:
                print("FAIL case %d differs" % idx)
                print("  oracle:", want[:10])
                print("  C     :", got[:10])
    if len(base) != len(oracle):
        print("FAIL case count: oracle %d, C %d" % (len(oracle), len(base)))
        failures += 1
    print("RESULT baseline: %d/%d cases equal" % (len(oracle) - failures, len(oracle)))

    mutant_fail = 0
    src_text = open(SRC).read()
    for name, old, new in MUTANTS:
        if src_text.count(old) != 1:
            print("FAIL mutant %s: anchor not unique (%d)" % (name, src_text.count(old)))
            mutant_fail += 1
            continue
        mpath = os.path.join(a.work_dir, "mutant_%s.c" % name)
        with open(mpath, "w") as f:
            f.write(src_text.replace(old, new))
        mbin = os.path.join(a.work_dir, "harness_%s" % name)
        build(mpath, mbin)
        got = split_cases(run_harness(mbin, text))
        killed = sum(1 for want, g in zip(oracle, got) if want != g)
        print("MUTANT %s: %s (%d of %d cases differ)" % (name, "KILLED" if killed else "SURVIVED", killed, len(oracle)))
        if not killed:
            mutant_fail += 1

    ok = failures == 0 and mutant_fail == 0 and tot["arity"] == 0
    print("RESULT %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
