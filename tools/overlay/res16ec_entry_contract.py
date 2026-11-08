#!/usr/bin/env python3
"""RES_00BA_16EC admitted-entry contract: static original-byte evidence for t24's NC1 and NC5.

Enumerates every direct entry of 00BA:16EC (relocated overlay far calls and decoded
resident near calls), classifies each entry's argument setup, derives a read-time
bound on the message length by a forward dataflow over the owner body, and checks
the selector-read invariant (DS:49F3 and DS:6DDA are read after StrStore and cannot
alias its frame write because the real stack segment lies above the data segment).

This is static evidence only: no execution, no natural-arrival claim, and no
identity, readiness, closure or credit verdict. The reviewer decides those.
See docs/re/RES_00BA_16EC_ENTRY_CONTRACT.md.
"""
import argparse
import csv
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_REG_BP, X86_REG_SP, X86_REG_SS
from far_call_sites import sites
from function_queue import DIGESTS
from overlay_inventory import find_units, parse_mz
from resident_ds_operands import load_image, IMAGE_DIGEST

ROOT = Path(__file__).resolve().parents[2]
TARGET = (0x00BA, 0x16EC)
BODY_SHA256 = 'c8507a56b6044087a23b724d368ac78558bae1ce5fb42f6eace1d29985feb929'
DATA_SEG = 0x0C7C                 # SYSTEM init 0AF8:0000 MOV DX,0C7Ch; MOV DS,DX (checked below)
SELECTOR_CELLS = {'mode_49F3': 0x49F3, 'colour_6DDA': 0x6DDA}
EXEPACK = dict(real_ip=0x0006, real_cs=0x0000, real_sp=0x4000, real_ss=0x14F2, dest_len=0x10BF)
SYSTEM_SEG = 0x0AF8
STRSTORE, STRLOAD, STRCAT = (0x0AF8, 0x0B25), (0x0AF8, 0x0B0B), (0x0AF8, 0x0B98)
NEAR_SEGMENT = (0x00BA, 0x03F1)   # the resident segment holding 16EC and its next segment
UNBOUNDED = 255
MD = Cs(CS_ARCH_X86, CS_MODE_16)
MD.detail = True
PUSHFORM = {'push', 'lea', 'les', 'mov', 'xor', 'inc', 'cbw', 'cwde'}

