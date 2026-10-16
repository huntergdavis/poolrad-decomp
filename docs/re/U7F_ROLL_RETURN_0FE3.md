# U7F +0FE3 supplied-poststate continuation

`pr_u7f_roll_return_0fe3` resumes at original U7F +0FE3 after the
`LCALL B0:0048` callback has supplied live full RAM and CPU14 state.
It models the original conditional tier classification and far return
through +100F. It does not execute the far-call stub or U9E entry8.

The owner-supplied `GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The containing U7F function is file `[154715,154777)`, 62 bytes / 27
instruction starts, SHA-256
`9205417006da7a4c928b547d116107e3f4d269d3d267d17b2f7cad2f3e63fe27`.
The continuation spans +0FE3..+100F inclusive. Values 1..14 produce
1; 15..19 produce 2; zero and 20..255 produce 3. The final CMP flags,
both local byte writes, `MOV SP,BP`, `POP BP`, and `RETF` remain live.
The original-byte checker passes 40 supplied-frame cases, comparing
full 1 MiB A20-off RAM and all 14 CPU words at the return destination,
with all 19 continuation instruction starts and classification edges
covered. The focused C test separately covers BP=FFFC/FFFD return
frames: return IP and CS words are read from wrapped 16-bit SS offsets.

This is a bounded source continuation. It takes a controlled post-callee
frame, not a naturally reached frame. The separate +0FD2 prefix and
+0FDE call adapter do not yet compose a complete callee path. No game
reach, native parity, identity or strict full-C credit is claimed.
