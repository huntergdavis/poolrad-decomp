#!/usr/bin/env python3
"""Controlled original-byte U3B0 +0E06 -> +0067 -> +0E09 evidence."""

from hashlib import sha256
import json
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BP, UC_X86_REG_BX, UC_X86_REG_CS,
    UC_X86_REG_CX, UC_X86_REG_DI, UC_X86_REG_DS, UC_X86_REG_DX,
    UC_X86_REG_EFLAGS, UC_X86_REG_ES, UC_X86_REG_IP, UC_X86_REG_SI,
    UC_X86_REG_SP, UC_X86_REG_SS,
)

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "resources/u3b0-0e06-caller-gate"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LEAF_SHA = "965144eab404415b7ce5d3f6e22d1c6991af4725dac559160302d89305468693"
CALLER_SHA = "d23c4f4e4838df0b499375696ce48731a564594277605903bee06c3ee1b1fdf8"
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
CS = 0xF000
SS = 0x7000
SP = 0x7F72
BP = 0x8100
CPU = {
    UC_X86_REG_AX: 0xABCD, UC_X86_REG_BX: 0x1357,
    UC_X86_REG_CX: 0x2468, UC_X86_REG_DX: 0x3579,
    UC_X86_REG_SI: 0x468A, UC_X86_REG_DI: 0x579B,
    UC_X86_REG_BP: BP, UC_X86_REG_SP: SP,
    UC_X86_REG_CS: CS, UC_X86_REG_IP: 0x0DFF,
    UC_X86_REG_DS: 0x6000, UC_X86_REG_ES: 0x8000,
    UC_X86_REG_SS: SS, UC_X86_REG_EFLAGS: 0x0202,
}


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & MASK] = value >> 8


def get16(ram, seg, off):
    at = physical(seg, off)
    return ram[at] | (ram[(at + 1) & MASK] << 8)


def original_bytes():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    base = 0x2C8CF
    leaf = ovr[base + 0x0067:base + 0x00AB]
    caller = ovr[base + 0x0DFF:base + 0x0E0C]
    assert len(leaf) == 68 and sha256(leaf).hexdigest() == LEAF_SHA
    assert len(caller) == 13 and sha256(caller).hexdigest() == CALLER_SHA
    assert ovr[base + 0x0DAF:base + 0x0DB4] == bytes.fromhex("9a250bf80a")
    leaf_ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(leaf, 0x0067))
    caller_ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(caller, 0x0DFF))
    assert len(leaf_ins) == 25 and leaf_ins[-1].address == 0x00A8
    assert [(i.address, i.mnemonic) for i in caller_ins] == [
        (0x0DFF, "push"), (0x0E02, "push"), (0x0E05, "push"),
        (0x0E06, "call"), (0x0E09, "mov"),
    ]
    assert caller[7:10] == bytes.fromhex("e85ef2")
    return leaf, caller


def run_case(leaf, caller, count):
    assert count in (0, 1, 2, 3)
    ram = bytearray(RAM_SIZE)
    leaf_at = physical(CS, 0x0067)
    caller_at = physical(CS, 0x0DFF)
    ram[leaf_at:leaf_at + len(leaf)] = leaf
    ram[caller_at:caller_at + len(caller)] = caller
    put16(ram, SS, BP + 0x14, 0x2200 if count else 0)
    put16(ram, SS, BP + 0x16, 0x5000 if count else 0)
    for index in range(count):
        off = 0x2200 + 0x100 * index
        put16(ram, 0x5000, off + 0x2A,
              off + 0x100 if index + 1 < count else 0)
        put16(ram, 0x5000, off + 0x2C,
              0x5000 if index + 1 < count else 0)
    before = bytes(ram)
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, before)
    for reg, value in CPU.items():
        emu.reg_write(reg, value)
    trace = []

    def on_code(uc, address, size, user):
        del size, user
        cs = uc.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = uc.reg_read(UC_X86_REG_IP) & 0xFFFF
        assert address == physical(cs, ip)
        assert cs == CS
        assert ip in range(0x0067, 0x00AB) or ip in range(0x0DFF, 0x0E0D)
        trace.append(ip)
        if ip == 0x0E0C:
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(caller_at, RAM_SIZE, count=1000)
    assert trace[:5] == [0x0DFF, 0x0E02, 0x0E05, 0x0E06, 0x0067]
    assert trace[-2:] == [0x0E09, 0x0E0C]
    assert trace.count(0x0067) == 1
    assert trace.count(0x00A8) == 1
    assert emu.reg_read(UC_X86_REG_AX) & 0xFFFF == count
    assert emu.reg_read(UC_X86_REG_SP) & 0xFFFF == SP
    assert emu.reg_read(UC_X86_REG_BP) & 0xFFFF == BP
    assert emu.reg_read(UC_X86_REG_CS) & 0xFFFF == CS
    after = bytes(emu.mem_read(0, RAM_SIZE))
    assert get16(after, SS, BP - 0x56) == count
    allowed = {physical(SS, off) for off in range(SP - 18, SP)}
    allowed |= {physical(SS, BP - 0x56 + i) for i in range(2)}
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    assert changed <= allowed, sorted(changed - allowed)[:12]
    return {
        "node_count": count,
        "trace_instructions": len(trace) - 1,
        "reached_call": "0E06 -> 0067",
        "reached_successor": "0E09 store -> 0E0C",
        "return_ax": count,
        "caller_local_bp_minus_56": get16(after, SS, BP - 0x56),
        "stack_restored": True,
        "changed_ram_bytes": len(changed),
    }


def main():
    leaf, caller = original_bytes()
    cases = [run_case(leaf, caller, n) for n in (0, 1, 2, 3)]
    report = {
        "leaf_sha256": LEAF_SHA,
        "caller_sha256": CALLER_SHA,
        "first_entry4_dependency": "U3B0 +0DAF LCALL 0AF8:0B25",
        "start": "controlled original-byte U3B0 +0DFF after that dependency",
        "natural_saved_game_reach": False,
        "cases": cases,
    }
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("U3B0 +0E06 controlled original-byte caller: 4 cases PASS")


if __name__ == "__main__":
    main()