# Pinned measurement: (site, owner, kind, row, wait, message class, bound, title source). Regenerate with --print-table.
# Three entries have no sound static bound: an unmodelled callee runs after the message local's address was taken.
EXPECTED_UNBOUNDED = ['009E:0F5D', '009E:15F6', '00BA:208E']
EXPECTED = [
    ('001E:0042', '0x1e:0x0013', 'far', '0', 0, 'local', 16, 'ds:5D92'),
    ('001E:0217', '0x1e:0x00bf', 'far', '0', 1, 'literal_temp', 9, 'ds:5D92'),
    ('002F:1081', '0x2f:0x0fff', 'far', '10', 1, 'literal_temp', 11, 'frame[bp-0x5]'),
    ('0031:0101', '0x31:0x000f', 'far', '10', 1, 'literal_temp', 14, 'frame[bp+0x6]'),
    ('0031:1130', '0x31:0x10d7', 'far', '10', 1, 'literal_temp', 17, 'frame[bp+0x6]'),
    ('0038:0082', '0x38:0x0058', 'far', '10', 0, 'local', 16, 'frame[bp+0xc]'),
    ('0038:01A0', '0x38:0x017f', 'far', '10', 1, 'literal_temp', 15, 'frame[bp+0xc]'),
    ('0038:0254', '0x38:0x0233', 'far', '10', 1, 'literal_temp', 10, 'frame[bp+0xc]'),
    ('0038:073F', '0x38:0x070f', 'far', '10', 1, 'literal_temp', 11, 'frame[bp+0xc]'),
    ('0038:0902', '0x38:0x07e0', 'far', '10', 1, 'literal_temp', 13, 'frame[bp+0xc]'),
    ('0038:0A0D', '0x38:0x09c0', 'far', '10', 1, 'literal_temp', 13, 'frame[bp+0xc]'),
    ('0038:0A95', '0x38:0x0a65', 'far', '10', 1, 'literal_temp', 11, 'frame[bp+0xc]'),
    ('0038:0C8C', '0x38:0x0c4b', 'far', '10', 1, 'literal_temp', 4, 'frame[bp+0xc]'),
    ('0038:1004', '0x38:0x0fac', 'far', '10', 1, 'literal_temp', 9, 'frame[bp+0x8]'),
    ('0038:1126', '0x38:0x10de', 'far', '10', 1, 'literal_temp', 11, 'frame[bp+0xc]'),
    ('0038:13C4', '0x38:0x1399', 'far', '10', 1, 'literal_temp', 7, 'frame[bp+0xc]'),
    ('0038:1597', '0x38:0x1545', 'far', '10', 0, 'literal_temp', 11, 'ds:6826'),
    ('0038:182A', '0x38:0x176f', 'far', '10', 1, 'literal_temp', 16, 'frame[bp+0xc]'),
    ('0038:19CE', '0x38:0x1935', 'far', '10', 1, 'literal_temp', 12, 'ds:6826'),
    ('0038:1CE5', '0x38:0x1cad', 'far', '10', 0, 'literal_temp', 8, 'frame[bp+0xc]'),
    ('0038:1DBF', '0x38:0x1cad', 'far', '12', 0, 'literal_temp', 12, 'ds:6826'),
    ('0038:1F24', '0x38:0x1e77', 'far', '10', 0, 'literal_temp', 8, 'frame[bp+0xc]'),
    ('0038:2191', '0x38:0x2122', 'far', '10', 1, 'chain+1', 51, 'frame[bp+0x8]'),
    ('0038:2485', '0x38:0x23ec', 'far', '10', 1, 'literal_temp', 11, 'ds:6826'),
    ('0038:2585', '0x38:0x2556', 'far', '10', 1, 'literal_temp', 13, 'frame[bp+0xc]'),
    ('0038:2C8E', '0x38:0x2c1c', 'far', '10', 1, 'literal_temp', 10, 'frame[bp+0xc]'),
    ('0038:3056', '0x38:0x2f57', 'far', '10', 1, 'literal_temp', 11, 'frame[bp+0xc]'),
    ('0038:307D', '0x38:0x2f57', 'far', '10', 1, 'literal_temp', 11, 'frame[bp+0xc]'),
    ('005E:0359', '0x5e:0x02f9', 'far', '10', 0, 'local', 14, 'frame[bp+0x12]'),
    ('005E:0520', '0x5e:0x02f9', 'far', '12', 1, 'literal_temp', 12, 'frame[bp+0xe]'),
    ('005E:057A', '0x5e:0x02f9', 'far', 'frame[bp-0x1]', 0, 'literal_temp', 9, 'frame[bp+0xe]'),
    ('005E:05DD', '0x5e:0x02f9', 'far', 'frame[bp-0x1]', 0, 'literal_temp', 9, 'frame[bp+0xe]'),
    ('005E:0F8B', '0x5e:0x0e82', 'far', '10', 1, 'literal_temp', 6, 'frame[bp+0xa]'),
    ('005E:118B', '0x5e:0x116a', 'far', '10', 0, 'literal_temp', 15, 'frame[bp+0x6]'),
    ('005E:12C8', '0x5e:0x116a', 'far', '10', 0, 'literal_temp', 12, 'frame[bp-0xa]'),
    ('005E:2525', '0x5e:0x2414', 'far', '10', 1, 'literal_temp', 14, 'frame[bp-0xa]'),
    ('005E:3A45', '0x5e:0x39d2', 'far', '10', 1, 'literal_temp', 8, 'frame[bp+0xc]'),
    ('006A:047B', '0x6a:0x036c', 'far', '10', 1, 'local', 40, 'ds:5D92'),
    ('006A:0551', '0x6a:0x04a1', 'far', '10', 1, 'literal_temp', 23, 'ds:5D92'),
    ('006A:06A4', '0x6a:0x05a4', 'far', '10', 0, 'literal_temp', 13, 'ds:5D92'),
    ('006A:09C1', '0x6a:0x090a', 'far', '10', 1, 'literal_temp', 26, 'ds:5D92'),
    ('006A:0C26', '0x6a:0x0b5b', 'far', '10', 1, 'literal_temp', 23, 'ds:5D92'),
    ('006A:1864', '0x6a:0x1780', 'far', '10', 0, 'literal_temp', 17, 'ds:5D92'),
    ('006A:1964', '0x6a:0x18e5', 'far', '10', 0, 'literal_temp', 12, 'ds:5D92'),
    ('006A:19B6', '0x6a:0x18e5', 'far', '10', 1, 'literal_temp', 17, 'ds:5D92'),
    ('006A:19DA', '0x6a:0x18e5', 'far', '10', 1, 'literal_temp', 20, 'ds:5D92'),
    ('006A:1A27', '0x6a:0x18e5', 'far', '10', 1, 'literal_temp', 25, 'ds:5D92'),
    ('0077:1B4F', '0x77:0x1a8a', 'far', '10', 0, 'literal_temp', 12, 'ds:5D92'),
    ('0086:1220', '0x86:0x11dd', 'far', '10', 1, 'literal_temp', 13, 'frame[bp-0x8]'),
    ('0086:1334', '0x86:0x12a5', 'far', '10', 1, 'literal_temp', 11, 'ds:6C95'),
    ('0086:13C3', '0x86:0x136e', 'far', '10', 1, 'literal_temp', 16, 'ds:6C95'),
    ('0086:1ED4', '0x86:0x1b11', 'far', '10', 0, 'literal_temp', 23, 'ds:5D92'),
    ('0086:203C', '0x86:0x1f41', 'far', '10', 1, 'literal_temp', 11, 'frame[bp-0x4]'),
    ('0086:2DEF', '0x86:0x2c43', 'far', '10', 1, 'literal_temp', 11, 'frame[bp-0x4]'),
    ('0086:2F24', '0x86:0x2ee5', 'far', '10', 1, 'literal_temp', 11, 'ds:6C95'),
    ('0086:312F', '0x86:0x30d9', 'far', '10', 1, 'literal_temp', 9, 'frame[bp+0xc]'),
    ('0086:3356', '0x86:0x331a', 'far', '10', 1, 'literal_temp', 13, 'frame[bp+0xc]'),
    ('009E:0F5D', '0x9e:0x0f06', 'far', '10', 1, 'local', 255, 'frame[bp+0xc]'),
    ('009E:10BE', '0x9e:0x1081', 'far', '10', 1, 'literal_temp', 8, 'frame[bp+0x8]'),
    ('009E:153E', '0x9e:0x1344', 'far', '12', 1, 'literal_temp', 12, 'frame[bp+0xc]'),
    ('009E:15F6', '0x9e:0x1344', 'far', 'ds:5E25+1', 0, 'local', 255, 'frame[bp+0xc]'),
    ('009E:16B0', '0x9e:0x1656', 'far', '10', 1, 'literal_temp', 13, 'frame[bp+0x16]'),
    ('009E:1915', '0x9e:0x1869', 'far', '10', 1, 'local', 19, 'frame[bp+0x8]'),
    ('00BA:0A1A', 'RES_00BA_09E3', 'near', 'frame[bp-0x2]', 0, 'literal_temp', 1, 'frame[bp+0x6]'),
    ('00BA:208E', 'RES_00BA_1FEB', 'near', '10', 0, 'local', 255, 'frame[bp+0xc]'),
    ('00BA:218F', 'RES_00BA_1FEB', 'near', '10', 1, 'local', 40, 'frame[bp+0xc]'),
    ('00BA:239A', 'RES_00BA_234A', 'near', '10', 1, 'local', 19, 'frame[bp+0x6]'),
]


