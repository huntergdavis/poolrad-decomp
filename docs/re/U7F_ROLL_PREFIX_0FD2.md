# U7F +0FD2 prefix to dice call

`pr_u7f_roll_prefix_0fd2` translates the seven original instructions
at U7F +0FD2..+0FDD and stops before the `LCALL B0:0048` at +0FDE.
It is a bounded source prefix, not a complete U7F function.

The owner-supplied `GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The complete containing U7F function is file `[154715,154777)`, 62
bytes / 27 instructions, SHA-256
`9205417006da7a4c928b547d116107e3f4d269d3d267d17b2f7cad2f3e63fe27`.
The prefix saves BP, reserves two bytes with flag-writing `SUB SP,2`,
then pushes AX words whose low bytes are 1 and 20 while AH remains live.
It leaves the original two-word argument frame in RAM, keeps all 14 CPU
words, and sets IP +0FDE without executing the far call.

The original-byte checker executes the exact prefix and stops before
the far call. All 49 cases pass, covering 7/7 instruction starts and
comparing full 1 MiB A20-off RAM and all 14 CPU words,
including the `SUB SP,2` flags, stack offset wraps, A20 access, and
varied incoming state. The focused C test checks stack word order and
host guards; direct ASan+UBSan passes. The bounded `deja` query for U7F +0FD2 timed out with no
finding. Reused evidence is the census exact span and adjacent pG
U7F +0F7E boundary. pP i7023 and pG i7043 confirmed no collision.

The far-call stub, U9E entry8 +0DE3 body, SYSTEM random step, and
the outer conditional/return continuation remain separate. No game
reach, native parity, identity or strict full-C credit is claimed.
