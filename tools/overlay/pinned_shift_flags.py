#!/usr/bin/env python3
"""Pinned DOSBox-X (5a9c7c53) AF after a shift, for declared splits against raw Unicorn.

The pinned core's lazy `get_AF` returns `count & 1Fh` for SHL/SHR/SAR, so AF is SET after any shift with a non-zero
count; raw Unicorn leaves it clear. A model that follows the pinned rule differs from Unicorn in AF alone, and only
when a shift was the last instruction to write the arithmetic flags before the compared stop. A differential declares
exactly that split (never a blanket mask): track the last flag writer in the original, and when it is a non-zero
shift, the C flags must equal the Unicorn flags with AF set; on every other arrival they must be equal.
"""
# NOT_A_ROW_CHECKER: shared declared-split helper for audits; it checks no registered row
import capstone
from capstone import x86_const as X
from unicorn import x86_const as U

AF = 0x0010
SHIFTS = {"shl", "sal", "shr", "sar"}
_DECODER = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
_DECODER.detail = True


class LastFlagWriter:
    """Unicorn code-hook helper: remembers whether the last FLAGS-writing instruction was a non-zero shift."""

    def __init__(self):
        self._cache = {}
        self.shift_nonzero = False
        self.last = None            # (address, mnemonic) of the last FLAGS writer, for messages
        self.division = None        # pinned six flags of the last DIV/IDIV when it is the last writer
        self._pending_div = None

    def reset(self):
        self.shift_nonzero = False
        self.last = self.division = self._pending_div = None

    def _settle(self, uc):
        """Read a word divide's quotient (AX) and remainder (DX) once it has executed."""
        if self._pending_div is not None:
            from pinned_div_flags import division_flags
            self.division = division_flags(uc.reg_read(U.UC_X86_REG_DX), uc.reg_read(U.UC_X86_REG_AX))
            self._pending_div = None

    def finish(self, uc):
        """Call once emulation stops, so a divide that was the very last instruction is settled."""
        self._settle(uc)

    def step(self, uc, address):
        self._settle(uc)
        decoded = self._cache.get(address)
        if decoded is None:
            insn = next(_DECODER.disasm(bytes(uc.mem_read(address, 15)), address), None)
            if insn is None:
                decoded = (False, None, None)
            else:
                writes = X.X86_REG_EFLAGS in insn.regs_access()[1]
                count = None
                if insn.mnemonic in SHIFTS:
                    last = insn.operands[-1]
                    count = ("cl", None) if last.type == X.X86_OP_REG else ("imm", last.imm & 0x1F)
                decoded = (writes, insn.mnemonic, count)
            self._cache[address] = decoded
        writes, mnemonic, count = decoded
        if not writes:
            return
        if mnemonic in SHIFTS and ((uc.reg_read(U.UC_X86_REG_CL) & 0x1F) if count[0] == "cl" else count[1]) == 0:
            return
        self.last, self.division = (address, mnemonic), None
        if mnemonic in ("div", "idiv"):
            insn_bytes = bytes(uc.mem_read(address, 2))
            self._pending_div = mnemonic if insn_bytes[0] == 0xF7 else None  # word form only (F7 /6, /7)
        if mnemonic in SHIFTS:
            n = (uc.reg_read(U.UC_X86_REG_CL) & 0x1F) if count[0] == "cl" else count[1]
            if n == 0:
                return  # a zero-count shift leaves FLAGS untouched on both sides
            self.shift_nonzero = True
        else:
            self.shift_nonzero = False


def shift_af_split(c_flags, unicorn_flags, shift_nonzero):
    """None if C matches the declared split (AF set after a non-zero shift, else exact), otherwise a message."""
    expected = (unicorn_flags | AF) if shift_nonzero else unicorn_flags
    if c_flags == expected:
        return None
    reason = "declared AF split after a non-zero shift" if shift_nonzero else "exact"
    return f"flags C={c_flags:04x}, original={unicorn_flags:04x} ({reason} expects {expected:04x})"


def declared_flags_split(c_flags, unicorn_flags, writer):
    """The declared split for the last FLAGS writer: pinned DIV/IDIV six bits, AF after a shift, else exact."""
    where = f" (last writer {writer.last[1]} at {writer.last[0]:#x})" if writer.last else ""
    if writer.division is not None:
        from pinned_div_flags import declared_split
        problem = declared_split(c_flags, unicorn_flags, writer.division)
        return None if problem is None else problem + where
    problem = shift_af_split(c_flags, unicorn_flags, writer.shift_nonzero)
    return None if problem is None else problem + where


SPLIT_BY_RULE = {"div": 0, "shift": 0}


def split_used(c_flags, unicorn_flags, writer):
    """True when this arrival needed the declared split (C differs from raw Unicorn and the split allowed it).

    Also counts the arrival under its rule in SPLIT_BY_RULE, for the audit's report."""
    if c_flags == unicorn_flags:
        return False
    if writer.division is not None:
        SPLIT_BY_RULE["div"] += 1
        return True
    if writer.shift_nonzero:
        SPLIT_BY_RULE["shift"] += 1
        return True
    return False
