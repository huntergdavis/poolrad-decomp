"""Render a raw-CPU C model for a candidate ECL handler body.

Shared by emit_u1c_save_0292.py and emit_u1c_save_table_0ebc.py. The generated
source is one switch case per original instruction (tools/overlay/emit_raw_cpu_c.py),
with the unit-0x25 runtime calls routed to the shared candidate doubles in
src/recovered/candidate_ecl_doubles.c. Candidate-scout draft: no credit claimed.
"""
from __future__ import annotations

import re
from pathlib import Path

from emit_raw_cpu_c import Emitter

HELPERS = r'''
#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)
#define STATUS(s) PR_%(UP)s_##s
typedef pr_candidate_cpu pr_cpu_t;
typedef %(lower)s_status pr_status_t;
static inline size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static inline uint16_t word_at(uint8_t *b, size_t at) { return (uint16_t)(b[at] | ((uint16_t)b[(at + 1u) & MASK] << 8u)); }
static inline uint8_t rd8(uint8_t *b, uint16_t seg, uint16_t off) { return b[physical(seg, off)]; }
static inline uint16_t rd16(uint8_t *b, uint16_t seg, uint16_t off) { return word_at(b, physical(seg, off)); }
static inline void wr8(uint8_t *b, uint16_t seg, uint16_t off, uint8_t v) { b[physical(seg, off)] = v; }
static inline void wr16(uint8_t *b, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    b[at] = (uint8_t)v; b[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static inline void push(uint8_t *b, pr_cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static inline uint16_t pop(uint8_t *b, pr_cpu_t *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static inline void szp(pr_cpu_t *c, uint16_t r, unsigned bits)
{
    uint8_t p = (uint8_t)r;
    p ^= (uint8_t)(p >> 4u); p ^= (uint8_t)(p >> 2u); p ^= (uint8_t)(p >> 1u);
    if ((p & 1u) == 0u) c->flags |= 0x04u;
    if ((r & (bits == 8u ? 0xFFu : 0xFFFFu)) == 0u) c->flags |= 0x40u;
    if (r & (bits == 8u ? 0x80u : 0x8000u)) c->flags |= 0x80u;
}
static inline uint16_t arith(pr_cpu_t *c, uint16_t l, uint16_t r, unsigned carry, unsigned bits, unsigned sub)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t wide = sub ? (uint32_t)l - r - carry : (uint32_t)l + r + carry;
    uint16_t res = (uint16_t)(wide & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (sub ? (uint32_t)l < (uint32_t)r + carry : wide > mask) c->flags |= 1u;
    if ((l ^ r ^ res) & 0x10u) c->flags |= 0x10u;
    if ((sub ? (l ^ r) & (l ^ res) : (uint16_t)~(l ^ r) & (l ^ res)) & sign) c->flags |= 0x800u;
    szp(c, res, bits); return res;
}
static inline uint16_t logic(pr_cpu_t *c, uint16_t v, unsigned bits)
{
    v = (uint16_t)(v & (bits == 8u ? 0xFFu : 0xFFFFu)); c->flags &= (uint16_t)~0x08D5u; szp(c, v, bits); return v;
}
static inline uint16_t shift(pr_cpu_t *c, uint16_t v, uint8_t n, unsigned bits, unsigned right)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u, pre, res;
    n &= 0x1Fu;
    if (n == 0u) return v;
    v = (uint16_t)(v & mask);
    pre = right ? ((uint32_t)v >> (n - 1u)) : (((uint32_t)v << (n - 1u)) & mask);
    res = right ? (pre >> 1u) : ((pre << 1u) & mask);
    c->flags &= (uint16_t)~0x08D5u;
    c->flags |= 0x10u;
    if (right ? (pre & 1u) : (pre & sign)) c->flags |= 1u;
    if ((pre ^ res) & sign) c->flags |= 0x800u;
    szp(c, (uint16_t)res, bits); return (uint16_t)res;
}
static inline uint16_t incdec(pr_cpu_t *c, uint16_t v, unsigned bits, unsigned dec)
{
    uint16_t before = c->flags;
    uint16_t r = (uint16_t)(dec ? v - 1u : v + 1u);
    c->flags = (uint16_t)((before & (uint16_t)~(0x08D5u & ~0x0001u)) & 0xFFFFu);
    c->flags = before & (uint16_t)~0x08D5u;
    c->flags |= (uint16_t)((uint16_t)(~before) & 0x0001u);
    szp(c, r, bits);
    if (dec ? (r & (bits == 8u ? 0x80u : 0x8000u)) == (bits == 8u ? 0x80u : 0x8000u) : r == 0u)
        c->flags |= 0x800u;
    if (((before & 0x0001u) != 0u)) c->flags |= 0x0001u;
    return r;
}
'''

HEADER = '''#ifndef POOLRAD_RECOVERED_%(guard)s
#define POOLRAD_RECOVERED_%(guard)s

#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/candidate_ecl_cpu.h"

/* Candidate-scout draft, no credit claimed.
 * %(description)s
 * Raw-CPU C: one case per original instruction, operands reread from flat 1 MiB RAM.
 * Unit-0x25 runtime calls are shared candidate doubles, not the real helpers. */
typedef enum {
    PR_%(UP)s_OK = 0,
    PR_%(UP)s_ARGUMENT,
    PR_%(UP)s_TRANSFER
} %(lower)s_status;

%(lower)s_status %(lower)s_cpu(uint8_t *b, size_t ram_size,
                               pr_candidate_cpu *c, uint16_t caller_cs,
                               void *context);

#endif
'''

SOURCE = '''/* Candidate-scout draft, no credit claimed. %(description)s */
#include "poolrad/recovered/%(header)s"
%(helpers)s
%(lower)s_status %(lower)s_cpu(uint8_t *b, size_t ram_size,
                               pr_candidate_cpu *c, uint16_t caller_cs,
                               void *context)
{
    pr_candidate_ctx *dg = (pr_candidate_ctx *)context;
    uint16_t caller;
    (void)caller_cs;
    if (b == NULL || c == NULL || ram_size != RAM_SIZE) return STATUS(ARGUMENT);
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
CASES
        default: return STATUS(TRANSFER);
        }
    }
}
'''


def _remove_function(text: str, name: str) -> str:
    """Remove one `static inline ... name(...) { ... }` definition, brace-aware."""
    m = re.search(r"(?m)^static inline [^\n]*\b" + re.escape(name) + r"\s*\(", text)
    if not m:
        return text
    try:
        i = text.index("{", m.end())
    except ValueError:
        return text
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                end = j + 1
                if end < len(text) and text[end] == "\n":
                    end += 1
                return text[:m.start()] + text[end:]
    return text


def render(code: bytes, stem: str, fn: str, start: int, end: int,
           calls: dict[int, str], description: str):
    body = code[start:end]
    emitter = Emitter(body, start, end, calls)
    cases = "\n".join(emitter.cases())
    up = fn.upper()
    if up.startswith("PR_"):
        up = up[3:]
    header = HEADER % dict(guard=up + "_H", UP=up, lower=fn,
                           description=description)
    helpers = HELPERS % dict(UP=up, lower=fn)
    source = (SOURCE % dict(description=description, header=stem + ".h", lower=fn,
                            UP=up, helpers=helpers)).replace("CASES", cases)
    keep = {"physical", "word_at", "rd8", "rd16", "wr16", "push", "pop"}
    if "arith(c," in cases:
        keep.update({"arith", "szp"})
    for name in ("mul16", "shift", "incdec", "logic", "wr8", "arith", "szp"):
        if name not in keep and name + "(" not in cases:
            source = _remove_function(source, name)
    return header, source

