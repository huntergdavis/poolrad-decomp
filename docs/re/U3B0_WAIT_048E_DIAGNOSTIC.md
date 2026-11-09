# Unit 3B0 post-drawer wait integration

`pr_u3b0_wait_048e` starts at a caller-supplied live `+048E` state after the
prompt drawer. It follows the source's window/animation gate and signed
high-word/unsigned low-word clock gate. When the no-animation/no-clock route
reaches `+0566`, it invokes the existing single-pass prompt composition on the
same CPU, RAM, EGA bus and caller-owned services. The nested pass result is
reported directly; a no-key loop stops at `+048E` and is not automatically
repeated or treated as a resumable empty read.

A reached active animation branch stops **before `+04AD`**, where the original
begins preparing the `03D0:0025` work. A reached positive clock branch stops
**before `+0540`**, where the original calls `0709:0C3F`. Neither effect is
invented; earlier CPU flags, registers and bus reads remain. The caller must
supply or model those separate continuations. A negative high clock word takes
the original signed-less branch to `+0566`; when the high word is zero the
low word is compared unsigned against zero.

`LES DI,[DS:49D2]` and the word comparison use ordered bus reads; EGA aperture
reads load latches, including a word straddling the aperture boundary. The
caller owns live CPU, frame, code, bus and all nested pass services and must
satisfy `U3B0_PROMPT_PASS.md` and component restrictions. This slice requires
ordinary non-aperture CS/DS/SS windows and the pass-compatible BP/SP range.
It does not verify earlier prompt setup, list/string construction, the `+048B`
drawer, loaded source identity, stable code or host provenance. In particular,
`SLOT_A_POSTDRAWER_WAIT_PAGE.md` documents one conditional no-draw route and
unchanged pages, but did not retain same-run CPU rows proving the full route.
No entry2 provided flag, full prompt, native/gameplay/visual parity or strict
credit follows from this diagnostic.

Source: owner GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, unit
3B0 base `2C8CF`. `[048E,04AD)` 31 bytes SHA-256
`01ded5e81e4091083b8831af8d5066c24f3b310b9cca4004b5bbac93b592e2bb`;
`[0530,0540)` 16 bytes SHA-256
`e3ee1aacb2be3739db4726e5fd4c4c7dcd8eb07a17f8663b8c6901ad6bfe7ae4`.
The active animation and clock bodies are outside this slice. No original game
bytes are embedded.

Synthetic fixtures use the actual FIFO and nested prompt models for extended
Up and no-event loop, zero-window gate bypass, active animation and clock
stops, signed-negative high word, invalid frame atomicity, and ordered EGA
latch reads both within and across the aperture boundary. They verify this
bounded integration, not native reach or full prompt equality.
