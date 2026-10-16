# U86 entry 1: source-only +01ED JZ after runtime call

PM inbox i7696 assigned this previously dormant instruction to pC for a
separate local C gate. p5, p9 and pR cleared the read-only ownership screen in
i7690; pR requested PM adjudication before later coding. Prior `deja "U86
01ED"` found the earlier pR branch grant in session `01a1281b-85e` and
pR's +01ED queue in `01a13c73-c6d`. The adjacent
[`U86_ENTRY1_POSTDC0_JOINED_PUSH_SS_C.md`](U86_ENTRY1_POSTDC0_JOINED_PUSH_SS_C.md)
supplies the established 14-word CPU layout and original-byte/fixup audit
pattern, not +01ED runtime parity.

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The overlay inventory places U86 at file `0x27018`, code size `0x37B5`,
entry 1 `+008A`. Its original bytes at file `[0x27205,0x27207)` are
`74 AD`, SHA-256
`1494e884fa55e9a6a34261f4b652305a2bd9fd2085830ab6a6977abbc44a5234`.
The preceding far call at `+01E8` targets runtime `0AF8:0DC0`; its returned
ZF is **supplied** to this gate. `JZ` from next IP +01EF with signed `AD`
(-83) stops at +019C when ZF is set. Otherwise it stops at +01EF. The
target begins `LEA DI,[BP-32h]` (`8D 7E CE`); fallthrough begins
`MOV AX,[4766h]` (`A1 66 47`). The source audit confirms no loader fixup
touches the branch or either immediate stop. No loaded native U86 +01ED
capture is claimed.

`pr_u86_entry1_01ed_jz` takes supplied loaded U86 CS, a 14-word CPU, and
1 MiB RAM. It requires IP +01ED and loaded `74 AD` at the A20-masked CS:IP,
then changes only IP to +019C or +01EF. It stops before either successor
instruction, does not inspect the far callee's frame, and preserves flags,
all other CPU words, and every RAM byte. Unsupported input rejects without
mutation. Synthetic tests compare full CPU/RAM for both ZF outcomes and an
A20-wrapped code address; wrong CS/IP/code/size/null input rejects atomically.

The exact original-byte audit uses ignored local `START.EXE` and `GAME.OVR`.
Strict C11 and ASan/UBSan local checks exercise the synthetic gate. There is
no native, callee, whole-function, strict/full-C, or game-function credit.
