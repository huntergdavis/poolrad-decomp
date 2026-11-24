#!/usr/bin/env python3
"""Static audit of OVL_0038_017F (unit 0x38, handler +017F, 68 bytes). Evidence only: no native run, no credit.

Reads the owner's DOS media via --dos-dir (START.EXE and GAME.OVR, hashes pinned below). Prints PASS/FAIL per check
and never prints original bytes. --mutate NAME perturbs the decoded input (mutant check); baseline must pass.
"""
import argparse
import hashlib
import pathlib
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

REPO = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools" / "overlay"))
import overlay_inventory as oi  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
UNIT_BASE = 60507           # file offset of unit 0x38 code (overlay_inventory ovr_offset)
UNIT_STUB_SEG = 0x41        # overlay_inventory memory_segment for this unit (stub segment 0041)
HANDLER = 0x017F
HANDLER_LEN = 68
BODY_SHA = "62eda393ae8de0e238aba4c49606f91f4604171887ff26c3dd2695a381139fe8"
INSTRUCTIONS = 31
CALLS = [(0x0AF8, 0x0B0B), (0x00BA, 0x16EC), (0x00B0, 0x0052)]   # lcall targets, in body order
CALLBACK_OFF = 0x016F       # `mov di, 0x16f` then `push cs; push di`: far pointer passed to the first call
# Words pushed before each lcall (counted from the previous call, or from entry after `push bp`), and the words each
# callee pops, taken from the callee contracts documented in docs/re (RETF 4 for 0AF8:0B0B, RETF 0Ch for 00BA:16EC
# and 00B0:0052). The stack must balance to zero leftover words after the last call.
PUSHED_WORDS = [6, 2, 6]
POPPED_WORDS = [2, 6, 6]
FIXUP_SEG_WORDS = [HANDLER + 0x16 + 3, HANDLER + 0x21 + 3, HANDLER + 0x39 + 3]


def load(dos_dir, mutate):
    exe = (dos_dir / "START.EXE").read_bytes()
    ovr = (dos_dir / "GAME.OVR").read_bytes()
    if mutate == "exe-byte":
        exe = exe[:100] + bytes([exe[100] ^ 1]) + exe[101:]
    return exe, ovr


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
    u = cov[0] if cov else None
    if u is not None:
        check("unit base is 60507", u["ovr_offset"] == UNIT_BASE, str(u["ovr_offset"]))
        check("unit stub segment is 0041", u["memory_segment"] == UNIT_STUB_SEG, str(u["memory_segment"]))
        fixups = set(u["fixups"])
    else:
        fixups = set()
    body = ovr[UNIT_BASE + HANDLER: UNIT_BASE + HANDLER + HANDLER_LEN]
    if mutate == "body-byte":
        body = body[:0x41] + bytes([body[0x41] ^ 1]) + body[0x42:]     # retf count byte
    if mutate == "push-count":
        body = body[:0x20] + b"\x90" + body[0x21:]                   # second call's push ax -> nop
    if mutate == "lcall-seg":
        body = body[:0x1A] + bytes([body[0x1A] ^ 1]) + body[0x1B:]     # first lcall segment word
    check("handler body sha256 pinned", hashlib.sha256(body).hexdigest() == BODY_SHA)

    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = True
    ins, pos = [], 0
    while pos < len(body):
        got = list(md.disasm(body[pos:pos + 15], pos))
        if not got:
            break
        ins.append(got[0])
        pos += got[0].size
    check("decodes to 31 instructions tiling 68 bytes", len(ins) == INSTRUCTIONS and pos == HANDLER_LEN,
          f"{len(ins)} insns, {pos} bytes")
    last = ins[-1] if ins else None
    check("final instruction is RETF 0Ah", last is not None and last.mnemonic == "retf" and last.op_str == "0xa")
    lcalls = [i for i in ins if i.mnemonic == "lcall"]
    got_calls = []
    for i in lcalls:
        seg_s, off_s = i.op_str.split(",")
        got_calls.append((int(seg_s.strip(), 16), int(off_s.strip(), 16)))
    check("three far calls with pinned targets", got_calls == CALLS, str(got_calls))
    check("callback offset 0x016F loaded into DI", any(i.mnemonic == "mov" and i.op_str == "di, 0x16f" for i in ins))
    # stack balance under the documented callee contracts: words pushed before each call, minus words the callee pops
    pushes_before, count = [], 0
    for i_ in ins:
        if i_.mnemonic == "lcall":
            pushes_before.append(count)
            count = 0
        elif i_.mnemonic == "push" and i_.op_str != "bp":
            count += 1
    check("pushed words per call match (6, 2, 6)", pushes_before == PUSHED_WORDS, str(pushes_before))
    leftover, balanced = 0, True
    for pushed, popped in zip(pushes_before, POPPED_WORDS):
        leftover += pushed - popped
        if leftover < 0:
            balanced = False
    check("stack balances to 0 leftover words after the last call", balanced and leftover == 0,
          f"leftover {leftover}")
    check("segment words of the three far calls are loader fixups",
          all(a in fixups for a in FIXUP_SEG_WORDS) if u is not None else False,
          str([hex(a) for a in FIXUP_SEG_WORDS]))
    return results


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dos-dir", required=True, type=pathlib.Path)
    ap.add_argument("--mutate", default="none", choices=["none", "exe-byte", "body-byte", "lcall-seg", "push-count"])
    a = ap.parse_args()
    results = run(a.dos_dir, a.mutate)
    for name, ok, detail in results:
        print(("PASS " if ok else "FAIL ") + name + (f"  [{detail}]" if detail else ""))
    failed = [r for r in results if not r[1]]
    print(f"mutate={a.mutate} checks={len(results)} failed={len(failed)}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
