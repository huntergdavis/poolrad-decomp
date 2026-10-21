# RES_0709 +0BF5: translated, evidenced message-line prompt (evidence only; no credit proposed)

This raw CPU body copies a Pascal message onto the stack, clears row 24 of the display, prints the message there in the caller's
colour, waits for a key, and returns the key. It is a body of the game's resident segment 0709, a callee of `OVL_006F_0EC4` and of nine
other overlay sites, and a **direct dependency of the 0EC4 program**. This change adds the model, its checker and this document. It
proposes **no strict row, no identity row and no manifest row**; par-e accepts or refuses.

## Media and executable extent

START.EXE SHA-256: d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd. Loader-expanded image: 68,592 bytes, SHA-256
a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e.

The decoded span is **0709:[0BF5,0C3F), 74 bytes / 38 instructions**, physical [07C85,07CCF), ending in the only return, `RETF 6` at 0C3C. Body
SHA-256: `4a3f940caae95794f257a25f5241bcf3a04c2749b212f5b8d9def5befd63a4cc`. The return was located by decoding forward from the entry (the body is
straight-line, so the first return is the only one) and the extent was taken from the expanded executable bytes, not from a queue row. The checker
asserts the hash, the 38 starts, the single return, the absence of any jump, interrupt or port instruction, the four callees, and that **exactly two
loader relocations fall inside the span, at CS:0C0C and CS:0C34** (the segment words of the two far calls). It reads both live.

## What the body does

Frame: a far call leaves `[SP]` IP, `[SP+2]` CS, `[SP+4]/[SP+6]` the message's far pointer (offset, segment), `[SP+8]` the colour (a byte in a word slot).

1. `PUSH BP; MOV BP,SP; SUB SP,2Ah` (flags as `SUB`).
2. **StrStore** `0AF8:0B25`: copy the Pascal message into `[BP-29h]`, at most 28h characters.
3. **05EB** (near call, `PUSH CS; CALL`) with the words 0, 18h, 0, 28h: the row clear. 05EB draws the glyph for a space through +01DF.
4. **0626** (near call) with x=0, y=18h, the colour `[BP+0Ah]`, and the copy at `SS:BP-29h`: it prints the message through +01DF.
5. **ReadKey** `0899:0059`: wait for a key (with its hotkeys); `[BP-2Ah] := AL`.
6. `MOV SP,BP; POP BP; RETF 6`. **AL, the key, is the result.**

## Role (identity is not claimed)

The role is the game's **message-line prompt**: its subject is the game's own text on row 24 and the key the player presses. The measured ownership facts:
**ten fixup-backed far-call sites in the overlays** (`001C:1A76`, `001C:2A99`, `0020:162C`, `0025:21B2`, `006F:0234`, `006F:031A`, `006F:08A4`,
`006F:0F44`, `006F:1E58`, `03D2:00DB`) and two far transfers inside the resident image (`0x20C9`, `0x2158`). Its three display callees and ReadKey
are credited bodies. This note makes no claim about identity: the two-factor test is the reviewer's, and the callers named here are evidence of use.

## Raw contract and stops

`pr_r0709_msgbox_0bf5(bus, size, cpu, env, diagnostic)`: exactly 1 MiB of A20-off RAM, IP=0BF5, entry SP in [016A,FFF6] (the 2Ah frame and the composed
frames below it, the argument words and the RETF frame above it), immutable code, distinct CPU/bus objects, an attached EGA. The message, the stack and
the data segment stay in ordinary RAM (they exclude A0000..AFFFF): the display writes go through the EGA. No interrupt, fault, self-modification or
arbitrary-IP resume. Every callee's segment word and near-call displacement is read live.

Every stop, with its class, where it comes from and the check that pins it:

