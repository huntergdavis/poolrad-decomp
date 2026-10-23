#!/usr/bin/env python3
"""Per-instruction raw-CPU C emitter (provenance for generated recovered sources): emit a raw-CPU C translation (one switch case per original instruction) for a 16-bit body.

House style follows resident_0709_text_06a7_cpu.c: every operand is reread from the bus in source
order, offsets wrap at 16 bits, word bytes are physically contiguous (1 MiB wrap), stack words are
addressed at SS:(SP mod 64K). Flags follow the conventions already accepted in this tree:
ADD/ADC/SUB/SBB/CMP/INC/DEC exact; AND/OR/XOR clear CF/OF/AF; SHL/SHR: CF last bit out, OF for count 1
(SHL: MSB(result)^CF, SHR: MSB(operand)), AF := 0 (Unicorn) - see the undefined-flag report; MUL: CF=OF=(high!=0),
SZP from the low result word's low byte/sign, AF := 0. The report lists every undefined-flag producer and
whether any path observes it before it is overwritten.
"""
import re
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from capstone import x86_const as X

R16 = {X.X86_REG_AX: 'ax', X.X86_REG_BX: 'bx', X.X86_REG_CX: 'cx', X.X86_REG_DX: 'dx', X.X86_REG_SI: 'si',
       X.X86_REG_DI: 'di', X.X86_REG_BP: 'bp', X.X86_REG_SP: 'sp'}
R8 = {X.X86_REG_AL: ('ax', 0), X.X86_REG_AH: ('ax', 8), X.X86_REG_BL: ('bx', 0), X.X86_REG_BH: ('bx', 8),
      X.X86_REG_CL: ('cx', 0), X.X86_REG_CH: ('cx', 8), X.X86_REG_DL: ('dx', 0), X.X86_REG_DH: ('dx', 8)}
SEG = {X.X86_REG_ES: 'es', X.X86_REG_DS: 'ds', X.X86_REG_SS: 'ss', X.X86_REG_CS: 'cs'}
JCC = {'je': 'ZF', 'jne': '!ZF', 'jb': 'CF', 'jae': '!CF', 'jbe': '(CF||ZF)', 'ja': '!(CF||ZF)',
       'jl': '(SF!=OF)', 'jge': '(SF==OF)', 'jle': '(ZF||SF!=OF)', 'jg': '(!ZF&&SF==OF)', 'js': 'SF', 'jns': '!SF'}
READS = {'je': 'Z', 'jne': 'Z', 'jb': 'C', 'jae': 'C', 'jbe': 'CZ', 'ja': 'CZ', 'jl': 'SO', 'jge': 'SO',
         'jle': 'ZSO', 'jg': 'ZSO', 'js': 'S', 'jns': 'S', 'adc': 'C', 'sbb': 'C'}


def cond(expr):
    m = {'ZF': '(c->flags&0x40u)', 'CF': '(c->flags&1u)', 'SF': '((c->flags>>7)&1u)', 'OF': '((c->flags>>11)&1u)'}
    out = expr
    for k, v in m.items():
        out = out.replace(k, v)
    return out.replace('!(c', '!(c')