def require(ok, message):
    if not ok:
        raise ValueError(message)


def far_target(insn):
    """(segment, offset) of a direct far call; None for near, indirect or non-call instructions."""
    if insn.mnemonic != 'lcall' or len(insn.operands) != 2 or any(op.type != X86_OP_IMM for op in insn.operands):
        return None
    return (insn.operands[0].imm & 0xFFFF, insn.operands[1].imm & 0xFFFF)


def bp_disp(insn, index):
    op = insn.operands[index]
    return op.mem.disp if op.type == X86_OP_MEM and op.mem.base == X86_REG_BP else None


class Body:
    """A linearly decoded body with a direct-jump CFG; a misaligned tail is cut at the last decoded start."""

    def __init__(self, code, start, end, label):
        self.label = label
        self.ins = []
        reach = start
        for i in MD.disasm(bytes(code[start:end]), start):
            self.ins.append(i)
            if i.mnemonic in ('jmp', 'ja', 'jae', 'jb', 'jbe', 'je', 'jne', 'jg', 'jge', 'jl', 'jle', 'jo', 'jno', 'js', 'jns', 'jp', 'jnp',
                              'jcxz', 'loop', 'loope', 'loopne') and i.operands and i.operands[0].type == X86_OP_IMM:
                reach = max(reach, i.operands[0].imm & 0xFFFF)
            if i.mnemonic in ('ret', 'retf', 'iret') and reach <= i.address:
                break    # the function's own epilogue; what follows is data or another body
        require(self.ins, f'{label}: nothing decodes')
        self.index = {i.address: k for k, i in enumerate(self.ins)}
        self.succ = []
        self.targets = set()      # indices entered by a jump: a linear walk backwards must not cross into them
        for k, i in enumerate(self.ins):
            nxt = k + 1 if k + 1 < len(self.ins) else None
            if i.mnemonic in ('ret', 'retf', 'iret', 'ljmp'):
                self.succ.append(())
            elif i.mnemonic == 'jmp' or i.mnemonic.startswith('j') or i.mnemonic.startswith('loop'):
                require(i.operands and i.operands[0].type == X86_OP_IMM, f'{label}: indirect jump at {i.address:04X}')
                target = i.operands[0].imm & 0xFFFF
                require(target in self.index, f'{label}: jump at {i.address:04X} leaves the decoded body')
                self.targets.add(self.index[target])
                self.succ.append((self.index[target],) if i.mnemonic == 'jmp' else tuple(x for x in (self.index[target], nxt) if x is not None))
            else:
                self.succ.append((nxt,) if nxt is not None else ())

    def contiguous(self, lo, hi):
        """True when instructions lo..hi-1 run only by falling through: no index after lo is a jump target."""
        return not any(j in self.targets for j in range(lo + 1, hi))

    def window(self, k):
        """The push-forming run before instruction k, cut at a jump target."""
        j = k
        while j > 0 and self.ins[j - 1].mnemonic in PUSHFORM and j not in self.targets:
            i = self.ins[j - 1]
            if i.mnemonic == 'mov' and i.operands[0].type == X86_OP_MEM:
                break
            j -= 1
        return self.ins[j:k]


def triple(ins):
    """Classify a three-instruction far-pointer push: ('local', n) | ('lit', off) | ('param', k) | None."""
    if len(ins) != 3 or ins[1].mnemonic != 'push' or ins[2].mnemonic != 'push' or ins[2].op_str != 'di':
        return None
    a, b = ins[0], ins[1].op_str
    if a.mnemonic == 'lea' and b == 'ss' and bp_disp(a, 1) is not None and bp_disp(a, 1) < 0:
        return ('local', -bp_disp(a, 1))
    if a.mnemonic == 'mov' and a.op_str.startswith('di, ') and a.operands[1].type == X86_OP_IMM and b == 'cs':
        return ('lit', a.operands[1].imm & 0xFFFF)
    if a.mnemonic == 'les' and b == 'es' and bp_disp(a, 1) is not None and bp_disp(a, 1) > 0:
        return ('param', bp_disp(a, 1))
    return None


SUBREG = {'al': 'ax', 'ah': 'ax', 'bl': 'bx', 'bh': 'bx', 'cl': 'cx', 'ch': 'cx', 'dl': 'dx', 'dh': 'dx'}
STACK_ORIGIN = ('ss', 'bp', 'sp')          # reading any of these as a value yields a stack address
CLOBBERED_BY_CALL = ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'es')


def reg_name(insn, reg):
    name = insn.reg_name(reg)
    return SUBREG.get(name, name) if name else None


def mem_regs(insn, op):
    """(segment name, [base/index register names]) of a memory operand; the default segment follows the base."""
    seg = reg_name(insn, op.mem.segment) if op.mem.segment else None
    regs = [reg_name(insn, r) for r in (op.mem.base, op.mem.index) if r]
    if seg is None:
        seg = 'ss' if any(r in ('bp', 'sp') for r in regs) else 'ds'
    return seg, regs


