# Standalone second RES0464 postreturn at `0C88:29CB`

This bounded model starts from an **independently supplied** second Move return
frame. It executes only the loaded `+29CB..+29D7` continuation and stops at
`+29AB`, before the next counter increment. It does not run Move, prove a
natural second return, or establish full resident routine parity.

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The frozen run015 640 KiB source image is held only under ignored `resources/`;
its SHA-256 is
`21efc0155f250c31578643ac2d3f5030379fdedf92a155367ffc221b05ce1867`.
Direct run015 fourth-call native evidence reaches only `0C88:277D`.

The original loaded continuation is `83 46 EA 04 A0 C7 70 3A 46 DC 75 D4`
at `0C88:29CB`, SHA-256
`9d893af05bfb7d93b096a1078bcc70fc147eda8b258d5df2832c770a62dbeb70`.
The loaded far CALL `9A 85 16 1C 13` at `0C88:29C6` has SHA-256
`42737177f1cc0a3636541ccf5768dcac8a48e4b368807d7c12bb8fbe63c45118`.
The loaded 35-byte Move target at `131C:1685` has SHA-256
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.
The test checks these bytes in the frozen image before it supplies any state.

The supplied prestate CPU is `CS:IP=0C88:29CB AX=0004 BX=3E4E CX=0000
DX=14A0 SI=3E68 DI=1487 BP=3E80 SP=3E5C DS=14A0 ES=3DE4 SS=1D16
FLAGS=7206`. RAM has the five Move argument words at `SS:3E52..3E5A`,
return residue `29CB:0C88` at `SS:3E4E`, `SS:[BP-24]=0B`, local
`SS:[BP-16]=1464`, `DS:70C7=02`, and source plus two destination masks
`00 FF FF 00`. This is the reviewed supplied-return contract, not a
measured natural return.

On that exact prestate, `ADD [BP-16],4` makes the local word `1468`,
`MOV AL,DS:70C7` makes `AX=0002`, `CMP 02,0B` makes `FLAGS=7293`,
and `JNE` selects `+29AB`. No other CPU field or RAM byte changes.
Unsupported prestates and invalid host inputs have no effects.

The strict C11 ASan/UBSan focused test runs synthetic and supplied frozen
640 KiB cases. It compares the complete CPU tuple and RAM, and rejects
changed continuation/CALL/Move bytes, count, destination mask, and flags.
The frozen case overlays the independent return frame onto the earlier
`+277D` image; it is not a same-run native second-return capture.

Prior local evidence: reviewed commit `70eea39182fb8812c96f1fbf1414386aad70b9ec`
and its parent `0869bdfda59625e3466b41c135b7fffed305299b`, including the
same supplied-return tuple, loaded byte pins, and frozen test contract. The
required bounded `deja` query timed out, so indexed recall is inconclusive.