class Emitter:
    def __init__(self, code, start, end, calls):
        self.md = Cs(CS_ARCH_X86, CS_MODE_16); self.md.detail = True
        self.ins = list(self.md.disasm(code, start))
        assert self.ins[-1].address + self.ins[-1].size == end, 'span does not end on an instruction'
        self.start, self.end, self.calls = start, end, calls
        self.next = {i.address: i.address + i.size for i in self.ins}

    def mem(self, op, i):
        m = op.mem
        seg = SEG.get(m.segment) if m.segment else ('ss' if m.base == X.X86_REG_BP else 'ds')
        parts = []
        if m.base:
            parts.append(f'c->{R16[m.base]}')
        if m.index:
            parts.append(f'c->{R16[m.index]}')
        if m.disp or not parts:
            parts.append(f'0x{m.disp & 0xFFFF:04X}u')
        return seg, f'(uint16_t)({"+".join(parts)})'

    def rd(self, op, i, size=None):
        size = size or op.size
        if op.type == X.X86_OP_REG:
            if op.reg in R16:
                return f'c->{R16[op.reg]}'
            if op.reg in R8:
                r, sh = R8[op.reg]
                return f'(uint8_t)(c->{r}>>{sh}u)' if sh else f'(uint8_t)c->{r}'
            return f'c->{SEG[op.reg]}'
        if op.type == X.X86_OP_IMM:
            return f'0x{op.imm & (0xFF if size == 1 else 0xFFFF):0{2 if size == 1 else 4}X}u'
        seg, off = self.mem(op, i)
        return f'rd8(b,c->{seg},{off})' if size == 1 else f'rd16(b,c->{seg},{off})'

    def wr(self, op, value, i):
        if op.type == X.X86_OP_REG:
            if op.reg in R16:
                return f'c->{R16[op.reg]}=(uint16_t)({value});'
            if op.reg in R8:
                r, sh = R8[op.reg]
                return (f'c->{r}=(uint16_t)((c->{r}&0x00FFu)|((uint16_t)(uint8_t)({value})<<8u));' if sh
                        else f'c->{r}=(uint16_t)((c->{r}&0xFF00u)|(uint8_t)({value}));')
            return f'c->{SEG[op.reg]}=(uint16_t)({value});'
        seg, off = self.mem(op, i)
        return (f'wr8(b,c->{seg},{off},(uint8_t)({value}));' if op.size == 1
                else f'wr16(b,c->{seg},{off},(uint16_t)({value}));')

    def case(self, i):
        a, nx, mn, ops = i.address, self.next[i.address], i.mnemonic, i.operands
        go = f'c->ip=0x{nx:04X}u;'
        bits = (ops[0].size * 8) if ops else 16
        if mn == 'mov':
            return self.wr(ops[0], self.rd(ops[1], i, ops[0].size), i) + go
        if mn in ('add', 'adc', 'sub', 'sbb', 'cmp'):
            carry = '(c->flags&1u)' if mn in ('adc', 'sbb') else '0u'
            sub = '1u' if mn in ('sub', 'sbb', 'cmp') else '0u'
            v = f'arith(c,{self.rd(ops[0], i)},{self.rd(ops[1], i, ops[0].size)},{carry},{bits}u,{sub})'
            return (f'(void){v};' if mn == 'cmp' else self.wr(ops[0], v, i)) + go
        if mn in ('and', 'or', 'xor'):
            sym = {'and': '&', 'or': '|', 'xor': '^'}[mn]
            v = f'logic(c,(uint16_t)({self.rd(ops[0], i)}{sym}{self.rd(ops[1], i, ops[0].size)}),{bits}u)'
            return self.wr(ops[0], v, i) + go
        if mn in ('inc', 'dec'):
            return self.wr(ops[0], f'incdec(c,{self.rd(ops[0], i)},{bits}u,{1 if mn == "dec" else 0}u)', i) + go
        if mn in ('shl', 'shr'):
            cnt = self.rd(ops[1], i, 1) if len(ops) > 1 else '1u'
            return self.wr(ops[0], f'shift(c,{self.rd(ops[0], i)},(uint8_t)({cnt}),{bits}u,{1 if mn == "shr" else 0}u)', i) + go
        if mn == 'mul':
            assert ops[0].size == 2
            return f'mul16(c,{self.rd(ops[0], i)});' + go
        if mn == 'cwde':  # 98h in 16-bit mode is CBW; no flags
            return 'c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));' + go
        if mn == 'cdq':  # 99h in 16-bit mode is CWD
            return 'c->dx=(c->ax&0x8000u)?0xFFFFu:0u;' + go
        if mn == 'lea':  # effective offset only; no memory access, no flags
            _, off = self.mem(ops[1], i)
            return f'c->{R16[ops[0].reg]}={off};' + go
        if mn == 'les':
            seg, off = self.mem(ops[1], i)
            return (f'{{uint16_t o={off};size_t at=physical(c->{seg},o);c->{R16[ops[0].reg]}=word_at(b,at);'
                    f'c->es=word_at(b,(at+2u)&MASK);}}' + go)
        if mn == 'push':
            return f'push(b,c,{self.rd(ops[0], i)});' + go
        if mn == 'pop':
            return self.wr(ops[0], 'pop(b,c)', i) + go
        if mn == 'out':
            return (f'if(pr_bus_out(b,c->dx,(uint8_t)c->ax,1u,c->cs,0x{a:04X}u)){{c->ip=0x{a:04X}u;return STATUS(PORT);}}' + go)
        if mn == 'jmp' or mn in JCC:
            t = int(i.op_str, 16)
            back = t <= a
            taken = f'if(loops==max_loopbacks){{c->ip=0x{a:04X}u;return STATUS(LIMIT);}}++loops;' if back else ''
            if mn == 'jmp':
                return f'{taken}c->ip=0x{t:04X}u;'
            return f'if({cond(JCC[mn])}){{{taken}c->ip=0x{t:04X}u;}}else ' + go
        if mn == 'lcall':
            seg_imm, off = [int(x, 16) for x in i.op_str.split(',')]
            handler = self.calls[off]
            return (f'push(b,c,c->cs);push(b,c,0x{nx:04X}u);c->cs=rd16(b,c->cs,0x{a + 3:04X}u);c->ip=0x{off:04X}u;'
                    f'{{int s={handler}(b,c,dg);if(s)return (pr_status_t)s;}}'
                    f'if(c->cs!=caller||c->ip!=0x{nx:04X}u)return STATUS(TRANSFER);')
        if mn == 'retf':
            n = int(i.op_str, 16) if i.op_str else 0
            return f'c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+{n}u);return STATUS(OK);'
        raise NotImplementedError(f'{a:04X} {mn} {i.op_str}')

    def cases(self):
        return [f'        case 0x{i.address:04X}: {self.case(i)}break; /* {i.mnemonic} {i.op_str} */' for i in self.ins]

    # ---- undefined-flag observability ------------------------------------------------------------
    def undefined_report(self):
        """For each instruction whose flags are architecturally undefined, follow every successor path
        until each undefined flag is overwritten by a defining instruction, read by a branch/ADC/SBB, or
        a stop/return/call boundary makes it visible. Prints the verdict per producer."""
        writes_all = {'add', 'adc', 'sub', 'sbb', 'cmp', 'and', 'or', 'xor', 'inc', 'dec', 'neg'}
        by = {i.address: i for i in self.ins}
        out = []
        for i in self.ins:
            und = ''
            if i.mnemonic in ('shl', 'shr'):
                und = 'A' + ('' if len(i.operands) == 1 or (i.operands[1].type == X.X86_OP_IMM and i.operands[1].imm == 1) else 'O')
            if i.mnemonic == 'mul':
                und = 'SZAP'
            if not und:
                continue
            seen, work, visible = set(), [(self.next[i.address], set(und))], []
            while work:
                a, live = work.pop()
                if not live or (a, frozenset(live)) in seen:
                    continue
                seen.add((a, frozenset(live)))
                j = by.get(a)
                if j is None:
                    visible.append(f'leaves span at {a:04X}'); continue
                mn = j.mnemonic
                if mn in READS:
                    hit = live & set(READS[mn])
                    if hit:
                        visible.append(f'{mn}@{a:04X} reads {"".join(sorted(hit))}')
                if mn in ('retf', 'out', 'lcall'):
                    visible.append(f'{mn}@{a:04X} stop/boundary with {"".join(sorted(live))} live'); continue
                if mn in writes_all:
                    live = live - set('SZAPCO' if mn not in ('inc', 'dec') else 'SZAPO')
                if mn in ('shl', 'shr'):
                    live = live - set('SZPC')
                if mn == 'mul':
                    live = live - set('CO')
                nxt = []
                if mn == 'jmp':
                    nxt = [int(j.op_str, 16)]
                    if nxt[0] <= a:
                        visible.append(f'LIMIT stop at jmp@{a:04X} with {"".join(sorted(live))} live')
                elif mn in JCC:
                    t = int(j.op_str, 16); nxt = [t, self.next[a]]
                    if t <= a:
                        visible.append(f'LIMIT stop at {mn}@{a:04X} with {"".join(sorted(live))} live')
                else:
                    nxt = [self.next[a]]
                for n in nxt:
                    work.append((n, set(live)))
            out.append((i.address, i.mnemonic, i.op_str, und, sorted(set(visible))))
        return out


