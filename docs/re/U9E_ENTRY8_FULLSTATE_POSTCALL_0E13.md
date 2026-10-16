# U9E entry8 supplied post-SYSTEM return side

`pr_u9e_entry8_fullstate_postcall_0e13` translates only U9E entry8
`+0E13..+0E1E`. Its caller supplies live 1 MiB RAM and CPU14 after the
original nested SYSTEM `RETF 2` would return to +0E13. It does not execute
the SYSTEM call, simulate its return, or enter the shared +0E1F tail.
This bounded source gate carries no native, full-function, or strict-count
credit. PM assigned it separately from the +0DE3 prefix and +0E1F tail
(i7580). The exact `deja` query found no indexed prior +0E13 session;
the [entry8 callback map](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md) provides
the checked-in frame ABI, which was checked again against ignored media.

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U9E entry8 is 75 bytes / 29 starts at file `[0x2BCCD,0x2BD18)`, SHA-256
`7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba`.
The post-call slice is file `[0x2BCFD,0x2BD09)`, 12 bytes / five starts,
SHA-256
`ca9171f30531cf4c50426daac59c488b59c46622436ee26859f0f63155a66ba0`.
It executes `INC AX`, adds the new AL to live SS:`[BP-3]`, reads loop
index from `[BP-2]` into AL, compares it with count snapshot `[BP-4]`,
then stops at +0E05 when different or +0E1F when equal. The final CMP
defines all six arithmetic FLAGS. BP, SP, segments and the other general
registers remain live and unchanged. The source rejects a write into
the pinned loaded U9E entry8 code before mutation; it assumes that code
was loaded from the original body.

The focused C11 test checks both destinations, the exact 1 MiB RAM diff,
all 14 CPU words, AX low-byte rollover into AH, signed CMP overflow,
wrapped SS:BP local offsets, and unchanged unsupported input. A separate
manual static audit pins the original/loaded bytes and exercises the C
gate without an emulator. Native/original post-SYSTEM RAM and CPU14
parity remains open under the host hold. The +0E05 loop successor and
+0E1F shared tail are separate follow-on source boundaries.
