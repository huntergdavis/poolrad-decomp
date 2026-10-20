"""Edge-SP probe for SYSTEM Move (0AF8:1685) and GetMem's exit (0AF8:0248 POP BP; JB; RETF 2).

For each entry SP the far-return frame is written twice with DIFFERENT CS values:
  wrapped copy at SS:((SP+k) mod 64K)  -> return CS = WRAP_CS
  contiguous copy at linear SS*16+SP+k -> return CS = CONT_CS (only where the two differ)
Both targets hold a recorder, so the CS actually taken identifies the read.
Three oracles: DOSBox-X (native COM), Unicorn 2.1.4, and the merged C models.
Original bytes are read from the owner START.EXE and staged only in the scratch COM.
Usage: python3 tools/overlay/retf_stack_edge_probe.py . resources/retf-edge   (owner media required; the COM is written only under <workdir>)
Recorded in REVIEW_LANE_HANDOFF.md ("RETF stack-top wrap"); not a row checker.
"""
import ctypes, struct, subprocess, sys, os
from pathlib import Path

root, work = Path(sys.argv[1]), Path(sys.argv[2]); work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(root / "tools/overlay"))
import resident_loader_map as R
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import *

img = R.unpack((root / "resources/DOS/POOLRAD/START.EXE").read_bytes()); img = img[0] if isinstance(img, tuple) else img
SYS = 0xAF8
MOVE = img[SYS * 16 + 0x1685: SYS * 16 + 0x16A8]            # 1685..16A7 incl. RETF 0Ah
EXIT = img[SYS * 16 + 0x0248: SYS * 16 + 0x024E]            # POP BP; JB 024E; RETF 2
assert MOVE[-3:] == b"\xca\x0a\x00" and EXIT[-3:] == b"\xca\x02\x00", (MOVE[-3:].hex(), EXIT[-3:].hex())

SS, BODY_CS, WRAP_CS, CONT_CS, RET_IP = 0x2000, 0x5000, 0x4000, 0x4100, 0x0100
SPS = (0xFFF8, 0xFFFA, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF)


def frame_bytes(kind, sp):
    """(offset-in-SS -> byte) for the wrapped copy and (linear -> byte) for the contiguous copy."""
    if kind == "move":   # IP, CS, CX=0, dest far (0:0x0200 in SS), src far
        words = [RET_IP, None, 0x0000, 0x0200, SS, 0x0300, SS]
    else:                # GetMem exit entered at 0248: [SP]=saved BP, then IP, CS, size
        words = [0x1234, RET_IP, None, 0x0010]
    wrapped, contiguous = {}, {}
    for i, w in enumerate(words):
        for b in range(2):
            off = sp + 2 * i + b
            val_w = (w if w is not None else WRAP_CS) >> (8 * b) & 0xFF
            val_c = (w if w is not None else CONT_CS) >> (8 * b) & 0xFF
            wrapped[off & 0xFFFF] = val_w
            if off > 0xFFFF:
                contiguous[SS * 16 + off] = val_c
    return wrapped, contiguous


def flat_image(kind, sp):
    mem = bytearray(1 << 20)
    body, entry = (MOVE, 0x1685) if kind == "move" else (EXIT, 0x0248)
    mem[BODY_CS * 16 + entry: BODY_CS * 16 + entry + len(body)] = body
    wrapped, contiguous = frame_bytes(kind, sp)
    for off, v in wrapped.items(): mem[SS * 16 + off] = v
    for lin, v in contiguous.items(): mem[lin & 0xFFFFF] = v
    return mem, entry


