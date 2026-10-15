#!/usr/bin/env python3
"""Controlled original-byte U3B0 entry4 through two SYSTEM copies."""

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
OUT = ROOT / "resources/u3b0-0daf-predecessor"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PREFIX_SHA = "489519ea721f349921a6888b4172dc71c7ea692def347afc64dfc1afb0deea62"
COPY_SHA = "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"
LEAF_SHA = "965144eab404415b7ce5d3f6e22d1c6991af4725dac559160302d89305468693"
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
CS, SS, DS, SRC = 0xF000, 0x7000, 0x6000, 0x5000
ENTRY_SP = 0x8000
BP = ENTRY_SP - 2
LOCAL_SP = BP - 0x18E


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & MASK] = (value >> 8) & 255


def get16(ram, seg, off):
    at = physical(seg, off)
    return ram[at] | (ram[(at + 1) & MASK] << 8)


def original_bytes():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    base = 0x2C8CF
    prefix = ovr[base + 0x0D9A:base + 0x0E0C]
    leaf = ovr[base + 0x0067:base + 0x00AB]
    copy = exe[0xB79B:0xB7BF]
    assert len(prefix) == 114 and sha256(prefix).hexdigest() == PREFIX_SHA
    assert len(leaf) == 68 and sha256(leaf).hexdigest() == LEAF_SHA
    assert len(copy) == 36 and sha256(copy).hexdigest() == COPY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(prefix, 0x0D9A))
    assert [(i.address, i.bytes.hex()) for i in ins
            if i.mnemonic == "lcall"] == [
                (0x0DAF, "9a250bf80a"), (0x0DC2, "9a250bf80a")]
    assert prefix[-6:-3] == bytes.fromhex("e85ef2")
    return prefix, leaf, copy


def source(ram, off, length, seed):
    assert 0 <= length <= 60
    ram[physical(SRC, off)] = length
    for i in range(length):
        ram[physical(SRC, off + 1 + i)] = (seed + i * 13) & 255


