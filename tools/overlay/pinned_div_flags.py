#!/usr/bin/env python3
"""Pinned DOSBox-X (5a9c7c53) DIVW/IDIVW arithmetic flags, for declared splits against raw Unicorn.

`instructions.h:655-673` (DIVW) and `:743-761` (IDIVW) write all six arithmetic flags after FillFlags():
AF=SF=OF=0, ZF=(rem==0 && quo odd), CF=((rem&3) in 1..2), PF=even parity of the four bytes of rem:quo.
Raw Unicorn leaves these bits at its own profile, so a differential declares the split (never a blanket mask):
the C flags must equal the Unicorn flags outside ARITH, and the pinned value inside it.
"""
ARITH = 0x08D5
AF = 0x0010


def division_flags(remainder, quotient):
    """The six arithmetic flag bits (a subset of ARITH) for a successful word divide."""
    rem, quo = remainder & 0xFFFF, quotient & 0xFFFF
    parity = bin((rem & 0xFF) ^ (rem >> 8) ^ (quo & 0xFF) ^ (quo >> 8)).count("1") & 1
    flags = 0
    if rem == 0 and quo & 1:
        flags |= 0x0040
    if 1 <= (rem & 3) <= 2:
        flags |= 0x0001
    if parity == 0:
        flags |= 0x0004
    return flags


def declared_split(c_flags, unicorn_flags, pinned_six, bits=ARITH):
    """None if C == (unicorn outside `bits`) | pinned_six inside it (the declared split), else a message."""
    expected = (unicorn_flags & ~bits & 0xFFFF) | (pinned_six & bits)
    if c_flags == expected:
        return None
    return f"flags {c_flags:04X}: declared split expects {expected:04X} (unicorn {unicorn_flags:04X}, pinned bits {pinned_six & bits:04X})"
