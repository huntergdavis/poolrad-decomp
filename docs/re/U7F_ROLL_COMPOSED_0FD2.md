# U7F +0FD2 conditional roll composition

`pr_u7f_roll_composed_0fd2` composes the checked-in U7F +0FD2
prefix, +0FDE far-call adapter, and +0FE3 continuation. The callback
receives the live 1 MiB RAM and all 14 CPU words at B0:0048. It owns
the overlay stub, U9E entry8 +0DE3, SYSTEM calls, and RETF 4. The
wrapper preserves callback abort and transfer outcomes, and continues
only when the callback returns to the original CS:+0FE3.

The original containing function is GAME.OVR file `[154715,154777)`,
62 bytes / 27 instruction starts, SHA-256
`9205417006da7a4c928b547d116107e3f4d269d3d267d17b2f7cad2f3e63fe27`.
The bounded deja query for the composition and U9E callback timed out.
Reused merged prefix, call, and continuation evidence, plus the
full-state U9E entry9 callback convention. These are source and ABI
dependencies, not a natural U7F route or verified U9E callback body.

The focused C test supplies a synthetic full-state callback, checks
argument/return frames, propagated register and RAM effects, all tier
edges, two stack wraps, abort, and transfer. The separate original-byte
whole-caller checker has exactly eight controlled cases, one at a time,
stopping at B0:0048 to supply matching callback effects before executing
the original continuation. PM granted one bounded host slot; prelaunch
load was 3.07 and CPU PSI avg10 was 4.87. The single serial run under
`nice -n19` and a 30-second timeout passed all eight full 1 MiB RAM/CPU14
comparisons, reached 27/27 caller instruction starts, and covered both
outcomes of all four conditional decisions (eight measured edges).
The callback is synthetic and supplied identically at the original stub
and C adapter. Known Unicorn RETF CS offset-wrap cases are outside this
eight-case differential; focused C tests cover two wrapped frames.
No natural game frame,
native reach, original U9E callee parity, or strict full-C credit is
claimed.