class Analysis:
    """Forward dataflow of string-length bounds per BP-relative local; join is the maximum, unknown is 255.

    Soundness rules (t3677 R1/R2, t3683 R1/R3). A write is applied precisely only when it is BP-relative
    with a negative displacement (this body's locals). Every write through a register pointer, whatever the
    pointer's syntactic origin (a parameter's contents may be a stack address), and every write through SS
    or an explicit ES/CS segment, sets every local's bound to 255. A DS-absolute store is safe only when DS
    is untainted and its physical range lies below the stack base (DATA_SEG and the EXEPACK real SS). The
    taint set (registers, segment registers, locals holding LEA [BP..], SS, BP, SP, POP or call-clobbered
    values) decides whether a register load is a stack pointer and keeps string stores conservative. Any
    direct call other than the three SYSTEM string leaves and 16EC itself invalidates every local at or above
    the lowest address this body has taken so far (a pointer that escaped earlier may be written by the
    callee); an indirect call invalidates every local."""

    def __init__(self, world, kind, container, body, depth=0):
        self.world, self.kind, self.container, self.body, self.depth = world, kind, container, body, depth
        self.notes = []

    def literal_length(self, off):
        code = self.world.code_of(self.kind, self.container)
        require(0 <= off < len(code), f'{self.body.label}: literal {off:04X} outside the code')
        return code[off]

    def value(self, kind, state, extra=None):
        if kind[0] == 'lit':
            return self.literal_length(kind[1])
        if kind[0] == 'local':
            return state[0].get(kind[1], UNBOUNDED)
        if kind[0] == 'param':
            return self.world.param_bound(self.kind, self.container, self.body, kind[1], self.depth)
        if kind[0] == 'chain':
            return state[0].get(state[1], UNBOUNDED) if state[1] is not None else UNBOUNDED
        if kind[0] == 'temp':
            return state[0].get(kind[1], UNBOUNDED)
        return UNBOUNDED

    def invalidate(self, bounds, why):
        for local in list(bounds):
            bounds[local] = UNBOUNDED
        self.notes.append(why)

    def transfer(self, k, state):
        bounds, chain, taint, taken = dict(state[0]), state[1], set(state[2]), state[3]
        i = self.body.ins[k]
        if i.mnemonic == 'lea' and bp_disp(i, 1) is not None and bp_disp(i, 1) < 0:
            taken = max(taken, -bp_disp(i, 1))       # a pointer at [BP-n] can write any local at or above it
        local_ops = [op for op in i.operands if op.type == X86_OP_MEM and op.mem.base == X86_REG_BP and not op.mem.index
                     and op.mem.disp < 0 and (not op.mem.segment or op.mem.segment == X86_REG_SS)]
        # 1. memory writes: precise for this body's locals, otherwise sound against the taint set
        for op in i.operands:
            if op.type != X86_OP_MEM or not (op.access & CS_AC_WRITE):
                continue
            if op in local_ops:
                lo, hi = -op.mem.disp - op.size + 1, -op.mem.disp
                for local in list(bounds):
                    if lo <= local <= hi:
                        bounds[local] = UNBOUNDED
                if i.mnemonic == 'mov' and op.size == 1 and i.operands[1].type == X86_OP_IMM:
                    bounds[-op.mem.disp] = i.operands[1].imm & 0xFF
                else:
                    bounds[-op.mem.disp] = UNBOUNDED
                continue
            if op.mem.base == X86_REG_BP and not op.mem.index and op.mem.disp >= 0 and not op.mem.segment:
                continue       # the caller's frame (arguments), never this body's locals
            seg, regs = mem_regs(i, op)
            if regs:
                self.invalidate(bounds, f'{i.address:04X}: write through a register pointer')
            elif seg != 'ds' or 'ds' in taint or DATA_SEG * 16 + (op.mem.disp & 0xFFFF) + op.size > EXEPACK['real_ss'] * 16:
                self.invalidate(bounds, f'{i.address:04X}: absolute store that may reach the stack segment')
        if i.mnemonic.endswith(('stosb', 'stosw', 'movsb', 'movsw', 'stosd', 'movsd')):
            if 'es' in taint or 'di' in taint:
                self.invalidate(bounds, f'{i.address:04X}: string store through a stack-derived ES:DI')
        # 2. taint propagation into written registers and locals
        reads, writes = i.regs_access()
        read_names = {reg_name(i, r) for r in reads} - {None, 'flags', 'eflags', 'ip', 'cs'}
        address_regs = set()          # registers that only form an address are not value sources (BP in [BP+6])
        for op in i.operands:
            if op.type == X86_OP_MEM:
                seg, regs = mem_regs(i, op)
                address_regs.update(regs); address_regs.add(seg)
        tainted_source = bool((read_names - address_regs) & (set(taint) | set(STACK_ORIGIN)))
        if i.mnemonic == 'lea':
            tainted_source = any(r in ('bp', 'sp') or r in taint for r in mem_regs(i, i.operands[1])[1])
        for op in i.operands:
            if op.type == X86_OP_MEM and (op.access & 1):
                if op in local_ops and any(('L', -op.mem.disp + d) in taint for d in range(op.size)):
                    tainted_source = True
                elif op not in local_ops and any(r in taint for r in mem_regs(i, op)[1]):
                    tainted_source = True
        if i.mnemonic == 'pop':
            tainted_source = True
        if i.mnemonic in ('xor', 'sub') and len(i.operands) == 2 and all(op.type == 1 for op in i.operands) \
                and i.operands[0].reg == i.operands[1].reg:
            tainted_source = False
        written_names = {reg_name(i, r) for r in writes} - {None, 'flags', 'eflags', 'ip', 'sp', 'bp', 'ss', 'cs'}
        if i.mnemonic in ('les', 'lds'):
            written_names.add(i.mnemonic[1:])     # Capstone omits the implicit segment register LES/LDS load
        if i.mnemonic.startswith('push'):
            written_names = set()
        for name in written_names:
            (taint.add if tainted_source else taint.discard)(name)
        for op in local_ops:
            if op.access & CS_AC_WRITE:
                for d in range(op.size):
                    (taint.add if tainted_source else taint.discard)(('L', -op.mem.disp + d))
        if i.mnemonic in ('lcall', 'call'):
            taint.update(CLOBBERED_BY_CALL)
        if tainted_source and any(op.type == X86_OP_MEM and (op.access & CS_AC_WRITE) and op not in local_ops for op in i.operands):
            taken = 0xFFFF                            # a stack address escaped to memory a callee can read
        if i.mnemonic not in ('lcall', 'call'):
            return bounds, chain, taint, taken
        if any(op.type != X86_OP_IMM for op in i.operands):
            self.invalidate(bounds, f'{i.address:04X}: indirect call')
            return bounds, None, taint, taken
        window = self.body.window(k)
        target = far_target(i)
        locals_in_window = [triple(window[j:j + 3]) for j in range(len(window) - 2)]
        locals_in_window = [t[1] for t in locals_in_window if t and t[0] == 'local']
        if target == STRSTORE:
            cap = window[-2] if len(window) >= 2 else None
            require(cap is not None and cap.mnemonic == 'mov' and cap.op_str.startswith('ax, ') and cap.operands[1].type == X86_OP_IMM
                    and window[-1].mnemonic == 'push', f'{self.body.label}: StrStore at {i.address:04X} without an immediate cap')
            dest = triple(window[-5:-2]) if len(window) >= 5 else None
            if len(window) == 5 and k >= 6 and far_target(self.body.ins[k - 6]) in (STRLOAD, STRCAT):
                source = ('chain',)
            else:
                source = (triple(window[-8:-5]) if len(window) >= 8 else None) or ('unknown',)
            if dest and dest[0] == 'local':
                bounds[dest[1]] = min(cap.operands[1].imm & 0xFF, self.value(source, (bounds, chain)))
            else:
                self.invalidate(bounds, f'{i.address:04X}: StrStore into an unresolved destination')
            for local in locals_in_window:
                if not (dest and dest[0] == 'local' and local == dest[1]) and not (source[0] == 'local' and local == source[1]):
                    bounds[local] = UNBOUNDED
            return bounds, None, taint, taken
        if target == STRLOAD:
            source = (triple(window[-3:]) if len(window) >= 3 else None) or ('unknown',)
            dest = triple(window[-6:-3]) if len(window) >= 6 else None
            if dest and dest[0] == 'local':
                bounds[dest[1]] = self.value(source, (bounds, chain))
                chain = dest[1]
            else:
                self.invalidate(bounds, f'{i.address:04X}: StrLoad into an unresolved destination')
                chain = None
            for local in locals_in_window:
                if not (dest and dest[0] == 'local' and local == dest[1]) and not (source[0] == 'local' and local == source[1]):
                    bounds[local] = UNBOUNDED
            return bounds, chain, taint, taken
        if target == STRCAT:
            source = triple(window[-3:]) if len(window) >= 3 else None
            if source is None:
                source = ('callresult',) if (not window and k >= 1 and self.body.ins[k - 1].mnemonic in ('lcall', 'call')) else ('unknown',)
            if chain is None:
                self.invalidate(bounds, f'{i.address:04X}: concatenation with an unknown destination')
            else:
                bounds[chain] = min(UNBOUNDED, bounds.get(chain, UNBOUNDED) + self.value(source, (bounds, chain)))
            for local in locals_in_window:
                if not (source[0] == 'local' and local == source[1]):
                    bounds[local] = UNBOUNDED
            return bounds, chain, taint, taken
        if target == TARGET or (i.mnemonic == 'call' and i.operands[0].type == X86_OP_IMM and (i.operands[0].imm & 0xFFFF) == TARGET[1]
                                and self.kind == 'res' and self.container == TARGET[0]):
            return bounds, None, taint, taken   # the window message is read, not written
        for local in locals_in_window:
            bounds[local] = UNBOUNDED      # any other callee may write through the pointer
        for w in window:                   # a pushed stack segment or tainted register outside a recognised unit hands the callee a stack pointer
            if w.mnemonic == 'push' and w.operands and w.operands[0].type == 1 and (reg_name(w, w.operands[0].reg) in taint or reg_name(w, w.operands[0].reg) in STACK_ORIGIN):
                taken = 0xFFFF
        for local in list(bounds):
            if local <= taken:
                bounds[local] = UNBOUNDED  # a pointer taken earlier may have escaped to this callee
        if taken and any(local <= taken for local in bounds):
            self.notes.append(f'{i.address:04X}: call invalidates locals reachable from a taken pointer')
        # A call whose arguments include the pending chain destination consumes it; an operand call
        # (a string function whose result feeds the next StrCat) leaves the destination on the stack.
        return bounds, (None if chain in locals_in_window else chain), taint, taken

    def before(self, k_target):
        n = len(self.body.ins)
        state = [None] * n
        state[0] = ({}, None, frozenset(), 0)
        work = [0]
        while work:
            k = work.pop()
            out = self.transfer(k, state[k])
            for t in self.body.succ[k]:
                old = state[t]
                if old is None:
                    state[t] = (dict(out[0]), out[1], frozenset(out[2]), out[3])
                    work.append(t)
                else:
                    merged = {key: max(old[0].get(key, UNBOUNDED), out[0].get(key, UNBOUNDED)) for key in set(old[0]) | set(out[0])}
                    chain = old[1] if old[1] == out[1] else None
                    taint = old[2] | frozenset(out[2])
                    taken = max(old[3], out[3])
                    if merged != old[0] or chain != old[1] or taint != old[2] or taken != old[3]:
                        state[t] = (merged, chain, taint, taken)
                        work.append(t)
        require(state[k_target] is not None, f'{self.body.label}: call at {self.body.ins[k_target].address:04X} is unreachable from the entry')
        return state[k_target]


