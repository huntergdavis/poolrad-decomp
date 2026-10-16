# U9E entry8 far call into SYSTEM random wrapper

`pr_u9e_entry8_system_call_0e0e` models only U9E entry8's five-byte
`LCALL 0AF8:11C0` at `+0E0E..+0E12` and delegates to the merged
[`SYSTEM +11C0` full-state source](SYSTEM_RANDOM_FULLSTATE_11C0.md).
It accepts the same live 1 MiB A20-off RAM and CPU14 at +0E0E and stops
at original U9E CS:+0E13 on normal return. Changed destinations and
SYSTEM failures remain explicit statuses with live state. PM cleared
this pF adapter after p5 released the SYSTEM interface (i7705).

The exact `deja "U9E +0E0E SYSTEM +11C0 full-state far call return adapter"`
query found no indexed prior session. The checked-in
[entry8 callback map](U9E_ENTRY8_FULLSTATE_CALLBACK_MAP.md),
[prefix](U9E_ENTRY8_FULLSTATE_PREFIX_0DE3.md), and SYSTEM source were
reused; the bounds and call bytes were checked again against ignored
owner-supplied DOS v1.3 media. `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The full U9E entry8 body is file `[0x2BCCD,0x2BD18)`, 75 bytes / 29
starts, SHA-256
`7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba`.
This call is file `[0x2BCF8,0x2BCFD)`, bytes `9A C0 11 F8 0A`, SHA-256
`362d77932929304eb813cc915a525b9851526311ab34f6ad21a17b1533b9f402`.
SYSTEM `+11C0..+11D5` and `+120F..+1244` are separately pinned by the
manual audit. The source does not model U9E's later loopback or return.

At +0E0E the preceding prefix has pushed a range WORD at SS:SP, and
BP is six bytes above SP. The far call pushes U9E CS at SS:SP-2, then
return IP +0E13 at SS:SP-4. SYSTEM enters at 0AF8:11C0 with return IP,
CS, and range WORD at offsets 0, 2, and 4. It writes a near return
+11C3, reads and writes the live DS seed, rereads the range after seed
writes, and returns with `RETF 2`; a normal U9E continuation has
SP increased by two from its +0E0E value. Every WORD byte is physically
contiguous, while each subsequent SS offset wraps to 16 bits before
address formation. The focused wrap case puts the far IP at FFFE and
far CS at 0000 with a false linear-CS decoy.

The adapter preflights its far frame, SYSTEM's near return word, and
the seed DWORD against the pinned loaded U9E and SYSTEM instruction
bodies. A host argument or such code alias rejects unchanged. The
merged SYSTEM source may report an argument rejection, callee failure,
or changed near-return transfer; the adapter preserves each live stop.
After SYSTEM `RETURNED`, a changed outer CS:IP reports a separate
return transfer. Seed/stack aliases outside code remain live, including
seed writes that redirect either return. Loaded instruction bytes are
assumed original; this C source does not verify arbitrary code RAM.

Strict C11 focused cases check normal and zero range, exact 1 MiB RAM
changes and all 14 CPU words against a separately framed invocation
of the merged SYSTEM source, a wrapped far frame, outer and near return aliases, and
unchanged invalid/code-alias inputs. A manual static audit hash-checks
both media, decodes the literal call, loads original U9E and SYSTEM
bodies, and checks controlled normal/wrapped C stops. It is not an
original-instruction or native execution oracle. SYSTEM's AF and DIV
undefined FLAGS remain unverified natively; the following U9E post-call
CMP defines final arithmetic FLAGS only on a normal continuation.
The +0E05 loopback, B0:0048 dispatch, complete entry8 composition, and
natural U7F caller frame remain open. No native/full-function/strict
full-C count credit follows from this bounded source gate.