| Stop | Class | Origin | Pinned by |
| --- | --- | --- | --- |
| RETURNED | RETURN | `RETF 6` at 0C3C | Unicorn: C and the original agree on all 14 CPU words, all 1 MiB, the EGA state, the OUT and VRAM events and the environment log at the return, in 112 of 123 cases |
| KEY(EXIT) | EXIT | ReadKey's hotkey 03 with ParamStr 'STING' reaches QUIT and Halt, so the prompt never returns | Unicorn: 9 cases, same state at the Halt boundary on both sides |
| KEY(BREAK) | EXIT (Crt Ctrl-Break) | ReadKey's `Crt` break path | Unicorn: 2 cases, equal at the break boundary |
| KEY(LIMIT, CRT, DRIVER, LEAF, QUIT, TRANSFER) | SERVICE / LIMIT | ReadKey stops otherwise (its diagnostic names it) | C test (drain limit); the Unicorn run reaches none of them, and says so (**non-architectural: 0 measured**) |
| SPACE, PRINTER | LIMIT (host glyph budget) | 05EB / 0626 did not return normally | C test only: budgets 0 stop 05EB (callee status) and 0626 (LIMIT); the differential runs with budgets never reached |
| CALLEE | non-architectural | a call site's target is wrong, or a callee returned elsewhere | C test (a corrupted displacement stops before the callee runs); the original shows the same behaviour as TRANSFER if the return frame is corrupted |
| ARGUMENT | ARGUMENT | null/size/IP/SP out of contract | C test: CPU and RAM untouched |

There is no HANG of its own: the body is straight-line. ReadKey's wait is a service (the key script); an empty script is an environment refusal, which is **counted and
must be zero**: the final run has none.

## Evidence

**Original-byte Unicorn differential on the composed closure: 123 cases, 0 mismatches.** The whole loader-expanded image is loaded at a test base with the loader's
relocation table applied, so every original byte of the closure runs: this body, StrStore, 05EB, 0626, the +01DF glyph writer, SYSTEM UpCase, and ReadKey with
its closure (Crt, the 08AE driver members, SYSTEM leaves, QUIT). The A000 aperture is an independent Python EGA (planes, latches, indexed registers) on an MMIO
window; every OUT is forwarded to it. After every case, **unmasked**:

- CPU14 (**dirty incoming flag words 0202, 0602, 0A87, 0402 and 0E57 are in the case set**, with the six clean ones);
- **all 1,048,576 RAM bytes**, the aperture's backing bytes included (`ram_masked_bytes = 0`);
- the **full EGA state**, 262,164 bytes (planes, latches, indexed registers);
- the ordered EGA OUT events with their CS:IP (**80,036 events**) and the ordered **VRAM write operands (150,656)**;
- the ordered ReadKey environment events (INT 16h, INT 21h, the driver ports), and the stop.

Outcomes: **112 RETURN, 9 EXIT, 2 BREAK**. Starts: this body **38/38 (100%)**; there are no conditional branches in the body, so no successor accounting.
Closure members (their own audits own their coverage): 01DF 400/403, 05EB 26/27, 0626 50/51, StrStore 16/16, UpCase 8/8, ReadKey 61/61.
**The five unexecuted member starts are unreachable from this caller, not untested:** 05EB `0606` and 0626 `064D` are the x/y range rejects, and this
body passes x=0, y=18h (both in range); 01DF `021A`, `021D` are the transform=0 path and `022D` is the count=0 reject, and every call here passes transform 1 and count 28h
or 1. (01DF's zero-font-pointer return at `01FA` *is* reached, by the `nofont` cases.)

Cases: message lengths 0, 1, 2, 11, 39, 40, 41, 60 and 255 in each game display mode 0..3; colours 0..15 and random; ReadKey's keys (plain, type-ahead drain, extended,
zero, buffered, ctrl-S sound toggle in all nine saved states, ctrl-D, ctrl-C with and without 'STING', Ctrl-Break); display pages, font offsets and a zero font pointer;
SP at both bounds, odd, SS=DS; the message aliasing the stack buffer it is copied to (a source equal to the destination); the message offset wrapping at 64 KiB; 24 random
registers/flags/keys/pages.

### Claim-to-check