class World:
    def __init__(self, root=ROOT):
        root = Path(root)
        self.exe = (root / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
        self.ovr = (root / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
        require(sha256(self.exe).hexdigest() == DIGESTS['start_exe_sha256'], 'START.EXE hash mismatch')
        require(sha256(self.ovr).hexdigest() == DIGESTS['game_ovr_sha256'], 'GAME.OVR hash mismatch')
        self.units = {u['stub_para']: u for u in find_units(self.exe[parse_mz(self.exe)['header_bytes']:], self.ovr)}
        self.image = load_image(root / 'resources/DOS/POOLRAD/START.EXE')
        with (root / 'docs/re/overlay_function_ledger.csv').open() as f:
            self.ledger = list(csv.DictReader(f))
        with (root / 'docs/re/resident_callable_ledger.csv').open() as f:
            self.resident = [r for r in csv.DictReader(f) if r['id'].startswith('RES_')]
        with (root / 'docs/re/overlay_classification.csv').open() as f:
            self.classes = {r['id']: r for r in csv.DictReader(f)} if False else {}
            f.seek(0)
            reader = csv.DictReader(f)
            key = 'id' if 'id' in reader.fieldnames else reader.fieldnames[0]
            self.classes = {r[key]: r for r in reader}

    def code_of(self, kind, container):
        if kind == 'ovl':
            u = self.units[container]
            return self.ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
        base = container * 16
        return self.image[base:base + 0x10000]

    def owner(self, kind, container, site):
        if kind == 'ovl':
            rows = [r for r in self.ledger if int(r['unit'], 0) == container and int(r['offset'], 0) <= site < int(r['offset'], 0) + int(r['bound'], 0)]
            require(len(rows) == 1, f'{container:04X}:{site:04X} has no unique ledger owner')
            r = rows[0]
            return int(r['offset'], 0), int(r['offset'], 0) + int(r['decoded'], 0), r['id']
        starts = sorted((int(r['image_offset'], 0) - container * 16, r['id']) for r in self.resident if r['id'].startswith(f'RES_{container:04X}_'))
        before = [s for s in starts if s[0] <= site]
        after = [s for s in starts if s[0] > site]
        require(before, f'{container:04X}:{site:04X} has no preceding resident body')
        return before[-1][0], (after[0][0] if after else 0x10000), before[-1][1]

    def body(self, kind, container, site):
        start, end, owner = self.owner(kind, container, site)
        body = Body(self.code_of(kind, container), start, end, f'{owner} ({container:04X}:{start:04X})')
        require(site in body.index, f'{owner}: call site {site:04X} is not an instruction start of its owner')
        return body, owner

    def param_bound(self, kind, container, body, k, depth):
        """Depth-limited caller analysis of the far-pointer string argument at [BP+k]."""
        if depth >= 1:
            return UNBOUNDED
        start = body.ins[0].address
        code = self.code_of(kind, container)
        callers = []
        for pos in range(1, len(code) - 2):
            if code[pos] == 0xE8 and code[pos - 1] == 0x0E and (pos + 3 + int.from_bytes(code[pos + 1:pos + 3], 'little', signed=True)) & 0xFFFF == start:
                callers.append(('near', pos))
        if kind == 'ovl':
            u = self.units[container]
            for index, entry in enumerate(u['entries']):
                if entry == start:
                    callers += [('far', (unit, off)) for unit, off, _ in sites(self.exe, self.ovr, container, 0x20 + 5 * index)]
        if not callers:
            return UNBOUNDED
        worst = 0
        for how, where in callers:
            if how == 'near':
                cbody, _ = self.body(kind, container, where)
                k_call = cbody.index[where]
                analysis = Analysis(self, kind, container, cbody, depth + 1)
                units = push_units(cbody, k_call, near=True)
            else:
                unit, off = where
                cbody, _ = self.body('ovl', unit, off)
                k_call = cbody.index[off]
                analysis = Analysis(self, 'ovl', unit, cbody, depth + 1)
                units = push_units(cbody, k_call)
            state = analysis.before(k_call)
            arg = argument_at(units, k)
            worst = max(worst, analysis.value(arg, state) if arg else UNBOUNDED)
            if worst >= UNBOUNDED:
                break
        return worst


def push_units(body, k, near=False):
    """The pushed argument units before the call at index k, last pushed first.

    Each unit is (size, entity). A StrLoad/StrCat chain leaves its destination temp on the
    stack and consumes its sources, so the chain counts as one far-pointer unit."""
    units = []
    j = k
    if near and j > 0 and body.ins[j - 1].mnemonic == 'push' and body.ins[j - 1].op_str == 'cs':
        j -= 1
    def group(lo, hi):
        """Instructions lo..hi-1 form one unit only when control cannot enter them part-way (t3677 R2)."""
        return lo >= 0 and body.contiguous(lo, hi)
    while j > 0 and j not in body.targets:
        i = body.ins[j - 1]
        if far_target(i) == STRCAT:
            concats, j = 1, j - 1
            while True:
                if j >= 3 and group(j - 3, j + 1) and triple(body.ins[j - 3:j]):
                    j -= 3
                elif j > 0 and group(j - 1, j + 1) and body.ins[j - 1].mnemonic in ('lcall', 'call'):
                    j -= 1
                else:
                    break
                if j > 0 and group(j - 1, j + 1) and far_target(body.ins[j - 1]) == STRCAT:
                    concats, j = concats + 1, j - 1
                    continue
                break
            require(j > 0 and group(j - 7, j + 1) and far_target(body.ins[j - 1]) == STRLOAD,
                    f'{body.label}: concatenation chain without a contiguous load before {body.ins[k].address:04X}')
            source, dest = triple(body.ins[j - 4:j - 1]), triple(body.ins[j - 7:j - 4])
            require(dest and dest[0] == 'local', f'{body.label}: chain destination is not a stack local before {body.ins[k].address:04X}')
            units.append((4, ('temp', dest[1], concats, source)))
            j -= 7
        elif far_target(i) == STRLOAD:
            if not group(j - 7, j):
                break
            source, dest = triple(body.ins[j - 4:j - 1]), triple(body.ins[j - 7:j - 4])
            require(dest and dest[0] == 'local', f'{body.label}: load destination is not a stack local before {body.ins[k].address:04X}')
            units.append((4, ('temp', dest[1], 0, source)))
            j -= 7
        elif j >= 3 and group(j - 3, j) and triple(body.ins[j - 3:j]):
            units.append((4, triple(body.ins[j - 3:j])))
            j -= 3
        elif i.mnemonic == 'push' and i.operands[0].type == X86_OP_MEM:
            units.append((2, ('word', i.operands[0].mem.base, i.operands[0].mem.disp)))
            j -= 1
        elif i.mnemonic == 'push' and i.op_str == 'ax' and j >= 2 and group(j - 2, j):
            prev = body.ins[j - 2]
            if prev.mnemonic == 'mov' and prev.op_str.startswith(('al, ', 'ax, ')) and prev.operands[1].type == X86_OP_IMM:
                units.append((2, ('imm', prev.operands[1].imm & 0xFF)))
                j -= 2
            elif prev.mnemonic == 'mov' and prev.op_str.startswith('al, byte ptr [bp') and bp_disp(prev, 1) is not None:
                units.append((2, ('frame8', bp_disp(prev, 1))))
                j -= 2
            elif (j >= 4 and group(j - 4, j) and prev.mnemonic == 'inc' and body.ins[j - 3].mnemonic == 'xor'
                  and body.ins[j - 4].mnemonic == 'mov' and body.ins[j - 4].op_str.startswith('al, byte ptr [0x')):
                units.append((2, ('ds8+1', body.ins[j - 4].operands[1].mem.disp & 0xFFFF)))
                j -= 4
            else:
                units.append((2, ('other',)))
                j -= 1
        elif i.mnemonic == 'push':
            units.append((2, ('other',)))
            j -= 1
        else:
            break
    return units


def argument_at(units, k):
    """The pushed entity the callee reads at [BP+k]; its first argument is [BP+6]."""
    offset = 6
    for size, entity in units:
        if offset == k:
            return entity
        offset += size
    return None


def describe(entity):
    if entity is None:
        return 'other'
    if entity[0] == 'imm':
        return f'{entity[1]}'
    if entity[0] == 'frame8':
        return f'frame[bp{entity[1]:+#x}]'
    if entity[0] == 'ds8+1':
        return f'ds:{entity[1]:04X}+1'
    if entity[0] == 'word':
        return f'frame[bp{entity[2]:+#x}]' if entity[1] == X86_REG_BP else (f'ds:{entity[2] & 0xFFFF:04X}' if entity[1] == 0 else 'other')
    return 'other'


def entry_row(world, kind, container, site):
    body, owner = world.body(kind, container, site)
    k = body.index[site]
    call = body.ins[k]
    if kind == 'ovl':
        require(far_target(call) == TARGET, f'{owner}: {site:04X} is not LCALL 00BA:16EC')
    else:
        require(call.mnemonic == 'push' and call.op_str == 'cs' and body.ins[k + 1].mnemonic == 'call'
                and (body.ins[k + 1].operands[0].imm & 0xFFFF) == TARGET[1], f'{owner}: {site:04X} is not PUSH CS; CALL 16EC')
        k += 1
    units = push_units(body, k, near=kind == 'res')
    wait, row, message = argument_at(units, 6), argument_at(units, 8), argument_at(units, 10)
    title = argument_at(units, 14)
    require(wait and wait[0] == 'imm', f'{owner}: the wait byte at {site:04X} is not an immediate')
    require(row and row[0] in ('imm', 'frame8', 'ds8+1'), f'{owner}: the row byte at {site:04X} is not immediate, frame byte or DS byte + 1')
    require(message and message[0] in ('local', 'temp'), f'{owner}: the message argument at {site:04X} is not a stack local or literal temp')
    analysis = Analysis(world, kind, container, body, 0)
    state = analysis.before(k)
    local = message[1]
    bound = state[0].get(local, UNBOUNDED)
    literal = None
    if message[0] == 'temp':
        require(message[3] and message[3][0] == 'lit', f'{owner}: the message temp at {site:04X} is not loaded from a CS literal')
        literal = message[3][1]
        message_class = 'literal_temp' if message[2] == 0 else f'chain+{message[2]}'
    else:
        message_class = 'local'
    cls = world.classes.get(owner, {}).get('classification') or world.classes.get(owner, {}).get('cls') or ('resident' if kind == 'res' else 'unknown')
    return dict(site=f'{container:04X}:{site:04X}', owner=owner, classification=cls, kind='far' if kind == 'ovl' else 'near',
                row=describe(row), wait=wait[1], message=message_class, message_local=f'bp-{local:#x}',
                literal=f'{literal:04X}' if literal is not None else None, bound=bound, title=describe(title), notes=analysis.notes)


def census(world):
    far = [(unit, off) for unit, off, _ in sites(world.exe, world.ovr, *TARGET)]
    segment = world.code_of('res', TARGET[0])
    limit = (NEAR_SEGMENT[1] - NEAR_SEGMENT[0]) * 16
    near = [pos - 1 for pos in range(1, limit - 2)
            if segment[pos] == 0xE8 and segment[pos - 1] == 0x0E
            and (pos + 3 + int.from_bytes(segment[pos + 1:pos + 3], 'little', signed=True)) & 0xFFFF == TARGET[1]]
    rows = [entry_row(world, 'ovl', unit, off) for unit, off in far] + [entry_row(world, 'res', TARGET[0], pos) for pos in near]
    return rows, len(far), len(near)


def selector_invariant(world):
    base = TARGET[0] * 16
    body = world.image[base + 0x16EC:base + 0x17E3]
    require(sha256(body).hexdigest() == BODY_SHA256, '16EC body pin mismatch')
    ins = {i.address: i for i in MD.disasm(bytes(body), 0x16EC)}
    text = [f'{ins[a].mnemonic} {ins[a].op_str}' for a in sorted(ins) if a < 0x1707]
    require(text == ['push bp', 'mov bp, sp', 'sub sp, 0x102', 'les di, ptr [bp + 0xa]', 'push es', 'push di', 'lea di, [bp - 0x100]',
                     'push ss', 'push di', 'mov ax, 0xff', 'push ax', 'lcall 0xaf8, 0xb25'], 'prologue/StrStore sequence changed')
    require(f'{ins[0x1707].mnemonic} {ins[0x1707].op_str}' == 'cmp byte ptr [0x49f3], 5' and f'{ins[0x170C].mnemonic} {ins[0x170C].op_str}' == 'jne 0x1757'
            and f'{ins[0x1757].mnemonic} {ins[0x1757].op_str}' == 'cmp byte ptr [0x6dda], 0' and f'{ins[0x175C].mnemonic} {ins[0x175C].op_str}' == 'je 0x1765',
            'selector reads changed')
    header = parse_mz(world.exe)
    loader = header['header_bytes'] + 16 * header['cs']
    fields = dict(zip(('real_ip', 'real_cs', 'mem_start', 'exepack_size', 'real_sp', 'real_ss', 'dest_len', 'skip_len'),
                      struct.unpack('<8H', world.exe[loader:loader + 16])))
    require(world.exe[loader + 16:loader + 18] == b'RB' and all(fields[k] == v for k, v in EXEPACK.items()), 'EXEPACK header differs from the pinned values')
    init = list(MD.disasm(bytes(world.image[SYSTEM_SEG * 16:SYSTEM_SEG * 16 + 5]), 0))
    require([f'{i.mnemonic} {i.op_str}' for i in init[:2]] == [f'mov dx, {DATA_SEG:#x}', 'mov ds, dx'], 'SYSTEM init DS constant changed')
    data_top = max(SELECTOR_CELLS.values())
    stack_base = fields['real_ss'] * 16
    require(DATA_SEG * 16 + data_top < stack_base, 'selector cells are not below the stack segment')
    return dict(prologue_writes='SS:[BP-10Ah,BP) frame pushes and StrStore D=SS:[BP-100h], m=FFh (at most 256 bytes)',
                reads={k: f'DS:{v:04X}' for k, v in SELECTOR_CELLS.items()},
                data_segment=f'{DATA_SEG:04X}', real_ss=f'{fields["real_ss"]:04X}', real_sp=f'{fields["real_sp"]:04X}',
                margin_bytes=stack_base - (DATA_SEG * 16 + data_top),
                path_1707_to_1757='170C JNE 1757 is direct; no write between the two reads',
                outcomes={'mode==5': 'box bypassed (0137, 1816, 0768)', 'mode!=5 && colour==0': 'box 03F1:0000 then clear/title/printer',
                          'mode!=5 && colour!=0': 'no box; clear/title/printer with colour 12h'},
                claim='the selector reads see entry values of the data cells for every admitted entry; not a natural-arrival claim')


def coverage():
    import res00ba_window_16ec_cpu_audit as A
    cases = A.cases()
    selectors = sorted({(bool(kw.get('mode5', True)), bool(kw.get('flag6dda', 0))) for _, kw in cases})
    return dict(cases=len(cases), selector_outcomes=[f'mode5={m} colour_nonzero={c}' for m, c in selectors],
                waits=sorted({kw.get('wait', 0) for _, kw in cases}), rows=sorted({kw.get('row', 3) for _, kw in cases}),
                message_lengths=sorted({len(kw['msg']) for _, kw in cases if kw.get('msg') is not None}))


def table(rows):
    return [(r['site'], r['owner'], r['kind'], r['row'], r['wait'], r['message'], r['bound'], r['title']) for r in rows]


def check(world):
    rows, far, near = census(world)
    require(far == 63 and near == 4, f'entry census changed: {far} far, {near} near')
    measured = table(rows)
    if EXPECTED is not None:
        require(measured == EXPECTED, 'entry table differs from the pinned measurement')
    unbounded = [r['site'] for r in rows if r['bound'] >= UNBOUNDED]
    if EXPECTED is not None:
        require(unbounded == EXPECTED_UNBOUNDED, 'the set of entries without a sound bound changed')
    bounds = [r['bound'] for r in rows if r['bound'] < UNBOUNDED]
    invariant = selector_invariant(world)
    return dict(entries=rows, far_sites=far, near_sites=near, bounded=len(bounds), max_bound=max(bounds), unbounded=unbounded,
                selector_invariant=invariant, model_coverage=coverage(),
                claim='static admitted-entry contract; a sound read-time message bound for the bounded entries only; no execution, identity, readiness, closure or credit verdict',
                not_verified=['natural arrivals and indirect or computed calls', 'frame preservation across 16EC callees (NC2)',
                              'loaded overlay target (NC4)', 'live callee effects (NC3)', 'any accounting change'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--print-table', action='store_true')
    args = parser.parse_args()
    world = World()
    if args.print_table:
        rows, _, _ = census(world)
        for t in table(rows):
            print(f'    {t!r},')
        return
    report = check(world)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'entries'}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
