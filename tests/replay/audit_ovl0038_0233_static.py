#!/usr/bin/env python3
"""Static audit of OVL_0038_0233 (unit 0x38, handler +0233, 324 bytes). Evidence only: no native run, no credit.

Reads the owner's DOS media via --dos-dir (START.EXE and GAME.OVR; hashes pinned below). Prints PASS/FAIL per check and
never prints original bytes. --mutate NAME perturbs the in-memory input (mutant check); the baseline must pass and every
mutant must fail.
"""
import argparse
import hashlib
import pathlib
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from capstone.x86 import X86_OP_IMM

REPO = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools" / "overlay"))
import overlay_inventory as oi  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
UNIT_BASE = 60507           # overlay_inventory ovr_offset of unit 0x38
UNIT_STUB_SEG = 0x41
HANDLER = 0x0233
HANDLER_LEN = 324
BODY_SHA = "18b0cf6db70feb85e97737e9a9cccec0d5b66fec55423fc06a2638288990aa2c"
INSTRUCTIONS = 119
RETF_AT = 0x0374            # the only return; RETF 0Ah
EPILOGUE_AT = 0x0371        # mov sp,bp
CALLS = [(0x0249, 0x0AF8, 0x0B0B), (0x0254, 0x00BA, 0x16EC), (0x027D, 0x00B0, 0x0052)]
CALLBACK_OFF = 0x0228       # `mov di,0x228` at +0244, then `push cs; push di` before the first call
PUSHED_BEFORE = [6, 2, 6]   # words pushed after entry (push bp excluded) before each call, straight-line order
POPPED_WORDS = [2, 6, 6]    # cited callee contracts: RETF 4 (0AF8:0B0B), RETF 0Ch (00BA:16EC), RETF 0Ch (00B0:0052)
RECORD_DISPS = {0x14, 0x32, 0x96, 0x98, 0xB1, 0x11B}   # es:[di+disp] accesses; 0x96 is the base of an 8-byte array
GUARD = {0x032B: "cmp byte ptr [bp - 2], 0", 0x032F: "jne 0x335", 0x0331: "mov byte ptr [bp - 2], 1",
         0x0342: "idiv cx"}
LOOP_BOUND = (0x0325, "cmp byte ptr [bp - 3], 7", 0x0329, "jne 0x2b6")
COUNT_GATE = (0x0263, "cmp byte ptr es:[di + 0x14], 0x14", 0x0268, "jb 0x285")
FIXUP_SEG_WORDS = [0x024C, 0x0257, 0x0280]


def load(dos_dir, mutate):
    exe = bytearray((dos_dir / "START.EXE").read_bytes())
    ovr = (dos_dir / "GAME.OVR").read_bytes()
    if mutate == "exe-byte":
        exe[100] ^= 1
    return bytes(exe), ovr


def decode(body):
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = True
    ins, pos = {}, 0
    while pos < len(body):
        got = list(md.disasm(body[pos:pos + 15], HANDLER + pos))
        if not got:
            break
        ins[got[0].address] = got[0]
        pos += got[0].size
    return ins, pos