def unicorn(kind, sp):
    mem, entry = flat_image(kind, sp)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); uc.mem_map(0, 1 << 20); uc.mem_write(0, bytes(mem))
    for r, v in ((UC_X86_REG_SS, SS), (UC_X86_REG_SP, sp), (UC_X86_REG_DS, 0x3000), (UC_X86_REG_CS, BODY_CS),
                 (UC_X86_REG_IP, entry), (UC_X86_REG_FLAGS, 0x0002), (UC_X86_REG_BP, 0)):
        uc.reg_write(r, v)
    out = {}
    def hook(e, a, s, u):
        cs = e.reg_read(UC_X86_REG_CS)
        if cs in (WRAP_CS, CONT_CS) or cs != BODY_CS:
            out.update(cs=cs, ip=e.reg_read(UC_X86_REG_IP), sp=e.reg_read(UC_X86_REG_SP)); e.emu_stop()
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(BODY_CS * 16 + entry, 1 << 20, count=500)
    return out


def c_models(kind, sp):
    """Merged C models: Move whole body; GetMem's RETF read reproduced from its source (physical(ss,sp), +2 & MASK)."""
    mem, entry = flat_image(kind, sp)
    if kind == "move":
        so = work / "leaves.so"
        if not so.exists():
            subprocess.run(["cc", "-shared", "-fPIC", "-O1", "-I", str(root / "include"),
                            str(root / "src/recovered/system_string_move_leaves_cpu.c"), "-o", str(so)], check=True)
        lib = ctypes.CDLL(str(so))
        class Cpu(ctypes.Structure):
            _fields_ = [(n, ctypes.c_uint16) for n in ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip", "ds", "es", "ss", "flags")]
        cpu = Cpu(sp=sp, ss=SS, ds=0x3000, cs=BODY_CS, ip=entry, flags=0x0002)
        ram = (ctypes.c_uint8 * (1 << 20)).from_buffer(mem)
        st = lib.pr_system_move_1685(ram, 1 << 20, ctypes.byref(cpu))
        return dict(status=st, cs=cpu.cs, ip=cpu.ip, sp=cpu.sp)
    # GetMem 024B RETF 2 as the merged source reads it: at = physical(ss, sp); ip = word(at); cs = word((at+2) & MASK)
    sp2 = (sp + 2) & 0xFFFF                     # after POP BP
    at = (SS * 16 + sp2) & 0xFFFFF
    w = lambda a: mem[a & 0xFFFFF] | mem[(a + 1) & 0xFFFFF] << 8
    return dict(cs=w(at + 2), ip=w(at), sp=(sp2 + 6) & 0xFFFF)


def native(cases):
    """One COM: for each case set SS:SP, write both frame copies, far-jump into the staged body; recorders log CS."""
    lines = [".code16", ".text", "start:", "  cli", "  mov ax, cs", "  mov [main_cs], ax", "  push ds", "  xor bx, bx", "  mov ds, bx", "  mov [0x4f0], ax", "  pop ds"]
    # stage bodies
    for name, body, entry in (("move", MOVE, 0x1685), ("exit", EXIT, 0x0248)):
        lines += [f"  mov ax, {BODY_CS:#x}", "  mov es, ax", f"  mov di, {entry:#x}", f"  mov si, offset body_{name}",
                  f"  mov cx, {len(body)}", "  cld", "  rep movsb"]
    # recorders at WRAP_CS:0100 and CONT_CS:0100: store tag, SP, then jump back to main via stored far pointer
    for tag, seg in ((0x57, WRAP_CS), (0x43, CONT_CS)):
        lines += [f"  mov ax, {seg:#x}", "  mov es, ax", "  mov di, 0x100", f"  mov si, offset rec_{tag:x}",
                  f"  mov cx, rec_end_{tag:x} - rec_{tag:x}", "  rep movsb"]
    for n, (kind, sp) in enumerate(cases):
        wrapped, contiguous = frame_bytes(kind, sp)
        lines += [f"case{n}:", f"  mov word ptr cs:[cur], {n}", f"  mov word ptr cs:[back], offset after{n}"]
        lines += [f"  mov ax, {SS:#x}", "  mov es, ax"]
        for off, v in sorted(wrapped.items()): lines.append(f"  mov byte ptr es:[{off:#x}], {v:#x}")
        for lin, v in sorted(contiguous.items()):
            seg, off = (lin >> 4) & 0xF000, lin - (((lin >> 4) & 0xF000) << 4)
            lines += [f"  mov ax, {seg:#x}", "  mov es, ax", f"  mov byte ptr es:[{off:#x}], {v:#x}"]
        entry = 0x1685 if kind == "move" else 0x0248
        lines += [f"  mov ax, {SS:#x}", "  mov ss, ax", f"  mov sp, {sp:#x}", "  mov ax, 0x3000", "  mov ds, ax",
                  "  xor bp, bp", "  push 0x0002", "  popf", f"  .byte 0xea", f"  .word {entry:#x}", f"  .word {BODY_CS:#x}", f"after{n}:"]
    lines += ["  mov ax, cs", "  mov ds, ax", "  mov ss, ax", "  mov sp, 0xFFF0", "  sti",
              "  mov ah, 0x3c", "  xor cx, cx", "  mov dx, offset fname", "  int 0x21", "  mov bx, ax",
              "  mov ah, 0x40", f"  mov cx, {6 * len(cases)}", "  mov dx, offset results", "  int 0x21",
              "  mov ah, 0x3e", "  int 0x21", "  mov ax, 0x4c00", "  int 0x21"]
    for tag in (0x57, 0x43):   # recorder: results[cur] = (tag, cs, sp); far-return to main via cs:[back]
        lines += [f"rec_{tag:x}:", "  mov bx, sp", "  mov dx, cs", "  xor ax, ax", "  mov ds, ax", "  mov ds, [0x4f0]",
                  "  mov si, [cur]", "  shl si, 1", "  mov di, si", "  shl si, 1", "  add si, di",
                  f"  mov word ptr [results + si], {tag:#x}", "  mov [results + si + 2], dx", "  mov [results + si + 4], bx",
                  "  push word ptr [main_cs]", "  push word ptr [back]", "  retf", f"rec_end_{tag:x}:"]
    lines += ["main_cs: .word 0", "cur: .word 0", "back: .word 0", 'fname: .asciz "EDGE.BIN"',
              f"results: .fill {6 * len(cases)}, 1, 0xEE"]
    for name, body in (("move", MOVE), ("exit", EXIT)):
        lines.append(f"body_{name}: .byte " + ", ".join(f"{b:#x}" for b in body))
    asm = work / "edge.s"; asm.write_text(".intel_syntax noprefix\n" + "\n".join(lines) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(work / "edge.o")], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(work / "edge.o"), "-o", str(work / "EDGE.COM")], check=True)
    (work / "dosbox.conf").write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[autoexec]\n"
                                      f"mount c {work}\nc:\nEDGE.COM\nexit\n")
    if (work / "EDGE.BIN").exists(): (work / "EDGE.BIN").unlink()
    subprocess.run(["dosbox-x", "-conf", str(work / "dosbox.conf"), "-fastlaunch", "-exit", "-noconsole"],
                   env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"), capture_output=True, timeout=120)
    raw = (work / "EDGE.BIN").read_bytes()
    return [dict(zip(("tag", "cs", "sp"), struct.unpack_from("<3H", raw, 6 * n))) for n in range(len(cases))]


cases = [(k, sp) for k in ("move", "exit") for sp in SPS]
nat = native(cases)
print(f"{'body':5s} {'SP':>5s} | {'DOSBox-X CS':>11s} | {'Unicorn CS':>10s} | {'C model CS':>10s}   (W={WRAP_CS:04X} wrapped, C={CONT_CS:04X} contiguous)")
for (kind, sp), n in zip(cases, nat):
    u, c = unicorn(kind, sp), c_models(kind, sp)
    name = lambda cs: "W" if cs == WRAP_CS else ("C" if cs == CONT_CS else f"{cs:04X}")
    print(f"{kind:5s} {sp:04X}  | {name(n['cs']) if n['tag'] in (0x57, 0x43) else 'none':>11s} | {name(u.get('cs', -1)):>10s} | {name(c['cs']):>10s}")
