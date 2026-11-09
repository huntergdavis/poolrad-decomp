# Unit 3B0 restricted prompt selection branch

`pr_u3b0_selection_05c4` continues a caller-supplied `+05C4` state, including
one from `pr_u3b0_character_059c`. Comma decrements the live selected index,
wrapping 1 to the live maximum; period increments it, wrapping the live
maximum to 1. It forms the original argument, BP and return stack and composes
the existing `pr_u3b0_entry_01da_cpu` drawer on the **same** CPU/RAM/EGA bus.
A returned drawer reaches `+06CF`; other characters stop at `+060C` before
the subsequent UpCase/selection path. The reached near-call opcode and target
are checked after the original earlier pushes. CODE, nested CALLEE and bad
TRANSFER stops retain prior index, stack, CPU, RAM and EGA effects.

The caller supplies loaded source and all previous prompt effects, live
registers and frame, EGA/bus, drawing and port services, and the drawer's
limits. Entry requires 1 MiB A20-off RAM, EGA, ordinary non-aperture code,
DS and SS windows, BP at least `008E`, and SP in `[0048,FFFC]`. The
`+01DA` model's documented code/set immutability, non-aperture operand,
nonwrapping reached frames, storage separation, and callback restrictions
remain caller obligations. Source identity and prior prompt provenance are
not inferred from the supplied state. Offsets are 16-bit and physical
addresses wrap at 1 MiB. There is no rollback or arbitrary-PC resume.

Source: owner GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, unit
3B0 base `2C8CF`, `[05C4,060C)` 72 bytes SHA-256
`a3ce6b5f63c499b6d1b4669a904c744fd18de774bdae359b20c86aed4783e7d3`.
The prior [`U3B0_CHARACTER_059C_DIAGNOSTIC.md`](U3B0_CHARACTER_059C_DIAGNOSTIC.md)
and existing [`U3B0_ENTRY_01DA_CPU.md`](U3B0_ENTRY_01DA_CPU.md) define adjacent
models. No original game data are included.

Synthetic tests use the actual drawer, bus, EGA and glyph models, covering the
`+059C` handoff, comma/period selection wrapping, actual redraw port and VRAM
writes, a refused draw with earlier changes retained, a bad call with pushes
retained, byte increment wrap, another-character boundary and atomic invalid
argument. They do not prove native execution. This is a branch diagnostic;
it neither runs the whole prompt nor provides an entry2 hook, initialized
saved gameplay, native/visual parity or strict credit.