def run(dos_dir, mutate="none"):
    results = []

    def check(name, ok, detail=""):
        results.append((name, bool(ok), detail))

    exe, ovr = load(dos_dir, mutate)
    check("START.EXE sha256 pinned", hashlib.sha256(exe).hexdigest() == START_SHA)
    check("GAME.OVR sha256 pinned", hashlib.sha256(ovr).hexdigest() == OVR_SHA)
    mz = oi.parse_mz(exe)
    units = oi.find_units(exe[mz["header_bytes"]:], ovr)
    cov = [u for u in units if u["ovr_offset"] <= UNIT_BASE + HANDLER < u["ovr_offset"] + u["code_size"]]
    check("unit 0x38 found covering the handler", len(cov) == 1)
    u = cov[0] if cov else {"fixups": [], "memory_segment": -1, "ovr_offset": -1}
    check("unit base is 60507 and stub segment 0041",
          u["ovr_offset"] == UNIT_BASE and u["memory_segment"] == UNIT_STUB_SEG, f"{u['ovr_offset']} {u['memory_segment']}")
    fixups = set(u["fixups"])
    body = bytearray(ovr[UNIT_BASE + HANDLER: UNIT_BASE + HANDLER + HANDLER_LEN])
    # ---- in-memory mutants (decoded input only) ----
    if mutate == "retf-count":
        body[RETF_AT + 1 - HANDLER] = 0x0C
    if mutate == "lcall-seg":
        body[0x0249 + 3 - HANDLER] ^= 0x01
    if mutate == "push-nop":
        body[0x027C - HANDLER] = 0x90
    if mutate == "guard-removed":
        for k in range(4):
            body[0x0331 - HANDLER + k] = 0x90
    if mutate == "callback":
        body[0x0244 + 1 - HANDLER] ^= 0x01
    if mutate == "loop-bound":
        body[0x0325 + 3 - HANDLER] = 0x06
    check("handler body sha256 pinned", hashlib.sha256(bytes(body)).hexdigest() == BODY_SHA)
    ins, covered = decode(bytes(body))
    check("decodes to 119 instructions tiling 324 bytes", len(ins) == INSTRUCTIONS and covered == HANDLER_LEN,
          f"{len(ins)} insns, {covered} bytes")
    rets = [a for a, i in ins.items() if i.mnemonic in ("retf", "ret", "iret")]
    check("single return, RETF 0Ah at +0374", rets == [RETF_AT] and ins[RETF_AT].op_str == "0xa",
          str([hex(a) for a in rets]))
    # far calls: target and position
    got = []
    for a in sorted(ins):
        i = ins[a]
        if i.mnemonic == "lcall":
            s, o = i.op_str.split(",")
            got.append((a, int(s, 16), int(o, 16)))
    check("three far calls with pinned sites and targets in order", got == CALLS, str([(hex(a), hex(s), hex(o)) for a, s, o in got]))
    check("callback CS:0228 set by mov di and pushed before call 1",
          ins.get(0x0244) is not None and ins[0x0244].mnemonic == "mov" and ins[0x0244].op_str == f"di, {hex(CALLBACK_OFF)}"
          and ins.get(0x0247) is not None and ins[0x0247].mnemonic == "push" and ins[0x0247].op_str == "cs"
          and ins.get(0x0248) is not None and ins[0x0248].mnemonic == "push" and ins[0x0248].op_str == "di")
    # straight-line pushes between calls: entry (after push bp) to call 1, call 1 to call 2, call 2 to call 3
    bounds = [HANDLER] + [a for a, _, _ in got]
    pushed = []
    for lo, hi in zip(bounds, bounds[1:]):
        n = sum(1 for a in sorted(ins) if lo <= a < hi and ins[a].mnemonic == "push" and ins[a].op_str != "bp")
        pushed.append(n)
    check("pushed words per call are 6, 2, 6", pushed == PUSHED_BEFORE, str(pushed))
    leftover, balanced = 0, True
    for p, q in zip(pushed, POPPED_WORDS):
        leftover += p - q
        balanced = balanced and leftover >= 0
    check("stack balances to 0 leftover words under cited contracts", balanced and leftover == 0, f"leftover {leftover}")
    check("segment words of the three far calls are loader fixups", all(a in fixups for a in FIXUP_SEG_WORDS),
          str([hex(a) for a in FIXUP_SEG_WORDS]))
    # division guard and divisor
    guard_ok = all(a in ins and (ins[a].mnemonic + (" " + ins[a].op_str if ins[a].op_str else "")) == m
                   for a, m in GUARD.items())
    check("division guard present and divisor is CX", guard_ok, "")
    a0, m0, a1, m1 = LOOP_BOUND
    check("index loop bounded at 7 (eight iterations)",
          ins.get(a0) is not None and ins[a0].mnemonic == "cmp" and ins[a0].op_str == "byte ptr [bp - 3], 7"
          and ins.get(a1) is not None and ins[a1].mnemonic == "jne" and ins[a1].op_str == "0x2b6")
    b0, n0, b1, n1 = COUNT_GATE
    check("record count gate at 0x14 selects the call-3 path",
          ins.get(b0) is not None and ins[b0].mnemonic == "cmp" and ins[b0].op_str == "byte ptr es:[di + 0x14], 0x14"
          and ins.get(b1) is not None and ins[b1].mnemonic == "jb" and ins[b1].op_str == "0x285")
    disps = set()
    for a, i in ins.items():
        txt = i.op_str
        if "es:[di +" in txt:
            disps.add(int(txt.split("es:[di +")[1].split("]")[0].strip(), 16))
    check("record displacements are exactly the pinned set", disps == RECORD_DISPS, str(sorted(hex(d) for d in disps)))
    return results


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dos-dir", required=True, type=pathlib.Path)
    ap.add_argument("--mutate", default="none", choices=["none", "exe-byte", "retf-count", "lcall-seg", "push-nop",
                                                         "guard-removed", "callback", "loop-bound"])
    a = ap.parse_args()
    results = run(a.dos_dir, a.mutate)
    for name, ok, detail in results:
        print(("PASS " if ok else "FAIL ") + name + (f"  [{detail}]" if detail else ""))
    failed = [r for r in results if not r[1]]
    print(f"mutate={a.mutate} checks={len(results)} failed={len(failed)}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
