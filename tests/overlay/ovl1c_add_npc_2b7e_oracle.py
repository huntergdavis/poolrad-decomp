#!/usr/bin/env python3
"""Differential oracle for unit 0x1c +0x2B7E (ECL opcode 0x36, ADD NPC), with live active-record resolution.

The 133 owner bytes at GAME.OVR file offset 0x038C4 (inclusive span 0x038C4..0x03948) run under Unicorn (16-bit real mode),
loaded at unit offset 0x2B7E of the code segment 0x0100 so the unit's own jump targets resolve, entered at IP 0x2B7E.
The active far pointer is the word pair DS:0x5D92 (offset) and DS:0x5D94 (segment). Three far records (A, B, C) are
0x11D-byte regions at segments 0x3000, 0x4000 and 0x5000. Five external calls go to stubs keyed by their target:
  lcall 0x2B:0x2A      loader, one word, RETF 2
  lcall 0x2B:0x25      slot value, one word, result in AX, RETF 2
  lcall 0x7C:0x43      add-NPC, one word, RETF 2; it may retarget the active pointer (a successful insertion does)
  lcall 0xBA:0x0BB8    resident, segment then offset, RETF 4; it may retarget the active pointer
  lcall 0xBA:0x0767    resident, segment then offset, RETF 4
Each stub logs its arguments, applies the case's retarget (after logging), returns the case's slot word for slot values,
and pops its arguments with RETF. The same cases run through the C model (the vector driver
tests/overlay/ovl1c_add_npc_2b7e_harness.c linked with src/recovered/overlay_u1c_add_npc_2b7e.c and src/recovered/ecl_helpers.c
is not needed by the driver, which binds only the model). Compared per case: the call log (with the segment:offset of each
resident call), the status, and every byte of each record that differs from its input.

Status: the stub stack discipline must hold (SP at each stub entry = entry SP - 12 - 2 x argument words, SP and BP restored at
the end). Mutants: one-edit copies of the translation must each differ on at least one case. The owner GAME.OVR is read at
run time and never committed; the work directory must be outside the repository.

Scope: the stubs return OK slot statuses only. HOOK, BOUNDS, UNRESOLVED and ARGUMENT statuses are covered by the focused test.
The oracle always supplies resolvable pointers; the unresolvable-replacement case is covered by the focused test.

Usage: ovl1c_add_npc_2b7e_oracle.py --game-ovr PATH --work-dir DIR [--cases N] [--seed S]
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
SRC = os.path.join(ROOT, "src", "recovered", "overlay_u1c_add_npc_2b7e.c")
INC = os.path.join(ROOT, "include")
HARNESS = os.path.join(ROOT, "tests", "overlay", "ovl1c_add_npc_2b7e_harness.c")
ECL_SRC = os.path.join(ROOT, "src", "recovered", "ecl_helpers.c")

FILE_OFF = 0x038C4
SPAN_LEN = 133
SPAN_SHA = "5b711971cd12bfbcd60d87f2ac6a6cb836292b4729229417fa73962d48efeca1"
UNIT_OFF = 0x2B7E
CS_SEG = 0x0100
RET_CS = 0x0000
RET_IP = 0x0F00
DS_SEG = 0x2000
SS_SEG = 0x9000
REC_LEN = 0x11D
ACTIVE_OFF_ADDR = 0x5D92   # DS:0x5D92 offset of the active far pointer
ACTIVE_SEG_ADDR = 0x5D94   # DS:0x5D94 segment of the active far pointer
REC_SEGS = [0x3000, 0x4000, 0x5000]   # records A, B, C

# (target segment, target offset): (name, word arguments)
STUBS = {
    (0x2B, 0x2A): ("load", 1),
    (0x2B, 0x25): ("slot", 1),
    (0x7C, 0x43): ("npc", 1),
    (0xBA, 0x0BB8): ("r0bb8", 2),
    (0xBA, 0x0767): ("r0767", 2),
}

MUTANTS = [
    ("class-without-or-80", "r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 1) | 0x80u);",
     "r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 1));"),
    ("class-shift-2", "r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 1) | 0x80u);",
     "r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 2) | 0x80u);"),
    ("flag-id-19", "(v1 & 0xFFu) == PR_U1C_ADD_NPC_FLAG_ID ? 1u : 0u",
     "(v1 & 0xFFu) == 0x19u ? 1u : 0u"),
    ("flag-inverted", "(v1 & 0xFFu) == PR_U1C_ADD_NPC_FLAG_ID ? 1u : 0u",
     "(v1 & 0xFFu) == PR_U1C_ADD_NPC_FLAG_ID ? 0u : 1u"),
    ("npc-word-low-byte-only", "h->add_npc(h->context, v1);",
     "h->add_npc(h->context, (uint16_t)(v1 & 0xFFu));"),
    ("class-stored-at-flag", "bytes[PR_U1C_ADD_NPC_OFF_CLASS] = r->class_byte;",
     "bytes[PR_U1C_ADD_NPC_OFF_FLAG] = r->class_byte;"),
    ("resident-0767-uses-0bb8-pair", "h->resident_0767(h->context, r->use_segment[PR_U1C_ADD_NPC_USE_RES_0767],",
     "h->resident_0767(h->context, r->use_segment[PR_U1C_ADD_NPC_USE_RES_0BB8],"),
]


def rd16(uc, lin):
    return int.from_bytes(uc.mem_read(lin, 2), "little")


def gen_case(rng):
    case = {"slot": {1: rng.randrange(65536), 2: rng.randrange(65536)},
            "rec": [(rng.choice([seg]), rng.randrange(0x0000, 0xE000)) for seg in REC_SEGS],
            "data": [bytes(rng.randrange(256) for _ in range(REC_LEN)) for _ in REC_SEGS],
            "active": rng.randrange(3),
            "npc_retarget": rng.choice([-1, -1, 0, 1, 2]),
            "r0bb8_retarget": rng.choice([-1, -1, 0, 1, 2]),
            "entry_sp": rng.randrange(0x0400, 0xE000), "bp0": rng.randrange(65536)}
    if rng.random() < 0.3:
        case["slot"][1] = (case["slot"][1] & 0xFF00) | 0x18
    if rng.random() < 0.3:
        case["slot"][2] = (case["slot"][2] & 0xFF00) | rng.choice([0x00, 0x01, 0x7F, 0x80, 0xFF])
    return case


def case_text(case):
    lines = ["case", "slot 1 %d" % case["slot"][1], "slot 2 %d" % case["slot"][2]]
    for idx, (seg, off) in enumerate(case["rec"]):
        lines.append("rec %d %d %d" % (idx, seg, off))
        lines.append("recdata %d %s" % (idx, case["data"][idx].hex()))
    lines += ["active %d" % case["active"], "npc_retarget %d" % case["npc_retarget"],
              "r0bb8_retarget %d" % case["r0bb8_retarget"], "end"]
    return "\n".join(lines) + "\n"


def run_unicorn_case(body, case):
    log = []
    cov = {"retarget_npc": 0, "retarget_0bb8": 0, "r0767_moved": 0, "calls": 0, "arity": 0}
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 0x100000, UC_PROT_ALL)
    uc.mem_write(CS_SEG * 16 + UNIT_OFF, body)
    for idx, (seg, off) in enumerate(case["rec"]):
        uc.mem_write(seg * 16 + off, case["data"][idx])

    def set_active(idx):
        seg, off = case["rec"][idx]
        uc.mem_write(DS_SEG * 16 + ACTIVE_OFF_ADDR, off.to_bytes(2, "little"))
        uc.mem_write(DS_SEG * 16 + ACTIVE_SEG_ADDR, seg.to_bytes(2, "little"))

    set_active(case["active"])
    entry_sp = case["entry_sp"]
    uc.mem_write(SS_SEG * 16 + entry_sp, RET_IP.to_bytes(2, "little") + RET_CS.to_bytes(2, "little"))
    uc.reg_write(UC_X86_REG_CS, CS_SEG)
    uc.reg_write(UC_X86_REG_IP, UNIT_OFF)
    uc.reg_write(UC_X86_REG_DS, DS_SEG)
    uc.reg_write(UC_X86_REG_ES, 0)
    uc.reg_write(UC_X86_REG_SS, SS_SEG)
    uc.reg_write(UC_X86_REG_SP, entry_sp)
    uc.reg_write(UC_X86_REG_BP, case["bp0"])
    uc.reg_write(UC_X86_REG_AX, 0x5A5A)
    first_0767_seen = {"seen": False}

    def on_code(mu, address, size, _user):
        cs = mu.reg_read(UC_X86_REG_CS)
        ip = mu.reg_read(UC_X86_REG_IP)
        key = (cs, ip)
        if key not in STUBS:
            return
        name, nwords = STUBS[key]
        sp = mu.reg_read(UC_X86_REG_SP)
        ss = mu.reg_read(UC_X86_REG_SS) * 16

        def word_at(k):
            return rd16(mu, ss + ((sp + k) & 0xFFFF))

        # pushed words in push order: the first pushed is the deepest
        args = [word_at(4 + 2 * (nwords - 1 - k)) for k in range(nwords)]
        ret_ip, ret_cs = word_at(0), word_at(2)
        if sp != (entry_sp - 12 - 2 * nwords) & 0xFFFF:
            cov["arity"] += 1
        cov["calls"] += 1
        if name == "load":
            log.append("load %d" % (args[0] & 0xFF))
        elif name == "slot":
            slot = args[0] & 0xFF
            value = case["slot"][slot]
            log.append("slot %d %d" % (slot, value))
            mu.reg_write(UC_X86_REG_AX, value)
        elif name == "npc":
            log.append("npc %d" % args[0])
            if case["npc_retarget"] >= 0:
                set_active(case["npc_retarget"])
                cov["retarget_npc"] += 1
        elif name == "r0bb8":
            log.append("r0bb8 %d %d" % (args[0], args[1]))
            if case["r0bb8_retarget"] >= 0:
                set_active(case["r0bb8_retarget"])
                cov["retarget_0bb8"] += 1
        elif name == "r0767":
            log.append("r0767 %d %d" % (args[0], args[1]))
            if case["r0bb8_retarget"] >= 0 and case["r0bb8_retarget"] != case["active"]:
                cov["r0767_moved"] += 1
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
        uc.emu_start(CS_SEG * 16 + UNIT_OFF, RET_CS * 16 + RET_IP, timeout=5000000)
    except UcError as e:
        raise RuntimeError("oracle emulation stopped (%s); %s" % (e, "; ".join(faults) or "no unmapped access"))
    if faults:
        raise RuntimeError("oracle touched unmapped memory: " + "; ".join(faults))
    returned = uc.reg_read(UC_X86_REG_CS) == RET_CS and uc.reg_read(UC_X86_REG_IP) == RET_IP
    balanced = uc.reg_read(UC_X86_REG_SP) == (entry_sp + 4) & 0xFFFF and uc.reg_read(UC_X86_REG_BP) == case["bp0"]
    ok = returned and balanced and cov["arity"] == 0
    out = list(log)
    out.append("status 0" if ok else "status 9")
    for idx, (seg, off) in enumerate(case["rec"]):
        final = bytes(uc.mem_read(seg * 16 + off, REC_LEN))
        for k in range(REC_LEN):
            if final[k] != case["data"][idx][k]:
                out.append("rec %d %d %d" % (idx, k, final[k]))
    out.append("endcase")
    return out, cov


def run_harness(binary, text):
    res = subprocess.run([binary], input=text, capture_output=True, text=True, timeout=600)
    if res.returncode != 0:
        raise RuntimeError("harness failed: %s" % res.stderr.strip())
    return res.stdout.splitlines()


def build(src, out):
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", INC, src, ECL_SRC, HARNESS, "-o", out],
                   check=True)


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
        print("FAIL the 133-byte span at file 0x038C4 (unit +0x2B7E..+0x2C02) is not the pinned bytes")
        return 1
    os.makedirs(a.work_dir, exist_ok=True)

    rng = random.Random(a.seed)
    cases = [gen_case(rng) for _ in range(a.cases)]
    text = "".join(case_text(c) for c in cases)
    results = [run_unicorn_case(body, c) for c in cases]
    oracle = [r[0] for r in results]
    tot = {k: sum(r[1][k] for r in results) for k in results[0][1]}
    print("COVERAGE (executed in emulation): calls %d; add-NPC retargets %d; 0BB8 retargets %d; "
          "0767 sees a pointer moved from the entry record %d; stub arity mismatches %d"
          % (tot["calls"], tot["retarget_npc"], tot["retarget_0bb8"], tot["r0767_moved"], tot["arity"]))

    base_bin = os.path.join(a.work_dir, "harness_base")
    build(SRC, base_bin)
    base = split_cases(run_harness(base_bin, text))
    failures = 0
    for idx, (want, got) in enumerate(zip(oracle, base)):
        if want != got:
            failures += 1
            if failures <= 3:
                print("FAIL case %d differs" % idx)
                print("  oracle:", want[:12])
                print("  C     :", got[:12])
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
