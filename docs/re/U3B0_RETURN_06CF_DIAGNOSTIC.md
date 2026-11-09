# Unit 3B0 restricted +06CF return continuation

`pr_u3b0_return_06cf` continues the caller-supplied CPU/RAM state at unit
3B0 `+06CF`, including a state produced by the bounded `+0566` poll diagnostic.
It reads the live stop byte and either stops at loop entry `+048E` or executes
`+06D8..+06F8`: load window pointer, clear its `+03FE` word, copy the live key
and by-reference flag in original order, restore the frame and perform
`RETF 16h`. The caller supplies the expected return CS:IP; a mismatch stops at
`+06F8` after earlier effects, without rolling them back. No next-step or
resumption API is implied by a loop or transfer stop.

The caller owns the single A20-off 1 MiB RAM and CPU, live stack/frame,
expected target, loaded code identity and prior prompt/draw/timer effects.
None is proved by supplying a pointer or target. Entry frame offsets are
restricted so the modeled return frame does not wrap; physical reads and writes
wrap at 1 MiB as in the existing CRT models. Live pointers are reread in source
order, so a window clear may change an aliased flag before it is copied. The
caller must ensure loaded instructions remain the source-bound instructions
throughout the step; this projection does not inspect instruction bytes.

The original source span `[06CF,06FB)` is documented in
`U3B0_POLL_0566_DIAGNOSTIC.md`. `DONE` means only this restricted epilogue
returned to the caller-supplied target; it is not proof that the full prompt,
earlier draw or timer, initialized gameplay, or a native invocation occurred.
It does not set the host entry2 hook's `provided` flag, or claim native parity
or strict credit. A nonzero stop value may have been supplied independently;
this API does not infer reach from an Up key.

The synthetic integration fixture feeds the existing FIFO through actual
KeyPressed/0899 models and this continuation. It checks full 1 MiB RAM and CPU
for a return and loop case, an output/window alias, a return-target mismatch
with retained effects, and an invalid frame. No game bytes or native runtime
are used in the test.