def run_case(prefix, leaf, copy, count, lengths):
    ram = bytearray(RAM_SIZE)
    ram[physical(CS, 0x0D9A):physical(CS, 0x0D9A) + len(prefix)] = prefix
    ram[physical(CS, 0x0067):physical(CS, 0x0067) + len(leaf)] = leaf
    ram[physical(0x0AF8, 0x0B25):physical(0x0AF8, 0x0B25) + len(copy)] = copy
    put16(ram, SS, ENTRY_SP, 0x1234)
    put16(ram, SS, ENTRY_SP + 2, 0xD000)
    put16(ram, SS, BP + 0x2A, 0x1000)
    put16(ram, SS, BP + 0x2C, SRC)
    put16(ram, SS, BP + 0x26, 0x1100)
    put16(ram, SS, BP + 0x28, SRC)
    put16(ram, SS, BP + 0x14, 0x2200)
    put16(ram, SS, BP + 0x16, SRC)
    ram[physical(SS, BP + 0x1A)] = 2
    ram[physical(SS, BP + 0x1E)] = 5
    source(ram, 0x1000, lengths[0], 0x21)
    source(ram, 0x1100, lengths[1], 0x62)
    for i in range(count):
        off = 0x2200 + i * 0x100
        put16(ram, SRC, off + 0x2A,
              off + 0x100 if i + 1 < count else 0)
        put16(ram, SRC, off + 0x2C,
              SRC if i + 1 < count else 0)
    before = bytes(ram)
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, before)
    registers = {
        UC_X86_REG_AX: 0xABCD, UC_X86_REG_BX: 0x1357,
        UC_X86_REG_CX: 0x2468, UC_X86_REG_DX: 0x3579,
        UC_X86_REG_SI: 0x468A, UC_X86_REG_DI: 0x579B,
        UC_X86_REG_BP: 0x68AC, UC_X86_REG_SP: ENTRY_SP,
        UC_X86_REG_CS: CS, UC_X86_REG_IP: 0x0D9A,
        UC_X86_REG_DS: DS, UC_X86_REG_ES: 0x8000,
        UC_X86_REG_SS: SS, UC_X86_REG_EFLAGS: 0x0202,
    }
    for reg, value in registers.items():
        emu.reg_write(reg, value)
    trace = []
    boundaries = {}

    def on_code(uc, address, size, user):
        del size, user
        cs = uc.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = uc.reg_read(UC_X86_REG_IP) & 0xFFFF
        assert address == physical(cs, ip)
        assert (cs == CS and (0x0D9A <= ip <= 0x0E0C or
                              0x0067 <= ip <= 0x00AA)) or (
                                  cs == 0x0AF8 and 0x0B25 <= ip <= 0x0B46)
        trace.append((cs, ip))
        if cs == CS and ip in (0x0DAF, 0x0DB4, 0x0DC2, 0x0DC7,
                               0x0E06, 0x0E09, 0x0E0C):
            boundaries[ip] = {
                "sp": uc.reg_read(UC_X86_REG_SP) & 0xFFFF,
                "ds": uc.reg_read(UC_X86_REG_DS) & 0xFFFF,
            }
        if cs == CS and ip == 0x0E0C:
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, on_code)
    emu.emu_start(physical(CS, 0x0D9A), RAM_SIZE, count=3000)
    assert trace[0] == (CS, 0x0D9A) and trace[-1] == (CS, 0x0E0C)
    assert trace.count((0x0AF8, 0x0B25)) == 2
    assert trace.count((CS, 0x0067)) == 1
    assert trace.count((CS, 0x0E09)) == 1
    assert set(boundaries) == {0x0DAF, 0x0DB4, 0x0DC2, 0x0DC7,
                              0x0E06, 0x0E09, 0x0E0C}
    assert all(boundaries[ip]["ds"] == DS for ip in (0x0DB4, 0x0DC7))
    assert boundaries[0x0DAF]["sp"] == LOCAL_SP - 10
    assert boundaries[0x0DB4]["sp"] == LOCAL_SP
    assert boundaries[0x0DC2]["sp"] == LOCAL_SP - 10
    assert boundaries[0x0DC7]["sp"] == LOCAL_SP
    assert boundaries[0x0E0C]["sp"] == LOCAL_SP
    assert emu.reg_read(UC_X86_REG_BP) & 0xFFFF == BP
    assert emu.reg_read(UC_X86_REG_AX) & 0xFFFF == count
    after = bytes(emu.mem_read(0, RAM_SIZE))
    for source_off, dest_off, length, seed in (
        (0x1000, BP - 0x2A, lengths[0], 0x21),
        (0x1100, BP - 0x53, lengths[1], 0x62),
    ):
        copied = min(length, 0x28)
        assert after[physical(SS, dest_off)] == copied
        assert bytes(after[physical(SS, dest_off + 1 + i)]
                     for i in range(copied)) == bytes(
                         (seed + i * 13) & 255 for i in range(copied))
        assert after[physical(SRC, source_off)] == length
    assert after[physical(DS, 0x6B2B)] == 1
    assert get16(after, SS, BP - 0x56) == count
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    allowed = {physical(SS, off) for off in range(LOCAL_SP - 24, ENTRY_SP)}
    allowed.add(physical(DS, 0x6B2B))
    assert changed <= allowed, sorted(changed - allowed)[:12]
    return {
        "node_count": count, "source_lengths": lengths,
        "copied_lengths": [min(n, 0x28) for n in lengths],
        "trace_instructions": len(trace) - 1,
        "copy_entries": trace.count((0x0AF8, 0x0B25)),
        "node_count_stored": get16(after, SS, BP - 0x56),
        "changed_ram_bytes": len(changed),
        "stop": "U3B0 +0E0C",
    }


def main():
    prefix, leaf, copy = original_bytes()
    cases = [run_case(prefix, leaf, copy, count, lengths)
             for count, lengths in ((1, (0, 0)), (2, (5, 9)),
                                    (3, (44, 60)))]
    report = {
        "prefix_sha256": PREFIX_SHA, "copy_sha256": COPY_SHA,
        "leaf_sha256": LEAF_SHA,
        "start": "controlled original-byte U3B0 entry4 +0D9A",
        "natural_saved_game_reach": False,
        "cases": cases,
    }
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("U3B0 +0DAF/+0DC2 predecessor copies to +0E0C: 3 controlled cases PASS")


if __name__ == "__main__":
    main()