| Claim | Check |
| --- | --- |
| the C model equals the original closure | Unicorn differential above (CPU14, 1 MiB, EGA state, OUT/VRAM/env logs, stop) |
| 0BF5 hands each callee the frame the listing says | independent Python statement of the call frames, read at each callee's entry as its own bytes run: StrStore (`0C0E`, CS, 28h, `BP-29h`, SS, message offset, segment), 05EB (`0C1E`, CS, 28h, 0, 18h, 0), 0626 (`0C31`, CS, `BP-29h`, SS, colour, 18h, 0) and ReadKey (`0C36`, CS) |
| the Pascal copy is the message truncated to 40 | independent: length byte and bytes at `SS:BP-29h` for every returning case |
| `[BP-2Ah]` and AL are the key; the hotkey state is right | ReadKey audit's independent statement (`expected`) |
| the model's stops are the original's | stop compared per case; C-only stops are asserted in the C test (38 checks, ASan/UBSan) |
| the checker detects a wrong model | 5 deliberate mutations (wrong row, wrong colour slot, wrong key slot, wrong StrStore max, wrong string segment): **5 of 5 caught** |

### Declared environment and substitutions

The environment is declared and identical in the oracle and the C callbacks: ReadKey's (INT 21h 25h/35h/2Ch, ports 40h/43h/61h/C0h, INT 16h from a per-case script,
QUIT's INT 10h stub) and the EGA sequencer/graphics-controller ports 3C4h/3C5h/3CEh/3CFh. Nothing is patched in the image. **Substitution:** the EGA is an independent Python
device, not a real one; the BIOS and DOS are the scripted services above, not real. Every unlisted access is an environment refusal (counted, zero).

### Native corroboration (partial; packaged DOSBox-X, not the pinned build)

DOSBox-X normal core, 386, **machine=ega**, video mode 0Dh, **27 synthetic cases** (game display modes 1 and 3; six messages from empty to 60 characters; three page pairs; five key
scripts; the clean and dirty-without-IF flag images). The relocated image is read from IMAGE.BIN before every case, the four planes are filled with the C model's pattern through
the real ports, INT 16h is a real `INT` into a hook that replays the key script, and the planes are read back through graphics-controller read mode 0. Compared with the C
model: all 14 CPU words, the stack window (minus the 10 DOS-side bytes under each INT 16h call), the whole 64 KiB data segment, the key count and **all four planes
(262,144 bytes per case)**. **21 of the 27 cases drew** (52,468 plane bytes differ from the fill in total); the six that left the planes untouched are exactly the six display-mode-3 cases (mode 3 does not write the EGA planes), so for those only
the CPU words, the stack window and the data segment are compared natively. **Not compared natively:** the OUT events (no port trap), the EGA indexed registers, the remaining RAM, and any path through QUIT, the ctrl-S
driver or Ctrl-Break (those are Unicorn-only). The native run used the host's **packaged `/usr/bin/dosbox-x` 2026.01.02** (SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`), **not the pinned `5a9c7c53` build**. RESULT.BIN SHA-256:
`3376c1f208778b6862728071e99a2a9a9275cff0410ce6626531da499bb5aa44`. Frames are synthetic; this does not establish natural reachability.

## Not established

- Real BIOS, DOS or EGA behaviour beyond the 27 native cases; modes 0 and 2 (packed memory) and any CGA or Tandy hardware.
- Host-budget stops (SPACE, PRINTER, KEY LIMIT) and the CALLEE and ARGUMENT stops are covered by the C test, not by the differential.
- Interrupts, faults, self-modification, arbitrary-IP resume.
- Identity and credit: this document supplies evidence and a role; the decision is the reviewer's.
- The prior-session memory tool (`deja`) was unavailable; nothing was reused from earlier sessions. The 0193 and 0626 audits and native harnesses were reused as templates,
  and every figure above was measured on this change.

## Reproduction

    python3 tools/overlay/res0709_msgbox_0bf5_cpu_audit.py --unicorn --native --output resources/0bf5-evidence.json
    python3 tools/overlay/res0709_msgbox_0bf5_cpu_audit.py --compile-only
    python3 tools/overlay/check_checker_builds.py --all --require-current-main
    cmake -S . -B resources/build0bf5 -DCMAKE_BUILD_TYPE=Debug
    cmake --build resources/build0bf5 --target poolrad_resident_0709_msgbox_0bf5_cpu_tests
    ctest --test-dir resources/build0bf5 -R '^poolrad_resident_0709_msgbox_0bf5_cpu_tests$' --output-on-failure

Use the audit Python environment (Capstone, Unicorn). Original bytes, generated assembly/COM and capture receipts stay under ignored `resources/`.
